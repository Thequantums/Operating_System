#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>


pid_t pid;
struct termios saved_attributes;
int pipeptc[2]; //parent to child pipe
int pipectp[2]; //child to parent pipe
char crlf[2] = {'\r','\n'};
int shell_mode = 1; //default shell mode on
int sockfd, newsockfd; // sockefd for socket, newsockfd for connection with client
int compress_mode = 0; //compression flag
z_stream input_shell; //decompress to input to shell
z_stream output_shell; //compress output from shell to send to client

void  server_end(void) {
	int st;
	if (shell_mode) {
	waitpid(pid, &st,0);
	int wt = WTERMSIG(st); 
	int we = WEXITSTATUS(st);
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", wt, we);
	close(sockfd);
	close(newsockfd);
	exit(0);
	}
}

void createpipe(int p[2]) {
	if (pipe(p) < 0 ) {
		fprintf(stderr, "error creating a pipe");
		exit(1);
	}
}

void initialize_inflation() {
	// intialize inflation - decompression input from client to shell to process
	input_shell.zalloc = Z_NULL;
	input_shell.zfree = Z_NULL;
	input_shell.opaque = Z_NULL;
	//initalize inflation function
	if (inflateInit(&input_shell) != Z_OK) {
		fprintf(stderr, "Erorr: initalizing inflate function\n");
		exit(1);
	}
}

void initialize_deflation() {
	//compress data (output from shell) to send to client
	output_shell.zalloc = Z_NULL;
	output_shell.zfree = Z_NULL;
	output_shell.opaque = Z_NULL;
	//initalize deflation function
	if (deflateInit(&output_shell, Z_DEFAULT_COMPRESSION) != Z_OK) {
		fprintf(stderr, "Erorr: initalizing deflate function.\n");
		exit(1);
	}
}

