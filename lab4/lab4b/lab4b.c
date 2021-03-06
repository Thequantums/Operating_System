#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <mraa.h>
#include <mraa/aio.h>

int stop = 0;
char* log_file = NULL;
FILE *fptr;
char scale = 'C';
int period = 1; 
float temperature;

mraa_aio_context sensor;
mraa_gpio_context button;

float get_temperature() {
	int a = mraa_aio_read(sensor);
	int b = 4275;
	float R = 1023.0 /a - 1.0;
	R = 100000.0 * R; 
	float temp_c = 1.0 /( log(R/100000.0)/b + 1/298.15) - 273.15;
	if (scale == 'F') {
		
		temp_c = (temp_c* 9/5) + 32;
		return temp_c;	
	}
	else {
		return temp_c;
	}
}


void turnoff() {
struct tm* time_info;
time_t rawtime;
	time(&rawtime);

time_info = localtime(&rawtime);
printf("%02d:%02d:%02d SHUTDOWN\n",time_info->tm_hour,time_info->tm_min,time_info->tm_sec);
if (log_file != NULL) {
	fprintf(fptr,"%d:%d:%d SHUTDOWN\n",time_info->tm_hour,time_info->tm_min,time_info->tm_sec);
}
exit(0);
}

void process_command(char* command) {
	int i = 0;
	int len = strlen(command);
	int begin = 0;
	//processing multiple  commands
    for (i = 0; i < len; i++) { 
      if (command[i] == '\n') {
		command[i] = '\0';
	if (!strcmp(command+begin, "START")) {
		stop =0;
		if (log_file !=NULL) {
			fprintf(fptr,"%s\n",command + begin);
			fflush(fptr);
		}
	}
	else if (!strcmp(command+begin, "STOP")) {
		stop =1;
		if (log_file !=NULL) {
			fprintf(fptr,"%s\n",command + begin);
			fflush(fptr);
		}		
	}
	else if (!strcmp(command+begin, "OFF")) {
		if (log_file !=NULL) {
			fprintf(fptr,"%s\n",command + begin);
			fflush(fptr);
		}
		turnoff();
	}
	else if (!strcmp(command + begin,"SCALE=F")) {
		scale = 'F';
		if (log_file !=NULL) {
			fprintf(fptr,"%s\n",command + begin);
			fflush(fptr);
		}
	}
	else if (!strcmp(command + begin,"SCALE=C")) {
		scale = 'C';
		if (log_file !=NULL) {
			fprintf(fptr,"%s\n",command + begin);
			fflush(fptr);
		}
	}
	else if (command[begin] == 'L' && command[begin + 1] == 'O' && command[begin + 2] == 'G') {
			if (log_file != NULL) {
				fprintf(fptr, "%s\n",command + begin);
				fflush(fptr);
			}
	}
	else if (command[begin] == 'P' && command[begin + 1] == 'E' && command[begin + 2] == 'R' && command[begin + 3] == 'I' && command[begin + 4] == 'O' && command[begin + 5] == 'D') {
		char dest[5]; // digits max
		int str_len = strlen(command + begin);
		strncpy(dest,&command[begin + 7],str_len - 7);
		dest[str_len - 7] = '\0';
		period = atoi(dest);
		if(log_file != NULL) {
			fprintf(fptr, "%s\n",command+begin);
			fflush(fptr);
		}
	}
	else {
		fprintf(stderr, "ERROR: wrong commands.\n");
		exit(1);
	}	
     	begin = i + 1;
      }
    }	
}

int main(int argc, char* argv[]) {

int opt;
struct timeval p_time; // getting time interval
struct tm *time_info; // getting local time
time_t interval = 0;
time_t rawtime;
int num_read = 0;

//initialize
sensor = mraa_aio_init(1);
button = mraa_gpio_init(60);
mraa_gpio_dir(button, MRAA_GPIO_IN);

static struct option long_options[] = 
{
	{"period", required_argument, NULL,'p'},
	{"scale", required_argument, NULL,'s'},
	{"log", required_argument, NULL, 'l'},
	{0,0,0,0}
};


while( (opt = getopt_long(argc, argv, ":s:p:l:;", long_options, NULL)) != -1) {
	switch(opt) {
		case 'p':
			period = atoi(optarg);
			break;
		case 's':
			scale = optarg[0];
			if (scale != 'C' && scale != 'F') {
				fprintf(stderr, "ERROR: wrong scale argument\n");
				exit(1);
			}
			break;
		case 'l':
			log_file = optarg;
			break;
		case ':':
			fprintf(stderr, "%s: option -%c requires an argument\n",argv[0],optopt);
			exit(1);
			break;
		case '?':
			fprintf(stderr, "option -%c is invalid. Correct usage: --input=filename --output=filename --segfault --catch",optopt);
			exit(1);
			break;			
	}
}

//open file
fptr = fopen(log_file,"w");
// poll
struct pollfd pfds[1];
pfds[0].fd = 0;
pfds[0].events = POLLIN | POLLERR | POLLHUP;
char* input = malloc(sizeof(char) * 1024); // to read fron stdin

while(1) {

		if (mraa_gpio_read(button)) {
			turnoff();
		}

	gettimeofday(&p_time, 0);
	temperature = get_temperature();
	if (!stop && p_time.tv_sec >= interval) {
		interval = p_time.tv_sec + period;
		time(&rawtime);
		time_info = localtime(&rawtime);
		printf("%02d:%02d:%02d %.1f\n",time_info->tm_hour,time_info->tm_min,time_info->tm_sec,temperature);
		if (log_file != NULL) {
			fprintf(fptr,"%02d:%02d:%02d %.1f\n",time_info->tm_hour,time_info->tm_min,time_info->tm_sec,temperature);
		}
	}
	if (poll(pfds,1,0) < 0 ) {
		fprintf(stderr, "ERROR: creating poll\n");
		exit(1);
	}
	if ( pfds[0].revents & POLLIN ) {
		memset(input,0,1024); // clear buffer
		num_read = read(0,input,1024);
		if (num_read < 0 ) {
			fprintf(stderr,"Error: reading error.\n");
		}
		input[num_read] = '\0';
		//process input
		process_command(input); //possibly many or partial
	} 

}

mraa_aio_close(sensor);
mraa_gpio_close(button);

fclose(fptr);
exit(0);
}



