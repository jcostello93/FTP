/********************************************************************
* Name: John Costello
* Date: 11/24/2018
* Class: CS372
* Assignment: Project 2
* Program name: ftserver.c
* Description: This program uses the sockets API and TCP protocol
*              to function as a FTP server with a Python client. 
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

// Error function used for reporting issues
void error(const char *msg) { perror(msg); exit(0); } 

// Arbitrary buffer for data connection 
int MAX_BUFFER = 2000;

// Global so this can be accessed by the signal handler
int listenSocketFD;


/*****************************************************************************************
* Function: setupConnectionAddress
* This function uses code from the 344 class. I broke this code up into several functions
* for cleaner code and so that I understand the individual parts better. 
*
* It takes in a portNumber (specified as a command line argument), as well
* as a pointer to a struct sockaddr_in struct  It sets up the serverAddress, which is 
* then used to connect to the client. It follows the TCP protocol from the SERVER side 
******************************************************************************************/

void setupConnectionAddress(int portNumber, struct sockaddr_in *serverAddress) {
    int socketFD;
    struct hostent* serverHostInfo;

    // Clear out the address struct
    memset((char*)serverAddress, '\0', sizeof(*serverAddress)); 

    // Create a network-capable socket
    serverAddress->sin_family = AF_INET; 

    // Store the port number
    serverAddress->sin_port = htons(portNumber); 

    // Any address is allowed for connection to this process
    serverAddress->sin_addr.s_addr = INADDR_ANY; 
}

/*****************************************************************************************
* Function: setupAddress
* This function uses code from the 344 class. I broke this code up into several functions
* for cleaner code and so that I understand the individual parts better. 
*
* It takes in a portNumber and hostname (specified as a command line argument), as well
* as a pointer to a struct sockaddr_in struct  It sets up the serverAddress, which is 
* then used to connect to the server. It follows the TCP protocol from the CLIENT side 
******************************************************************************************/

void setupDataAddress(int portNumber, char host[], struct sockaddr_in *serverAddress) {
    int socketFD;
    struct hostent* serverHostInfo;

    // Clear out the address struct
    memset((char*)serverAddress, '\0', sizeof(*serverAddress)); 

    // Create a network-capable socket
    serverAddress->sin_family = AF_INET; 

    // Store the port number
    serverAddress->sin_port = htons(portNumber); 

    // Convert the machine name into a special form of address
    serverHostInfo = gethostbyname(host); 

    // Check for error
    if (serverHostInfo == NULL) { fprintf(stderr, "ERROR, no such host\n"); exit(0); }

    // Copy in the address
    memcpy((char*)&serverAddress->sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); 
}

/*****************************************************************************************
* Function: createSocket
* This function uses code from the 344 class. I broke this code up into several functions
* for cleaner code and so that I understand the individual parts better. 
*
* It creates a socket and returns its FD so that it can be used elsewhere in the program. 
******************************************************************************************/

int createSocket() {
    int socketFD;
    
    // Create the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 

    // Check for error
    if (socketFD < 0) error("SERVER: ERROR opening socket");

    return socketFD;
}

/*****************************************************************************************
* Function: beginListening
* This function uses code from the 344 class. I broke this code up into several functions
* for cleaner code and so that I understand the individual parts better. 
*
* It binds a socket to the serverAdress/port (set up in setupConnectionAddress) and starts
* listening on the socket. It updates the value of sizeOfClientInfo, which is needed to 
* accept new connections. 
******************************************************************************************/

void beginListening(int listenSocketFD, struct sockaddr_in *serverAddress, struct sockaddr_in clientAddress, socklen_t *sizeOfClientInfo) {
    // Connect socket to port and enable the socket to begin listening
    if (bind(listenSocketFD, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) < 0) 
	error("ERROR on binding");

    // Flip the socket on - it can now receive up to 5 connections
    listen(listenSocketFD, 5); 

    // Get the size of the address for the client that will connect
    *sizeOfClientInfo = sizeof(clientAddress); 
}

/*****************************************************************************************
* Function: validatePort 
* This function validates the port sent in as a command line argument. 0 indicates an error. 
******************************************************************************************/

int validatePort(int port) {
    if (port >= 1024 && port <= 65535) {
        return port;
    }
    return 0; 
}

/*****************************************************************************************
* Function: getHandleSize 
* This function receives the size of the incoming handle and returns it as an int. 
* getHandle() later uses this int value. 
*
* Example: Before we receive "!!flip1##-l$$60000&&None, we receive "24" so we know the
* exact size of the incoming handle. 0 < len(handle) < 99
******************************************************************************************/

