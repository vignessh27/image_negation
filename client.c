#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_writer.h"

#define IMAGE_SIZE 307200 // 640*480
#define QUEUE_SIZE 100

#define READY            0
#define CLIENT_TAKEN     1
#define CLIENT_FILLED    2
#define CLIENT_DONE      3 
#define SERVER_DONE      4

struct Memory {
     unsigned int pid;
     unsigned short front;
     unsigned short rear;
     unsigned short status;
     unsigned int queue[QUEUE_SIZE];
     unsigned char data[IMAGE_SIZE];
};

void  main(int argc, char *argv[])
{
     key_t          ShmKEY;
     int            ShmID;
     struct Memory  *ShmPTR;
     
     int height, width, channel, length, pid;
     char output_image[20];
     unsigned char* image_data = stbi_load("input_image.png", &width, &height, &channel, 1);
     if(width!=640 || height!=480) {
          printf("Image size must be 640x480 pixels, not %dx%d\n", width, height);
          exit(1);
     }
     if(channel!=1) {
          printf("Image must be a grayscale image (1 channel), not %d channel\n", channel);
          exit(1);
     }
     pid = getpid();

     // getting the shared memory
     ShmKEY = ftok(".", 3);
     ShmID = shmget(ShmKEY, sizeof(struct Memory), 0666);
     if (ShmID < 0) {
          printf("*** shmget error (client) ***\n");
          printf("errno: %d\n", errno);
          exit(1);
     }     
     ShmPTR = (struct Memory *) shmat(ShmID, NULL, 0);
     if ((int) ShmPTR == -1) {
          printf("*** shmat error (client) ***\n");
          printf("errno: %d\n", errno);          
          exit(1);
     }

     // adding this process to shared queue
     printf("\nPid: %d\n", pid);
     ShmPTR->queue[ShmPTR->front%100] = pid;
     ShmPTR->front++;
     if(ShmPTR->front==100)
          ShmPTR->front = 0;
     printf("Locking\n");

     // waiting for the slot from server
     while (ShmPTR->pid != pid)
          sleep(1);
     printf("Processing\n");

     // copying input image to shared memory
     memcpy(ShmPTR->data, image_data, IMAGE_SIZE);
     ShmPTR->status = CLIENT_FILLED;

     // waiting for server to finish processing
     while (ShmPTR->status != SERVER_DONE)
          sleep(1);

     // writing the processed image
     sprintf(output_image, "output_%d.png", pid);
     stbi_write_png(output_image, width, height, channel, ShmPTR->data, width*channel);
     ShmPTR->status = CLIENT_DONE;

     printf("Done\n");
     shmdt((void *) ShmPTR);
     exit(0);
}
