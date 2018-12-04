# FTP
This project allows a client (Python) to request text files to be sent from a server (C)

## ftserver.c
This program acts as a server for the initial TCP control connection. After it receives a valid request from the client, it connects to the second TCP data transmission connection and assumes the role of the client. Depending on the request, it sends a listing of the directory or the contents of the requested text file if it exists in the directory. When the transmission is finished, it closes the second TCP connection and returns to listening for new connections on the initial TCP control connection. 

## ftclient.py
This program acts as a client for the initial TCP control connection. The user may request a listing of the server's directory or the transmission of a specific text file. In the latter case, this program opens a second TCP data transmission connection and acts as the server, waiting to receive the text contents from its counterpart. After receiving all contents, it saves the contents in a new file (as long as it doesn't exist in the directory) and closes both connections. 

## Commands
* -l requests a listing of the server's directory
* -g requests the transmission of a text file 

## Getting started 
1. Run the server:   
   ```
   $ make
   $ ./ftserver $CONTROL_PORT
   ```

2. Run the client. If command is -g, use the first format. If -l, use the second. 
   ```
   $ chmod +x ftclient
   $ ./ftclient $SERVER_HOST $CONTROL_PORT $COMMAND $FILE_NAME $DATA_PORT
   $ ./ftclient $SERVER_HOST $CONTROL_PORT $COMMAND $DATA_PORT
   ```
   
## Notes
* Duplicate files will not be saved. Therefore, it's critical to place the server and client in different directories. 
