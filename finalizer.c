#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/syscall.h>
#include <string.h>


enum AlgorithmType{BEST=1, FIRST=2, WORST=3};
enum ProcessState{RUNNING, BLOCKED, IN_CRITICAL_REGION};
sem_t mutex;

enum AlgorithmType selected_algorithm;

/* info used by our threadsx */
struct processInfo {

    pthread_t     PID;         /* ID returned by pthread_create() */
    int           base_register;/* Stores the process' starting memory address*/
    int           size;        /* The size (in lines) of the process */
    int           execution_time;/*Amount of time the process is going to sleep*/
    enum ProcessState state;   /* The current state of the process */
};



void releaseMemory() {
    struct sembuf operations[2];
    char         *shm_address;
    int semid, shmid, rc;
    key_t semkey, shmkey;
    struct shmid_ds shmid_struct;

    /* Generate an IPC key for the semaphore set and the shared      */
    /* memory segment.  Typically, an application specific path and  */
    /* id would be used to generate the IPC key.                     */
    semkey = ftok(SEMKEYPATH,SEMKEYID);
    if ( semkey == (key_t)-1 )
      {
        printf("main: ftok() for sem failed\n");
        return -1;
      }
    shmkey = ftok(SHMKEYPATH,SHMKEYID);
    if ( shmkey == (key_t)-1 )
      {
        printf("main: ftok() for shm failed\n");
        return -1;
      }

    /* Get the already created semaphore ID associated with key.     */
    /* If the semaphore set does not exist, then it will not be      */
    /* created, and an error will occur.                             */
    semid = semget( semkey, NUMSEMS, 0666);
    if ( semid == -1 )
      {
        printf("main: semget() failed\n");
        return -1;
      }

    /* Get the already created shared memory ID associated with key. */
    /* If the shared memory ID does not exist, then it will not be   */
    /* created, and an error will occur.                             */

    shmid = shmget(shmkey, SIZEOFSHMSEG, 0666);
    if (shmid == -1)
      {
        printf("main: shmget() failed\n");
        return -1;
      }

    /* Attach the shared memory segment to the client process.       */
    shm_address = (char*) shmat(shmid, NULL, 0);
    if ( shm_address==NULL )
      {
        printf("main: shmat() failed\n");
        return -1;
      }

    /* Check if the second semaphore is 0. If its no, the the spy    */
    /* process i allowed to read the shared memory                   */
    /* for the semaphore to reach zero before running the semop().   */
    operations[0].sem_num = 0;
    operations[0].sem_op =  0;
    operations[0].sem_flg = 0;
    
    operations[1].sem_num = 0;
    operations[1].sem_op =  1;
    operations[1].sem_flg = 0;

    rc = semop( semid, operations, 2 );
    if (rc == -1)
      {
        printf("main: semop() failed\n");
        return -1;
      }

    /* Release the shared memory segment by decrementing the in-use  */
    /* semaphore (the first one).  Increment the second semaphore to */
    /* show that the client is finished with it.                     */
    operations[0].sem_num = 0;
    operations[0].sem_op =  -1;
    operations[0].sem_flg = 0;
                                    


    rc = semop( semid, operations, 1 );
    if (rc == -1)
      {
        printf("main: semop() failed\n");
        return -1;
      }


     if (1) {
      printf("Finalizing processes. Freeing memory.  \n");
        rc = semctl( semid, 1, IPC_RMID );
        if (rc==-1)
          {
            printf("main: semctl() remove id failed\n");
            return -1;
          }
        rc = shmdt(shm_address);
        if (rc==-1)
          {
            printf("main: shmdt() failed\n");
            return -1;
          }
        rc = shmctl(shmid, IPC_RMID, &shmid_struct);
        if (rc==-1)
          {
            printf("main: shmctl() failed\n");
            return -1;
          }

     }


}

int main(){
    sem_init(&mutex, 0, 1);
    FILE *fp;
    char* filename = "log.txt";
    /*Waiting for the semaphore*/
    sem_wait(&mutex);
    /*Reading file and creating maze */
    fp = fopen(filename, "a");
    if (fp == NULL){
        printf("Could not open file %s",filename);
    }
    /*Getting the datetime for the log entry*/
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    assert(strftime(s, sizeof(s), "%c", tm));

    fprintf(fp,
      "----- Execution ended on %s -----", s);
    fflush(fp);
    fclose(fp);

    /*Releasing the semaphore*/
    sem_post(&mutex);

    /* Releasing memory segment */



    return 0;
}



//Bibliography
//https://www.geeksforgeeks.org/generating-random-number-range-c/
//http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html
//http://www.zentut.com/c-tutorial/c-write-text-file/
//https://www.geeksforgeeks.org/use-posix-semaphores-c/
