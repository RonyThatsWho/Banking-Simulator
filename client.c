/*
 * Rony Vargas
 * 155005725
 * CS214 - Systems Programming
 * Spring 2015
 *
 * client.c
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
#include	<signal.h>


#define FALSE (0)
#define TRUE (1)

int CLOSE = FALSE;


/*
 * connect_to_server  sets up the the the connection given the destination server and port number. Once connected
 * it returns a socket descriptor. The function takes the server name (char) and port number (char) as arguments.
 *
 * The port and server are used in setup of an addr struct that is required to request a tcp connection. Once connected,
 * socket options are set. When finished the socket is returned.
 * 
 * On success an int socket descriptor is returned. Else -1 returned on errors.
 */

int
connect_to_server( const char * server, const char * port )
{
	int					sd;
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	char				message[256];

	addrinfo.ai_flags = 0;
	addrinfo.ai_family = AF_INET;		/* IPv4 only */
	addrinfo.ai_socktype = SOCK_STREAM;	/* Want TCP/IP */
	addrinfo.ai_protocol = 0;		/* Any protocol */
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( server, port, &addrinfo, &result ) != 0 )
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed.  File %s line %d.\x1b[0m\n", server, __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		freeaddrinfo( result );
		return -1;
	}
	else
	{
		do {
			if ( errno = 0, connect( sd, result->ai_addr, result->ai_addrlen ) == -1 )
			{
				sleep( 1 );
				write( 1, message, sprintf( message, "\x1b[2;33mConnecting to server %s ...\x1b[0m\n", server ) );
			}
			else
			{
				freeaddrinfo( result );
				return sd;		/* connect() succeeded */
			}
		} while ( errno == ECONNREFUSED );
		freeaddrinfo( result );
		return -1;
	}
}


/*
 * client_response_output_thread is a function that is responsible for receiving all responses from a certain socket.
 * This function is spawned as a thread that will print out responses from the server. Responses are recieved and this
 * function will display them on the client's screen.
 *
 * The function take a a void * as an argument which is then casted as an int (int *). This is a socket descriptor.
 *
 * The function usually remains looping until the server connections is closed, when it ends it will return NULL.
 */

void *
client_response_output_thread( void * arg)
{	
	int 			sd;
	char			buffer[512];
	char			output[512];
	sd = 			*(int *)arg;

	pthread_detach( pthread_self() );
	printf("\x1b[2;33mResponse Output Thread Spawned.\x1b[0m\n");
	while(read( sd, buffer, sizeof(buffer) ) > 0){
		sprintf( output, "\n\x1b[1;35mSERVER RESPONSE: \x1b[1;34m%s \x1b[0m\n", buffer );
		write( 1, output, strlen(output) );

	}
	close(sd);
	printf("\x1b[1;31mSERVER CONNECTION CLOSED\x1b[0m\n"); /* need to send signal */
	CLOSE = TRUE;
	return NULL;

}

/*
 * client_command_input_thread is a function that is responsible for sending all commands to the server through the socket.
 * This function is spawned as a thread that will take input from the user and send them to the server. 
 *
 * The function take a a void * as an argument which is then casted as an int (int *). This is a socket descriptor used to send
 * commands.
 *
 * The function usually remains looping until the server connections is closed, when it ends it will return NULL.
 */

void *
client_command_input_thread( void * arg)
{

	char			prompt[] = "\x1b[1;36mEnter your command: \x1b[0m\n";
	int				len;
	int 			sd;
	char			string[512];
	sd = 			*(int *)arg; 

	pthread_detach( pthread_self() );
	printf("\x1b[2;33mCommand Input Thread Spawned.\x1b[0m\n");
	usleep(100000);
	write( 1, prompt, sizeof(prompt) );
	while ((len = read( 0, string, sizeof(string) )) > 0 ){
		string[len-1]= '\0';
		write( sd, string, strlen( string ) + 1 );
		usleep(100000);
		write( 1, prompt, sizeof(prompt) );
		sleep(2);


	}
	return NULL;
}

/*
 * main is a reponsible for calling the connect_to_server function which will setup up a socket where the server
 * can be reached. Once connected with the server a client_response_output_thread and a client_command_input_thread 
 * are spawned and the socket is passed to each of them.
 *
 * The function take the server name/address from the command line as an argument. 
 *
 * The function usually remains looping, until CLOSE == TRUE which is caused from the client_response_output_thread.
 */

int
main( int argc, char ** argv ) 
{
	int				sd;
	char			message[256];
	pthread_t		tid;
	pthread_attr_t	kernel_attr;


	if ( argc < 2 )
	{
		fprintf( stderr, "\x1b[1;31mNo host name specified.  File %s line %d.\x1b[0m\n", __FILE__, __LINE__ );
		exit( 1 );
	}
	else if ( (sd = connect_to_server( argv[1], "90210" )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not connect to server %s errno %s\x1b[0m\n", argv[1], strerror( errno ) ) );
		return 1;
	}
	else
	{
		printf( "\x1b[1;32mConnected to server \x1b[0;32m%s\x1b[0m\n", argv[1] );
	}

	/* Spawn Response Output Thread */
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
	
	else if ( pthread_create( &tid, &kernel_attr, client_response_output_thread, &sd ) != 0 )
	{
		printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}

	/* Spawn Command Input Thread */
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
	
	else if ( pthread_create( &tid, &kernel_attr, client_command_input_thread, &sd ) != 0 )
	{
		printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}

	/* RUN until CLOSE == TRUE; Changed by Response Output Thread. */
	while(1){  
		if (CLOSE){
			close(sd);
			printf("\x1b[1;31mSHUTTING DOWN CLIENT\x1b[0m\n");
			break;
		}
	}
	return 0;
}
