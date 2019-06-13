
							DROPBOX_CLIENT

Instruction for compilation :   make

Instruction to remove .o files :  make clean

Examples for execution : 
	./dropbox_client -d ./dir -p 6666 -w 2 -b 100 -sp 9991 -sip 195.134.65.102

	./dropbox_client -d ./dir2 -p 7777 -w 2 -b 100 -sp 9991 -sip 195.134.65.102

	./dropbox_client -d ./dir3 -p 8888 -w 2 -b 100 -sp 9991 -sip 195.134.65.102


							DROPBOX_SERVER

Instruction for compilation :   gcc -o dropbox_server dropbox_server.c

Examples for execution : 

	./dropbox_server -p 9991


Series of arguments is irrelevant as long as flags are inserted correctly!


The project files are organised as:
-> XTypes.h
-> XImplementation.c

where X is the corresponding types are being implemented, for example inside FilesImplementation.c are the implementation 
of functions that are used to handle the files and directories.
In Messages.h, i have declared the messages that play part in communication between client-server, and the prototypes of 
functions from FilesImplementation.c

At the beginning, the server opens up and waits for connections with the clients.

When the first client arrives, he sends LOG_ON message to the server with his IP-port, which are being taken using a pointer
at struct hostent.This LOG_ON is being sent from the main thread of client by calling the function logOn. This function opens
up a connection using another function named establishConnection.

Then, the main thread of client sends a GET_CLIENTS  message in order to get ip-port tuples regarding the active, currently
logged in clients from the server.

The main thread calls the waitForSelect() function which turns the client as a server by listening to client's portNum.

For the synchronization, i use two mutexes, mutex_buffer and mutex_list for buffer and list access respectively.
The condVarP,condVarC condition variables are reffered to the producer consumer problem by signaling the corresponding function
to push a bufferElement inside the circular buffer or to pop(consume) one as well.

The BufferElement(request) contains: ipaddr,port,filename[128],version and a file descriptor of the client that tries a connection.
If a request contains a file descriptor then the corresponding client answers another's client request.
If there is not a file descriptor inside a request in the circular buffer, then the client asks for the file list or the specific file.

 Sources : 
 	https://www.gnu.org/software/libc/manual/html_node/Server-Example.html    used for select method
 	https://stackoverflow.com/questions/7666509/hash-function-for-string      used for hash extraction of the contents of the files shared
	https://www.geeksforgeeks.org/linked-list-set-3-deleting-node/		used both at client and server for updating their client lists

