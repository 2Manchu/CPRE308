#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include "Bank.h"

#define MAX_REQ_LEN 250

struct command_entry {
    TAILQ_ENTRY(command_entry) tailq;
    char* command;
    int req_id;
};

int quit_cmd_received = 0;
sem_t* mutex;
TAILQ_HEAD(tailhead, command_entry) head;

//Helper function to add to the queue of commands
void queue_add(char* command) {
    static int req_id = 1;
    char* command_copy = NULL;
    struct command_entry* q_elem;

    //Allocate space for command
    strcpy(command_copy, command);

    //Create a data structure and enter it into the queue
    q_elem = malloc(sizeof(struct command_entry));
    if (q_elem) {
        q_elem->req_id = req_id;
        q_elem->command = command_copy;
    }
    TAILQ_INSERT_TAIL(&head, q_elem, tailq);
    req_id++;
}

//--------------Worker thread code--------------
void* process_request() {
    while(1) {
        if (quit_cmd_received) {
            return 0;
        }


    }
}

int main (int argc, char* argv[]) {
    int num_threads = 0;
    int num_accounts = 0;
    char* output_filename;
    //--------------Do the initial setup--------------
    if (argc < 4) {
        printf("Invalid commandline config attempted: appserver [thread_count] [account_count] [output_filename]\n");
        return 255;
    } else {
        num_threads = atoi(argv[1]);
        num_accounts = atoi(argv[2]);
        output_filename = argv[3];
    }

    //--------------Initialize desired number of bank accounts--------------
    printf("Initializing %d accounts", num_accounts);
    initialize_accounts(num_accounts);

    //--------------Get requests and start worker threads as necessary--------------
    char* command = NULL;
    pthread_t processing_threads[num_threads];
    mutex = sem_open("SEM", O_CREAT, 666, 1);
    printf("Creating %d worker threads...", num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&processing_threads[i], NULL, process_request, NULL);
    }

    while (!quit_cmd_received) {
        //Get input from stdin and add to queue
        fgets(command, MAX_REQ_LEN, stdin);
        queue_add(command);
    }
    //Stop taking input once quit has been received
    printf("Waiting on threads to finish processing requests\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_join(processing_threads[i], NULL);
    }

}
