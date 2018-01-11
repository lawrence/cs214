
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

#define ACCERR 8
#include "server.h"

bank* ok_Bank;

void * client_service_thread( void * arg )
{
	int * nsd = (int *)arg;
	char commandCmp[256], buffer[256], accName[100];
	int slen, n, cmd;
	float amountChange = 0.0;

	// Current acc that the thread is in; resets if finish is inputted.
	acc* acc_ptr = NULL;

	while(1){
		memset( &buffer, 0, sizeof(buffer));
		n = read(*nsd, buffer, 255);

		if(n == 0){
			printf("A client has crashed. Closing its connection.\n");
			close(*nsd);
			free(nsd);
			pthread_exit(0);
		}
		if(n < 0)
			printf("Error reading from socket.\n");

		slen = strlen(buffer);
		printf("incoming string length: %d\n", slen);
		sscanf(buffer, "%s", commandCmp);

		printf("incoming message: %s\n", buffer);
		//printf("checking %s\n", commandCmp);
		cmd = commandCheck( commandCmp );

		if( cmd == 7){
			n = write(*nsd, "exit",5 );
			close(*nsd);
			free(nsd);
			pthread_exit(0);
		}

		else if(cmd == 1 || cmd == 2){
			sscanf(buffer, "%s %s",commandCmp, accName);
			if(accName[0] == 0){
				n = write(*nsd, "Did not input an account name. Operation failed.\n", 50);
				continue;
			}
			printf("Account Name: %s\n", accName);
			//DO SOME SERVER LOOKUP HERE
			if (cmd == 1){
				// Open the account.
				open_Account(ok_Bank, accName, *nsd);
				continue;
			}

			if (cmd == 2){
				if (acc_ptr != NULL){
					// Doesn't let you do anything else!
					n = write(*nsd, "You're already logged in somewhere else! Please log out.",57);
					continue;
				}
				else{
					// Start a new session and sets acc ptr.
					acc_ptr = start_Account(ok_Bank, accName, *nsd);
					//printf("acc->ptr is: %s\n",acc_ptr->name);
					continue;	
				}
			}
			memset( &accName, 0, sizeof(accName) );
			
		}else if( cmd == 3 || cmd == 4 ){

			sscanf(buffer, "%s %f",commandCmp, &amountChange);
			if( amountChange == 0.0){
				n = write(*nsd, "Did not input specific dollar amount. Operation failed.\n", 56 );
				continue;
			}
			printf("Amount credited or debited: %f.\n", amountChange);
			//DO SOME SERVER LOOKUP HERE

			if (acc_ptr == NULL){
				// Need to start a session!
				n = write(*nsd,"Did not start an account session.",35);
				continue;
			}
			if(cmd == 3){
				//pthread_mutex_trylock(&(acc_ptr->acc_lock));
				credit_Account(ok_Bank,acc_ptr,amountChange, *nsd);
				continue;
			}
			if(cmd == 4){
				//pthread_mutex_trylock(&(acc_ptr->acc_lock));
				debit_Account(ok_Bank,acc_ptr,amountChange, *nsd);
				continue;
			}
			
			amountChange = 0.0;
		}
		
		else if(cmd == 5){
				//pthread_mutex_trylock(&(acc_ptr->acc_lock));
				if(acc_ptr == NULL){
					n = write(*nsd, "You are not in an account...",29);
					continue;
				}
				
				else {
					balance(ok_Bank,acc_ptr, *nsd);
					continue;
				}
		}

		else if(cmd == 6){
			if (acc_ptr == NULL){
				// If no account was ever selected...
				n = write(*nsd, "You were not in an account...",30);
				continue;
			}
			else {
				pthread_mutex_unlock( &(acc_ptr->acc_lock) );
				acc_ptr = NULL;
				n = write(*nsd, "Logged out!",12);
				continue;
			}
		}
	}	n = write(*nsd, "Message received", 17);
}

