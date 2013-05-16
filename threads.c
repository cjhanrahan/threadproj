//
//  threads.c
//  
//
//  Created by Scott Brandt on 5/6/13.
//
//

#include <stdio.h>
#include <stdlib.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

// 5 threads, 10 tickets, switching every 1 ms
#define MAX_THREADS 5
#define NUM_TICKETS 10
#define QUANT_MS 1

static ucontext_t ctx[MAX_THREADS];
static int ticket[NUM_TICKETS];

static void test_thread(void);
static int thread = 0;
void thread_exit(int);
int thread_switch();
int get_newthread(int);
int get_prevthread(int);
int get_randthread();

// This is the main thread
// In a real program, it should probably start all of the threads and then wait for them to finish
// without doing any "real" work
int main(void) {
    printf("Main starting\n");
    
    printf("Main calling thread_create\n");

    // Start n other threads
    struct itimerval timer;
    int i;
    for (i = 0; i < MAX_THREADS; i++) thread_create(&test_thread);
    for (i = 0; i < NUM_TICKETS; i++) ticket[i] = i%MAX_THREADS;
    // For testing different weights
    ticket[0] = 1;
    ticket[2] = 1;
    ticket[4] = 1;
    ticket[8] = 1;
    
    printf("Main returned from thread_create\n");

    // signal & timer setup
    srand(time(NULL));
    if (signal(SIGALRM, (void (*)(int)) thread_switch) == SIG_ERR) {
      perror("Signal failed.");
      exit(1);
    }
    timer.it_value.tv_sec = QUANT_MS / 1000;
    timer.it_value.tv_usec = QUANT_MS * 1000;
    timer.it_interval.tv_sec = QUANT_MS / 1000;
    timer.it_interval.tv_usec = QUANT_MS * 1000;
    if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
      perror("itimer not set.");
      exit(1);
    }
     
    while(1) {
      test_thread();
    }
    
    // We should never get here
    exit(0);
    
}

// This is the thread that gets started by thread_create
static void test_thread(void) {
    printf("In test_thread\n");
    int i;
    // Loop, doing a little work
    while(1) {
        for (i = 0; i < 10000; i++) {
          usleep(QUANT_MS * 100);
          printf("Thread %d: i = %d\n", thread, i);
        }
    }
    
    thread_exit(0);
}

// Switch to another thread
int thread_switch() {
    int old_thread = thread;
    
    // This is the lottery scheduler
    thread = get_randthread(); // get_prevthread(thread);

    printf("Interrupting thread %d for thread %d\n", old_thread, thread);
    
    // This will stop the current thread from running and restart the other thread
    swapcontext(&ctx[old_thread], &ctx[thread]);  //ctx[old_thread].uc_link
}

// Create a thread
int thread_create(int (*thread_function)(void)) {
    int newthread = get_newthread(thread);
    
    printf("Thread %d in thread_create\n", newthread);

    // First, create a valid execution context the same as the current one
    getcontext(&ctx[newthread]);

    // Now set up its stack
    ctx[newthread].uc_stack.ss_sp = malloc(8192);
    ctx[newthread].uc_stack.ss_size = 8192;

    // This is the context that will run when this thread exits
    ctx[newthread].uc_link = &ctx[thread];
    thread = newthread;
    // Now create the new context and specify what function it should run
    makecontext(&ctx[newthread], test_thread, 0);
    
    printf("Thread %d done with thread_create\n", thread);
}

int get_newthread(int thread) {
    int tmp_thread = thread + 1;
    if (tmp_thread >= MAX_THREADS) tmp_thread = 0;
    return tmp_thread;
}

int get_prevthread(int thread) {
    int tmp_thread = thread - 1;
    if (tmp_thread < 0) tmp_thread = MAX_THREADS - 1;
    return tmp_thread;
}

// 'Draw' random thread
int get_randthread() {
    return ticket[rand() % NUM_TICKETS];
}

// This doesn't do anything at present
void thread_exit(int status) {
    printf("Thread %d exiting\n", thread);
}
