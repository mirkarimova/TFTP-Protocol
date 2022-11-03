#include <stdio.h>
#include <string.h>

int main() {
  printf("Hello World!\n");

  FILE *fp = fopen("text.txt", "r");
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  int test = size;
  printf("%d\n\n", test);
  fseek(fp, 0, SEEK_SET);

  char buffer[512];
 
  while(1){
    memset(buffer, 0, sizeof(buffer));
    int numbyte = fread(buffer, 1, 511, fp);
    buffer[512] = '\0';
    printf("%s\n", buffer);
    printf("Size of Buffer:%d\n", numbyte);
    printf("Ftell: %d\n", ftell(fp));
    if(numbyte < 511){   /// IF 0 THEN THAT IS WHEN YOU SEND THE LAST ACK 
        break;
    }
  }

  printf("\nTEST:%d", test);   //IF NUMBYTE IS LESS THAN 0 that signifies the end of the file, full was read -- no need to send ackk.. 
  fclose(fp);
  return 0;
}