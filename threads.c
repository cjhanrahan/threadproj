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

#define MAX_THREADS 5
#define NUM_TICKETS 15

static ucontext_t ctx[MAX_THREADS];
static int ticket[NUM_TICKETS];

static void test_thread(void);
static int thread = 0;
void thread_exit(int);
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
    int i;
    for (i = 0; i < MAX_THREADS; i++) thread_create(&test_thread);
    for (i = 0; i < NUM_TICKETS; i++) ticket[i] = i%MAX_THREADS;
    // For testing
    ticket[0] = 2;
    ticket[1] = 2;
    ticket[5] = 2;
    ticket[6] = 2;
    ticket[8] = 2;
    
    srand(time(NULL));
    printf("Main returned from thread_create\n");

    // Loop, doing a little work then yielding to the other thread
    while(1) {
        printf("Main calling thread_yield\n");
        
        thread_yield();
        
        printf("Main returned from thread_yield\n");
    }

    // We should never get here
    exit(0);
    
}

// This is the thread that gets started by thread_create
static void test_thread(void) {
    printf("In test_thread\n");

    // Loop, doing a little work then yielding to the other thread
    while(1) {
        
        printf("Test_thread calling thread_yield\n");
        
        thread_yield();
        
        printf("Test_thread returned from thread_yield\n");
    }
    
    thread_exit(0);
}

// Yield to another thread
int thread_yield() {
    int old_thread = thread;
    
    // This is the scheduler, it is a bit primitive right now
    thread = get_randthread(); // get_prevthread(thread);

    printf("Thread %d yielding to thread %d\n", old_thread, thread);
    printf("Thread %d calling swapcontext\n", old_thread);
    
    // This will stop us from running and restart the other thread
    swapcontext(&ctx[old_thread], &ctx[thread]);  //ctx[old_thread].uc_link

    // The other thread yielded back to us
    printf("Thread %d back in thread_yield\n", thread);
}

// Create a thread
int thread_create(int (*thread_function)(void)) {
    int newthread = get_newthread(thread);
    
    printf("Thread %d in thread_create\n", thread);
    
    printf("Thread %d calling getcontext and makecontext\n", thread);

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

int get_randthread() {
    return ticket[rand() % NUM_TICKETS];
}

// This doesn't do anything at present
void thread_exit(int status) {
    printf("Thread %d exiting\n", thread);
}
