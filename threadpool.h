#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

typedef void (*thread_func_t)(void *arg);

typedef struct ThreadPool_job_t {
    thread_func_t func;              // function pointer
    void *arg;                       // arguments for that function
    struct ThreadPool_job_t *next;   // pointer to the next job in the queue
    unsigned int length;             // length of the job 
    // add other members if needed
} ThreadPool_job_t;

typedef struct {
    unsigned int size;               // no. jobs in the queue
    ThreadPool_job_t *head;          // pointer to the first (shortest) job
} ThreadPool_job_queue_t;

typedef struct {
    pthread_t *threads;              // pointer to the array of threads
    ThreadPool_job_queue_t jobs;     // queue of jobs waiting for a thread to run
    unsigned int size;               // no. of threads in pool
    pthread_mutex_t mutex_t;         // mutex lock
    unsigned int working_threads;    // no. of working threads
    pthread_cond_t cond; 
    bool stop;
} ThreadPool_t; 


/**
* C style constructor for creating a new ThreadPool object
* Parameters:
*     num - Number of threads to create
* Return:
*     ThreadPool_t* - Pointer to the newly created ThreadPool object
*/
ThreadPool_t *ThreadPool_create(unsigned int num);

/**
* C style destructor to destroy a ThreadPool object
* Parameters:
*     tp - Pointer to the ThreadPool object to be destroyed
*/
void ThreadPool_destroy(ThreadPool_t *tp);

/**
* Add a job to the ThreadPool's job queue
* Parameters:
*     tp   - Pointer to the ThreadPool object
*     func - Pointer to the function that will be called by the serving thread
*     arg  - Arguments for that function
* Return:
*     true  - On success
*     false - Otherwise
*/
bool ThreadPool_add_job(ThreadPool_t *tp, thread_func_t func, void *arg, int length);

/**
* Get a job from the job queue of the ThreadPool object
* Parameters:
*     tp - Pointer to the ThreadPool object
* Return:
*     ThreadPool_job_t* - Next job to run
*/
ThreadPool_job_t *ThreadPool_get_job(ThreadPool_t *tp);

/**
* Start routine of each thread in the ThreadPool Object
* In a loop, check the job queue, get a job (if any) and run it
* Parameters:
*     tp - Pointer to the ThreadPool object containing this thread
*/
void *Thread_run(ThreadPool_t *tp);

/**
* Ensure that all threads are idle and the job queue is empty before returning
* Parameters:
*     tp - Pointer to the ThreadPool object that will be destroyed
*/
void ThreadPool_check(ThreadPool_t *tp);

#endif
