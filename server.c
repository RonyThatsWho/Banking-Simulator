/*
 * Rony Vargas
 * 155005725
 * CS214 - Systems Programming
 * Spring 2015
 *
 * server.c
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<pthread.h>
#include	"server.h"

 #define FALSE (0)
 #define TRUE (1)
 #define MAX_ATTEMPTS (1) /* MAX number of Attempts; must be >= 1 */ 
 #define RETRY_DELAY (0) /* Wait time (in sec.) before next attempt; must be >= 0 */
 #define REFRESH_RATE (6) /* Server Ouput Refresh Rate (in sec.) */

 Bank bank;

/*
 * claim_port sets up the the the connection given a port number and returns a socket descriptor. 
 * The funtion a port number (char) as an argument.
 *
 * The port is used in setup of an addr struct that is required to request a tcp connection. Once connected,
 * socket options are set. When finished the socket is returned.
 * 
 * On success an int socket descriptor is returned. Else -1 returned on errors.
 */

int
claim_port( const char * port )
{
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	int					sd;
	char				message[256];
	int					on = 1;

	addrinfo.ai_flags = AI_PASSIVE;		/* for bind() */
	addrinfo.ai_family = AF_INET;		/* IPv4 only */
	addrinfo.ai_socktype = SOCK_STREAM;	/* Want TCP/IP */
	addrinfo.ai_protocol = 0;		/* Any protocol */
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( 0, port, &addrinfo, &result ) != 0 )	/* Requesting port 90210 */
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed errno is %s.  File %s line %d.\x1b[0m\n", port, strerror( errno ), __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		write( 1, message, sprintf( message, "socket() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		return -1;
	}
	else if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
	{
		write( 1, message, sprintf( message, "setsockopt() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		close( sd );
		return -1;
	}
	else if ( bind( sd, result->ai_addr, result->ai_addrlen ) == -1 )
	{
		freeaddrinfo( result );
		close( sd );
		write( 1, message, sprintf( message, "\x1b[2;33mBinding to port %s ...\x1b[0m\n", port ) );
		return -1;
	}
	else
	{
		write( 1, message, sprintf( message,  "\x1b[1;32mSUCCESS : Bind to port %s\x1b[0m\n", port ) );
		freeaddrinfo( result );
		return sd;			/* bind() succeeded; */
	}
}

/*
 * serve_account attempts to find the requested account by id from the provided bank and return 
 * a pointer to the account.
 *
 * The funtion takes a bank (bank pointer) and an account name/id (string) as arguments.
 * 
 * On success an int socket descriptor is returned. Else -1 returned on errors.
 */

Account *
serve_account (Bank * bank, char *acc_name) /* Find/Lock Account & Change Service Flag */
{
	int				i;
	int				a_attempts = 1;
	int				b_attempts = 1;

	while (b_attempts <= MAX_ATTEMPTS){
		if ( pthread_mutex_trylock(&bank->bank_lock) == 0 ){/* Try Locking Bank -> If Lock Successful -> try to find account */
			for (i = 0; i < 20; i++){
				if ( strcmp(bank->accounts[i].accountName, acc_name) == 0){ /* found account */

					while (a_attempts <= MAX_ATTEMPTS){
						if ( pthread_mutex_trylock(&bank->accounts[i].account_lock) == 0 ){ /* try to lock account */
							*&bank->accounts[i].session_flag = 1;
							printf("\x1b[1;36mSUCCESS: SERVING ACCOUNT %s TO CLIENT\x1b[0m\n", bank->accounts[i].accountName );
							pthread_mutex_unlock(&bank->bank_lock); /* unlock bank */
							return &bank->accounts[i];
						}
						else { /* Could Not Lock account... */ 
							printf("\x1b[2;31mCOULD NOT LOCK ACCOUNT. ATTEMPT %i.\x1b[0m\n", a_attempts);
							a_attempts ++;
							sleep(RETRY_DELAY); 

						}
					}
					pthread_mutex_unlock(&bank->bank_lock);
					return NULL;	
				}
			}
			pthread_mutex_unlock(&bank->bank_lock); /* unlock bank */
			return NULL; /* Did not find account */
		}
		else{ /* Could not Lock Bank... */
			printf("\x1b[2;31mCOULD NOT LOCK BANK. ATTEMPT %i.\x1b[0m\n", b_attempts);
			b_attempts ++;
			sleep(RETRY_DELAY); 
			/* maybe send signal saying server busy? */ 
		}
	}

	return NULL;

}

/*
 * exit_account will change the session flag to 0 (not in session) and keep attempting to 
 * unlock the account until successful.
 *
 * The function takes an account (account pointer) as an argument.
 *
 * On success EXIT_SUCCESS (int) is returned.
 */

int 
exit_account (Account * account) /* Change Session Flag, Unlock account */
{
	account->session_flag = 0;
	while (1){
		if (pthread_mutex_unlock(&account->account_lock) == 0 ){
			printf("\x1B[1;34mACCOUNT SESSION CLOSED\x1B[0m\n");
			return EXIT_SUCCESS;
		}
	}

}

/*
 * query_account returns the current balance of the account.
 *
 * The function take an account (account *) as an argument.
 *
 * On success the account balance is returned (float).
 */

float
query_account(Account * account)
{

	return account->balance;

}

/*
 * deposit_account adds the requested ammount to the current balance of the account.
 *
 * The function take an account (Account pointer) and an amount (float) as parameters.
 *
 * On success EXIT_SUCCESS (int) is returned.
 */

int
deposit_account(Account * account, float amount)
{

	account->balance += amount;
	return EXIT_SUCCESS;

}

/*
 * withdraw_account deducts the requested amount from the current balance of the account.
 * If the requested amount is larger than the current balance nothing is changed.
 *
 * The function take an account (Account pointer) and an amount (float) as parameters.
 *
 * On success EXIT_SUCCESS (int) is returned. Otherwise EXIT_FAILURE (int) is returned.
 */

int
withdraw_account(Account * account, float amount )
{

	if (amount <= account->balance){

		account->balance -= amount;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

/*
 * transfer_account first attempts to create a session with the destination account. Once connected
 * it withdraws the amount from the first accounts balance and deposits that same amount to the
 * destination account. The destination account session is then closed.
 *
 * The function take an account (Account pointer), an amount (float), a bank (Bank *) and the account
 * name (char *) of the destination account.
 *
 * On success EXIT_SUCCESS (int) is returned. Otherwise EXIT_FAILURE (int) is returned.
 */

int
transfer_account(Account * account, float amount, Bank * _bank, char * acc_name )
{

	Account * 			destination = NULL;
	if ( (destination = serve_account(_bank, acc_name)) ){
		if(withdraw_account(account,  amount) == EXIT_SUCCESS){
			deposit_account(destination, amount);
			exit_account(destination);
			return EXIT_SUCCESS;
		}
		exit_account(destination);
	}
	return EXIT_FAILURE;
}

/*
 * create_account creates an account with the given name/id in the given bank. The function will attempt to lock
 * the bank and on success it will search through accounts for same name/id. If no duplicate is found it looks for
 * an empty slot and will initialize the account with the given information. 
 *
 * The function takes a bank (bank *), and an account name (char *) as inputs.
 *
 * On successful creation EXIT_SUCCESS (int) is returned. Otherwise EXIT_FAILURE (int) is returned.
 */

int
create_account (Bank * bank, char *acc_name )
{
	int				i;

	if ( pthread_mutex_trylock(&bank->bank_lock) == 0 ){/* Try Locking Bank -> If Lock Successful -> try to create account */
		/* printf ("Bank Locked - Attempting to create account \n"); */

		if (bank->total_accounts < 20){ 
			for (i = 0; i < bank->total_accounts; i++){
				if(strcmp(acc_name, bank->accounts[i].accountName) == 0){
					pthread_mutex_unlock(&bank->bank_lock);
					printf("ERROR: ACCOUNT ALREADY EXISTS\n");
					return EXIT_FAILURE;
				}
			}
			for (i = 0; i < 20; i++){ /* If free accounts available look for empty account */
				if ( bank->accounts[i].exists == 0){ /* account space is free. */
					bank->total_accounts++;
					*&bank->accounts[i].exists = 1;
					*&bank->accounts[i].session_flag = 1;
					/* printf("name -%s, length - %i destinaiton - %s\n", acc_name, (int)strlen(acc_name), bank->accounts[i].accountName); */
					strncpy(bank->accounts[i].accountName, acc_name, strlen(acc_name)+ 1);
					*&bank->accounts[i].balance = 0;
					*&bank->accounts[i].session_flag = 0;

					/* printf("new slot - %i, total accounts - %i, account exists? - %i\n", i, bank->total_accounts, bank->accounts[i].exists); */

					pthread_mutex_unlock(&bank->bank_lock);
					printf( "\x1b[1;32mSUCCESS : New Account created.\n Account id: %s. \x1b[0m\n\n", acc_name);
					/* printf("Made My New Account - Unlocked.\n"); */

					return EXIT_SUCCESS;
				}
				/* printf("In While - Out of IF\n"); */
			}
			/* printf("Out of While Loop\n"); */
		}
		else {
			pthread_mutex_unlock(&bank->bank_lock);
			printf("ERROR: BANK REACHED MAX ACCOUNTS\n");
			
		}
	}/* END oF ADDING ACCOUNT */


return EXIT_FAILURE;
}

/*
 * init_bank initializes the mutex and the variables of the given bank.
 *
 * The function take a bank (Bank *) as an argument
 *
 * The function does not return as it initializes values.
 */

void
init_bank ( Bank* bank )
{
	int 		i;

	pthread_mutex_init (&bank->bank_lock, NULL);
	pthread_mutex_lock (&bank->bank_lock);
	bank->total_accounts = 0;
	for (i = 0; i < 20; i++ ){
		init_account(&bank->accounts[i]);
		/* bank->accounts[i].exists = 0; */
	}
	pthread_mutex_unlock (&bank->bank_lock);

}

/*
 * init_account initializes the mutex and the variables of the given account,
 *
 * The function take an account(Account *) as an argument.
 *
 * The function does not return as it initializes values.
 */

void
init_account ( Account* account)
{
	pthread_mutex_init (&account->account_lock, NULL);
	pthread_mutex_lock (&account->account_lock);
	account->exists = 0;
	account->balance = 0;
	account->session_flag = 0;
	pthread_mutex_unlock (&account->account_lock);
}

/*
 * server_printer_thread is a function that is responsible for outputting server status messages every 20 seconds.
 * This function is spawned as a thread that will handle the server messages. To print, it must lock the bank and 
 * check for account existance/in-session flag. If bank could not be locked it will re-attempt to do so in 3 seconds.
 *
 * The function take a a void * as an argument which is then casted as a bank (Bank *).
 *
 * The function usually remains looping, when signal is sent it may return NULL.
 */

void *
server_printer_thread( void * arg )
{
	int  			i;
	Bank * 			bank;
	char 			timebuff[100];
	time_t 			now;

	bank = (Bank *)arg;

	while (1){
		if ( pthread_mutex_trylock(&bank->bank_lock) == 0 ){/* Try Locking Bank -> If Lock Successful print acount information */
			
			now = time (0);
			strftime (timebuff, 100, "%Y-%m-%d %H:%M:%S", localtime (&now));
			printf("\n\x1b[0;35m+------------- \x1b[0m%s\x1b[0;35m -------------+\n",timebuff);
			/* printf("\n\x1b[0;35m+º°º¤ø,¸¸,ø¤º° \x1b[0m%s\x1b[0;35m °º¤ø,¸¸,ø¤º°º+\n",timebuff); */
			printf("|                                               |\n");
			printf("|                                               |\n");
			printf("|\x1b[1;36m\tServer Acount Information Output\x1b[0;35m        |\n");
			printf("|\x1b[1;36m\tCurrent Accounts = %i\x1b[0;35m\t\t\t|\n", bank->total_accounts);
			printf("|                                               |\x1b[0m\n");

			for (i = 0; i < 20; i++){
				if ( bank->accounts[i].exists == 1){
					printf("\x1b[1;35m|\x1b[0;35m\tAccount ID: %s\x1b[0m\n", bank->accounts[i].accountName );
					printf("\x1b[1;35m|\x1b[0;35m\tCurrent Balance: %.2f\x1b[0m\n", bank->accounts[i].balance );
					if (bank->accounts[i].session_flag == 1){
						printf ("\x1b[1;35m|\x1b[1;34m\tStatus: IN SERVICE\t\t\t\x1b[1;35m|\x1b[0m\n");
					}
					printf("\x1b[1;35m|                                               |\x1b[0m\n");

				}
			}
			printf("\x1b[1;35m|                                               |\x1b[0m");
			printf("\n\x1b[1;35m=================================================\x1b[0m\n\n");
			pthread_mutex_unlock(&bank->bank_lock);
			sleep (REFRESH_RATE);
		}
		else sleep(3); /* Else Sleep for 3 sec and try again (top of loop) */
	}
	return NULL;
}

/*
 * client_session_thread is a function that is responsible for receiving all commands from a certain socket.
 * This function is spawned as a thread that will handle requests from the socket. Requests are recieved as a string
 * and is then splits into separate strings and tested for conditions which may call a function to handle the command.
 * Command/server status is then sent back through the socket descriptor.
 *
 * The function take a a void * as an argument which is then casted as an int (int *). This is a socket descriptor.
 *
 * The function usually remains looping, when it ends it will return 0.
 */

void *
client_session_thread( void * arg )
{
/*	int				j; */ /* Used for testing arguments */
	int				sd, n_spaces;
	int				serve_flag = FALSE;
	float			amount;
	char			request[2048];
	char			response[2048];
	char ** 		input = NULL;
	char *			current;
	Account *		served_acc = NULL;


	sd = *(int *)arg;
	free( arg );					/* keeping to memory management covenant */
	pthread_detach( pthread_self() );		/* Don't join on this thread */
	while ( read( sd, request, sizeof(request) ) > 0 )
	{
		/*printf( "\n\nserver receives input:  %s\n", request ); */

		/* Bank Commands and Manipulation recieved from Client. */
		n_spaces = 0;


		/* Parse input into tokens/Array of tokens */
		current = strtok (request, " ");
		while (current){ /* while not NULL increase size of array */
			input = realloc (input, sizeof(char *)  * ++n_spaces); 
			if (input == NULL){
				exit(-1); /* Mem allocation failed. */
			}

			input[n_spaces - 1] = current;
			current = strtok (NULL, " ");

		}

		/* Testing Arguments/Printing Array 
		printf("Args = %i \n", n_spaces);
		for (j = 0; j < n_spaces; ++j){
  			printf ("input[%d] = %s\n", j, input[j]);
		}
		*/

		if (serve_flag && n_spaces == 3 && (strcmp(input[0], "transfer") == 0) && (amount = atof(input[1])) > 0 ){
			if (transfer_account(served_acc, amount, &bank, input[2] ) == EXIT_SUCCESS){
				sprintf(response, "SUCCESS: TRANSFERRED $%.2f FROM %s TO %s", amount, served_acc->accountName, input[2]);
			}
			else strcpy(response, "TRANSFER FAILED.");
		}

		else if (n_spaces == 1 || n_spaces == 2){

			/* Currently Serving account */
			if (serve_flag){
				if (n_spaces == 2){ /* Two Arguments */
					if ((strcmp(input[0], "deposit") == 0) && (amount = atof(input[1])) > 0 ) {

						deposit_account(served_acc, amount);
						sprintf(response, "SUCCESS: $%.2f DEPOSIT COMPLETE.", amount);
					}
					else if ((strcmp(input[0], "withdraw") == 0) && (amount = atof(input[1])) > 0 ) {

						if (withdraw_account(served_acc, amount) == EXIT_SUCCESS){
							strcpy(response, "WITHDRAW SUCCESS");
						}
						else {
							sprintf(response, "ERROR: $%.2f WITHDRAW FAILED. \n\tAccount Balance $%.2f", amount, query_account(served_acc) );
						}
					}
					else {strcpy(response, "INVALID COMMAND");}
				}
			
				else{ /* One Argument */
					if (strcmp(input[0], "query") == 0){
						sprintf(response, "Account Balance: $%.2f", query_account(served_acc));

					}
					else if (strcmp(input[0], "end") == 0){
						if ( (exit_account(served_acc)) == EXIT_SUCCESS ){
							strcpy(response, "LOGGED OFF ACCOUNT");
							serve_flag = FALSE;
							served_acc = NULL;
						}
					}
					else if (strcmp(input[0], "quit") == 0){/* breaks loop where socket is closed */
						strcpy(response, "CLOSING CLIENT CONNECTION");
						break;
					}
					else{
						strcpy(response, "INVALID COMMAND");
					}
				}
			}

			/* Not Currently Serving account */
			else{
				/* printf ("Not Currently Serving account \n"); */
				if (n_spaces == 2){

					if ((strcmp(input[0], "create") == 0) && (strlen(input[1]) <= 100) ){
						if ( create_account (&bank, input[1] ) == EXIT_SUCCESS ){
							sprintf(response, "SUCCESS: NEW ACOUNT %s CREATED.\n", input[1]);
						}
						else strcpy(response, "COMMAND FAILURE: COULD NOT CREATE ACCOUNT.\n");
					}
					else if (strcmp(input[0], "serve") == 0){
						if( (served_acc = serve_account(&bank, input[1])) ){ 
							strcpy(response, "SERVE SUCCESS: LOGGED INTO ACCOUNT.\n");
							serve_flag = TRUE;
						}
						else strcpy(response, "FAILURE: COULD NOT SERVE ACCOUNT.\n");

					}
					else strcpy(response, "INVALID COMMAND");
				}

				else{ /* n_spaces == 1 */
					if (strcmp(input[0], "quit") == 0){/* send signal to kill client process. */
						strcpy(response, "CLOSING CLIENT CONNECTION");
						break;
					}
					else strcpy(response, "INVALID COMMAND");
				}
			}
		}
		else { /* Incorrect Number of Arguments */
			strcpy(response, "INVALID COMMAND: Incorrect number of arguments.");

		}
 
		/* Send information back to client */

		write( sd, response, strlen(response) + 1 );
		/* write( sd, request, strlen(request) + 1 ); */
	}
	printf("\x1B[1;31mCLIENT DISCONNECTED: \x1B[0m");
	if (served_acc){
		printf("\x1B[1;34mLOGGED OFF ACCOUNT\x1B[0m\n");
		exit_account(served_acc);

	}
	printf("\n");
	/* strcpy(response, "CLOSING CONNECTION:\n"); */
	/* write( sd, response, strlen(response) + 1 ); */
	close( sd );
	return 0;
}


/*
 * main is a reponsible for initializing the bank and then spawning the printer thread. It setups up a socket 
 * to recieve requests from potential clients. On successful connect with the client a new client_session_thread is
 * spawned that
 *
 * The function take a a void * as an argument which is then casted as a bank (Bank *).
 *
 * The function usually remains looping, when signal is sent it may return 0.
 */

int
main()
{
	int					sd;
	char				message[256];
	pthread_t			tid;
	pthread_attr_t		kernel_attr;
	socklen_t			ic;
	int					fd;
	struct sockaddr_in	senderAddr;
	int *				fdptr;



	/* Initialize Bank Structure with default values. */
	init_bank (&bank);
	printf( "\x1b[1;32mSUCCESS : Bank Initialized. \x1b[0m\n");


	/* Spawn account printer thread */
		if ( pthread_attr_init( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( pthread_attr_setscope( &kernel_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	
	else if ( pthread_create( &tid, &kernel_attr, server_printer_thread, &bank ) != 0 )
	{
		printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}

	sleep(1);

	/* Create initial server connection and Spawn new client sesssion thread for every incoming connections. */
	if ( pthread_attr_init( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( pthread_attr_setscope( &kernel_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( (sd = claim_port( "90210" )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not bind to port %s errno %s\x1b[0m\n", "3000", strerror( errno ) ) );
		return 1;
	}
	else if ( listen( sd, 100 ) == -1 )
	{
		printf( "listen() failed in file %s line %d\n", __FILE__, __LINE__ );
		close( sd );
		return 0;
	}
	else
	{
		ic = sizeof(senderAddr);
		while ( (fd = accept( sd, (struct sockaddr *)&senderAddr, &ic )) != -1 )
		{
			printf( "\x1b[1;32mSUCCESS : Client Connection Accepted \x1b[0m\n");
			fdptr = (int *)malloc( sizeof(int) );
			*fdptr = fd;					/* pointers are not the same size as ints any more. */
			if ( pthread_create( &tid, &kernel_attr, client_session_thread, fdptr ) != 0 )
			{
				printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
				return 0;
			}
			else
			{
				continue;
			}
		}
		close( sd );
		return 0;
	}
}

