#include <stdio.h>


int main()
{
    enum AlgorithmType{BEST=1, FIRST=2, WORST=3};
    int proc_size, exec_time,producer_wait;

    enum AlgorithmType algorithmNumber;
    printf(" 1) Best-fit \n 2) First-fit \n 3) Worst-fit \n");
    printf("Select the algorithm: ");
    scanf("%d",&algorithmNumber);

    printf("Number = %d\n",(int)algorithmNumber);

    while (1){
      proc_size = rand() % 10 + 1;
      exec_time = (rand() % (60 - 20 + 1)) + 20; //num = (rand() % (upper – lower + 1)) + lower
      printf("create_thread with %d lines and %d s of execution time\n", proc_size, exec_time);

      producer_wait = (rand() % (60 - 30 + 1)) + 30;
      sleep(producer_wait);

    }

    return 0;
}

//Bibliography
//https://www.geeksforgeeks.org/generating-random-number-range-c/
