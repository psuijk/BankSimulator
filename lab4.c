#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#define NUM_RDRS 6
#define NUM_WTRS 2
#define NUM_ACCS 3
#define FILE1 "writerFile1"
#define FILE2 "writerFile2"

bool done;

struct Account {
    sem_t m;
    sem_t wrt;
    int readcount;
    int accID;
    double bal;
    bool updated;
};

typedef struct WriterData {
    struct Account *acc;
    FILE* fptr;
} WriterData;

//function for writing to account balances
void *write(void *data) {
    int accID;
    int pid = pthread_self();
    int scan_value = 0;
    char token;
    double val;
    struct WriterData *wData = data;
    /* while the end of file has not been reached, continue to
     * read lines and update acc balance accordingly */
    while(scan_value != EOF) {
        fscanf((*wData).fptr, " %d", &accID); //get account ID
        //acquire wrt lock
        sem_wait(&(*wData).acc[accID].wrt);
        //start critical section
        printf("wThread %d has started writing to acc%d\n", pid, accID);
        fscanf((*wData).fptr, "%c", &token); //eat unwanted characters
        if (scan_value != EOF) {
            scan_value = fscanf((*wData).fptr, "%lf", &val); //get value for transaction
        }
        if (scan_value != EOF) {
            (*wData).acc[accID].bal += val; //carry out transaction
        }
        //end critical section
        //relinquish wrt lock
        sem_post(&(*wData).acc[accID].wrt);
        printf("wThread %d has finished writing to acc%d\n", pid, accID);
    }
    return 0;
}

//function for reading account balances
void *read(void *data) {
    int accID;
    int pid = pthread_self();
    struct Account *accX = data;
    /* boolean global variable "done" is used to communicate whether
     * writer threads have finished all potential writes. Once they are
     * finished, the reader threads stop auditing accounts */
    while (!done){
        //acquire m lock and readcount is incremented
        sem_wait(&((*accX).m));
        accID = (*accX).accID;
        (*accX).readcount++;
        /* if this is the first reader thread attempting to access
         * the thread grabs the wrt lock so no writer threads
         * may write to the acc balance */
        if ((*accX).readcount == 1) {
            sem_wait(&((*accX).wrt));
            printf("rThread %d acquired the 'wrt' lock for acc%d\n", pid, (accID + 1));
        }
        /* m lock is relinquished so other readers may also enter
         * the critical section as long as a reader still holds the
         * wrt lock */
        sem_post(&((*accX).m));

        //start critical section
        printf("acc%d has bal = %lf : read by rThread %d\n", ((*accX).accID+1), (*accX).bal, pid);
        //end creitical section

        //the m lock is acquired again and readcount is decremented
        sem_wait(&((*accX).m));
        (*accX).readcount--;
        /*if this is the last reader then that reader relinquishes
         * the wrt lock so that writers may acquire it  */
        if ((*accX).readcount == 0) {
            sem_post(&((*accX).wrt));
            printf("rThread %d released the 'wrt' lock for acc%d\n", pid, (accID + 1));
        }
        //m lock is relinquished
        sem_post(&((*accX).m));
    }
    return 0;
}

//function for initializing variables in each account
void initializeAccs(struct Account *accs) {
    for (int i=0; i < NUM_ACCS; i++) {
        sem_init(&(accs[i].m), 0, 1);
        sem_init(&(accs[i].wrt), 0, 1);
        accs[i].accID = i;
        accs[i].bal = 0;
    }
}

//function to create reader threads
void createReaders(pthread_t *rdrs, struct Account *accs, bool *error) {
    for (int i=0; i < NUM_RDRS; i++) {
        int pid = pthread_create(&rdrs[i], NULL, read, &accs[(i%NUM_ACCS)]);
        if (pid == 0) {
            printf("rThread %d was created\n", (i+1));
        } else {
            *error = true;
            printf("failed to create rThread %d\n", (i+1));
        }
    }
    return;
}

//function to create writer threads
void createWriters(pthread_t *wtrs, char fNames[NUM_WTRS][12], WriterData* wData, bool *error) {
    for (int i=0; i < NUM_WTRS; i++) {
       (*(wData + i)).fptr = fopen((fNames[i]), "r");
        int pid = pthread_create((wtrs + i), NULL, write, &(*(wData + i)));
        if (pid==0) {
            printf("wThread %d was created\n", (i+1));
        } else {
            *error = true;
            printf("failed to create wThread %d\n", (i+1));
        }
    }
    return;
}

//function to join reader threads
void joinReaders(pthread_t *rdrs, bool *error) {
    for (int i=0; i < NUM_RDRS; i++) {
        int pid = pthread_join(rdrs[i], NULL);
        if (pid==0) {
            printf("rThread %d has finished\n", (i+1));
        } else {
            *error = true;
            printf("failed to join rThread %d\n", (i+1));
        }
    }
    return;
}

//function to join writer threads
void joinWriters(pthread_t *wtrs, WriterData* wData, bool *error) {
    for (int i=0; i< NUM_WTRS; i++) {
        int pid = pthread_join(wtrs[i], NULL);
        fclose((*(wData + i)).fptr);
        if (pid==0) {
            printf("wThread %d finished\n", (i+1));
        } else {
            *error = true;
            printf("failed to join wThread %d\n", (i+1));
        }
    }

    return;
}

//function to print out successs/fail message upon exit
void printExitMsg(bool *error) {
    if (*error) {
        printf("there was an error when creating one or more threads\n");
        printf("please contact customer service for assistance\n");
    } else {
        printf("all threads have completed successfully\n");
    }


    return;
}

int main() {
    //intitialize variables
    done = false;
    bool error = false;
    pthread_t rdrs[NUM_RDRS];
    pthread_t wtrs[NUM_WTRS];
    WriterData wData[NUM_WTRS];
    char fNames[NUM_WTRS][12] = {FILE1, FILE2};
    struct Account accs[NUM_ACCS];
    /*set both wData instances to have references
     * to the array of Accounts*/
    wData[0].acc = accs;
    wData[1].acc = accs;
    initializeAccs(accs);
    createReaders(rdrs, accs, &error);
    createWriters(wtrs, fNames, wData, &error);
    joinWriters(wtrs, wData,&error);
    /* done is set to true following the completion of
     * all writer threads, signaling readers to stop auditing */
    done = true;
    joinReaders(rdrs, &error);
    printExitMsg(&error);
    return 0;
}
