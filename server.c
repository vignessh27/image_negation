#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define IMAGE_SIZE 307200 // 640*480
#define QUEUE_SIZE 100

#define READY           0
#define CLIENT_TAKEN    1
#define CLIENT_FILLED   2
#define CLIENT_DONE     3 
#define SERVER_DONE     4

struct Memory {
     unsigned int pid;
     unsigned short front;
     unsigned short rear;
     unsigned short status;
     unsigned int queue[QUEUE_SIZE];
     unsigned char data[IMAGE_SIZE];
};

char* process_data(char* image_data) {
     for(int i=0;i<IMAGE_SIZE;i++)
          image_data[i] = 255 - image_data[i];
}

void  main(int  argc, char *argv[])
{
     key_t          ShmKEY;
     int            ShmID;
     struct Memory  *ShmPTR;
     
     ShmKEY = ftok(".", 3);
     ShmID = shmget(ShmKEY, sizeof(struct Memory), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          printf("errno: %d\n", errno);
          exit(1);
     }
     
     ShmPTR = (struct Memory *) shmat(ShmID, NULL, 0);
     if ((int) ShmPTR == -1) {
          printf("*** shmat error (server) ***\n");
          printf("errno: %d\n", errno);
          exit(1);
     }
     ShmPTR->pid = 0;
     ShmPTR->front = 0;
     ShmPTR->rear = 0;
     ShmPTR->status = READY;
     memset(ShmPTR->data, 0, IMAGE_SIZE);
     memset(ShmPTR->queue, 0, QUEUE_SIZE);

     while(1) {
          // waiting if queue is empty
          while(ShmPTR->front == ShmPTR->rear);

          // getting the first element in queue
          ShmPTR->pid = ShmPTR->queue[ShmPTR->rear%100];
          ShmPTR->rear++;
          if(ShmPTR->rear==100)
               ShmPTR->rear = 0;

          // waiting for client to copy the data
          while(ShmPTR->status != CLIENT_FILLED);

          // processing the image
          printf("\nClient %d: Processing\n", ShmPTR->pid);
          process_data(ShmPTR->data);
          sleep(5);
          ShmPTR->status = SERVER_DONE;

          // waiting for client's completion
          while(ShmPTR->status != CLIENT_DONE)
               sleep(1);
          printf("Client %d: Done\n", ShmPTR->pid);
     }

     shmdt((void *) ShmPTR);
     shmctl(ShmID, IPC_RMID, NULL);
     exit(0);
}