int getHandleSize(int establishedConnectionFD) {
    char handle_size_str[3]; 									
    memset(handle_size_str, '\0', 3); 
    recv(establishedConnectionFD, handle_size_str, 2, 0);
    return atoi(handle_size_str);
}

/*****************************************************************************************
* Function: getHandle
* This function receives the handle, which contains information about the client's IP, 
* command, data port, and file name if applicable. 
* 
* handle_size is passed in from getHandleSize()
******************************************************************************************/

void getHandle(int establishedConnectionFD, char *handle, int handle_size) {
    memset(handle, '\0', handle_size + 1);
    recv(establishedConnectionFD, handle, handle_size, 0);
}

/*****************************************************************************************
* Function: traverseHandle
* This function gets the meaningful information from the handle and puts it into the 
* relevant string. It returns the data port integer, which is used to establish the 
* data connection. 
*
* Code: 
* !! precedes client host
* ## precedes command
* $$ precedes data port
* && precedes file name ('None' if -l command)
******************************************************************************************/

int traverseHandle(char *handle, char *IP, char *command, char *file_name) {
    if (handle[0] != '!' || handle[1] != '!') { error("BAD HANDLE"); };

    memset(IP, '\0', 256);
    memset(command, '\0', 3);
    memset(file_name, '\0', 256);

    int handle_idx = 2;
    int i = 0; 
    while (handle[handle_idx] != '#') {
        IP[i] = handle[handle_idx];
        i = i + 1; 
        handle_idx = handle_idx + 1; 
    } 

    handle_idx = handle_idx + 2; 
    i = 0;

    while (handle[handle_idx] != '$') {
        command[i] = handle[handle_idx];
        handle_idx = handle_idx + 1;
        i = i + 1; 
    }

    handle_idx = handle_idx + 2; 
    i = 0;

    //Max Port size: 65535
    char port_str[6]; 
    memset(port_str, '\0', 6);

    while (handle[handle_idx] != '&') {
        port_str[i] = handle[handle_idx];
        handle_idx = handle_idx + 1;
        i = i + 1; 
    }

    handle_idx = handle_idx + 2; 
    i = 0;

    while (handle[handle_idx] != '*') {
        file_name[i] = handle[handle_idx];
        handle_idx = handle_idx + 1;
        i = i + 1; 
    }

    return atoi(port_str);
}

/*****************************************************************************************
* Function: connectServer
* This function uses code from the 344 class. I broke this code up into several functions
* for cleaner code and so that I understand the individual parts better. 
*
* It takes in socketFD (returned from createSocket) and a pointer to a struct sockaddr_in
* (updated in setupAddress). With these parameters, it creates a connection with the server.
******************************************************************************************/

