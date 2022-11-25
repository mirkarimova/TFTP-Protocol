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
	struct sockaddr pcli_addr;
	
	/* Temporary variables, counters and buffers.                      */
	int    n, clilen;
	char   buffer[MAX_BUFF_SIZE];

	/* Main echo server loop. Note that it never terminates, as there  */
	/* is no way for UDP to know when the data are finished.           */
	for ( ; ; ) {
		printf("\nServer listening to requests\n");
	
		// Initialize max size of structure holding clients address
		clilen = sizeof(struct sockaddr);


		// pcli_addr: senders address stored here // clilien: structure size 
		n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, &pcli_addr, &clilen);
		
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
		
			if (access(fileName, F_OK) != 0) 
			{
				// File does not exist

				int len2 = 0; 

				// Construct error packet
				char errBuffer[MAX_BUFF_SIZE];
				bzero (errBuffer, sizeof(errBuffer));

				unsigned short opCode = htons(5);
				memcpy(errBuffer, &opCode, 2);
				len2 += 2;

				unsigned short errCode = htons(1);
				memcpy(errBuffer + len, &errCode, 2);
				len2 += 2;
				
				/* ---------- FOR DEBUGGING ---------- */
				// Print the error packet that is sent to the client
				fprintf(stderr, "\n-------------------\n");
				fprintf(stderr, "Sent Error Packet\n");
				for (int i = 0; i < 30; i++) 
				{
					fprintf(stderr, "0x%X,", errBuffer[i]);
				}
				fprintf(stderr, "\n-------------------\n");
				/* ------------------------------------ */

				// Send error packet
				if (sendto(sockfd, errBuffer, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) != MAX_BUFF_SIZE) 
				{
					printf("%s: sendto error on socket\n",progname);
					exit(3);
				}
			} 
			else if (access(fileName, R_OK) != 0)
			{
				// No access permissions for file

				int len2 = 0; 

				// Construct error packet
				char errBuffer[MAX_BUFF_SIZE];
				bzero (errBuffer, sizeof(errBuffer));

				unsigned short opCode = htons(5);
				memcpy(errBuffer, &opCode, 2);
				len2 += 2;

				unsigned short errCode = htons(2);
				memcpy(errBuffer + len, &errCode, 2);
				len2 += 2;
				
				/* ---------- FOR DEBUGGING ---------- */
				// Print the error packet that is sent to the client
				fprintf(stderr, "\n-------------------\n");
				fprintf(stderr, "Sent Error Packet\n");
				for (int i = 0; i < 30; i++) 
				{
					fprintf(stderr, "0x%X,", errBuffer[i]);
				}
				fprintf(stderr, "\n-------------------\n");
				/* ------------------------------------ */

				// Send error packet
				if (sendto(sockfd, errBuffer, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) != MAX_BUFF_SIZE) 
				{
					printf("%s: sendto error on socket\n",progname);
					exit(3);
				}
			}
			else 
			{
				// Read file
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
					if(bytesLeft != 0){
						totalPackets = numPackets +1;
					}else{
						totalPackets = numPackets;
					}

				// Construct Data packet for full packet
					char dataPacket[MAX_BUFF_SIZE];

				// Construct Data packet for partial packet 
					char partialPacket[bytesLeft + 4];

				
				int fileEnd = 0;
				unsigned short blockNum = 0;
				while(fileEnd <= totalPackets){
					
					
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

					if(fileEnd < totalPackets || bytesLeft == 0 && (sizeFile != ftell(fp))){
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

						if (sendto(sockfd, dataPacket, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) != MAX_BUFF_SIZE) 
						{
							printf("%s: sendto error on socket\n",progname);
							exit(3);
						}
						
					}else{

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
						fprintf(stderr, "Sent RRQ datapacket\n");
						for (int i = 0; i < 30; i++) 
						{
						fprintf(stderr, "0x%X,", partialPacket[i]);
						}
						fprintf(stderr, "\n-------------------\n");
						fprintf(stderr, "\n");
						/* ------------------------------------ */

						if (sendto(sockfd, partialPacket, (bytesLeft +4), 0, &pcli_addr, clilen) != (bytesLeft +4)) 
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
					int n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, &pcli_addr, &clilen);
			
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
			}
		}
		// WRQ
		else if (opCodeRcv == 2) 
		{	
			// Retrieve filename
			char fileName[100];
			bcopy(buffer + REQUEST_OFFSET, fileName, sizeof(fileName));

			/* ---------- FOR DEBUGGING ---------- */
			// Print filename recieved from client
			fprintf(stderr, "Recieved filename: %s\n", fileName);
			/* ------------------------------------ */

			if (access(fileName, F_OK) == 0)
			{
				// File exists, overwrite error
				char errMsg[100] = "Overwrite error, file already exists";

				int len2 = 0; 

				// Construct error packet
				char errBuffer[MAX_BUFF_SIZE];
				bzero (errBuffer, sizeof(errBuffer));

				unsigned short opCode = htons(5);
				memcpy(errBuffer, &opCode, 2);
				len2 += 2;

				unsigned short errCode = htons(6);
				memcpy(errBuffer + len, &errCode, 2);
				len2 += 2;
				
				/* ---------- FOR DEBUGGING ---------- */
				// Print the error packet that is sent to the client
				fprintf(stderr, "\n-------------------\n");
				fprintf(stderr, "Sent Error Packet\n");
				for (int i = 0; i < 30; i++) 
				{
					fprintf(stderr, "0x%X,", errBuffer[i]);
				}
				fprintf(stderr, "\n-------------------\n");
				/* ------------------------------------ */

				// Send error packet
				if (sendto(sockfd, errBuffer, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) != MAX_BUFF_SIZE) 
				{
					printf("%s: sendto error on socket\n",progname);
					exit(3);
				}
			}
			else 
			{
				// Write file
				// Construct 0th ACK Packet
				char ackPacket[MAX_BUFF_SIZE];
				bzero(ackPacket, sizeof(ackPacket));

				unsigned short *opCodePtr = (unsigned short*) ackPacket;
				// Opcode for data packet is 4 (RFC 1350)
				*opCodePtr = htons(4);
				opCodePtr++;
				
				unsigned short blockNum = 0;
				unsigned short *blockNumPtr = opCodePtr;
				*blockNumPtr = htons(blockNum);

				/* ---------- FOR DEBUGGING ---------- */
				// Print the ACK packet that is sent to the client
				fprintf(stderr, "-------------------\n");
				fprintf(stderr, "Sent 0th ACK packet\n");
				for (int i = 0; i < 30; i++) 
				{
					fprintf(stderr, "0x%X,", ackPacket[i]);
				}
				fprintf(stderr, "\n-------------------\n");
				fprintf(stderr, "\n");
				/* ------------------------------------ */

				// Send 0th ACK
				if (sendto(sockfd, ackPacket, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) != MAX_BUFF_SIZE) 
				{
					printf("%s: sendto error on socket\n",progname);
					exit(3);
				}

				// Recieve Data
				int fileEnd = 0;
				FILE *fp = fopen(fileName, "w+");

				while (fileEnd == 0) 
				{
					blockNum++;

					// Reset buffer
					bzero (buffer, sizeof(buffer));

					// Recieve file from server
					int n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, &pcli_addr, &clilen);
					
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
					char data[513];
					bcopy(buffer + DATA_OFFSET, data, sizeof(data));
					data[513] = '\0';

					// Write the data to a file
					// FILE *fp = fopen(fileName, "w+");
					if (fputs(data, fp) == EOF)
					{
						perror("Error fputs");
						exit(1);
					}

					// Check if partial or full packet
					if (n < MAX_BUFF_SIZE) 
					{
						fileEnd = 1;
						break;
					}
					else if (n == 516)
					{
						// Construct ACK packet
						char moreAckPacket[MAX_BUFF_SIZE];
						bzero(moreAckPacket, sizeof (moreAckPacket));

						unsigned short *opCodePtr2 = (unsigned short*) moreAckPacket;
						// Opcode for ack packet is 4 (RFC 1350)
						*opCodePtr2 = htons(4);
						opCodePtr2++;
				
						unsigned short *blockNumPtr2 = opCodePtr2;
						*blockNumPtr2 = htons(blockNum);

						/* ---------- FOR DEBUGGING ---------- */
						// Print the ACK packet that is sent to the client
						fprintf(stderr, "-------------------\n");
						fprintf(stderr, "Sent ACK packet\n");
						for (int i = 0; i < 30; i++) 
						{
							fprintf(stderr, "0x%X,", moreAckPacket[i]);
						}
						fprintf(stderr, "\n-------------------\n");
						fprintf(stderr, "\n");
						/* ------------------------------------ */

						// Send ACK
						if (sendto(sockfd, moreAckPacket, MAX_BUFF_SIZE, 0, &pcli_addr, clilen) == -1 )  
						{
							printf("%s: sendto error on socket\n",progname);
							printf("Errno: %d", errno);
							exit(3);
						}	
					}
				}
				fclose(fp);
			}
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
	int port = atoi(argv[1]);

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
if (port) 
{
	serv_addr.sin_port        = htons(port);
}
else 
{
	serv_addr.sin_port        = htons(SERV_UDP_PORT);
}

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
