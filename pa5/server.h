#ifndef server_h
#define server_h

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

typedef struct acc{
    char* name;
    float balance;
    
    pthread_mutex_t acc_lock;
} acc;

typedef struct bank{
    struct acc accounts[20];
    pthread_mutex_t bank_lock;
} bank;

void * client_service_thread(void * arg);
void session_acceptor_thread(void * arg);
void * print_account_thread(void * data);
int commandCheck(char* cmdcmp);

bank* create_Bank();

bank* open_Account(bank* bk, char* accname, int* w);
acc* start_Account(bank *bk, char* accname, int* w);

acc* credit_Account(bank* bk, acc* account, float crd, int* w);
acc* debit_Account(bank* bk, acc* account, float deb, int* w);
void balance(bank* bk, acc* account, int* w);

#endif
