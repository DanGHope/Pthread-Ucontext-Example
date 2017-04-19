// Daniel Hope - 100453916
// Coroutine/Fiber Worker threads

#define _GNU_SOURCE
#include <ucontext.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define STACK_SIZE 2048

// CPU set
cpu_set_t cpus;

// Sample functions to run in contexts
void func();
void func2();
// Entry point for each thread
void * run_thread();

// Create a new context to do work in
void create_context(ucontext_t *new_ctx, ucontext_t *return_ctx, void *work) {

  // Create new stack
  new_ctx->uc_stack.ss_sp = (char *) malloc(STACK_SIZE);
  new_ctx->uc_stack.ss_size = STACK_SIZE;
  new_ctx->uc_stack.ss_flags = 0;
  // Context to return to when execution ends
  new_ctx->uc_link = return_ctx;
  // Create the context
  makecontext(new_ctx, work, 0);

}

int main(int argc, char* argv[]) {
  // Set random seed
  srand(time(NULL));
  
  // Determine cpu count on current hardware
  int number_of_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  printf("You Have %d Processors Available\n", number_of_cpus);

  // Create a thread variable for each cpu
  pthread_t thread[number_of_cpus];
  pthread_attr_t thread_attr;

  // Create threads
  int i;
  for(i=0;i<number_of_cpus;i++) {
    // Init thread propertys
    pthread_attr_init(&thread_attr);
    // Clear current cpu set
    CPU_ZERO(&cpus);
    // Set cpu to i
    CPU_SET(i, &cpus);
    // Lock thread to cpu i
    pthread_attr_setaffinity_np(&thread_attr, sizeof(cpu_set_t), &cpus);
    // Create thread
    if(pthread_create(&thread[i], &thread_attr, (void *) run_thread, NULL)) {
      printf("Failed to create thread\n");
      return -1;
    }
  }

  // wait for threads to join
  for(i = 0; i<number_of_cpus; i++) {
    pthread_join(thread[i], NULL);
  }

  pthread_attr_destroy(&thread_attr);

  printf("End Of Main\n");
  return 0;
}

void * run_thread() {
  // Get id of cpu i am running on
  int thread_id = sched_getcpu();

  printf("Thread %d Running\n", thread_id);

  // Remember this threads context at this current location
  // Since the return context is set to here everytime we come back
  // into this function it will resume from here rather then the end
  ucontext_t my_ctx;
  ucontext_t work_ctx;
  void *work;
  int r;
  getcontext(&my_ctx);

  // randomly select the work to be done
  r = rand()%5;
  printf("(%d) Current r: %d\n", thread_id, r);
  switch(r) {
    // End execution
    case 0:
      printf("Thread %d Terminating\n", thread_id);
      return NULL;
    // Sleep
    case 1:
      work = func;
      break;
    // Spin lock
    default:
      work = func2;
      break;
  }

  // Create a new context to run work on
  getcontext(&work_ctx);
  create_context(&work_ctx, &my_ctx, work);
  // Set context to the new context
  setcontext(&work_ctx);

  printf("Thread %d Ended\n", thread_id);
  return NULL;
}

// Sleep for 3 seconds
void func() {
  printf("Sleep Running\n");
  sleep(3);
  return;
}

// Spin for many cycles (Good for seeing the cpu under load)
void func2() {
  printf("Spin Running\n");
  int i = 0;
  while(i<100000000) {
    i++;
  }
  return;
}