void session_acceptor_thread( void * arg )
{
//	char buffer[256];
	int portnum, sd, ai, nsd;

//	portnum = strtol( argv[0], (char**)NULL, 10);
	
	struct addrinfo request, *result, *ptr;
	struct sockaddr_storage client;

	memset( &request, 0, sizeof(request));
	request.ai_family = AF_INET;
	request.ai_flags = AI_PASSIVE;
	request.ai_socktype = SOCK_STREAM;

	if( (ai = getaddrinfo(NULL, "5670", &request, &result)) != 0){
		printf("Error getting address information: %s\n", strerror(errno));
		return;
	}
	
	for( ptr = result; ptr != NULL; ptr = ptr->ai_next){
		if((sd = socket(request.ai_family,request.ai_socktype, 
				request.ai_protocol)) == -1){
			continue;
		}
		if(bind(sd, ptr->ai_addr, ptr->ai_addrlen) == -1){
			close(sd);
			continue;
		}
		break;
	}

	if( ptr == NULL) {
		printf("error opening socket. Errno: %s\n", strerror(errno));
		exit(0);
	}
//	int nsd;
	freeaddrinfo(result);
	while(1){
		if( listen(sd, 10) == -1){
			printf("%s error while listening\n", strerror(errno));
			return;
		}
		int len = sizeof(client);
		if( (nsd = accept(sd, (struct sockaddr *)&client, &len)) == -1){
			printf("Error while accepting: %s.\n Did not connect.\n", strerror(nsd));
			continue;
		}else{
			printf("Server accepted connection from client.\n");
			//spawn a new thread for this client

			unsigned int error;
			pthread_attr_t attr;
			pthread_t clientService;

			if((error = pthread_attr_init( &attr )) != 0){
				printf("pthread_attr_init() has failed. Connection reset\n");
				continue;
			}else if( (error = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)) != 0){
				printf("pthread_attr_setscope() has failed. Connection reset.\n");
				continue;
			}	
	
			int * sdptr = (int *)malloc(sizeof(int));
			*sdptr = nsd;
			if((error = pthread_create(&clientService, &attr, client_service_thread, sdptr)) != 0){
				printf("pthread_create() has failed. Connection reset.\n");
				continue;
			}
		}

	}
}


void * print_account_thread(void * data){
	bank* bk = ok_Bank;
	
	while(1){
		if(pthread_mutex_trylock(&(bk->bank_lock)) != 0){
			printf("Bank is already being checked.\n");
			return NULL;
		}

		else{
			pthread_mutex_trylock( &(bk->bank_lock) );
			int i;
			printf("\n20 second bank print out for account sessions:\n");
			
			for (i = 0; i < 20; i++){
				int j;
				j = i+1;

				if(strcmp(bk->accounts[i].name, "000_init") == 0){
					printf("(%d) --- \n",j);
				}
				else if( (pthread_mutex_trylock(&(bk->accounts[i].acc_lock)) == 0) ){
					printf("(%d) NOT IN SERVICE: %s (balance: %f)\n",j,bk->accounts[i].name,bk->accounts[i].balance);
					pthread_mutex_unlock(&(bk->accounts[i].acc_lock));
				}
				else if (pthread_mutex_trylock(&(bk->accounts[i].acc_lock)) != 0){
					printf("(%d) IN SERVICE: %s (balance: %f)\n",j,bk->accounts[i].name,bk->accounts[i].balance);
				}
			}

			pthread_mutex_unlock(&(bk->bank_lock));
		}

		sleep(20);
	}
	
	return NULL;
}

int main(int argc, char ** argv)
{	
	unsigned int error;
	pthread_attr_t attr;
	pthread_t sessionAcceptor;
	pthread_t printThread;

	ok_Bank = create_Bank();


	if((error = pthread_attr_init( &attr )) != 0){
		printf("pthread_attr_init() has failed. Exiting\n");
		_exit(1);
	}else if( (error = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)) != 0){
		printf("pthread_attr_setscope() has failed. Exiting.\n");
		_exit(1);
	}	
	if((error = pthread_create(&sessionAcceptor, &attr, session_acceptor_thread, NULL)) != 0){
		printf("pthread_create() has failed. Exiting.\n");
		_exit(1);
	}
	
	//print bank
	if((error = pthread_create(&printThread, &attr, print_account_thread, NULL)) != 0){
		printf("pthread_create() has failed. Exiting.\n");
		_exit(1);
	}
	
	else{
		printf("Accepted. Waiting for clients to connect...\n");
		pthread_join(sessionAcceptor, NULL);
		pthread_join(printThread, NULL);

		pthread_attr_destroy(&attr);
		printf("Server finished.\n");
		return 0;
	}		
}

