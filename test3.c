#include <stdio.h>
#include <string.h>

int main() {
    
    char buffer[516];
    bzero(buffer, sizeof(buffer));

    int len = 0; 

    unsigned short opCode = htons(1);
    memcpy(buffer, &opCode, 2);
	len += 2;


    
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
}