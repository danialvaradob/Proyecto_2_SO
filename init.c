// reference: https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_73/apiref/apiexusmem.htm

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>

#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 1              /* Id used on ftok for shmget key    */

#define LINESIZE 1
#define NUMSEMS 1               /* Num of sems in created sem set    */
//#define SIZEOFSHMSEG 1024       /* Size of the shared mem segment    */

#define NUMMSG 2                /* Server only doing two "receives"
                                   on shm segment                    */
#define SNAME "/state_sem"

void write_to_log(int size){
  FILE *fp;
  char* filename = "log.txt";
  char* msg = "Program initialized at %s with a shared memory of %d lines\n\n";

  /*Getting the datetime for the log entry*/
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char s[64];
  assert(strftime(s, sizeof(s), "%c", tm));

  /*Reading file and creating maze */
  fp = fopen(filename, "w");
  if (fp == NULL) {
    printf("Could not open file %s",filename);
  }

  fprintf(fp, msg, s, size);

  fclose(fp);

}

/*Saves the size of the memory in
the config.txt file*/
void save_config(int size){
  FILE *fp;
  char* filename = "config.txt";
  char* msg = "%d";

  /*Reading file and creating maze */
  fp = fopen(filename, "w");
  if (fp == NULL) {
     printf("Could not open file %s",filename);
  }

  fprintf(fp, msg, size);

  fclose(fp);

}

void resetting_states_file(){
  char* states_filename = "states.txt";
  char* states__temp_filename = "states_temp.txt";
  FILE* fp = fopen(states_filename, "w");
  FILE* fp_temp = fopen(states__temp_filename, "w");
  if (fp == NULL) {
      printf("Could not open file %s",states_filename);
  }
  fclose(fp);
  fclose(fp_temp);
}

int main(int argc, char *argv[])
{
    int SIZEOFSHMSEG;
    printf("Enter the size of the memory (in lines): ");
    scanf("%d",&SIZEOFSHMSEG);
    save_config(SIZEOFSHMSEG);
    resetting_states_file();
    sem_open(SNAME, O_CREAT, 0644, 3);

    int retval, semid, shmid, i;
    key_t semkey, shmkey;

    struct sembuf sem_buf;
    struct shmid_ds shmid_struct;
    short  sarray[NUMSEMS];
    int *s,  *shm_address;

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
    shmid = shmget(shmkey, SIZEOFSHMSEG * LINESIZE, 0666 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1) {
        printf("main2: shmget() failed\n");
        return -1;
    }
    /* Attach the shared memory segment to the server process.       */
    shm_address = (int *) shmat(shmid, 0, 0);
    if ( shm_address== (int *) -1 ) {
        printf("main: shmat() failed\n");
        return -1;
    }


    // TEST
    /*for (int i = 5; i < (12); i++) {
        if (i !=10)
            shm_address[i] = i;
    }*/



    printf("Initializer DONE\n");
    write_to_log(SIZEOFSHMSEG);


    /*Set the shared memory segment for use using the semaphore.     */
    sem_buf.sem_num = 0;
    sem_buf.sem_op =  0;
    sem_buf.sem_flg = IPC_NOWAIT;

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
