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



// Timeout occured update retransmit tries
void sig_handler(int signum)
{
	printf("\nTimeout Occured\n");
}



int send_packet(sockfd, pserv_addr, servlen, request, fileName)
int sockfd;
struct sockaddr *pserv_addr;
int servlen;
char *request;
char *fileName; 
{


	struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    if(sigfillset(&handler.sa_mask) < 0){
        return 1;
	}

    if(sigaction(SIGALRM, &handler, 0) < 0){
        return 2;
	}

	int tries = 0; 

	char buffer[516];
	bzero (buffer, sizeof(buffer));

	// RRQ
	if (strcmp(request, "-r") == 0)
	{


		// Construct RRQ packet
		int len = 0;

		unsigned short opCode = htons(1);
		memcpy(buffer, &opCode, 2);
		len += 2;

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
		if (sendto(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != MAX_BUFF_SIZE) 
		{
			printf("%s: sendto error on socket\n",progname);
			exit(3);
		}
		printf("Setting alarm\n");
		alarm(5); // Initial RRQ alarm


		int fileEnd = 0; 
		FILE *fp;
		unsigned short blockNum = 0;
		while(fileEnd == 0){
			
			blockNum++;

			// Reset buffer
			bzero (buffer, sizeof(buffer));

			
			// Recieve file from server
			// Error check recieve
			int n; 
			while (	(n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, &servlen)) < 0)
			{	
				if(errno = EINTR){
					printf("Timeout triggered!\n");
					tries++; 
					if(tries <= 10){
						printf("Retransmission Try #: %d\n", tries);
							if (sendto(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != MAX_BUFF_SIZE) 
							{
								printf("%s: sendto error on socket\n",progname);
								exit(3);
							}
						alarm(5);
					}else{
						printf("Terminating transmission tries exceeded\n");
						exit(1);
					}
				}else{
				  exit(3);
				}
			}
			alarm(0);
			tries = 0; 


			fprintf(stderr, "Successful recieve: %d\n: ", n);

			
			// Uncomment below to test packet loss for RRQ ack
			// sleep(60);

			/* ---------- FOR DEBUGGING ---------- */
			// Print the recieved data packet from the server
			fprintf(stderr, "-------------------\n");
			fprintf(stderr, "Recieved data packet\n");
			for (int i = 0; i < 520; i++) 
			{
			fprintf(stderr, "0x%X,", buffer[i]);
			}
			fprintf(stderr, "\n-------------------\n");
			fprintf(stderr, "\n");
			/* ------------------------------------ */

			// ERROR CHECK: File does not exist on server/Does not have permission to read 
			
			// Determine opcode
			unsigned short *opCodePtrRcv = (unsigned short*) buffer;
			unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
			fprintf(stderr, "Recieved opcode is %d\n", opCodeRcv);
			fprintf(stderr, "\n-------------------\n");

			// If Error Packet 
			if(opCodeRcv == 5){
				opCodePtrRcv++;
				unsigned short errorCode = ntohs(*opCodePtrRcv);
				printf("Recieved error code is %d\n", errorCode);
				printf("-----------------------------\n");

				if(errorCode == 1){
					printf("Error: File not found on server\n");
				}else if(errorCode == 2){
					printf("Error: File does not have read permissions\n");
				}

				exit(1);
			}

			if(blockNum == 1){
				fp = fopen(fileName, "w+");
			}

			// Copy only the file data to write
			char data[512];
			bcopy(buffer + DATA_OFFSET, data, sizeof(data));

			// Write the data to a file
			//FILE *fp = fopen(fileName, "w+");
			if (fputs(data, fp) == EOF)
			{
				perror("Error fputs");
				exit(1);
			}
			if(n < 516){
				fileEnd = 1; 
				break;
			}else if(n == 516){  // SEND ACK PACKET 
				// Construct ACK Packet
				char ackPacket[MAX_BUFF_SIZE];
				bzero(ackPacket, sizeof(ackPacket));

				unsigned short *opCodePtr = (unsigned short*) ackPacket;
				// Opcode for ack packet is 4 (RFC 1350)
				*opCodePtr = htons(4);
				opCodePtr++;
			
				unsigned short *blockNumPtr = opCodePtr;
				*blockNumPtr = htons(blockNum);

				/* ---------- FOR DEBUGGING ---------- */
				// Print the ACK packet that is sent to the server
				fprintf(stderr, "-------------------\n");
				fprintf(stderr, "Sent ACK packet\n");
				for (int i = 0; i < 30; i++) 
				{
					fprintf(stderr, "0x%X,", ackPacket[i]);
				}
				fprintf(stderr, "\n-------------------\n");
				fprintf(stderr, "\n");
				/* ------------------------------------ */

				// Send ACK
				if (sendto(sockfd, ackPacket, MAX_BUFF_SIZE, 0, pserv_addr, servlen) == -1 )  
				{
					printf("%s: sendto error on socket\n",progname);
					printf("Errno: %d", errno);
					exit(3);
				}
				alarm(5); // Set alarm for ACK packet 
			}
		}
		fclose(fp);

	}
	// WRQ
	else if (strcmp(request, "-w") == 0) 
	{

		// Construct WRQ packet
		int len = 0;

		unsigned short opCode = htons(2);
		memcpy(buffer, &opCode, 2);
		len += 2;

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

		// Send WRQ packet
		if (sendto(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != MAX_BUFF_SIZE) 
		{
			printf("%s: sendto error on socket\n",progname);
			exit(3);
		}
		alarm(5); // Set alarm for receiving WRQ ACK packet 

		// Reset buffer
		bzero (buffer, sizeof(buffer));

		// Recieve 0th ACK

		// Recieve file from server
		int n; 
		while (	(n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, &servlen)) < 0)
		{	
			if(errno = EINTR){
				printf("Timeout triggered!\n");
				tries++; 
				if(tries <= 10){
					printf("Retransmission Try #: %d\n", tries);
						if (sendto(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != MAX_BUFF_SIZE) 
						{
							printf("%s: sendto error on socket\n",progname);
							exit(3);
						}
					alarm(5);
				}else{
					printf("Terminating transmission tries exceeded\n");
					exit(1);
				}
			}else{
				exit(3);
			}
		}
		alarm(0);
		tries = 0; 

		/* ---------- FOR DEBUGGING ---------- */
		// Print the recieved data packet from the server
		fprintf(stderr, "-------------------\n");
		fprintf(stderr, "Recieved ack packet\n");
		for (int i = 0; i < 30; i++) 
		{
			fprintf(stderr, "0x%X,", buffer[i]);
		}
		fprintf(stderr, "\n-------------------\n");
		/* ------------------------------------ */

		// Determine opcode
		unsigned short *opCodePtrRcv = (unsigned short*) buffer;
		unsigned short opCodeRcv = htons(*opCodePtrRcv);
		fprintf(stderr, "Recieved opcode is %d\n", opCodeRcv);

		// Uncomment below to test packet loss for WRQ ack
		// sleep(60);

		if (opCodeRcv == 4) 
		{
			FILE *fp = fopen(fileName, "r");
  			fseek(fp, 0, SEEK_END);
  			int sizeFile = ftell(fp);
 			printf("Size of file: %d\n\n", sizeFile);
  			fseek(fp, 0, SEEK_SET);

			// how many bytes left in extra packet 
			int bytesLeft = sizeFile % 512;
			printf("Bytes left in extra packet: %d\n", bytesLeft);

			// how many full packets sent 
			int numPackets = sizeFile / 512;
			printf("Number of full packets sent: %d\n", numPackets);

			// total packets sent
			int totalPackets = 0;
			if(bytesLeft != 0)
			{
				totalPackets = numPackets + 1;
			}
			else
			{
				totalPackets = numPackets;
			}

			// Construct Data packet for full packet
			char dataPacket[MAX_BUFF_SIZE];

			// Construct Data packet for partial packet 
			char partialPacket[bytesLeft + 4];
			
			int fileEnd = 0;
			unsigned short blockNum = 0;
			while(fileEnd <= totalPackets)
			{
				fileEnd++;
				blockNum++;

				// Full packets 
				bzero(dataPacket, sizeof(dataPacket));
				unsigned short *opCodePtr = (unsigned short*) dataPacket;
				*opCodePtr = htons(3);   // Opcode for data packet is 3 (RFC 1350)
				opCodePtr++;
				unsigned short *blockNumPtr = opCodePtr;
				*blockNumPtr = htons(blockNum);
				char *fileData = dataPacket + DATA_OFFSET;

				// Partial Packet
				bzero(partialPacket, sizeof(partialPacket));
				unsigned short *opCodePtr2 = (unsigned short*) partialPacket;
				*opCodePtr2 = htons(3);
				opCodePtr2++;
				unsigned short *blockNumPtr2 = opCodePtr2;
				*blockNumPtr2 = htons(blockNum);
				char *fileData2 = partialPacket + DATA_OFFSET;


				// Get file data from fileName
				char *file;
				int numbyte = 0;
				

				if(fileEnd < totalPackets || bytesLeft == 0 && (sizeFile != ftell(fp)))  // full packet 
				{
					// Allocate mem to the file variable then read the bytes to file
					file = (char *)malloc(512 * sizeof(char));   /// len = 512
    				numbyte = fread(file, 1, 512, fp);	


    				printf("\nSize of Buffer:%d\n", numbyte);
    				printf("Ftell: %d\n", ftell(fp));

					// Copy the file data bytes into the correct location of the data packet
			        bcopy(file, fileData, strlen(file));

					// Clear mem
					free(file);

					/* ---------- FOR DEBUGGING ---------- */
					// Print the datapacket that is sent to the server
					fprintf(stderr, "-------------------\n");
					fprintf(stderr, "Sent WRQ datapacket\n");
					for (int i = 0; i < 30; i++) 
					{
						fprintf(stderr, "0x%X,", dataPacket[i]);
					}
					fprintf(stderr, "\n-------------------\n");
					fprintf(stderr, "\n");
					/* ------------------------------------ */

					if (sendto(sockfd, dataPacket, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != MAX_BUFF_SIZE) 
					{
						printf("%s: sendto error on socket\n",progname);
						exit(3);
					}
					alarm(5);
				} 
				else // partial packet 
				{

					file = (char *)malloc(bytesLeft * sizeof(char));
					numbyte = fread(file, 1, bytesLeft, fp);


    				printf("\nSize of Buffer:%d\n", numbyte);
    				printf("Ftell: %d\n", ftell(fp));

					bcopy(file, fileData2, strlen(file));

					// Clear mem
					free(file);

					/* ---------- FOR DEBUGGING ---------- */
					// Print the datapacket that is sent to the client
					fprintf(stderr, "-------------------\n");
					fprintf(stderr, "Sent WRQ datapacket\n");
					for (int i = 0; i < 30; i++) 
					{
					fprintf(stderr, "0x%X,", partialPacket[i]);
					}
					fprintf(stderr, "\n-------------------\n");
					fprintf(stderr, "\n");
					/* ------------------------------------ */

					if (sendto(sockfd, partialPacket, (bytesLeft + 4), 0, pserv_addr, servlen) != (bytesLeft + 4)) 
					{
						printf("%s: sendto error on socket\n",progname);
						exit(3);
					}

				}

				// If last packet is less than 512 break no need for ACK 
				if(numbyte < 512){
					break;
				}

				// Reset buffer
				bzero (buffer, sizeof(buffer));

				// Recieve ACK
				// Recieve file from server
				int n; 
				while (	(n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, &servlen)) < 0)
				{	
					if(errno = EINTR){
					printf("Timeout triggered!\n");
					tries++; 
					if(tries <= 10){
						printf("Retransmission Try #: %d\n", tries);
							if (sendto(sockfd, buffer, MAX_BUFF_SIZE, 0, pserv_addr, servlen) != MAX_BUFF_SIZE) 
							{
								printf("%s: sendto error on socket\n",progname);
								exit(3);
							}
						alarm(5);
					}else{
						printf("Terminating transmission tries exceeded\n");
						exit(1);
					}
				}else{
					exit(3);
				}
				}
				alarm(0);
				tries = 0; 

				/* ---------- FOR DEBUGGING ---------- */
				// Print the recieved data packet from the server
				fprintf(stderr, "-------------------\n");
				fprintf(stderr, "Recieved ack packet\n");
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
				fprintf(stderr, "\n-------------------\n");
			}
		} else if(opCodeRcv == 5)
		{
			opCodePtrRcv++;
			unsigned short errorCode = ntohs(*opCodePtrRcv);
			printf("Recieved error code is %d\n", errorCode);
			printf("-----------------------------\n");

			if(errorCode == 6){
				printf("Error: File already exists \n");
			}

		}		
	}
} 	

/* The main program sets up the local socket for communication     */
/* and the server's address and port (well-known numbers) before   */
/* calling the dg_cli main loop.                                   */

int main(argc, argv)
int     argc;
char    *argv[];
{
	int                     sockfd;
	
/* We need to set up two addresses, one for the client and one for */
/* the server.                                                     */
	
	struct sockaddr_in      cli_addr, serv_addr;
	progname = argv[0];
	
	
	// Args to input with ./client opcode file
	char request[50];
	strcpy(request, argv[1]);
	char filename[50];
	strcpy(filename, argv[2]);
	printf("Request: %s\n", request);
	printf("Filename: %s\n", filename);


	// Checking if file already exists RRQ, checking if file does not exist WRQ
	if(strcmp(request, "-r") == 0){
		FILE *file; 
		file = fopen(filename, "r");
		if(file){
			fclose(file);
			printf("ERROR: File already exists\n");
			exit(1);
		}else{
			printf("Success: File does not exist\n"); 
		}
	}else if(strcmp(request, "-w") == 0){
		FILE *file;
		file = fopen(filename, "r");
		if(file){
			printf("Success: File exists\n");
		}else{	
			printf("ERROR: File either does not exist or no access priviledges\n");
			exit(1);
		}
	}else{
		printf("Invalid request\n");
		exit(1);
	}
	


    // Port number command line arguments 
	char portNum[50];
	int iPortNum = 0;
	if(argv[3] != NULL && strcmp(argv[3], "-p") == 0 && argv[4] != NULL){
		strcpy(portNum, argv[4]);
		iPortNum = atoi(portNum);
		printf("Port number input: %d\n", iPortNum);
	}


/* Initialize first the server's data with the well-known numbers. */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	
/* The system needs a 32 bit integer as an Internet address, so we */
/* use inet_addr to convert the dotted decimal notation to it.     */

	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	
	if(iPortNum == 0){
		serv_addr.sin_port  = htons(SERV_UDP_PORT);
	}else{
		serv_addr.sin_port  = htons(iPortNum);
	}
	
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

	send_packet(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr), request, filename);

/* We return here after the client sees the EOF and terminates.    */
/* We can now release the socket and exit normally.                */

	close(sockfd);
	exit(0);
}























