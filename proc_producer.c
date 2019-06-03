#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>


#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 1              /* Id used on ftok for shmget key    */

#define NUMSEMS 1               /* Num of sems in created sem set    */
int SIZEOFSHMSEG;           /* Size of the shared mem segment    */
#define BLOCKSIZE 10

#define NUMMSG 2                /* Server only doing two "receives"
                                   on shm segment                    */

#define SNAME "/state_sem"
#define MAXCHAR 10000

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

/* Struct used to represent a block of avaiable memory */
struct memoryBlock {
  int                   start;            /* Start position of the memory block  inside the memory segment*/
  int                   avaiable_spaces;   /* Amount of avaiable bytes in the memory block */
  struct memoryBlock   *next;
};

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

void push(struct memoryBlock * head, int _start,int _avaiable_spaces) {
    struct memoryBlock *current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = malloc(sizeof(struct memoryBlock));
    current->next->start = _start;
    current->next->avaiable_spaces = _avaiable_spaces;
    current->next->next = NULL;
}


void change_state(char* state){
  FILE *fp;
  char* filename = "states.txt";
  char* msg = "%d - %s\n";
  sem_t *sem = sem_open(SNAME, O_CREAT, 0644, 3);
  /*Waiting for the semaphore*/
  sem_wait(sem);
  /*Reading file and creating maze */
  fp = fopen(filename, "a");
  if (fp == NULL) {
      printf("Could not open file %s",filename);
  }

  fprintf(fp,
    msg,
    syscall(SYS_gettid),
    state);


  fclose(fp);

  /*Releasing the semaphore*/
  sem_post(sem);
}

struct memoryBlock* create_memory_structure(int *_memory_segment, int _memory_size) {
  int *memory;
  int memory_size, i = 0,  start, avaiable_spaces, flag = 1;
  memory = _memory_segment;
  memory_size = _memory_size;

  //first memory block
  struct memoryBlock * head = NULL;
  head = malloc(sizeof(struct memoryBlock));

  
  while (i < memory_size) {
    if (memory[i] == 0) {
        avaiable_spaces = 0;
        start = i;
        while ((i < memory_size) && (memory[i] == 0)) {
            avaiable_spaces++;
            i++;
        }
        if (flag) {
            head->start = start;
            head->avaiable_spaces = avaiable_spaces;
            head->next = NULL;
            flag = 0;
        } else {
            push(head,start,avaiable_spaces);
        }

    }
    i++;
  }
  return head;
}

void print_list(struct memoryBlock * head) {
    struct memoryBlock * current = head;

    while (current != NULL) {
        printf("START:  %d\n", current->start);
        printf("SPACES: %d\n", current->avaiable_spaces);
        
        current = current->next;
    }
}

void best_fit(int *_memory, struct processInfo *args,int _memory_size) {

    int space_size, min_space = _memory_size, position_selected, thread_size, memory_block_pos, i,
            thread_id, size_difference;
    struct memoryBlock *current, *head;
    int *memory;

    head = create_memory_structure(_memory, _memory_size);
    thread_size = args->size;
    thread_id = syscall(SYS_gettid);
    memory = _memory;
    current = head;


    while (current != NULL) {
        space_size = current->avaiable_spaces;
        //printf("Space size: %d\n",space_size);
        memory_block_pos = current->start;
        //printf("Memory Block pos%d\n",memory_block_pos);

        size_difference = space_size - thread_size;
        //printf("Size difference: %d\n", size_difference);
        current = current->next;
        if ((size_difference < min_space) && (size_difference >= 0)) {
            min_space = size_difference;
            position_selected = memory_block_pos;
        }

    }

    //printf("START POS SELECTED %d\n", position_selected);
    //printf("MIN SIZE DIFFERENCE %d\n",  min_space);
    
    i = position_selected;
    while (i <= thread_size) {
        memory[i] = thread_id;
        //printf("Memory: %d\n In pos: %d", memory[i], i );
        i++;
    }

}