void connectServer(int socketFD, struct sockaddr_in *serverAddress) {
    // Connect socket to address
    if (connect(socketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0) 
	error("CLIENT: ERROR connecting");
}

/*****************************************************************************************
* Function: sendMessage
* This function sends a message to the server, using the socket that we created in 
* createSocket. 
******************************************************************************************/

void sendMessage(int socketFD, char *msg) {
    send(socketFD, msg, strlen(msg), 0); 
}

/*****************************************************************************************
* Function: sendFileName
* This function sends the name of a file when the client requests a listing of the directory. 
* It sends the length of the file name before sending the file, so the client knows
* exactly how many bytes to expect. 
* 
* Example: Send "08" before "file.txt". Client now expects 8 bytes. 
******************************************************************************************/

void sendFileName(int dataSocketFD, char *file_name) {
    char file_name_length_str[3];
    memset(file_name_length_str, '\0', 2);
    int file_name_length = strlen(file_name);
    sprintf(file_name_length_str, "%d", file_name_length);

    if (file_name_length < 10) {
        file_name_length_str[1] = file_name_length_str[0];
        file_name_length_str[0] = '0';
        file_name_length_str[2] = '\0';
    }
    
    sendMessage(dataSocketFD, file_name_length_str);
    sendMessage(dataSocketFD, file_name); 
}

/*****************************************************************************************
* Function: sendDirectory
* This function uses code from the 344 class to traverse the directory.
* 
* This function goes through the files in the directory one by one and sends their names 
* to the sendFileName function. It does not send "." and ".." because they were not in 
* the example. 
*
* Sending @@ markes the end of the transfer. 
******************************************************************************************/

void sendDirectory(int dataSocketFD, char *host, int port) {
    DIR* dirToCheck; 
    struct dirent *fileInDir; 
    struct stat dirAttributes; 

    dirToCheck = opendir("."); 		
    int length = 0; 
	
    // print first 5 char (flip{x}) of string. 
    // Source: https://stackoverflow.com/questions/7780809/is-it-possible-to-print-out-only-a-certain-section-of-a-c-string-without-making
    printf("Sending directory contents to %.5s:%d\n", host, port); 

    // Loop through all file names 
    if (dirToCheck > 0) {
    	while ((fileInDir = readdir(dirToCheck)) != NULL) {
	    if (strcmp(fileInDir->d_name, ".") && (strcmp(fileInDir->d_name, ".."))) {
		sendFileName(dataSocketFD, fileInDir->d_name); 
	    }
	}
    }

    // Let client know the data transfer is over
    sendMessage(dataSocketFD, "@@");

    closedir(dirToCheck);
 
}

/*****************************************************************************************
* Function: sendFile
* I referred to my 344 code for the I/O and the structure of the while loop
*
* This function sends the contents of a text file to the client. 
* If the file is found, the server first sends "!OK!" to alert the client. 
* Else it sends "!KO!" and an error message so no data transfer takes place.
*
* @_done_@ marks the end of the file transfer if the file is found. 
******************************************************************************************/

void sendFile(int dataSocketFD, char *file_name, char *host, int port) {
    char buffer[MAX_BUFFER];
    memset(buffer, '\0', MAX_BUFFER);

    FILE *file; 
    file = fopen(file_name, "r"); 
		
	if (file) {
	    // print first 5 char (flip{x}) of string. 
	    // Source: https://stackoverflow.com/questions/7780809/is-it-possible-to-print-out-only-a-certain-section-of-a-c-string-without-making
	    printf("Sending '%s' to %.5s:%d\n", file_name, host, port); 
	    sendMessage(dataSocketFD, "!OK!");
	    while (fgets(buffer, MAX_BUFFER, file)) {
	    	sendMessage(dataSocketFD, buffer);
		memset(buffer, '\0', MAX_BUFFER);
	    }
		
	    fclose(file); 
	}
	else {
	    printf("File not found. Sending error message to %s:%d\n", host, port); 
	    sendMessage(dataSocketFD, "!KO!");
	    sendMessage(dataSocketFD, "NOT FOUND");
	}
    sendMessage(dataSocketFD, "@_done_@");
}

/*****************************************************************************************
* Function: CatchSIGINT
* This function uses code from 344 to write to stdout. 
* 
* It exits the program and closes the listenSocketFD (global). All other sockets are handled
* in main. 
******************************************************************************************/

void CatchSIGINT(int signo) {					
    char *message = "\nSIGINT Detected. Ending control connection\n : "; 
    write(STDOUT_FILENO, message, 44);
    close(listenSocketFD);

    exit(0); 
} 

/*****************************************************************************************
* Function: HandleSIGINT
* This function uses code from 344 to handle the sigaction struct. 
* 
* This function sets the handler of SIGINT to CatchSIGINT
******************************************************************************************/

void HandleSIGINT() {						
    struct sigaction SIGINT_action = {0}; 
    SIGINT_action.sa_handler = CatchSIGINT;
    sigfillset(&SIGINT_action.sa_mask); 
    SIGINT_action.sa_flags = SA_RESTART; 
    sigaction(SIGINT, &SIGINT_action, NULL); 	
}

/*****************************************************************************************
* Function: printCommand
* This function prints the client's command to the server. 
******************************************************************************************/

void printCommand(char *command, char *file_name, char *client_host, int data_port) {
    // print first 5 char (flip{x}) of string. 
    // Source: https://stackoverflow.com/questions/7780809/is-it-possible-to-print-out-only-a-certain-section-of-a-c-string-without-making
    printf("\nConnection from %.5s\n", client_host);
    if (!strcmp(command, "-l")) {
    	printf("List directory requested on port %d\n", data_port);
    }
    else {
    	printf("File '%s' requested on port %d\n", file_name, data_port);
    }
}

/*****************************************************************************************
* Function: intializeConnection
* This function initializes the control connection by receiving "ready?" from the client
* and returning "ready!". 0 indicates an error, which will not allow data transfer to takes
* place, but will still allow the server to acccept new connections. 
******************************************************************************************/

int initializeConnection(int CONNECTION_FD) {
    char initial_message[6];
    memset(initial_message, '\0', 6);
    recv(CONNECTION_FD, initial_message, 6, 0);
    if (strcmp(initial_message, "ready?")) {
    	return 0;
    }

    sendMessage(CONNECTION_FD, "ready!");

    return 1;
}

/*****************************************************************************************
* Function: establishDataConnection
* This function establishes and initializes the data connection by sending "ready?" and
* receiving "ready!" from the client. -1 indicates an error, which will prevent the server
* from going through with the data connection and will immediately close the socket. The
* server will still be able to accept new connections. 
*
* Otherwise, the function returns the dataSocketFD, which is necessary for the data transfer.
******************************************************************************************/

int establishDataConnection(int data_port, char *client_host, char *file_name) {
    // Setup address, create socket, and connect socker to address
    struct sockaddr_in serverDataAddress;
    setupDataAddress(data_port, client_host, &serverDataAddress);
    int dataSocketFD = createSocket(); 
    connectServer(dataSocketFD, &serverDataAddress);  
    
    sendMessage(dataSocketFD, "ready?");
    char confirmation[6];
    memset(confirmation, '\0', 6);

    recv(dataSocketFD, confirmation, 6, 0);
    if (strcmp(confirmation, "ready!")) {
    	return -1;
    }
    
    return dataSocketFD;
}

/*****************************************************************************************
* Function: handleCommand
* This function calls sendDirectory or sendFile according to the commands -l and -g. 
* 
* All variables are passed in from main. 
******************************************************************************************/

void handleCommand(int dataSocketFD, char *file_name, char *client_host, int data_port, char *command) {
    if (command[1] == 'l') {
    	sendDirectory(dataSocketFD, client_host, data_port);
    }
    else {
    	sendFile(dataSocketFD, file_name, client_host, data_port);
    }
}

/*****************************************************************************************
* Function: cleanup
* This function frees allocated memory. 
*
* All variables are passed in from main. 
******************************************************************************************/

void cleanup(char *client_host, char *command, char *file_name, char *handle) {
    free(client_host);
    free(command);
    free(file_name);
    free(handle);
}

/*****************************************************************************************
* Function: main
* The main function validates parameters, opens itself on a port, and listens for new
* connections from the client. 
* 
* When it accepts a new connection, it initializes the connection with a short back and
* forth between the client and server. If this fails, the server will listen for new 
* connections again. 
*
* Otherwise, it receives a handle from the client containing it's host name, desired
* data port, command, and file_name if applicable. With this information, it establishes
* a data connection with the client and the roles are reversed. It initializes the data
* connection with a short back and forth. If this fails, the data connection is immediately
* closed and the server will again listen for new connections. 
*
* Otherwise, it carries out the relevant command. If command is -l, it sends all file
* names to the client. If command is -g, it sends the contents of the file to the client
* if it exists. Otherwise it sends an error message.
*
* The data and control sockets are closed with each iteration. The listenSocketFD is closed
* in the signal handler, which exits the program. 
******************************************************************************************/

int main(int argc, char *argv[]) {
    // Check usage & args
    if (argc < 1) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } 

    HandleSIGINT();

    // Exit if port is not valid
    int CONNECTION_PORT = validatePort(atoi(argv[1])); 
    if (!CONNECTION_PORT) {
        printf("ERROR: Invalid port\n");
        return 0;
    }

    // Prepare necessary objects for control connection
    int CONNECTION_FD;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverConnectionAddress, clientAddress;

    // Setup address, create socket, and begin listening on socket
    setupConnectionAddress(CONNECTION_PORT, &serverConnectionAddress);
    listenSocketFD = createSocket(); 
    beginListening(listenSocketFD, &serverConnectionAddress, clientAddress, &sizeOfClientInfo);

    printf("Server open on %d\n", CONNECTION_PORT);

    while (1) {
	// Accept a new connection on the control port 
        CONNECTION_FD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);  

	// Must receive "ready!" from client to establish data connection
	if (initializeConnection(CONNECTION_FD)){
	    char *client_host = malloc(256);
	    char *file_name = malloc(256);
	    char *command = malloc(3);		  

	    // Get (incoming) handle_size ('00' - '99') and handle and traverse through the handle
	    int handle_size = getHandleSize(CONNECTION_FD);
	    char *handle = malloc(handle_size + 1);
	    getHandle(CONNECTION_FD, handle, handle_size); 
	    int data_port = traverseHandle(handle, client_host, command, file_name);
    
	    printCommand(command, file_name, client_host, data_port);

	    // Initialize data connection and carry out the command
	    int dataSocketFD = establishDataConnection(data_port, client_host, file_name);
	    if (dataSocketFD != -1) {
	    	handleCommand(dataSocketFD, file_name, client_host, data_port, command);				
	    }
	    else { printf("Could not initialize data connection\n"); }

	    // Data connection is now over. Close socket and free allocated memory
	    close(dataSocketFD);
	    cleanup(client_host, file_name, command, handle);
	}
	else { printf("Could not initialize control connection\n"); }

	// Close this after each client process 
	close(CONNECTION_FD);
    }
	
    // Close when exiting the program. See signal handler
    close(listenSocketFD);
    return 0; 
}
