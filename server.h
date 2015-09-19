/*
 * Rony Vargas
 * 155005725
 * CS214 - Systems Programming
 * Spring 2015
 *
 * server.h
 */

#ifndef SERVER_H_
#define SERVER_H_

#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<pthread.h>


/*
 * Account structure will represent an individual account. It contains its own mutex to
 * protect the integrity of the structure. Two flag ints (exists & session_flag) and account
 * attributes accountName and balance.
 */

typedef struct _Account{
	int exists;
	pthread_mutex_t account_lock;
	char accountName[101];
	double balance;
	int session_flag;

} Account;


/*
 * Bank structure will represent a whole bank containing an array of 20 Accounts. It contains its own mutex to
 * protect the integrity of the structure. The int represents the current number of accounts in use.
 */

typedef struct _Bank{

	pthread_mutex_t bank_lock;
	Account accounts[20];
	int total_accounts;


} Bank;


/* Server functions */
int claim_port( const char * );
void * client_session_thread( void * );
void * server_printer_thread( void * arg );

/*Bank Manipulation functions */
Account * serve_account (Bank * bank, char *acc_name);
int exit_account (Account * account);
float query_account(Account * account);
int deposit_account(Account * account, float amount);
int withdraw_account(Account * account, float amount );
int transfer_account(Account * account, float amount, Bank * _bank, char * acc_name );
void init_bank ( Bank* bank );
void init_account ( Account* account );



#endif /* SERVER_H_ */