void worst_fit(int *_memory, struct processInfo *args,int _memory_size) {

    int space_size, max_space = 0, position_selected, thread_size, memory_block_pos, i,
            thread_id, size_difference;
    struct memoryBlock *current, *head;
    int *memory;

    head = create_memory_structure(_memory, _memory_size);
    thread_size = args->size;
    thread_id = syscall(SYS_gettid);
    memory = _memory;
    current = head;


    while (current != NULL) {
        space_size = current->avaiable_spaces;
        memory_block_pos = current->start;

        size_difference = space_size - thread_size;
        current = current->next;
        if (size_difference > max_space) {
            max_space = size_difference;
            position_selected = memory_block_pos;
        }

    }
    i = position_selected;
    while (i < thread_size) {
        memory[i] = thread_id;
        i++;
    }


}



void first_fit(int *_memory, struct processInfo *args,int _memory_size) {
  int *memory;
  int size,temp ,memory_size, i = 0, counter = 0, flag = 1, thread_id;


  memory = _memory;
  size = args->size;
  thread_id = syscall(SYS_gettid);
  memory_size = _memory_size;

 while (i < memory_size) {
    if (memory[i] == 0) {
        temp = i;
        flag = 1;
        for (counter = 0; counter < size; counter++) {
            if (memory[temp] != 0) {
                flag = 0;
            } else {
                temp++;
            }
        }


        if (flag) {
            //args->base_register = i;
            temp = i;
            for (counter = 0; counter < size; counter++) {
                memory[temp] =  thread_id;
                temp++;
            }
            break;
        } else {
            i = temp;
        }
    }
    i++;
 }
}




/*
Releases the memory used by a certain thread
Parameters:
memory    = shared memory segment used
thread_id = thread id used to release memory
memory size

*/
void release_memory(int *_memory, int _thread_id, int _memory_size) {
    int *memory;
    memory = _memory;

    for (int i = 0; i < _memory_size; i++) {
        if (memory[i] == _thread_id) {
            //Freeing memory
            memory[i] = 0;
        }
    }

}



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

/*void* print_process_state(void* pInfo){
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
}*/

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


int end_thread_memory(struct processInfo *args,int _memory_size) {
  struct sembuf operations[2];
  char         *pointer;
  int           *shm_address;
  int           semid,shmid, retval;
  key_t         semkey, shmkey;


  // Thread sleeps
  sleep(args->execution_time);
  //sleep(1);
  /* Generate the IPC key for the semaphore and memory segment */
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

  semid = semget( semkey, NUMSEMS, 0666);
  if ( semid == -1 ) {
      printf("main: semget() failed\n");
      return -1;
  }

  /* Get the created shared memory ID associated with the key */
  shmid = shmget(shmkey, SIZEOFSHMSEG, 0666);
  if (semid == -1) {
    printf("main: shmget() failed\n");
    return -1;
  }

 /* Memory segmente  */
  shm_address = (int *) shmat(shmid, 0, 0);


  operations[0].sem_num = 0;
  operations[0].sem_op =  0;
  operations[0].sem_flg = 0;

  operations[1].sem_num = 0;
  operations[1].sem_op =  1;
  operations[1].sem_flg = 0;

  retval = semop( semid, operations, 2 );
  if (retval == -1) {
    printf("main: semop() failed\n");
    return -1;
  }


  // memory released
  release_memory(shm_address,syscall(SYS_gettid), _memory_size);


  operations[0].sem_num = 0;
  operations[0].sem_op =  -1;
  operations[0].sem_flg = 0;


  retval = semop( semid, operations, 1 );
  if (retval == -1) {
    printf("main: semop() failed\n");
    return -1;
  }




  /* Detach the shared memory segment from the current process.    */
  retval = shmdt(shm_address);
  if (retval==-1) {
    printf("main: shmdt() failed\n");
    return -1;
  }



}



