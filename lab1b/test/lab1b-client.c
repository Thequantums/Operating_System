#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <zlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

pid_t pid;
struct termios saved_attributes;
int pipeptc[2]; //parent to child pipe
int pipectp[2]; //child to parent pipe
char crlf[2] = {'\r','\n'};
int shell_mode = 1;
int log_mode = 0;
int compress_mode = 0;
int logfd; // file descriptor for log file
int sockfd;

z_stream to_server;
z_stream from_server;



void initialize_inflation() {
	// intialize inflation - decompression
	from_server.zalloc = Z_NULL;
	from_server.zfree = Z_NULL;
	from_server.opaque = Z_NULL;
	//initalize inflation function
	if (inflateInit(&from_server) != Z_OK) {
		fprintf(stderr, "Erorr: initalizing inflate function.\n");
		exit(1);
	}
}

void initialize_deflation() {
	//compress data 
	to_server.zalloc = Z_NULL;
	to_server.zfree = Z_NULL;
	to_server.opaque = Z_NULL;
	//initalize deflation function
	if (deflateInit(&to_server, Z_DEFAULT_COMPRESSION) != Z_OK) {
		fprintf(stderr, "Erorr: initalizing deflate function.\n");
		exit(1);
	}
}

void reset_terminal_mode(void) {
	tcsetattr (0, TCSANOW, &saved_attributes);
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




void parentprocess(int socketfd) {

	//poll stdin and in from child
	struct pollfd pfds[2];
	pfds[0].fd = 0;
	pfds[1].fd = socketfd;
	pfds[0].events = POLLIN | POLLHUP | POLLERR;
	pfds[1].events = POLLIN | POLLHUP | POLLERR;
	
	unsigned char buffer[256];
	int rval;
	int byteread;
	int bytecomp;
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
				exit(1);
			}
		char c; // used to read one character at a time
		int i =0;

		//decompress data
		if (compress_mode) {
			unsigned char c_buff[256]; //creating buffer for decompression
			to_server.avail_in = byteread;
			to_server.avail_out = 256;
			to_server.next_in = buffer;
			to_server.next_out = c_buff;	
		
		do {
			//decompress until availbale buf is empty
			//Z_SYNC_Flush is recommended in discussion
			if (deflate(&to_server, Z_SYNC_FLUSH) != Z_OK) {
				fprintf(stderr, "ERROR: deflating buffer");
				exit(1);
			}
		} while(to_server.avail_in > 0);
			
		// end inflation
		deflateEnd(&to_server);
		bytecomp = 256 - to_server.avail_out ;
		i = 0;
		while (i < bytecomp) {
			c = c_buff[i];
				//default case writing to child
				write(socketfd, &c, 1); //write to server
			i++;
		}
		
		if (log_mode) {
			int ii = 0;
			while (ii < bytecomp) {
				c = c_buff[ii];
				//writting each log as a byte;
				write(logfd, "SENT 1 byte: ", 13);
				write(logfd, &c, 1);
				write(logfd, "\n", 1);
			ii++;
			}
		}
		memset(c_buff, 0, bytecomp);
	}
	// no compress flag
	else {
		int in = 0;
		char c1;
		while (in < byteread) {
			c1 = buffer[in];
			 if (c1 == cr || c1 == lf) {
				write(socketfd, &lf, sizeof(char)); // write to shell
			}
			else {
				//default case writing to child
				write(socketfd, &c1, 1); //write to shell
			}

			in++;
		}
		if (log_mode) {
			int ic = 0;
			while (ic < byteread) {
				c1 = buffer[ic];
				if (c1 == cr || c1==lf) {
				//writting each log as a byte;
				write(logfd, "SENT 1 byte: ", 13);
				write(logfd, &lf, 1);
				write(logfd, "\n", 1);
				}
				else {
				//writting each log as a byte;
				write(logfd, "SENT 1 byte: ", 13);
				write(logfd, &c1, 1);
				write(logfd, "\n", 1);
				}
				ic++;
			}
		}		
	    }
	memset(buffer,0,byteread);
	     } // from stdin read from poll
	
	//read from socketfd
	if (pfds[1].revents & POLLIN) {
		int br;
		int index = 0;
		int bytecomp1 = 0;
		char c4; // used to read one character at a time
		
		br = read(socketfd, buffer, sizeof(char)*256);
		if (br < 0) {
			fprintf(stderr, "error: reading output from shell");
			exit(1);
		}
		
		
		if (log_mode) {
		index = 0;
			while (index < br) {
				c4 = buffer[index];
				//writting each log as a byte;
				if (c4 == cr || c4 ==lf) {
				write(logfd, "RECEIVED 1 byte: ", 17);
				write(logfd, &crlf, 2);
				write(logfd, "\n", 1);
				}
				else {
				write(logfd, "RECEIVED 1 byte: ", 17);
				write(logfd, &c4, 1);
				write(logfd, "\n", 1);					
				}
			index++;
			}
		}

		if (compress_mode) {
			unsigned char c1_buff[256]; //creating buffer for decompression
			from_server.avail_in = br;
			from_server.avail_out = 256;
			from_server.next_in = buffer;
			from_server.next_out = c1_buff;	
		
		do{
			//decompress until availbale buf is empty
			//Z_SYNC_Flush is recommended in discussion
			if (inflate(&from_server, Z_SYNC_FLUSH) != Z_OK) {
				fprintf(stderr, "ERROR: inflating buffer");
				exit(1);
			}
		}while(from_server.avail_in > 0);
	
		// end inflation
		inflateEnd(&from_server);
		 bytecomp1 = 256 - from_server.avail_out ;
		
		index = 0;
		while (index < bytecomp1) {
			c4 = c1_buff[index];
 			if (c4 == cr || c4 == lf) {
				write(1, &crlf, 2); // write to stdin
			}
			else {
				//default case writing to stdin
				write(1, &c4, 1); //write to stdin
			}
			index++;
		}
		}
		// no compress mode
		else {
			index = 0;
			while (index < br) {
				c4 = buffer[index];
				if (c4 == cr || c4 == lf) {
					write(1, &crlf, 2);
				}
				else {
					write(1, &c4,1);
				}
				index++;
			}
		}
		
	}// end to read from server
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


