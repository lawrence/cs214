
// Chris McDonugh
// Lawrence Park 

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/* Multithreaded bank system. Client side.*/

/*
 * signal handler for when the server crashes and the client is still running
 * SIGPIPE
 * only invoked after client writes twice to the dead server
 * write() should throw this signal after one attempt on the closed socket
 */

static void sigpipe_handler( int signo ){
	printf("Socket has closed. Exiting.\n");
	_exit(0);
}

/*
 * helper function to see if the user or server wrote "exit"
 * trouble with just using strcmp()
 */

int exitCheck( char * input ){
	char check[5] = "exit";
	input[4] = '\0';
	//printf("check[4]: %c, input[4]: %c\n", check[4], input[4]);

	int i;
	for(i = 0; i < strlen(input); i++){
		input[i] = tolower(input[i]);
	}
	//printf("checking: %s, against: %s\n", check, input);
	//printf("strcmp value: %d\n", strcmp(input, check));
	if(strcmp(input, check) == 0){
		return 1;
	}else{
		return 0;
	}
}
/*
 * first of the two threads the client spawns
 * reads from stdin (user) then writes that to the server
 * only exits if "exit" is inputed or if the server crashes
 * problem with the latter
 */
void * write_thread_function( void * arg ){

	struct sigaction action;
	action.sa_flags = 0;
	action.sa_handler = sigpipe_handler;
	sigemptyset( &action.sa_mask );
	sigaction( SIGPIPE, &action, 0);	

	int * sd = (int *)arg;
	int n;	
	char buffer[256];
		
	while(1)
	{
		printf("Enter message: \n");
		memset( buffer, '\0', sizeof(buffer));

		//get input from user
		read(0, buffer, sizeof(buffer));
		
		//write to server
		n = write(*sd, buffer, strlen(buffer));
		printf("number of bytes written: %d\n",n);
		if(n <= 0){
			printf("error writing to socket\n");
		}
		printf("sent: %s of length: %lu\n", buffer, strlen(buffer));
		
		//check to see if the client is exiting
		if(exitCheck(buffer) == 1)
			break;
		//wait for next input
		sleep(2);
	}
	
	//once the client exits, exit thread
	free(sd);
	pthread_exit(0);
	
}

/*
 * the other thread the client spawns
 * reads from the server and writes to stdout (console)
 * exits when the server sends back an empty buffer (server crashes)
 * or when the server tells the client to exit
 */

void * read_thread_function( void * arg ){

	int * sd = (int *)arg;
	int n;
	char buffer[256];

	while(1){
		memset( buffer, '\0', sizeof(buffer));
		n = read( *sd, buffer, sizeof(buffer));
		
		if(n < 0)
			printf("error reading from socket\n");
		if(buffer[0] == '\0'){
			printf("Server has closed connection. Exitting.\n");
			_exit(0);
		}	
		printf("message from server: \"%s\"\n", buffer);
		if( exitCheck( buffer ) == 1){
			printf("Exiting from server.\n");
			break;
		}

	}

	free(sd);
	pthread_exit(0);
}	

/*
 * Main sets up the connection
 * defensive coding out the ass
 * nothing gettin by this joint
 * spawns the two threads
 * waits for them to exit then exits itself
 */

int main( int argc, char ** argv)
{
	if(argc != 2){
		printf("Did not specify server name. Exiting\n");
		return -1;
	}

	int sd, ai, len;
	struct addrinfo request, *serverinfo, *ptr;
	char buffer[256];
	
	memset(&request, 0, sizeof(request));
	request.ai_family = AF_INET;
	request.ai_socktype = SOCK_STREAM;

	if( (ai = getaddrinfo(argv[1], "5670", &request, &serverinfo)) != 0){
		printf("error getting address information\n");
		return 1;
	}

	for(ptr = serverinfo; ptr != NULL; ptr = ptr->ai_next){
		if((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1){
			printf("couldnt create socket going to next\n");
			continue;
		}
		int count = 0;
		while(1){
			count++;
			if(connect( sd, ptr->ai_addr, ptr->ai_addrlen) == -1){	
				//keep looping until we connect
				printf("Could not connect to server.\n");
				printf("Connection attmept: %d of 20.\n", count);
				if(count == 20){
					printf("Connection timeout. Exiting\n");
					return -1;
				}
				printf("Retrying.\n\n");
				sleep(3);
				continue;
			}
			else
				break;
		}	
		break;
	}

	if(ptr == NULL){
		printf("client falied to connect\n");
		return 1;
	}
	
	//connection complete
	printf("Client has connected to server on machine: \"%s\".\n", argv[1]);
	
	
	//spawn two threads here to read and write to the server
 
	unsigned int error;

	pthread_attr_t attr;
	pthread_t writeThread, readThread;
	
	if((error = pthread_attr_init( &attr )) != 0){
		printf("pthread_attr_init() has failed. Exiting\n");
		printf("Error: %s\n", strerror(error));
		_exit(1);
	}
	else if( (error = pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM )) != 0){
		printf("pthread_attr_setscope() has failed. Exiting\n");
		printf("Error: %s\n", strerror(error));
		_exit(1);
	}

	int * sdw = (int *)malloc(sizeof(int));
	*sdw = sd;
	if((error = pthread_create( &writeThread, &attr, write_thread_function, sdw)) != 0 ){
		printf("pthread_create() has failed on line: %d. Exiting\n", __LINE__);
		printf("Error: %s\n", strerror(error));
		_exit(1);
	}

	int * sdr = (int *)malloc(sizeof(int));
	*sdr = sd;
	if((error = pthread_create( &readThread, &attr, read_thread_function, sdr)) != 0 ){
		printf("pthread_create() has failed on line: %d. Exiting\n", __LINE__);
		printf("Error: %s\n", strerror(error));
		_exit(1);
	}

	//constantly check to see if connection is still open 
	int socketCheck;
	
//	while( (socketCheck = write( sd, "0", 2)) == 2){
//		printf("server alive. %d\n", socketCheck);
//		sleep(1);
//	}

	void** nothing = NULL;
	pthread_join(writeThread, nothing);
	pthread_join(readThread, nothing);
	pthread_attr_destroy(&attr);
	close(sd);
	printf("Disconnected from server.\n");
	return 0;
}
