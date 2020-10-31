#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>

pid_t pid;
struct termios saved_attributes;
int pipeptc[2]; //parent to child pipe
int pipectp[2]; //child to parent pipe
char crlf[2] = {'\r','\n'};
int shell_mode = 0;

void reset_terminal_mode(void) {
	tcsetattr (0, TCSANOW, &saved_attributes);
	int st;
	if (shell_mode) {
	waitpid(pid, &st,0);
	int wt = WTERMSIG(st); 
	int we = WEXITSTATUS(st);
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", wt, we);
	exit(0);
	}
}

void set_terminal_mode(void) {

	struct termios tattr;
	tcgetattr(0, &tattr);

	if (!isatty (0)) {
	   //if stdin is not a terminal
	   fprintf(stderr, "stdin is not a terminal.\n");
	   exit(1);	
	}
	
	//save original terminal setting
	tcgetattr(0, &saved_attributes);
	atexit(reset_terminal_mode);

	//set non-canonical mode terminal
	tattr.c_iflag = ISTRIP;
	tattr.c_oflag = 0;
	tattr.c_lflag = 0;
	
	tattr.c_lflag &= ~(ICANON|ECHO); //clear icanon and echo
	tattr.c_cc[VMIN]=1;
	tattr.c_cc[VTIME]=0;
	tcsetattr(0, TCSANOW, &tattr);
	
}

void createpipe(int p[2]) {
	if (pipe(p) < 0 ) {
		fprintf(stderr, "error creating a pipe");
		exit(1);
	}
}


void parentprocess() {
	//close unused fild descriptors in parent process
	close(pipeptc[0]);
	close(pipectp[1]);

	//poll stdin and in from child
	struct pollfd pfds[2];
	pfds[0].fd = 0;
	pfds[1].fd = pipectp[0];
	pfds[0].events = POLLIN | POLLHUP | POLLERR;
	pfds[1].events = POLLIN | POLLHUP | POLLERR;
	
	char buffer[256];
	int rval;
	int byteread;
	char cr = '\r'; 
	char lf = '\n';
	
	while(1) {
		if ( (rval = poll(pfds, 2, -1)) < 0 ) {
			//poll error
			fprintf(stderr, "error creating a poll\n");
			exit(1);
		}
		if (pfds[0].revents & POLLIN) {
			//sth to read from stdin
			byteread = read(0, buffer, 256*sizeof(char));
			if (byteread < 0) {
				fprintf(stderr, "error: cant read from stdin\n");
			}
		char c; // used to read one character at a time
		int i =0;

		while (i < byteread) {
			c = buffer[i];
			
			if (c == 0x03) {	
			// ^C
			kill(pid,SIGINT);
			printf("^C\n"); 
			}
			else if (c == 0x04) {
			// ^D
			close(pipeptc[1]);			
			}
			else if (c == cr || c== lf) {
				write(1,&crlf, 2*sizeof(char));
				write(pipeptc[1], &lf, sizeof(char));
			}
			else {
				//default case writing to stdin and to child
				write(1, &c, 1);
				write(pipeptc[1],&c,1);
			}

			i++;
		}
	     } // from stdin read from poll
	
	//read from shell
	if (pfds[1].revents & POLLIN) {
		int br;
		int index = 0;		
		br = read(pipectp[0], buffer, sizeof(char)*256);
		if (br < 0) {
			fprintf(stderr, "error: reading output from shell");
			exit(1);
		}
		
		char cc;
		while (index < br) {
			cc = buffer[index];
			if (cc == '\n' || cc== '\r') {
				write(1,&crlf, 2*sizeof(char));
			}
			else {
				write(1, &cc, 1);
			}
			index++;
		}
		
	}
	//pollerr pollhup
	if ((pfds[0].revents & POLLERR) || (pfds[0].revents & POLLHUP)) {
		fprintf(stderr, "error stdin");
		exit(1);
	}
	if((pfds[1].revents & POLLERR) || (pfds[1].revents & POLLHUP)) {
		close(pipectp[0]);
		exit(0);
	}
      }

}

void childprocess() {
	//close unsused pipes
	close(pipeptc[1]);
	close(pipectp[0]);

	//dup
	close(0);
	dup(pipeptc[0]);
	close(1);
	dup(pipectp[1]);
	close(pipectp[1]);
	close(pipeptc[0]);

	char* exec_path = "/bin/bash";
	char* exec_args[2];
	exec_args[0] = exec_path;
	exec_args[1] = NULL;
	
	if (execvp(exec_path, exec_args)== -1) {
		fprintf(stderr, "error executing child (shell)");
		exit(1);
	}

}



int main(int argc, char** argv) {

int opt;
static struct option long_options[] = {
	{"shell", no_argument,NULL, 's'},
	{0,0,0,0}	
};
	while( (opt = getopt_long(argc, argv, ":s;", long_options, NULL)) != -1) {
	    switch(opt) {
		case 's':
			shell_mode = 1; //turn on shell mode
			break;
		case ':':
			fprintf(stderr, "%s: option --%c does not requires argument\b", argv[0], optopt);
			exit(1);
			break;
		case '?':
			fprintf(stderr, "option --%c is invalid. correct usage --shell", optopt);
			exit(1);
			break;
		} 
	}

//set terminal to noncanonical mode first
	set_terminal_mode();
	
if (shell_mode) {

//create pipes first before fork
	createpipe(pipeptc); // pipeptc[0] is read from pipe for child. [1] is write to pipe from parent
	createpipe(pipectp); //	pipectp[0] is read from pipe for parent. [1] is write to pipe for child.
	
	pid = fork();
	if (pid > 0) {
		// parent process
		parentprocess();
	}
	else if (pid == 0) {
		//child process
		childprocess();
	}
	else if (pid == -1) {
		//error, no child created
		fprintf(stderr, "cant create child process");
		exit(1);
	}	
}

// if shell mode not entered

char buffer[256];
int byteread;
char cr = '\r';
char lf = '\n';
char c;

while (1) {
	if ( (byteread=read(0,buffer,256*sizeof(char))) < 0) {
		fprintf(stderr, "error: reading from stdin");
	}	
	int i=0;
	while(i < byteread) {
		c = buffer[i];

		if (c == 0x04) {
			exit(0);
		}
		if (c == '\r' || c== '\n') {
			write(1,&cr,sizeof(char));
			write(1,&lf,sizeof(char));
		}
		else {
			write(1,&c,sizeof(char));
		}
		i++;
	}
}


exit(0);

}