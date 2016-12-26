#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define NUMREAD 10			// Number of Readers
#define NUMWRITE 10			// Number of Writers

#define DEVICE_NAME "/dev/DUMMY_DEVICE"
#define NUMLOOP 2 
#define NUMMAX 10


/* Writer function */
/* arg - writer thread's index */
void *write_func(void *arg) {
    int fd; 
    int i;
    int value;
    int index;

    index = *(int *)arg;
    //open a device
    fd = open(DEVICE_NAME, O_RDWR);
    if(fd <= 0)
    {
        fprintf(stderr, "writer %d failed to open\n", index);
        return NULL;
    }

    //write a random value NUMPLOOP times
    for (i = 0; i < NUMLOOP; i++) {
        value = rand() % NUMMAX ;
        write(fd, &value, 1);
        printf("writer %d: write value %d\n", index, value);
    }

}

/* Reader function */
/* arg - reader thread's index */
void *read_func(void *arg) {
    int fd; 
    int i;
    int value;
    int index;

    index = *(int *)arg;
    //open a device
    fd = open(DEVICE_NAME, O_RDWR);
    if(fd <= 0)
    {
        fprintf(stderr, "reader %d failed to open\n", index);
        return NULL;
    }

    //read a random value NUMLOOP times
    for (i = 0; i < NUMLOOP; i++) {
        read(fd, &value, 1);
        printf("reader %d: read value %d\n", index, value);
    }
}

int main(void)
{
	pthread_t read_thread[NUMREAD];
	pthread_t write_thread[NUMWRITE];
    int i;
    int read_ids[NUMREAD], write_ids[NUMWRITE];

    //get a random seed
    srand(time(NULL));

    //create all writer thread
    for (i = 0; i < NUMREAD; i++) {
        write_ids[i] = i;
        pthread_create(&write_thread[i], NULL, write_func, &write_ids[i]); 
    }

    //for test convenience, I first start all writer threads and sleep for a second. 
    sleep(1);

    //create all reader threads
    for (i = 0; i < NUMWRITE; i++) {
        read_ids[i] = i;
        pthread_create(&read_thread[i], NULL, read_func, &read_ids[i]);
    }

    //wait for writer threads to be terminated
    for (i = 0; i < NUMWRITE; i++) {
        pthread_join(write_thread[i], NULL);
    }

    //wait for readder threads to be terminated
    for (i = 0; i < NUMREAD; i++) {
        pthread_join(read_thread[i], NULL);
    }

	return 0;
}


