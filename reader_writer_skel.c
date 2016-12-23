#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define NUMREAD 10			// Number of Readers
#define NUMWRITE 10			// Number of Writers

#define DEVICE_NAME "/dev/DUMMY_DEVICE"
#define NUMLOOP 10 


/* Writer function */
void *write_func(void *arg) {
    int fd; 
    int i;
    int value;
    int index;

    index = *(int *)arg;
    fd = open(DEVICE_NAME, O_RDWR);
    if(fd <= 0)
    {
        fprintf(stderr, "writer %d failed to open\n", index);
        return NULL;
    }

    for (i = 0; i < NUMLOOP; i++) {
        value = rand();
        write(fd, &value, 1);
        printf("writer %d: write value %d\n", index, value);
    }

}

/* Reader function */
void *read_func(void *arg) {
    int fd; 
    int i;
    int value;
    int index;

    index = *(int *)arg;
    fd = open(DEVICE_NAME, O_RDWR);
    if(fd <= 0)
    {
        fprintf(stderr, "reader %d failed to open\n", index);
        return NULL;
    }

    for (i = 0; i < NUMLOOP; i++) {
        value = rand();
        read(fd, &value, 1);
        printf("reader %d: write value %d\n", index, value);
    }
}

int main(void)
{
	pthread_t read_thread[NUMREAD];
	pthread_t write_thread[NUMWRITE];
    int i;
    int read_ids[NUMREAD], write_ids[NUMWRITE];

    srand(time(NULL));
    for (i = 0; i < NUMWRITE; i++) {
        write_ids[i] = i;
        pthread_create(&write_thread[i], NULL, read_func, &write_ids[i]);
    }

    for (i = 0; i < NUMREAD; i++) {
        read_ids[i] = i;
        pthread_create(&read_thread[i], NULL, write_func, &read_ids[i]); 
    }

    for (i = 0; i < NUMWRITE; i++) {
        pthread_join(write_thread[i], NULL);
    }

    for (i = 0; i < NUMREAD; i++) {
        pthread_join(read_thread[i], NULL);
    }

	return 0;
}


