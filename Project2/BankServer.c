#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "Bank.h"

#define MAX_REQ_LEN 250

int quit_cmd_received = 0;
sem_t* queue_mutex;
sem_t* acc_mutex;
FILE* output;

struct queue* q;

struct transaction {
    int acc_id;
    int amount;
};

struct request {
    //Pointer to next request
    struct request* next;
    struct request* prev;
    int request_id;
    int balchk_id;
    //Array of transactions in this request
    struct transaction* trans_list;
    int trans_cnt;
    struct timeval start;
    struct timeval end;
    //Is this an exit command?
    int exit;
};

struct queue {
    struct request* head;
    struct request* tail;
    int num_jobs;
};

void queue_add(struct request* req) {
    //Queue is empty
    if (q->num_jobs == 0) {
        q->head = req;
        q->tail = req;
        q->num_jobs++;
    } else {
        //Queue has jobs in it so we add to the tail
        req->next = q->tail;
        q->tail->prev = req;
        q->tail = req;
        q->num_jobs++;
    }
}

struct request* queue_pop() {
    if (q->head == NULL) {
        return NULL;
    } else {
        struct request* pop = q->head;
        q->num_jobs--;
        q->head = pop->prev;
        return pop;
    }
}
//--------------Worker thread code--------------
void* process_request() {
    struct request* req;
    while(1) {
        if (quit_cmd_received) {
            printf("THEAD: Exiting\n");
            return 0;
        }
        //Start by acquiring a lock on the queue to grab a request
//        printf("THREAD: Trying to acquire lock\n");
        sem_wait(queue_mutex);
//        printf("THREAD: Acquired lock\n");
        if (q->num_jobs > 0) {
            req = queue_pop();
        } else {
            //Wait until we have a job available
            printf("THREAD: Waiting for transaction\n");
            while (q->num_jobs == 0);
            req = queue_pop();
        }
        sem_post(queue_mutex);
        printf("THREAD: Got request\n");

        //Decide what type it is
        if (req->balchk_id >= 0) {
            //Then grab the mutex for the account we want to check
            sem_wait(&acc_mutex[req->balchk_id]);
            //Do the check
            int balance = read_account(req->balchk_id);
            printf("THREAD: ID %0d BAL %0d\n", req->balchk_id, balance);
            sem_post(&acc_mutex[req->balchk_id]);

            //Print the check to the file
            flockfile(output);
            struct timeval end;
            gettimeofday(&end, NULL);
            fprintf(output, "%0d BAL %0d TIME %.06d %.06d\n", req->request_id, balance, req->start.tv_usec, end.tv_usec);
            fflush(output);
            funlockfile(output);
        } else {
            //TODO We have a TRANS

        }

        if (req->exit == 1) {
            quit_cmd_received = 1;
            free(req);
            continue;
        }
        free(req);
    }
}

void create_trans(char command[]) {
    char command_copy[MAX_REQ_LEN];
    static int req_id = 1;
    struct request* req;

    //Create a copy of the command for later usage
    strcpy(command_copy, command);
    //Create the request trans
    req = malloc(sizeof(struct request));
    req->request_id = req_id;

    //Get the type of command we are executing
    char* trans_type;
    trans_type = strtok(command_copy, " ");
    if (strcmp(trans_type, "CHECK") == 0) {
        //All we need in a balance check trans is the account number
        req->balchk_id = atoi(strtok(NULL, " "));
        req->exit = 0;
    }
    else if (strcmp(trans_type, "TRANS") == 0) {
        req->trans_cnt = 0;
        //Start by grabbing the number of <account, amount> pairs from a copy of the command
        while (1) {
            if (strtok(NULL, " ") != NULL) {
                if (strtok(NULL, " ") != NULL) {
                    //Pair
                    req->trans_cnt++;
                }
                else {
                    //There's no amount associated with the account so this request is invalid
                    free(req);
                    return;
                }
            }
            else {
                //We're out of pairs to grab
                break;
            }
        }
        //Allocate an array of transactions
        struct transaction* trans_array;
        trans_array = (struct transaction*)malloc(req->trans_cnt * sizeof(struct transaction));
        //Also set balchk_id to -1 to denote that this is a trans request
        req->balchk_id = -1;
        int curr_trans = 0;

        //Throw away the first token in the original command since we already know it
        strtok(command, " ");
        while (curr_trans < req->trans_cnt) {
            struct transaction* t;
            t = malloc(sizeof(struct transaction));

            //"Command" is already stored in buffer for strtok so keep going through it
            int acc_id = atoi(strtok(NULL, " "));
            int amount = atoi(strtok(NULL, " "));
            t->acc_id = acc_id;
            t->amount = amount;

            trans_array[curr_trans] = *t;
            curr_trans++;
        }

        req->exit = 0;
        req->trans_list = trans_array;
//        for(int i = 0; i < req->trans_cnt; i++) {
//            printf("DEBUG: TRANS[%0d] -> %0d %0d\n", i, req->trans_list[i].acc_id, req->trans_list[i].amount);
//        }
    }
    else if (strcmp(trans_type, "END\n") == 0) {
        req->balchk_id = -1;
        req->exit = 1;
    }
    else {
        //This was not a valid transaction type
        printf("ERROR: Invalid request type\n");
        return;
    }

    //Get the start time
    struct timeval* start = malloc(sizeof(struct timeval));
    gettimeofday(start, NULL);
    req->start = *start;

    queue_add(req);
    printf("ID %0d\n", req->request_id);
    req_id++;
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

    //--------------Open our output file for editing--------------
    output = fopen(output_filename, "w");
    if (output == NULL) {
        printf("ERROR: Could not open file\n");
        return 254;
    }

    //--------------Initialize desired number of bank accounts--------------
    printf("Initializing %d accounts...\n", num_accounts);
    initialize_accounts(num_accounts);

    //--------------Initialize semaphores--------------
    acc_mutex = (sem_t*)malloc(num_accounts * sizeof(sem_t));
    for (int i = 0; i < num_accounts; i++) {
        sem_init(&acc_mutex[i], 0, 1);
    }
    sem_init(queue_mutex, 0, 1);
    //--------------Start worker threads--------------
    pthread_t processing_threads[num_threads];
    printf("Creating %d worker threads...\n", num_threads);
    for (int i = 0; i < num_threads; i++) {
        //Pass the starting memory addresses of the semaphore array
        pthread_create(&processing_threads[i], NULL, process_request, NULL);
    }

    //--------------Create a queue struct to hold our requests--------------
    q = malloc(sizeof(struct queue));
    q->head = NULL;
    q->tail = NULL;
    q->num_jobs = 0;

    //--------------Get input requests--------------
    char command[MAX_REQ_LEN];
    while (1) {
        //Get input from stdin and add to queue
        fgets(command, MAX_REQ_LEN, stdin);
        if (strcmp(command, "END\n") == 0) {
            create_trans(command);
            break;
        } else {
            create_trans(command);
        }
    }
    //Stop taking input once quit has been received
    printf("Waiting on threads to finish processing requests\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_join(processing_threads[i], NULL);
    }

    free(&acc_mutex[0]);
    free(q);
    free_accounts();
    fclose(output);

}