int main(int argc, char** argv) {

int opt;
int portnum;
int pflag = 0;
char* logfile;
static struct option long_options[] = {
	{"shell", no_argument,NULL, 's'},
	{"port", required_argument, NULL, 'p'},
	{"log", required_argument, NULL, 'l'},
	{"compress", no_argument,NULL,'c'},
	{0,0,0,0}	
};
	while( (opt = getopt_long(argc, argv, ":sp:l:c;", long_options, NULL)) != -1) {
	    switch(opt) {
		case 's':
			shell_mode = 1; //turn on shell mode
			break;
		case 'p':
			portnum = atoi(optarg);
			pflag = 1;
			break;
		case 'l':
			log_mode = 1;
			logfile = optarg;
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

if (pflag ==0) {
	fprintf(stderr, "Error: Port number required");
	exit(1);
}

if (log_mode) {
	logfd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
}

if (compress_mode) {
	//intialize compression and decompression
	initialize_inflation();
	initialize_deflation();

}


struct sockaddr_in serv_addr;
struct hostent *server;

if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	fprintf(stderr, "Error: Creating socket");
	exit(1);
}
server = gethostbyname("localhost");
if (server == NULL) {
	fprintf(stderr, "ERROR: cant find host");
	exit(1);
}

	serv_addr.sin_family = AF_INET;
	bzero((char*) &serv_addr, sizeof(serv_addr));
	//copy address of server to struct sockaddr_in
	bcopy( (char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portnum);
	//connecting socket to server
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "Error: Binding socket to server");
		exit(1);
	}

//set terminal to noncanonical mode first
	set_terminal_mode();

	parentprocess(sockfd);


exit(0);

}