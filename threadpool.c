#include "threadpool.h"
#include <stdio.h>

ThreadPool_t *ThreadPool_create(unsigned int num){
    
    // init threadpool
    ThreadPool_t *tp = (ThreadPool_t *)malloc(sizeof(ThreadPool_t));;
    // WE FAILED CHAT 
    if (!tp){
        return NULL;
    }
    tp->threads = (pthread_t *)malloc(num * sizeof(pthread_t));
    tp->size = num;
    tp->jobs.head = NULL;
    tp->jobs.size = 0;
    tp->working_threads = 0;
    tp->stop = false;
    pthread_mutex_init(&tp->mutex_t, NULL);
    pthread_cond_init(&tp->cond, NULL);

    for(int i = 0; i < num; i++){
        pthread_create(tp->threads + i, NULL, (void *)Thread_run, tp);
    }
    return tp;
}

void ThreadPool_destroy(ThreadPool_t *tp){
    
    pthread_mutex_lock(&tp->mutex_t);
    tp->stop = true;
    pthread_cond_broadcast(&tp->cond);
    pthread_mutex_unlock(&tp->mutex_t);

    // join threads
    for (unsigned int i = 0; i < tp->size; i++) {
    pthread_join(tp->threads[i], NULL);
  }

    // Free job queue, is this required?
    ThreadPool_job_t *job = tp->jobs.head;
    while (job) {
        ThreadPool_job_t *next_job = job->next;
        free(job);
        job = next_job;
    }

    // Free the threads array and the thread pool itself
    free(tp->threads);
    pthread_cond_destroy(&tp->cond);
    pthread_mutex_destroy(&tp->mutex_t);
    free(tp);
}

// remember to report the edit to the prototype 
bool ThreadPool_add_job(ThreadPool_t *tp, thread_func_t func, void *arg, int length) {
    // NULL check
    if (!tp || !func) return false;

    // Init the new job
    ThreadPool_job_t *job = (ThreadPool_job_t *)malloc(sizeof(ThreadPool_job_t));
    if (!job) return false;  // Check for successful allocation
    job->arg = arg;
    job->func = func;
    job->length = length;
    job->next = NULL;

    // Lock the job queue
    pthread_mutex_lock(&tp->mutex_t);

    // Insert the job in order based on `length`
    if (tp->jobs.head == NULL || tp->jobs.head->length > job->length) {
        // Insert at the head if list is empty or job has highest priority
        job->next = tp->jobs.head;
        tp->jobs.head = job;
    } else {
        // Traverse to find the correct insertion point
        ThreadPool_job_t *current_job = tp->jobs.head;
        while (current_job->next && current_job->next->length <= job->length) {
            current_job = current_job->next;
        }
        // Insert job in the found position
        job->next = current_job->next;
        current_job->next = job;
    }

    // Increment job count and signal waiting threads
    tp->jobs.size++;
    
    pthread_cond_signal(&tp->cond);  // Signal any waiting threads
    pthread_mutex_unlock(&tp->mutex_t);
    return true;
}

ThreadPool_job_t *ThreadPool_get_job(ThreadPool_t *tp){
    if(!tp) return NULL;
    
    ThreadPool_job_t *job = tp->jobs.head;
    // check for a job
    if (job) { 
        tp->jobs.head = job->next;
        tp->jobs.size--;
        return job;
    }
    return NULL;
    
}
 
void *Thread_run(ThreadPool_t *tp){
    
    while(true){
        pthread_mutex_lock(&tp->mutex_t);
        // if we are waiting, unlock
        while(!tp->stop && tp->jobs.head == NULL){
            pthread_cond_wait(&tp->cond, &tp->mutex_t);
        }
        if(tp->stop){
            pthread_mutex_unlock(&tp->mutex_t);
            break;
        }
        ThreadPool_job_t *job = ThreadPool_get_job(tp);
        if(job){
            tp->working_threads++;
            pthread_mutex_unlock(&tp->mutex_t);
            job->func(job->arg);

            pthread_mutex_lock(&tp->mutex_t);
            free(job); 
            tp->working_threads--;
            pthread_cond_broadcast(&tp->cond);
        }
        pthread_mutex_unlock(&tp->mutex_t);
    }

    return NULL;
}

void ThreadPool_check(ThreadPool_t *tp) {
    pthread_mutex_lock(&tp->mutex_t);
    while (tp->working_threads != 0 || tp->jobs.size != 0) {
        pthread_cond_wait(&tp->cond, &tp->mutex_t);
    }
    pthread_mutex_unlock(&tp->mutex_t);
}