int commandCheck( char* cmdcmp )
{
	int slen, retVal, i;
	slen = strlen(cmdcmp);
	
	for(i = 0; i < slen; i++){
		cmdcmp[i] = tolower(cmdcmp[i]);
	}
	if( strcmp(cmdcmp, "open") == 0){
		printf("OPEN command recieved\n");
		retVal = 1;
	}else if( strcmp(cmdcmp, "start") == 0){
		printf("START command received\n");
		retVal = 2;
	}else if( strcmp(cmdcmp, "credit") == 0){
		printf("CREDIT command received\n");
		retVal = 3;
	}else if( strcmp(cmdcmp, "debit") == 0){
		printf("DEBIT command received\n");
		retVal = 4;
	}else if( strcmp(cmdcmp, "balance") == 0){
		printf("BALANCE command received\n");
		retVal = 5;
	}else if( strcmp(cmdcmp, "finish") == 0){
		printf("FINISH command received\n");
		retVal = 6;
	}else if( strcmp(cmdcmp, "exit") == 0){
		printf("EXIT command received\n");
		retVal = 7;
	}else{
		printf("Invalid commnand\n");
		retVal = -1;
	}
	return retVal;
}

bank* create_Bank(){
    bank *this = (bank *)malloc(sizeof(bank));
    int i;
    for(i = 0; i < 20; i++){
        acc *init_acc = (acc *)malloc(sizeof(acc));
        this->accounts[i] = *init_acc;

        // size of name buffer
        this->accounts[i].name = (char*)malloc(sizeof(char)*9);
        strcpy((this->accounts[i].name), "000_init\0");
        this->accounts[i].balance = 0.0;
        pthread_mutex_init(&(this->accounts[i].acc_lock), NULL);
    }
    pthread_mutex_init(&(this->bank_lock), NULL);
    return this;
}

bank* open_Account(bank* bk, char* accname, int* w){
    if (pthread_mutex_trylock(&(bk->bank_lock)) == 0){
        
        int i; 
        for (i = 0; i < 20; i++){
            if(strcmp(bk->accounts[i].name, accname) == 0){
            	printf("NAME: %s\n",bk->accounts[i].name);
                
                // If already found.
            	char buff[256];
                sprintf(buff, "The account you are trying to put in is already here.");
                write(w,buff,strlen(buff)+1);

                pthread_mutex_unlock(&(bk->bank_lock));
                return NULL;
            }
        }
        
       	for (i = 0; i < 20; i++){
       		if (strcmp("000_init", bk->accounts[i].name) == 0){
       			
       			// Realloc for actual name input.
       			realloc(bk->accounts[i].name, (sizeof(char)*(strlen(accname)+1)) );
            	strcpy(bk->accounts[i].name,accname);
            
            	char buff[256];
            	sprintf(buff, "Account %s was added!",bk->accounts[i].name); 
            	write(w,buff,strlen(buff)+1);

            	pthread_mutex_unlock(&(bk->accounts[i].acc_lock));
            	pthread_mutex_unlock(&(bk->bank_lock));
            	return bk;
       		}
       	}

       	// No more space left for more accounts
	    char buff[256];
	    sprintf(buff,"All the space for accounts in the bank (20) is used.");
        write(w,buff,strlen(buff)+1);
        pthread_mutex_unlock(&(bk->bank_lock));
	    
	    return NULL;
    }

    char buff[256];
    sprintf(buff,"Maintence check. Please hold.");
    write(w,buff,strlen(buff)+1);
    
    return NULL;
}

