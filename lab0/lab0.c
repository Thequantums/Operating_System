#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

int openinputfile(char* name) {

	int fd;
	fd = open(name,O_RDONLY);
	if (fd == -1 ) { 
		fprintf(stderr, "%s file cant be open to read",name);
		exit(2);
	}
return fd;
}

int createoutputfile(char* name) {
	int fd;
	fd = creat(name,0644);
	if(fd == -1) {
		fprintf(stderr, "%s file cant be open to write", name);
		exit(3);
	}
return fd;
}

void sigsegv_handler(int sig) {
	fprintf(stderr, "Segmetation fault resulted from storing value to null pointer. signal number: %d\n", sig);
	exit(4);
	

}
int main(int argc, char* argv[]) {

int opt;
int newifd = -1;
int newofd = -1;
int fseg = 0;
int fcatch = 0;
static struct option long_options[] = 
{
	{"input", required_argument, NULL,'i'},
	{"output", required_argument, NULL,'o'},
	{"segfault",no_argument, NULL, 's'},
	{"catch",no_argument,NULL, 'c'},
	{0,0,0,0}
};


while( (opt = getopt_long(argc, argv, ":sci:o:;", long_options, NULL)) != -1) {
	switch(opt) {
		case 'i':
			newifd = openinputfile(optarg);
			break;
		case 'o':
			newofd = createoutputfile(optarg);
			break;
		case 's':			
			fseg = 1;
			break;
		case 'c':
			fcatch = 1;
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

if (newifd != -1) {
	close(0);
	dup(newifd);
	close(newifd);
}

if (newofd != -1) {
	close(1);
	dup(newofd);
	close(newofd);
}

if (fcatch == 1) {
	signal(SIGSEGV,sigsegv_handler);
}
if (fseg == 1) {
	char* c = NULL;
	*c = 'c';
}


char buf[10];
int num;
int numwrite;
while((num = read(0,&buf,10)) > 0) {
		numwrite = write(1,&buf,num);
		if(num == -1) {
			fprintf(stderr,"error with reading from stdin");
		}
		if (numwrite != num) {
			fprintf(stderr, "error with writting to stdin");
		}
	}

return 0;
}
