#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>


enum AlgorithmType{BEST=1, FIRST=2, WORST=3};
enum ProcessState{RUNNING, BLOCKED, IN_CRITICAL_REGION};
sem_t mutex;

/* info used by our threadsx */
struct processInfo {

    pthread_t     PID;         /* ID returned by pthread_create() */
    int           base_register;     /* Stores the process' starting memory address */
    int           size;        /* The size (in lines) of the process */
    int           execution_time; /* Amount of time the process is going to sleep */
    enum ProcessState state;   /* The current state of the process */
};

void* best_fit(void* pInfo){

}

void* printProcessState(void* pInfo){
  struct processInfo *args = (struct processInfo *)pInfo;
  printf("Hi. I'm the %lu process\n", args->PID);
  printf("I am going to sleep now \n");
  for (int i = 0; i<args->execution_time;i++){
    printf(".");//("%d",args->size);
    fflush(stdout);
    sleep(1);
  }
  printf("\nI'm wide awake now. Bye \n");
  //pthread_exit((void *)0);
  //sleep(args->execution_time);
}

void writeLog(int exec_time, int proc_size){
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
      "Process/thread created with %d lines and %d s of execution time on %s\n",
      proc_size, exec_time, s);
    fflush(fp);
    fclose(fp);

    /*Releasing the semaphore*/
    sem_post(&mutex);

}
int main(){
    int proc_size, exec_time, producer_wait;
    sem_init(&mutex, 0, 1);



    enum AlgorithmType algorithmNumber;
    printf(" 1) Best-fit \n 2) First-fit \n 3) Worst-fit \n");
    printf("Select the algorithm: ");
    scanf("%d",&algorithmNumber);

    printf("Number = %d\n",(int)algorithmNumber);

    while (1){
      proc_size = rand() % 10 + 1;
      exec_time = (rand() % (60 - 20 + 1)) + 20; //num = (rand() % (upper – lower + 1)) + lower

      pthread_t PID = pthread_self();
      struct processInfo pinfo = {PID, 0, proc_size, exec_time, BLOCKED};
      pthread_create(&PID, NULL, printProcessState, &pinfo);
      writeLog(exec_time, proc_size);
      pthread_join(PID, NULL);

      producer_wait = (rand() % (60 - 30 + 1)) + 30;
      //sleep(1);//
      sleep(producer_wait);

    }
    sem_destroy(&mutex);
    return 0;
}



//Bibliography
//https://www.geeksforgeeks.org/generating-random-number-range-c/
//http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html
//http://www.zentut.com/c-tutorial/c-write-text-file/
//https://www.geeksforgeeks.org/use-posix-semaphores-c/
