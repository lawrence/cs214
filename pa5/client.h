#ifndef client_h
#define client_h

// Chris McDonugh
// Lawrence Park

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

static void sigpipe_handler( int signo );
int exitCheck( char * input );
void * write_thread_function( void * arg );
void * read_thread_function( void * arg );

#endif