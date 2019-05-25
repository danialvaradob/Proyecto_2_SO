// reference: https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_73/apiref/apiexusmem.htm

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 1              /* Id used on ftok for shmget key    */

#define NUMSEMS 1               /* Num of sems in created sem set    */
#define SIZEOFSHMSEG 100       /* Size of the shared mem segment    */

#define NUMMSG 2                /* Server only doing two "receives"
                                   on shm segment                    */

int main(int argc, char *argv[])
{
    int retval, semid, shmid, i;
    key_t semkey, shmkey;
    void *shm_address;
    struct sembuf sem_buf;
    struct shmid_ds shmid_struct;
    short  sarray[NUMSEMS];

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
      printf("Semkey result: %d\n", semkey);
      printf("Shmkey result: %d\n", shmkey);

    /* Create a semaphore set using the IPC key.  The number of      */
    /* semaphores in the set is two.  If a semaphore set already     */
    /* exists for the key, return an error. The specified permissions*/
    /* give everyone read/write access to the semaphore set.         */
      
    semid = semget( semkey, NUMSEMS, 0666 | IPC_CREAT | IPC_EXCL );
    if ( semid == -1 )
      {
        printf("main 1: semget() failed\n");
        return -1;
      }

    printf("The semaphore id is: %d\n",semid );


    /* Create a shared memory segment using the IPC key.  The        */
    /* size of the segment is a constant.  The specified permissions */
    /* give everyone read/write access to the shared memory segment. */
    /* If a shared memory segment already exists for this key,       */
    /* return an error.                                              */
    shmid = shmget(shmkey, SIZEOFSHMSEG, 0666 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1) {
        printf("main2: shmget() failed\n");
        return -1;
    }   
    /* Attach the shared memory segment to the server process.       */
    shm_address = shmat(shmid, NULL, 0);
    if ( shm_address== (char *) -1 ) {
        printf("main: shmat() failed\n");
        return -1;
    }
    printf("Initializer DONE\n");

    
    /*Set the shared memory segment for use using the semaphore.     */
    sem_buf.sem_num = 0;
    sem_buf.sem_op =  0;
    sem_buf.sem_flg = IPC_NOWAIT;
    //operations[0].sem_flg = 0;

    retval = semop(semid, &sem_buf, 1 );
    if (retval == -1) {
        printf("main1: semop() failed\n");
        return -1;
    
    }
       

    
     /* Detach the shared memory segment from the current process.    */
    retval = shmdt(shm_address);
    if (retval==-1)
      {
        printf("main: shmdt() failed\n");
        return -1;
      }
    



return 0;
}
