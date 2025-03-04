#include "mapreduce.h"
#include "threadpool.h"
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

typedef struct Partition {
  char *key;              
  char *value;            
  struct Partition *next; // next partition
} Partition;

typedef struct {
  Partition **part_list;    // List of the partition heads
  unsigned int num_parts;   // Total no. of partition
  pthread_mutex_t mutex_t;  // mutex for partitions
  unsigned int *size;       // array to keep track of the size of partitions
  unsigned int *index;       // index of partitions
} Partitions;

typedef struct {
  unsigned int part_idx;
  Reducer func;
} Reducer_args;

Partitions partitions;

void MR_Run(unsigned int file_count, char *file_names[], Mapper mapper, 
Reducer reducer, unsigned int num_workers, unsigned int num_parts){
    
    ThreadPool_t *tp = ThreadPool_create(num_workers);
    //init partitions
    partitions.part_list = (Partition **)malloc(num_parts * sizeof(Partition *));
    partitions.size = (unsigned int*)malloc(num_parts * sizeof(unsigned int));
    partitions.index = (unsigned int *)malloc(num_parts * sizeof(unsigned int));
    // populate with 0
    for(int i = 0; i < num_parts; i++){
        partitions.size[i] = 0;
        partitions.index[i] = 0;
    }
    partitions.num_parts = num_parts;
    pthread_mutex_init(&partitions.mutex_t, NULL);
    
    // start with mappers
    struct stat *file_stat = (struct stat *)malloc(sizeof(struct stat));
    for(int i = 0; i < file_count; i++){
        bool job_added = false;
        stat(file_names[i], file_stat);
        size_t file_size = file_stat->st_size;

        job_added = ThreadPool_add_job(tp, (void *)mapper, file_names[i], file_size);
        if(!job_added){
            fprintf(stderr, "failed to add job\n");
        }
    }
    free(file_stat);
    // once maps are done we will do reducers
    ThreadPool_check(tp);

    for (int i = 0; i < num_parts; i++){
        bool job_added = false;
        Reducer_args *red_args = (Reducer_args *)malloc(sizeof(Reducer_args));
        red_args->part_idx = i;
        red_args->func = reducer;

        job_added = ThreadPool_add_job(tp, (void *)MR_Reduce, red_args, partitions.size[i]);
        if(!job_added){
            fprintf(stderr, "failed to add job\n");
        }
    }
    ThreadPool_check(tp);

    //cleanup
    ThreadPool_destroy(tp);
    free(partitions.part_list);
    free(partitions.size);
    free(partitions.index);
    pthread_mutex_destroy(&partitions.mutex_t);
    


}

void MR_Emit(char *key, char *value){
    // write the pair to a new partition
    Partition *new_part = (Partition *)malloc(sizeof(Partition));
    new_part->key = strdup(key);
    new_part->value = strdup(value);
    new_part->next = NULL;

    // find where the partition belongs
    unsigned int part_id = MR_Partitioner(key, partitions.num_parts);

    // lock to place in paritions
    pthread_mutex_lock(&partitions.mutex_t);
    if(!partitions.part_list[part_id] || strcmp(partitions.part_list[part_id]->key, key) > 0){
        // becomes head in that index
        new_part->next = partitions.part_list[part_id];
        partitions.part_list[part_id] = new_part;
    } else{
        Partition *part_i = partitions.part_list[part_id];
        while(part_i->next && strcmp(part_i->next->key, key) <= 0){
            part_i = part_i->next;
        }
        new_part->next = part_i->next;
        part_i->next = new_part;
    }
    partitions.size[part_id]++;

    pthread_mutex_unlock(&partitions.mutex_t);
}

// from assignment
unsigned int MR_Partitioner(char *key, unsigned int num_partitions){
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
    hash = hash * 33 + c;
    return hash % num_partitions;
}


void MR_Reduce(void *threadarg){
    Reducer_args *args = (Reducer_args *)threadarg;
    Partition *head_part = partitions.part_list[args->part_idx];
    Reducer func  = args->func;
    unsigned int part_idx = args->part_idx;

    while(head_part){
        char *key = head_part->key;
        func(key, part_idx);
        // advance and free
        while (head_part && strcmp(key, head_part->key) >= 0) {
            head_part = head_part->next;
        }
    }
    head_part = partitions.part_list[args->part_idx];
    Partition *next = NULL;
    while (head_part) {
    next = head_part->next;
    free(head_part->key);
    free(head_part);
    head_part = next;
  }
  free(args);

}


char *MR_GetNext(char *key, unsigned int partition_idx) {
  // partition exceeding bounds
  if (partitions.index[partition_idx] >= partitions.size[partition_idx] || 
  partitions.num_parts <= partition_idx) {
    return NULL;
  }
  // grab key value at index
  Partition *part = partitions.part_list[partition_idx];
  for (unsigned int i = 0; i < partitions.index[partition_idx]; i++) {
    part = part->next;
  }
//    if(curr == NULL){
//     return NULL;
//   }

  // Check if key at index match
  if (strcmp(key, part->key) != 0) {
    return NULL;
  }

  // Key matches, return value, increment index
  partitions.index[partition_idx]++;

  return part->value;
}