void parentprocess(int newsockfd) {
	//close unused fild descriptors in parent process
	close(pipeptc[0]);
	close(pipectp[1]);

	//poll stdin and in from child
	struct pollfd pfds[2];
	pfds[0].fd = newsockfd;   // polled from newsocketfd: read from clien.
	pfds[1].fd = pipectp[0];
	pfds[0].events = POLLIN | POLLHUP | POLLERR;
	pfds[1].events = POLLIN | POLLHUP | POLLERR;
	
	unsigned char buffer[256];
	int rval;
	int byteread;
	int bytecomp; //byte compressed
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
			byteread = read(newsockfd, buffer, 256*sizeof(char));
			if (byteread < 0) {
				fprintf(stderr, "error: cant read from stdin\n");
			}

		//decompress data
		if (compress_mode) {
			unsigned char c_buff[256]; //creating buffer for decompression
			input_shell.avail_in = byteread;
			input_shell.avail_out = 256;
			input_shell.next_in = buffer;
			input_shell.next_out = c_buff;	
		
		while(input_shell.avail_in > 0) {
			//decompress until availbale buf is empty
			//Z_SYNC_Flush is recommended in discussion
			if (inflate(&input_shell, Z_SYNC_FLUSH) != Z_OK) {
				fprintf(stderr, "ERROR: inflating buffer");
				exit(1);
			}
		}
			
		// end inflation
		inflateEnd(&input_shell);
		char c; // used to read one character at a time
		int i =0;
		bytecomp = byteread - input_shell.avail_out ;

		while (i < bytecomp) {
			c = c_buff[i];
			
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
				write(pipeptc[1], &lf, sizeof(char)); // write to shell
			}
			else {
				//default case writing to child
				write(pipeptc[1],&c,1); //write to shell
			}

			i++;
		}
	}
	// no compress flag
	else {
		int in = 0;
		char c1;
		while (in < bytecomp) {
			c1 = buffer[in];
			
			if (c1 == 0x03) {	
			// ^C
			kill(pid,SIGINT);
			printf("^C\n"); 
			}
			else if (c1 == 0x04) {
			// ^D
			close(pipeptc[1]);			
			}
			else if (c1 == cr || c1 == lf) {
				write(pipeptc[1], &lf, sizeof(char)); // write to shell
			}
			else {
				//default case writing to child
				write(pipeptc[1],&c1,1); //write to shell
			}

			in++;
		}		
	    }

	     } // from stdin read from poll
	
	//read from shell
	if (pfds[1].revents & POLLIN) {

		int br;
		int index = 0;
		char cc;	
	
		br = read(pipectp[0], buffer, sizeof(char)*256);
		if (br < 0) {
			fprintf(stderr, "error: reading output from shell");
			exit(1);
		}
		if (compress_mode) {
			unsigned char cc_buff[256]; //creating buffer for decompression
			output_shell.avail_in = br;
			output_shell.avail_out = 256;
			output_shell.next_in = buffer;
			output_shell.next_out = cc_buff;	
		
		while(input_shell.avail_in > 0) {
			//compress until availbale buf is empty
			//Z_SYNC_Flush is recommended in discussion
			if (deflate(&output_shell, Z_SYNC_FLUSH) != Z_OK) {
				fprintf(stderr, "ERROR: deflating buffer");
				exit(1);
			}
		  }
		deflateEnd(&output_shell);
		int byte_comp = br - output_shell.avail_out;
		while (index < byte_comp) {
			cc = cc_buff[index];
			if (cc == '\n' || cc== '\r') {
				write(newsockfd,&crlf, 2*sizeof(char));
			}
			else {
				write(newsockfd, &cc, 1);
			}
			index++;
		}
	    }
	    else {
			//no compress_mode
			index = 0;
			while(index < br) {
				cc = buffer[index];
				if (cc == '\n' || cc== '\r') {
					write(newsockfd,&crlf, 2*sizeof(char));
				}
				else {
					write(newsockfd, &cc, 1);
				}				
				index++;
			}
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
int pflag = 0;
int portnum; //port for connection
socklen_t clien;
struct sockaddr_in serv_addr, cli_addr;

static struct option long_options[] = {
	{"shell", no_argument,NULL, 's'},
	{"port", required_argument, NULL, 'p'},
	{"compress", no_argument, NULL, 'c'},
	{0,0,0,0}	
};
	while( (opt = getopt_long(argc, argv, ":sp:c;", long_options, NULL)) != -1) {
	    switch(opt) {
		case 's':
			shell_mode = 1; //turn on shell mode
			break;
		case 'p':
			pflag = 1;
			portnum = atoi(optarg);
			break;
		case 'c':
			compress_mode = 1;
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

if (pflag == 0) {
	fprintf(stderr, "Error: required port number\n.");
	exit(1);
}

atexit(server_end);

if (compress_mode) {
	//intialize compression and decompression
	initialize_inflation();
	initialize_deflation();
}

// create connection socket
if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "Error: opening socket");
	exit(1);
}

// set Data structure sockaddr_in of serv_addr.
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY get ip address of sever machine
serv_addr.sin_port = htons(portnum);

//bind socket to current host address.
if ( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	fprintf(stderr, "Error: binding socket to current host address");
	exit(1);
}

//listen for connect through the socket with 5 maximum connects a time
listen(sockfd,5);
clien = sizeof(cli_addr);

//accept connection if nothing is connected process's blocked here
newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clien);
if (newsockfd < 0 ) {
	fprintf(stderr, "Error: accepting connection from cleint");
}

	
if (shell_mode) {

//create pipes first before fork
	createpipe(pipeptc); // pipeptc[0] is read from pipe for child. [1] is write to pipe from parent
	createpipe(pipectp); //	pipectp[0] is read from pipe for parent. [1] is write to pipe for child.
	
	pid = fork();
	if (pid > 0) {
		// parent process
		parentprocess(newsockfd);
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

exit(0);

}