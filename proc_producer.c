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

/* The parameter type specifies the type of message:
      1: Successfull allocation
      2: The process couldn't find space in memory
      3: The process frees a memory segment*/
void write_to_log(char* msg, struct processInfo *args, int type){
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

  if (type == 1){
    fprintf(fp,
      msg,
      args->base_register,
      args->base_register + args->size,
      syscall(SYS_gettid),
      s);
    fflush(fp);
  }else if (type == 2){
    fprintf(fp,
      msg,
      syscall(SYS_gettid),
      s);
    fflush(fp);
  }else if (type == 3){
    fprintf(fp,
      msg,
      syscall(SYS_gettid),
      args->base_register,
      args->base_register + args->size,
      s);
    fflush(fp);
  }

  fclose(fp);

  /*Releasing the semaphore*/
  sem_post(&mutex);
}


void* allocate_memory(void* pInfo){
  struct processInfo *args = (struct processInfo *)pInfo;
  printf("Hi. I'm the process %li\n", syscall(SYS_gettid));
  printf("I am going to sleep now \n");
  for (int i = 0; i<args->execution_time;i++){
    printf(".");//("%d",args->size);
    fflush(stdout);
    sleep(1);
  }
  printf("\nI'm wide awake now. Bye \n");

  if (selected_algorithm == BEST){
    char* msg = "Test ";
    printf("Algoritmo Best-fit\n\n");

    //if the thread finds space in memory
    write_to_log("The memory segment from addresses %d to %d was allocated to the process/thread %li on %s\n", args, 1);

    //if the thread releases its assigned memory segment
    write_to_log("The process %li freed the memory segment from addresses %d to %d on %s\n", args, 3);

    //if the thread doesn't find s
    write_to_log("The process %li couldn't find space in memory and died on %s\n", args, 2);
  }else if (selected_algorithm == FIRST){
    printf("Algoritmo First-fit\n\n");
  }else if (selected_algorithm == WORST){
    printf("Algoritmo Worst-fit\n\n");
  }

}

void* print_process_state(void* pInfo){
  struct processInfo *args = (struct processInfo *)pInfo;
  printf("Hi. I'm the process %li\n", syscall(SYS_gettid));
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

void write_log(int exec_time, int proc_size){
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


void connect_shared_memory() {
  struct sembuf operations[2];
  void          *shm_address;
  int           semId,shmId, retval;
  key_t         semKey, shmKey;

  /* Generate the IPC key for the semaphore and memory segment */
    semKey = ftok(SEMKEYPATH,SEMKEYID);
    if ( semkey == (key_t)-1 )
      {
        printf("main: ftok() for sem failed\n");
        return -1;
      }
    shmKey = ftok(SHMKEYPATH,SHMKEYID);
    if ( shmKey == (key_t)-1 )
      {
        printf("main: ftok() for shm failed\n");
        return -1;
      }
}

int main(){
    int proc_size, exec_time, producer_wait;
    sem_init(&mutex, 0, 1);



    enum AlgorithmType algorithmNumber;

    /* Cycle for validating user's input */
    while (1){
      printf(" 1) Best-fit \n 2) First-fit \n 3) Worst-fit \n");
      printf("Select the algorithm: ");
      scanf("%d",&algorithmNumber);
      if (algorithmNumber > 0 && algorithmNumber < 4){
        break;
      }else{
        printf("\n ERROR \n Ingrese un numero del 1 al 3 \n");
      }
    }


    selected_algorithm = algorithmNumber;

    /* Cycle for creating the threads */
    while (1){
      proc_size = rand() % 10 + 1;
      exec_time = (rand() % (60 - 20 + 1)) + 20; //num = (rand() % (upper – lower + 1)) + lower

      pthread_t PID;// = pthread_self();
      struct processInfo pinfo = {PID, 0, proc_size, exec_time, BLOCKED};
      pthread_create(&PID, NULL, allocate_memory, &pinfo);
      write_log(exec_time, proc_size);
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
