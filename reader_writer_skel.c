#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#define NUMREAD 10			// Number of Readers
#define NUMWRITE 10			// Number of Writers

#define DEVICE_NAME "/dev/DUMMY_DEVICE"
#define NUMLOOP 100


/* Writer function */
void *write_func(void *arg) {

}

/* Reader function */
void *read_func(void *arg) {

}

int main(void)
{
	pthread_t read_thread[NUMREAD];
	pthread_t write_thread[NUMWRITE];


	return 0;
}


