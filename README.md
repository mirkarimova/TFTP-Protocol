# CSS432_Project
Group Project for CSS 432: Implementing and testing TFTP 

## Directory Structure
- /CSS432_Project 
	- build.sh
	- /common
		- tftp.h
		- tftp.c
	- /client
		- client.c
		- compile_client.sh (run with build.sh)
		- client (executable)
	- /server
		- server.c
		- compile_server.sh (run with build.sh)
		- server (executable)
		
## Compiling the program
1. In the root directory CSS432_Project run the command: **bash build.sh** to compile all the necessary program files
2. Navigate to the client directory with **cd client/** in one terminal and server directory with **cd server/** in another terminal
3. Both directories should have a **client** and **server** executable file created from the first step
4. To start up the server run the command **./server** or if you have a port number, **./server yourPortNum**
5. To create a request to the server you will run **./client ..args..** where the args are:
	- **-r filename** or **-w filename** for a read or write request respectively
	- **-p yourPortNum** for a specified port. The server port number must match the client port number (with no port args this is true by default)
6. To summarize, running the program with **no** port args might look like this:
	- Run **./server** in the server directory
	- Run **./client -r text.txt** in the client directory
7. Running the program **with** port args might look like this:
	- Run **./server 80081**
	- Run **./client -w text.txt -p 80081**

