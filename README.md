Multithreaded MapReduce Library

## Synchronization Primitives Used
### In threadpool.c
- **mutext locks:** ThreadPool_t->mutex_t 
- **condition variables** ThreadPool_t->cond

### in mapreduce.c
- **mutext locks:** Partitions->mutex_t

## Partition Implementation
- a custom threadpool library was created for the purposes of implementing this assignment
- MR_Run is run specifically to start up the threadpool and begin running our map jobs priorty based on job size.
- Emit is actually responsible for creating the partitions and finding in which order they belong in (uses a has function that was provided in the assignment description) 
- once map jobs are completed we move on to the reduce jobs, priorty based on partition size. At this point we wait till reduction of the paritions is completed
- Upon completion threadpool is destoryed results are printed out into txt format

## Testing
### threadpool.c
- Did testing of threadpool.c using a custom testing program to determine all functions are correct
### in mapreduce.c
- Once wordcount program was map, provided sample inputs and analyzed the results 





