#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

long long counter = 0;
int yield_flag = 0;
pthread_mutex_t mutex;
char sync_type = 'n'; //default no lock n
int num_i = 1;
int s_lock = 0; // spin lock

void add(long long* pointer, long long value) {
	long long sum = *pointer + value;
	if (yield_flag) {
		sched_yield();
	}
	*pointer = sum;
	//printf("%lld\n",*pointer);
}


void* func() {
	int i = 0;
	long long value = 1;
	//perform adding +1
	for (i =0; i< num_i; i++) {
 	  if (sync_type == 'm') {
		//mutex
		//printf("i'm m\n");
		pthread_mutex_lock(&mutex);
		add(&counter,value);
		pthread_mutex_unlock(&mutex);
	}
	else if (sync_type == 's') {
	//spin lock
		//printf("i'm s\n");
		while(__sync_lock_test_and_set(&s_lock,1));
		add(&counter,value);
		__sync_lock_release(&s_lock);
	}
	else if (sync_type == 'c') {
	//printf("i'm c\n");
	//compare and swap
		long long old;
		long long new;
		do {
			old = counter;
			new = old + value;
			if (yield_flag) {
				sched_yield();
			}
		}while (__sync_val_compare_and_swap(&counter, old, new) != old);

	}
	else {
		//no lock
		//printf("i'm n\n");

		add(&counter,value);
	}
	}
	value = -1;
	//perform adding -1
	for (i = 0; i <num_i; i++) {
		if (sync_type == 'm') {
		//mutex
		//printf("i'm m\n");
		pthread_mutex_lock(&mutex);
		add(&counter,value);
		pthread_mutex_unlock(&mutex);
	}
	else if (sync_type == 's') {
	//spin lock
	//	printf("i'm s\n");
		while(__sync_lock_test_and_set(&s_lock,1));
		add(&counter,value);
		__sync_lock_release(&s_lock);
	}
	else if (sync_type == 'c') {
	//printf("i'm c\n");
	//compare and swap
		long long old;
		long long new;
		do {
			old = counter;
			new = old + value;
			if (yield_flag) {
				sched_yield();
			}
		}while (__sync_val_compare_and_swap(&counter, old, new) != old);

	}
	else {
		//no lock
		//printf("i'm n\n");
		add(&counter,value);
	}				


	}
	return NULL;
}


void determine_lock(char* optarg) {
	if (strlen(optarg) > 1) {
		fprintf(stderr, "Error: wrong options\n");
		exit(1);
	}
				if (optarg[0] == 'm') {
					sync_type = 'm';
				}
				else if (optarg[0] =='s') {
					sync_type = 's';
				}
				else if( optarg[0]== 'c') {
					sync_type = 'c';
				}
				else {
					fprintf(stderr, "Erorr: Wrong options.\n");
					exit(1);
			}
//printf("lock type: %c\n", sync_type);
}


int main(int argc, char* argv[]) {

int opt;
int num_th = 1;
struct timespec start_time;
struct timespec end_time;


static struct option long_options[] = 
{
	{"iterations", required_argument, NULL,'i'},
	{"threads", required_argument, NULL,'t'},
	{"sync",required_argument, NULL, 's'},
	{"yield", no_argument, NULL, 'y'},
	{0,0,0,0}
};


while( (opt = getopt_long(argc, argv, ":i:t:s:y;", long_options, NULL)) != -1) {
	switch(opt) {
		case 'i':
			num_i = atoi(optarg);
			break;
		case 't':
			num_th = atoi(optarg);
			break;
		case 's':
			determine_lock(optarg);
			break;
		case 'y':
			yield_flag = 1;
			break;
		case ':':
			fprintf(stderr, "%s: option -%c requires an argument\n",argv[0],optopt);
			exit(1);
			break;
		case '?':
			fprintf(stderr, "option -%c is invalid. Correct usage: --threads=number --iterations=number",optopt);
			exit(1);
			break;			
	}
}



if (sync_type == 'm') {
	//initialize mutex
	if (pthread_mutex_init(&mutex, NULL)) {
		fprintf(stderr, "ERROR: initialize mutex");
		exit(1);
	}		
}

pthread_t *tid = malloc(num_th * sizeof(pthread_t));
int i;
int error;

//creating threads
clock_gettime(CLOCK_MONOTONIC, &start_time);
for (i = 0; i <num_th; i++) {
	error = pthread_create(&tid[i],NULL,func,NULL);
	//printf("%lld\n",counter);
	if (error) {
		fprintf(stderr, "Error: creating threads\n");
		exit(1);
	}
}
// wait for threads to finish
for (i = 0; i<num_th; i++) {
	error = pthread_join(tid[i],NULL);
	if (error) {
		fprintf(stderr, "Error: waiting for threads to join.\n");
		exit(1);
	}	
}

clock_gettime(CLOCK_MONOTONIC, &end_time);

long long el_time = 1000000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec);

long operations = num_i * num_th * 2;
long average_op_time = el_time / (1.0 * operations);

char* output;
//get report tags
if (yield_flag) {
	//add yield
	switch(sync_type) {
		case 'm': 
			output = "-yield-m";
			 break; 
		case 's':
			output = "-yield-s";
			break;
		case 'c':
			output = "-yield-c";
			break;
		case 'n':
			output = "-yield-none";
			break;
	}
}
else {	
	switch(sync_type) {
		case 'm': 
			output = "-m";
			 break; 
		case 's':
			output = "-s";
			break;
		case 'c':
			output = "-c";
			break;
		case 'n':
			output = "-none";
			break;
	}
}

printf("add%s,%d,%d,%ld,%lld,%ld,%lld\n",output,num_th,num_i,operations,el_time,average_op_time,counter);
free(tid);
exit(0);
}

