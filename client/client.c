//Sample program at client side for echo transmit-receive - CSS 432 - Autumn 2022
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>          // for retrieving the error number.
#include <string.h>         // for strerror function.
#include <signal.h>         // for the signal handler registration.
#include <unistd.h>




#define SERV_UDP_PORT 51145 //REPLACE WITH YOUR PORT NUMBER
#define SERV_HOST_ADDR "127.0.0.1" //REPLACE WITH SERVER IP ADDRESS


/* A pointer to the name of this program for error reporting.      */

char *progname;
const static int MAX_BUFF_SIZE = 516;
const static int DATA_OFFSET = 4;

send_packet(sockfd, pserv_addr, servlen, op)
int sockfd;
struct sockaddr *pserv_addr;
int servlen;
{
	char buffer[516];
	bzero (buffer, sizeof(buffer));

	// RRQ
	if (op == 1)
	{
		// Construct RRQ packet
		int len = 0;

		unsigned short opCode = htons(1);
		memcpy(buffer, &opCode, 2);
		len += 2;

		char fileName[] = "readtest.txt\0";
		strncpy(buffer + len, fileName, strlen(fileName));
		len += strlen(fileName) + 1;

		char mode[] = "NETASCII\0";
		strncpy(buffer + len, mode, strlen(mode));

		/* ---------- FOR DEBUGGING ---------- */
		// Print the request packet that is sent to the server
		fprintf(stderr, "\n-------------------\n");
		fprintf(stderr, "Sent Request Packet\n");
		for (int i = 0; i < 30; i++) 
		{
			fprintf(stderr, "0x%X,", buffer[i]);
		}
		fprintf(stderr, "\n-------------------\n");
		/* ------------------------------------ */

		// Send RRQ packet
		if (sendto(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != 516) 
		{
			printf("%s: sendto error on socket\n",progname);
			exit(3);
		}

		// Reset buffer
		bzero (buffer, sizeof(buffer));

		// Recieve file from server
		int n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, &pserv_addr, &servlen);
		
		// Error check recieve
		if (n < 0)
		{
			printf("%s: recvfrom error\n",progname);
			exit(3);
		}
		else 
		{
			fprintf(stderr, "Successful recieve\n");
		}

		/* ---------- FOR DEBUGGING ---------- */
		// Print the recieved data packet from the server
		fprintf(stderr, "-------------------\n");
		fprintf(stderr, "Recieved data packet\n");
		for (int i = 0; i < 30; i++) 
		{
			fprintf(stderr, "0x%X,", buffer[i]);
		}
		fprintf(stderr, "\n-------------------\n");
		fprintf(stderr, "\n");
		/* ------------------------------------ */
		
		// Copy only the file data to write
		char data[512];
		bcopy(buffer + DATA_OFFSET, data, sizeof(data));

		// Write the data to a file
		FILE *fp = fopen(fileName, "w+");
		if (fputs(data, fp) == EOF)
		{
			perror("Error fputs");
			exit(1);
		}
		fclose(fp);
	}
	// WRQ
	else if (op == 2) 
	{
		
	}
} 	

/* The main program sets up the local socket for communication     */
/* and the server's address and port (well-known numbers) before   */
/* calling the dg_cli main loop.                                   */

main(argc, argv)
int     argc;
char    *argv[];
{
	int                     sockfd;
	
/* We need to set up two addresses, one for the client and one for */
/* the server.                                                     */
	
	struct sockaddr_in      cli_addr, serv_addr;
	progname = argv[0];
	// Args to input with ./client opcode file
	int op = atoi(argv[1]);

/* Initialize first the server's data with the well-known numbers. */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	
/* The system needs a 32 bit integer as an Internet address, so we */
/* use inet_addr to convert the dotted decimal notation to it.     */

	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

/* Create the socket for the client side.                          */
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	       {
		printf("%s: can't open datagram socket\n",progname);
		exit(1);
	       }

/* Initialize the structure holding the local address data to      */
/* bind to the socket.                                             */

	bzero((char *) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family      = AF_INET;
	
/* Let the system choose one of its addresses for you. You can     */
/* use a fixed address as for the server.                          */
       
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
/* The client can also choose any port for itself (it is not       */
/* well-known). Using 0 here lets the system allocate any free     */
/* port to our program.                                            */


	cli_addr.sin_port        = htons(0);
	
/* The initialized address structure can be now associated with    */
/* the socket we created. Note that we use a different parameter   */
/* to exit() for each type of error, so that shell scripts calling */
/* this program can find out how it was terminated. The number is  */
/* up to you, the only convention is that 0 means success.         */

	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
		{
		 printf("%s: can't bind local address\n",progname);
		 exit(2);
		}

	send_packet(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr), op);

/* We return here after the client sees the EOF and terminates.    */
/* We can now release the socket and exit normally.                */

	close(sockfd);
	exit(0);
}






