/*
Method used to connect to the shared memory segment used throught the procceses.
IPC
*/
int connect_shared_memory(enum AlgorithmType _type, struct processInfo *args, int _memory_size) {
  struct sembuf operations[2];
  char          *pointer;
  int           *shm_address,semid,shmid, retval;
  key_t         semkey, shmkey;

  /* Generate the IPC key for the semaphore and memory segment */
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

  semid = semget( semkey, NUMSEMS, 0666);
  if ( semid == -1 ) {
      printf("main: semget() failed\n");
      return -1;
  }

  /* Get the created shared memory ID associated with the key */
  shmid = shmget(shmkey, SIZEOFSHMSEG, 0666);
  if (semid == -1) {
    printf("main: shmget() failed\n");
    return -1;
  }

 /* Memory segmente  */
 shm_address = (int*)shmat(shmid, NULL, 0);
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

  retval = semop( semid, operations, 2 );
  if (retval == -1) {
    printf("main: semop() failed\n");
    return -1;
  }



  /*  Used by threads       */
  if (_type == FIRST) {
    first_fit(shm_address, args, _memory_size);
      /*if (first_fit(shm_address, args, _memory_size)){
        write_to_log("The memory segment from addresses %d to %d was allocated to the process/thread %li on %s\n", args, 1);
      }else{
        write_to_log("The process %li couldn't find space in memory and died on %s\n", args, 2);
      }*/
  } else if (_type == WORST) {
      worst_fit(shm_address, args, _memory_size);
  } else if (_type == BEST) {
      best_fit(shm_address, args, _memory_size);
  }

  /*
  pointer = shm_address + 3;
  for (char A ='A'; A < 'G'; A++) {
    *pointer = A;
    pointer++;
  }
  */

  /* Release the shared memory segment by decrementing the in-use  */
  /* semaphore (the first one).  Increment the second semaphore to */
  /* show that the client is finished with it.                     */
  operations[0].sem_num = 0;
                                      /* Operate on the first sem      */
  operations[0].sem_op =  -1;
                                      /* Decrement the semval by one   */
  operations[0].sem_flg = 0;
                                      /* Allow a wait to occur         */


  retval = semop( semid, operations, 1 );
  if (retval == -1) {
    printf("main: semop() failed\n");
    return -1;
  }




  /* Detach the shared memory segment from the current process.    */
  retval = shmdt(shm_address);
  if (retval==-1) {
    printf("main: shmdt() failed\n");
    return -1;
  }

  //ending thread
  end_thread_memory(args, _memory_size);

}


void* allocate_memory(void* pInfo){
  struct processInfo *args = (struct processInfo *)pInfo;
  printf("Hi. I'm the process %li\n", syscall(SYS_gettid));
  //printf("Size of the memory: %d \n", SIZEOFSHMSEG);
  /*for (int i = 0; i<args->execution_time;i++){
    printf(".");//("%d",args->size);
    fflush(stdout);
    sleep(1);
  }*/
  //printf("\nI'm wide awake now. Bye \n");

  if (selected_algorithm == BEST){
    char* msg = "Test ";
    //printf("Algoritmo Best-fit\n\n");

    //Memory segment
    connect_shared_memory(BEST, args, SIZEOFSHMSEG);

    //if the thread finds space in memory
    //write_to_log("The memory segment from addresses %d to %d was allocated to the process/thread %li on %s\n", args, 1);

    //if the thread releases its assigned memory segment
    //write_to_log("The process %li freed the memory segment from addresses %d to %d on %s\n", args, 3);

    //if the thread doesn't find s
    //write_to_log("The process %li couldn't find space in memory and died on %s\n", args, 2);
  }else if (selected_algorithm == FIRST){

    //Memory segment
    connect_shared_memory(FIRST, args, SIZEOFSHMSEG);

    //printf("Algoritmo First-fit\n\n");
  }else if (selected_algorithm == WORST){

    //Memory segment
    connect_shared_memory(WORST, args, SIZEOFSHMSEG);

    //printf("Algoritmo Worst-fit\n\n");
  }


}

int main(){
    int proc_size, exec_time, producer_wait;
    SIZEOFSHMSEG = get_memory_size();
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
        fflush(stdout);

      proc_size = rand() % 10 + 1;
      exec_time = (rand() % (60 - 20 + 1)) + 20; //num = (rand() % (upper – lower + 1)) + lower

      pthread_t PID;// = pthread_self();
      struct processInfo pinfo = {PID, 0, proc_size, exec_time, BLOCKED};
      pthread_create(&PID, NULL, allocate_memory, &pinfo);
      write_log(exec_time, proc_size);
      //pthread_join(PID, NULL);

      producer_wait = (rand() % (60 - 30 + 1)) + 30;
      sleep(5);//
      //sleep(producer_wait);
    }

    //pthread_join(PID, NULL);
    sem_destroy(&mutex);
    return 0;
}






//Bibliography
//https://www.geeksforgeeks.org/generating-random-number-range-c/
//http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html
//http://www.zentut.com/c-tutorial/c-write-text-file/
//https://www.geeksforgeeks.org/use-posix-semaphores-c/
