#include <stdio.h>
#include "tftp.h"

void logPacket(int flag, char *buffer)
{
	if (flag == 0)
	{
		// Print the recieved packet from the client
		printf("-------------------\n");
		printf("Recieved packet\n");
		for (int i = 0; i < 30; i++) 
		{
			printf("0x%X,", buffer[i]);
		}
		printf("\n-------------------\n");
	}
	else if (flag == 1)
	{
		/* ---------- FOR DEBUGGING ---------- */
		// Print the packet that is sent to the client
		printf("\n-------------------\n");
		printf("Sent packet\n");
		for (int i = 0; i < 30; i++) 
		{
			printf("0x%X,", buffer[i]);
		}
		printf("\n-------------------\n");
		/* ------------------------------------ */
	}
}

