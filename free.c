// reference: https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_73/apiref/apiexusmem.htm

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>

#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 1              /* Id used on ftok for shmget key    */

#define NUMSEMS 1               /* Num of sems in created sem set    */
int SIZEOFSHMSEG;        /* Size of the shared mem segment    */

#define NUMMSG 2                /* Server only doing two "receives"
                                   on shm segment                    */
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

int main(int argc, char *argv[])
{
    struct sembuf operations[2];
    char         *shm_address;
    int semid, shmid, rc;
    key_t semkey, shmkey;
    struct shmid_ds shmid_struct;
    SIZEOFSHMSEG = get_memory_size();

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
    if (rc == -1)
      {
        printf("main: semop() failed\n");
        return -1;
      }




    //int cmp = strcmp(shm_address, "Hello from Client");
    //printf("%s \n",shm_address );

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