acc* start_Account(bank* bk, char* account, int* w){
    if (pthread_mutex_trylock(&(bk->bank_lock)) == 0){
        int i;

        for(i = 0; i < 20; i++){
        	printf("Bank acc: %s\n",bk->accounts[i].name);

            if (strcmp(account, bk->accounts[i].name) == 0){
            	for(;;){
            		// Attempt to access the account if locked every 2 seconds
	                if(pthread_mutex_trylock(&(bk->accounts[i].acc_lock)) != 0){
	                    char buff[256];
	                    sprintf(buff,"Account is in use! Reattempting...");
	                    write(w,buff,strlen(buff)+1);

	                    pthread_mutex_unlock(&(bk->bank_lock));
	                }
	                else{
	                	// Account found!
	                	char buff[256];
	                	sprintf(buff,"Account %s found!",account);
	                	write(w,buff,strlen(buff)+1);

	                	pthread_mutex_unlock(&(bk->bank_lock));
	                	pthread_mutex_trylock(&(bk->accounts[i].acc_lock));
	                	return &bk->accounts[i];
	                }
	                sleep(2);
	            }
            }
        }
        
        char buff[256];
        sprintf(buff,"Account is not found please try again.");
        write(w,buff,strlen(buff)+1);
        
        pthread_mutex_unlock(&(bk->bank_lock));
        return NULL;
    }

    char buff[256];
    sprintf(buff,"Maintence check. Please hold.");
    write(w,buff,strlen(buff)+1);
    return NULL;
}

acc* credit_Account(bank* bk, acc* account, float crd, int* w){
    if ( pthread_mutex_trylock( &(bk->bank_lock) ) == 0){
    	account->balance = account->balance + crd;
       	
       	// No check because credit can be negative.
       	char buff[256];
       	sprintf(buff,"Your balance after credit is now: %f", account->balance);
       	write(w,buff,strlen(buff)+1);

       	pthread_mutex_unlock(&(bk->bank_lock));
       	return account;
    }

    char buff[256];
    sprintf(buff,"Maintence check. Please hold.");
    write(w,buff,strlen(buff)+1);

    return NULL;
}

acc* debit_Account(bank* bk, acc* account, float deb, int* w){
    if (pthread_mutex_trylock(&(bk->bank_lock)) == 0){
        float check = 0.0;
        
    	if (deb < 0){
    		// If debit is a negative
    		check = account->balance + deb;
    		if (check < 0){
    		// Check the amount to see if debit is too much.	
    		   char buff[256];
        	   sprintf(buff,"You are withdrawing too much.");
        	   write(w,buff,strlen(buff)+1);

        	   pthread_mutex_unlock(&(bk->bank_lock));
        	   return NULL;
        	}

           	//printf("I AM IN!\n");
           	// Else subtract away.
           	check = account->balance;
           	
           	char buff[256];
           	sprintf(buff,"Your balance after debit is now: %f\n", check);
            write(w,buff,strlen(buff)+1);
           	pthread_mutex_unlock(&(bk->bank_lock));
            return account;
        	
    	}

    	else {
    		// If adding/positive debit.
    		account->balance = account->balance + deb;
    		
    		char buff[256];
           	sprintf(buff,"Your balance after debit is now: %f\n", account->balance);
            write(w,buff,strlen(buff)+1);

    		pthread_mutex_unlock(&(bk->bank_lock));
    		return account;
    	}
   }

   char buff[256];
   sprintf(buff,"Maintence check. Please hold.");
   write(w,buff,strlen(buff)+1);

   return NULL;
}
                                   
void balance(bank* bk, acc* account, int* w){
   if (pthread_mutex_trylock(&(bk->bank_lock)) == 0){
   		// Just print balance
        char buff[256];
        sprintf(buff,"Your balance now: %f", account->balance);
        write(w,buff,strlen(buff)+1);

        pthread_mutex_unlock(&(bk->bank_lock));
        return;
    }

    char buff[256];
    sprintf(buff,"Maintence check. Please hold.");
    write(w,buff,strlen(buff)+1);

    return;
}	
