#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include "SortedList.h"
#include <string.h>
#include <signal.h>

long long counter = 0;
int yield_flag = 0;
pthread_mutex_t mutex;
char sync_type = 'n'; //default no lock n
int num_i = 1;
int s_lock = 0; // spin lock
int opt_yield = 0; // flag for yield
SortedList_t* list;
int num_elements;
SortedListElement_t* elements;
int num_th = 1;

void* func( void* ID) {
	int start_index = *(int*) ID;
	int i = start_index;
//	printf("%d\n",start_index);
	//insert in list through number of iterations per start index 
	for (; i <num_elements; i = i + num_th) {
		if (sync_type == 'm') {
			//mutex
			pthread_mutex_lock(&mutex);
			SortedList_insert(list, &elements[i]);
			pthread_mutex_unlock(&mutex);
		}
		else if (sync_type == 's') {
			while(__sync_lock_test_and_set(&s_lock,1));
			SortedList_insert(list, &elements[i]);
			__sync_lock_release(&s_lock);
		}
		else {
			// no lock
			SortedList_insert(list, &elements[i]);
//			int llen;
//			llen=SortedList_length(list);
//			printf("length: %d\n",llen);
		}		
	}

	//getting length
	int len;
		if (sync_type == 'm') {
			//mutex
			pthread_mutex_lock(&mutex);
			len = SortedList_length(list);
			if (len == -1) {
				fprintf(stderr,  "Error: corrupted list\n");
				exit(1);
			}
			pthread_mutex_unlock(&mutex);
		}
		else if (sync_type == 's') {
			while(__sync_lock_test_and_set(&s_lock,1));
			len = SortedList_length(list);
			if (len == -1) {
				fprintf(stderr,  "Error: corrupted list\n");
				exit(1);
			}
			__sync_lock_release(&s_lock);
		}
		else {
			// no lock
			len = SortedList_length(list);
			if (len == -1) {
				fprintf(stderr,  "Error: corrupted list\n");
				exit(2);
			}			
		}	

	//lock up and delete 
	SortedListElement_t* node;
	for (i = start_index; i <num_elements; i= i + num_th) {
		if (sync_type == 'm') {
			//mutex
			pthread_mutex_lock(&mutex);
			node = SortedList_lookup(list,elements[i].key);
			if (node == NULL) {
				fprintf(stderr, "error: finding element\n");
				exit(1);
			}
			SortedList_delete(node);
			pthread_mutex_unlock(&mutex);
		}
		else if (sync_type == 's') {
			while(__sync_lock_test_and_set(&s_lock,1));
			node = SortedList_lookup(list,elements[i].key);
			if (node == NULL) {
				fprintf(stderr, "error: finding element\n");
				exit(1);
			}
			SortedList_delete(node);
			__sync_lock_release(&s_lock);
		}
		else {
			// no lock
			node = SortedList_lookup(list,elements[i].key);
			if (node == NULL) {
				fprintf(stderr, "error: finding element\n");
				exit(1);
			}
			SortedList_delete(node);
//			len = SortedList_length(list);
//			printf("length after delete: %d\n",len);
		}		
	}


	return NULL;
}

void setyield(char* str) {
	if (strlen(str) > 3) {
		fprintf(stderr, "Error: too many options for yields\n");
		exit(1);
	}
	int i, len=0;
	len = strlen(str); // minus the [ ]
	for (i =0; i < len; i++) {
		switch (str[i]) {
			case 'i':
				opt_yield = opt_yield | INSERT_YIELD;
				break;
			case 'd':
				opt_yield = opt_yield | DELETE_YIELD;
				break;
			case 'l':
				opt_yield = opt_yield | LOOKUP_YIELD;
				break;
			default:
				fprintf(stderr, "error: Invalid options of yield\n");
				exit(1);
		}
	}

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
				else {
					fprintf(stderr, "Erorr: Wrong options.\n");
					exit(1);
				}
//	printf("sync type: %c\n",sync_type);
}

void handler(){

	fprintf(stderr, "seg fault\n");
	exit(2);
}


int main(int argc, char* argv[]) {

int opt;
struct timespec start_time;
struct timespec end_time;
int i = 0;
char* yield_type;

signal(SIGSEGV, handler);
static struct option long_options[] = 
{
	{"iterations", required_argument, NULL,'i'},
	{"threads", required_argument, NULL,'t'},
	{"sync",required_argument, NULL, 's'},
	{"yield", required_argument, NULL, 'y'},
	{0,0,0,0}
};


while( (opt = getopt_long(argc, argv, ":i:t:s:y:;", long_options, NULL)) != -1) {
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
			yield_type = optarg;
			setyield(optarg);
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

// creating head of circular list
list = malloc(sizeof(SortedList_t));
list->next = list;
list->prev = list;
list->key = NULL;

// initialize elements with random numbers
num_elements = num_i * num_th;
elements = malloc(sizeof(SortedListElement_t) * num_elements);
srand(time(0));
int j = 0;
char* rand_temp = malloc(4 * sizeof(char));
for (i = 0; i < num_elements; i++) {
	for (j =0; j< 3; j++) {
		rand_temp[j] = (rand() % 26) + 'a'; // adding offset 'a' to num btw 0-25
	}
	rand_temp[3] = '\0';
	elements[i].key = rand_temp;
}



pthread_t *tid = malloc(num_th * sizeof(pthread_t));
int error;
int* ID = malloc(sizeof(int) * num_th);

//creating threads
clock_gettime(CLOCK_MONOTONIC, &start_time);
for (i = 0; i <num_th; i++) {
	ID[i] = i;
	error = pthread_create(&tid[i],NULL,func, &ID[i]);
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
long operations = num_i * num_th * 3; //three different ops per thread
double average_op_time = el_time / (1.0 * operations);

char* output = calloc(30,sizeof(char));

//get report tags
if (yield_flag) {
	int length;
	length = strlen(yield_type); // minus the [ ]
	for (i =0; i < length; i++) {
		switch (yield_type[i]) {
			case 'i':
				strcat(output,"i");
				break;
			case 'd':
				strcat(output,"d");
				break;
			case 'l':
				strcat(output,"l");
				break;
			default:
				fprintf(stderr, "error: Invalid options of yield\n");
				exit(1);
		}
	}
}
else {
	strcat(output, "none"); // none for no flag
		
}

	switch(sync_type) {
		case 'm': 
			strcat(output,"-m");
			 break; 
		case 's':
			strcat(output,"-s");
			break;
		default:
			strcat(output,"-none");
			break;
	}

/*
int ii = 0;
char * str_temp = malloc(25);
int lleen = strlen(output);
for (i = 0; i <lleen; i++) {
	if (output[i] == '-' || output[i] == 'm' || output[i] == 's' || output[i] == 'n' || output[i] == 'o' || output[i] == 'e' || output [i] == 'd' || output[i] == 'l' || output[i] == 'i') {
		str_temp[ii] = output[i];
		ii++; 		
	}
}

*/

printf("list-%s,%d,%d,%ld,%lld,%f\n",output,num_th,num_i,operations,el_time,average_op_time);
	if (SortedList_length(list) !=0) {
		printf("There are still elements in the list left\n");
		exit(2);
	}
exit(0);
}

