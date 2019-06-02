#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdlib.h>

#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 1              /* Id used on ftok for shmget key    */

#define NUMSEMS 1               /* Num of sems in created sem set    */
int SIZEOFSHMSEG;       /* Size of the shared mem segment    */

#define NUMMSG 2                /* Server only doing two "receives"
                                   on shm segment                    */

#define SNAME "/state_sem"
#define MAXCHAR 10000

int get_memory_size(){
  FILE *fp;
  char* filename = "config.txt";
  char* msg = "%d";
  char str[MAXCHAR];
  int size;
  fp = fopen(filename, "r");
  if (fp == NULL) {
     printf("Could not open file %s",filename);
  }

  if (fgets(str, MAXCHAR, fp) != NULL){
    size = atoi(str);
  }
  fclose(fp);
  return size;
}

int show_memory_state() {
    struct sembuf operations[2];
    int         *shm_address, *pointer;
    int semid, shmid, rc;
    key_t semkey, shmkey;
    struct shmid_ds shmid_struct;
    int segment_size;


    /* Generate an IPC key for the semaphore set and the shared      */
    /* memory segment.  Typically, an application specific path and  */
    /* id would be used to generate the IPC key.                     */
    semkey = ftok(SEMKEYPATH,SEMKEYID);
    if ( semkey == (key_t)-1 ) {
        printf("main: ftok() for sem failed\n");
        return -1;
      }
    shmkey = ftok(SHMKEYPATH,SHMKEYID);
    if ( shmkey == (key_t)-1 ) {
        printf("main: ftok() for shm failed\n");
        return -1;
      }

    /* Get the already created semaphore ID associated with key.     */
    /* If the semaphore set does not exist, then it will not be      */
    /* created, and an error will occur.                             */
    semid = semget( semkey, NUMSEMS, 0666);
    if ( semid == -1 ) {
        printf("main: semget() failed\n");
        return -1;
      }

    /* Get the already created shared memory ID associated with key. */
    /* If the shared memory ID does not exist, then it will not be   */
    /* created, and an error will occur.                             */

    shmid = shmget(shmkey, SIZEOFSHMSEG, 0666);
    if (shmid == -1) {
        printf("main: shmget() failed\n");
        return -1;
    }


    /* Attach the shared memory segment to the client process.       */
    shm_address = (int *) shmat(shmid, NULL, 0);
    if (shm_address == NULL) {
        printf("main: shmat() failed\n");
        return -1;
      }



    /* Check if the second semaphore is 0. If its no, the the spy    */
    /* process i allowed to read the shared memory                   */
    /* for the semaphore to reach zero before running the semop().   */
    operations[0].sem_num = 0;
                                    /* Operate on the first sem      */
    operations[0].sem_op =  0;
                                    /* Wait for the value to be=0    */
    operations[0].sem_flg = 0;
                                    /* Allow a wait to occur         */

    operations[1].sem_num = 0;
                                    /* Operate on the first sem      */
    operations[1].sem_op =  1;
                                    /* Increment the semval by one   */
    operations[1].sem_flg = 0;
                                    /* Allow a wait to occur         */

    rc = semop( semid, operations, 2 );
    if (rc == -1) {
        printf("main: semop() failed\n");
        return -1;
      }

    /*
    pointer = shm_address + 3;
    for (char A ='A'; A < 'G'; A++) {
        *pointer = A;
        pointer++;
    }

    */

      for (int i = 0; i < SIZEOFSHMSEG; i++) {

        printf("%d\n",shm_address[i]);

      }


    /* Release the shared memory segment by decrementing the in-use  */
    /* semaphore (the first one).  Increment the second semaphore to */
    /* show that the client is finished with it.                     */
    operations[0].sem_num = 0;
                                    /* Operate on the first sem      */
    operations[0].sem_op =  -1;
                                    /* Decrement the semval by one   */
    operations[0].sem_flg = 0;
                                    /* Allow a wait to occur         */


    rc = semop( semid, operations, 1 );
    if (rc == -1) {
        printf("main: semop() failed\n");
        return -1;
      }




    /* Detach the shared memory segment from the current process.    */
    rc = shmdt(shm_address);
    if (rc==-1) {
        printf("main: shmdt() failed\n");
        return -1;
    }

return 0;

}

int show_processes_states(){
  FILE *fp;
  char* filename = "states.txt";
  sem_t *sem = sem_open(SNAME, 0);
  char c;
  /*Waiting for the semaphore*/
  sem_wait(sem);
  /*Reading file and creating maze */
  fp = fopen(filename, "r");
  if (fp == NULL){
      printf("Could not open file %s",filename);
  }

  c = fgetc(fp);
  while (c != EOF)
  {
      printf ("%c", c);
      c = fgetc(fp);
  }


  fclose(fp);

  /*Releasing the semaphore*/
  sem_post(sem);
}

int main(int argc, char *argv[]) {
    int option;
    SIZEOFSHMSEG = get_memory_size();
    while (1) {
        printf("Welcome to the Spy Process.\nEnter 1 for the memory status\nEnter 2 for the proccesses status\nEnter 3 to exit\nResponse: ");
        scanf("%d",&option);

        if (option == 1) {
            show_memory_state();
        } else if (option == 2) {
            show_processes_states();
            //printf("Process 1\n");
        } else if (option == 3) {
            printf("Exiting....\n");
            break;
        }
    }
    return 0;
}
