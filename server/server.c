//for system calls, please refer to the MAN pages help in Linux 
// Extended echo demo code for the group project
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

#define SERV_UDP_PORT   51145 // REPLACE WITH YOUR PORT NUMBER

char *progname;
const static int MAX_BUFF_SIZE = 516;
const static int DATA_OFFSET = 4;
const static int REQUEST_OFFSET = 2;

server_listen(sockfd) 
int sockfd;
{
	printf("Server listening to requests\n");
/* struct sockaddr is a general purpose data structure that holds  */
/* information about a socket that can use a variety of protocols. */
/* Here, we use Internet family protocols and UDP datagram ports.  */
/* This structure receives the client's address whenever a         */
/* datagram arrives, so it needs no initialization.                */
	
	struct sockaddr pcli_addr;
	
/* Temporary variables, counters and buffers.                      */

	int    n, clilen;
	char   buffer[MAX_BUFF_SIZE];

/* Main echo server loop. Note that it never terminates, as there  */
/* is no way for UDP to know when the data are finished.           */

	for ( ; ; ) {
/* Initialize the maximum size of the structure holding the        */
/* client's address.                                               */

		clilen = sizeof(struct sockaddr);

/* Receive data on socket sockfd, up to a maximum of MAX_BUFF_SIZE */
/* bytes, and store them in buffer. The sender's address is stored   */
/* in pcli_addr and the structure's size is stored in clilen.      */
		
		n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, &pcli_addr, &clilen);
		
/* n holds now the number of received bytes, or a negative number  */
/* to show an error condition. Notice how we use progname to label */
/* the source of the error.                                        */

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
		// Print the recieved request packet from the client
		fprintf(stderr, "-------------------\n");
		fprintf(stderr, "Recieved request packet\n");
		for (int i = 0; i < 30; i++) 
		{
			fprintf(stderr, "0x%X,", buffer[i]);
		}
		fprintf(stderr, "\n-------------------\n");
		/* ------------------------------------ */

		// Determine opcode
		unsigned short *opCodePtrRcv = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
		fprintf(stderr, "Recieved opcode is %d\n", opCodeRcv);
		int len = 2;

		// RRQ
		if (opCodeRcv == 1) 
		{
			// Retrieve filename
			char fileName[100];
			bcopy(buffer + REQUEST_OFFSET, fileName, sizeof(fileName));

			/* ---------- FOR DEBUGGING ---------- */
			// Print filename recieved from client
			fprintf(stderr, "Recieved filename: %s\n", fileName);
			/* ------------------------------------ */
			
			// Construct data packet
			char dataPacket[MAX_BUFF_SIZE];
			bzero(dataPacket, sizeof(dataPacket));

			unsigned short *opCodePtr = (unsigned short*) dataPacket;
			// Opcode for data packet is 3 (RFC 1350)
			*opCodePtr = htons(3);
			opCodePtr++;
			
			unsigned short blockNum = 1;
			unsigned short *blockNumPtr = opCodePtr;
			*blockNumPtr = htons(blockNum);

			char *fileData = dataPacket + DATA_OFFSET;

			// Get file data from fileName
			char *file;
			FILE *fp;
			long len;

			fp = fopen(fileName, "rb"); 
			if (!fp)
			{
				perror("Error fopen");
				exit(1);
			}

			// Determine amount of bytes of file
			fseek(fp, 0L, SEEK_END);
			len = ftell(fp);
			rewind(fp);

			// Allocate mem to the file variable then read the bytes to file
			file = (char *)malloc(len * sizeof(char));
			fread(file, len, 1, fp);
			fclose(fp);

			// Copy the file data bytes into the correct location of the data packet
			bcopy(file, fileData, strlen(file));

			// Clear mem
			free(file);

			/* ---------- FOR DEBUGGING ---------- */
			// Print the datapacket that is sent to the client
			fprintf(stderr, "-------------------\n");
			fprintf(stderr, "Sent RRQ datapacket\n");
			for (int i = 0; i < 30; i++) 
			{
				fprintf(stderr, "0x%X,", dataPacket[i]);
			}
			fprintf(stderr, "\n-------------------\n");
			fprintf(stderr, "\n");
			/* ------------------------------------ */

			// Send datapacket
			if (sendto(sockfd, dataPacket, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) != MAX_BUFF_SIZE) 
			{
				printf("%s: sendto error on socket\n",progname);
				exit(3);
			}
		}
		// WRQ
		else if (opCodeRcv == 2) 
		{	

		}
		else 
		{
			printf("%s: invalid opcode recieved\n",progname);
			exit(3);
		}
	}
}

main(argc, argv)
int argc;
char *argv[];
{
	
/* General purpose socket structures are accessed using an         */
/* integer handle.                                                 */
	
	int sockfd;
	
/* The Internet specific address structure. We must cast this into */
/* a general purpose address structure when setting up the socket. */

	struct sockaddr_in serv_addr;

/* argv[0] holds the program's name. We use this to label error    */
/* reports.                                                        */

	progname=argv[0];

/* Create a UDP socket (an Internet datagram socket). AF_INET      */
/* means Internet protocols and SOCK_DGRAM means UDP. 0 is an      */
/* unused flag byte. A negative value is returned on error.        */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
		 printf("%s: can't open datagram socket\n",progname);
		 exit(1); 
		}

/* Abnormal termination using the exit call may return a specific  */
/* integer error code to distinguish among different errors.       */
		
/* To use the socket created, we must assign to it a local IP      */
/* address and a UDP port number, so that the client can send data */
/* to it. To do this, we fisrt prepare a sockaddr structure.       */

/* The bzero function initializes the whole structure to zeroes.   */
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
/* As sockaddr is a general purpose structure, we must declare     */
/* what type of address it holds.                                  */
	
	serv_addr.sin_family      = AF_INET;
	
/* If the server has multiple interfaces, it can accept calls from */
/* any of them. Instead of using one of the server's addresses,    */
/* we use INADDR_ANY to say that we will accept calls on any of    */
/* the server's addresses. Note that we have to convert the host   */
/* data representation to the network data representation.         */

	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

/* We must use a specific port for our server for the client to    */
/* send data to (a well-known port).                               */

	serv_addr.sin_port        = htons(SERV_UDP_PORT);

/* We initialize the socket pointed to by sockfd by binding to it  */
/* the address and port information from serv_addr. Note that we   */
/* must pass a general purpose structure rather than an Internet   */
/* specific one to the bind call and also pass its size. A         */
/* negative return value means an error occured.                   */

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	       { 
		printf("%s: can't bind local address\n",progname);
		exit(2);
	       }                                  

	server_listen(sockfd);                             
}
