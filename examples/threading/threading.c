#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data*) thread_param;

    int rc;  

    
    usleep(thread_func_args->wait_to_obtain_ms*1000);

    
    rc = pthread_mutex_lock(thread_func_args->mutex);
  

    if(rc!=0)
    {
      thread_func_args->thread_complete_success = false;
       ERROR_LOG("mutex  lock failed");
      pthread_exit(thread_func_args);

    }
    usleep(thread_func_args->wait_to_release_ms*1000);
       
    rc = pthread_mutex_unlock(thread_func_args->mutex); 

    if(rc!=0)
    {
      thread_func_args->thread_complete_success = false;
       ERROR_LOG("mutex  unlockfailed");
      pthread_exit(thread_func_args);

    }
    thread_func_args->thread_complete_success = true;
    pthread_exit(thread_func_args);

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     *
     *
     *IMPLEMENTATION

    **
    * Start a thread which sleeps @param wait_to_obtain_ms number of milliseconds, then obtains the
    * mutex in @param mutex, then holds for @param wait_to_release_ms milliseconds, then releases.
    *The start_thread_obtaining_mutex function should only start the thread and should not block
* for the thread to complete.
* The start_thread_obtaining_mutex function should use dynamic memory allocation for thread_data
* structure passed into the thread.  The number of threads active should be limited only by the
* amount of available memory.
* The thread started should return a pointer to the thread_data structure when it exits, which can be used
* to free memory as well as to check thread_complete_success for successful exit.
* If a thread was started succesfully @param thread should be filled with the pthread_create thread ID
* coresponding to the thread which was started.
* @return true if the thread could be started, false if a failure occurred.
*/
     
    struct thread_data *s = (struct thread_data*)malloc(sizeof(struct thread_data));
  
    s->mutex = mutex;
    s->wait_to_obtain_ms = wait_to_obtain_ms;
    s->wait_to_release_ms = wait_to_release_ms; 
    
    int rc;

     rc = pthread_create(thread,NULL,threadfunc,(void*)s);

    if(rc !=0)
    {
         ERROR_LOG("thread could not be created");
         
         free(s);

        return false;	
    }

  return true;
}

