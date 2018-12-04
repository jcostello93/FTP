'''
Name: John Costello
Date: 11/24/2018
Description: This program uses the sockets API and TCP protocol
             to function as a FTP client with a C server. 
'''

import sys
from socket import *

# Arbitrary buffer size 
MAX_BUFFER = 2000

# Valid commands as per the project specification
VALID_COMMANDS = set(["-l", "-g"])


'''
Function: receiveMessage

This function takes the connection socket created in main as well as the 
size of the message we are expecting to receive. It decodes this message
so we can print it as a normal string and returns it. 
'''

def receiveMessage(connectionSocket, size):
    sentence = connectionSocket.recv(size)
    return sentence.decode("utf-8")

'''
Function: sendMessage

This function takes the connection socket created in main as well as the 
message to be sent. It encodes it to a normal string and sends it to 
the server. 
'''

def sendMessage(connectionSocket, msg):
    connectionSocket.send(str.encode(msg))

'''
Function: validatePort

This function validates the port sent in as a command line argument. False indicates an error.

It is used to validate both the control port and the data port. 
'''

def validatePort(port):
    if port >= 1024 and port <= 65535: 
        return True
    return False

'''
Function: validateCommand

This function validates the command sent in as a command line argument. False indicates an error.
'''

def validateCommand(command):
    if command in VALID_COMMANDS:
        return True
    return False

'''
Function: validateParameters

This function validates the parameters sent in as a command line argument. It returns
an error message to indicate what exactly went wrong. 

It uses the validatePort and validateCommand functions but also checks the number of
parameters. It also makes sure the ports are integers before validating them. 
'''

def validateParameters(parameters):
    if len(parameters) < 5 or len(parameters) > 6: return "Invalid number of parameters"

    if len(parameters) == 5 and parameters[3] == "-g": return "Invalid parameters"

    if len(parameters) == 6 and parameters[3] == "-l": return("You cannot specify a file name with the -l command")
            

    try:
        connection_port = int(parameters[2])
        data_port = int(parameters[-1])
    except: 
        return "Port must be a number"

    if connection_port == data_port: 
        return "Connection port and data port must be different"

    if not validatePort(connection_port): return "Invalid connection port"
    if not validateCommand(parameters[3]): return "Invalid command"
    if not validatePort(data_port): return "Invalid data port"

'''
Function: createHandle

This function sends a handle to the server so the server knows which command to execute
and how to establish a data connection. 

Code: 
!! precedes IP
## precedes command
$$ precedes data port
&& precedes file_name if applicable. Otherwise it precedes "None"
'''

def createHandle(ip, command, data_port, file_name):
    handle = ''
    handle += "!!" + ip
    handle += "##" + command
    handle += "$$" + str(data_port)
    handle += "&&"
    if file_name:
        handle += file_name 
    else:
        handle += "None"

    handle += "**"

    return handle 


'''
Function: setupDataConnection

This function sets up the data connection from the server side. 
'''

def setupDataConnection(port):
    serverSocket = socket(AF_INET,SOCK_STREAM)
    # Refereed to this link regarding the next line
    # http://www.cs.utexas.edu/~cannata/networking/Homework/WebServer.py
    serverSocket.bind(("",port))
    serverSocket.listen(1)
    return serverSocket

'''
Function: receiveFileName

This function receives a file name after requesting a listing of the directory. 
It is called by the printDirectory function. 

It receives the length of the incoming file name before the file name itself
so the client knows exactly how many bytes to expect.

Example: Receives "08" before "file.txt". Client now expects 8 bytes. 

@@ marks the end of the directory listing. 
'''

def receiveFileName(connectionSocket):
    response = receiveMessage(connectionSocket, 2)
    if response != "@@":
        file_name_length = int(response)
        file_name = receiveMessage(connectionSocket, file_name_length)
        return file_name
    return None

'''
Function: printDirectory

This function receives file names (and their preceding lengths) from the server one by one. 
It calls the receiveFileName function to receive each file name. 
'''

def printDirectory(connectionSocket, host, port):
    print("Receiving directory structure from", host + ":" + str(port))

    # Pythonic do-while loop
    while True:
        response = receiveFileName(connectionSocket)
        if response: 
            print(response)
        else:
            break

'''
Function: writeFile

This function receives the contents of the requested text file. 
First it receives a confirmation message from the server whether the file
exists. If it doesn't, no data transfer takes place and no file is created

Otherwise, it receives the file in MAX_BUFFER increments until receiving @_done_@
If the file is a duplicate (aleady exists in the client's directory), the data is
received, but not written to the file. No new file is created and an error message
is printed to the console.

Otherwise, it creates a new file with the received contents. 

@_done_@ marks the end of the directory listing. 
'''

def writeFile(connectionSocket, file_name, host, port):
    import os.path
	
	# Initial message indicates whether file exists in server's directory
    existsOnServer = receiveMessage(connectionSocket, 4)
    if existsOnServer == "!KO!":
        error_msg = receiveMessage(connectionSocket, 9)
        print(host + ":" + str(port), "says", error_msg)
        return
	
	# Found is true if file already exists in client's directory 
    # Taken from: https://stackoverflow.com/questions/82831/how-do-i-check-whether-a-file-exists-without-exceptions
    found = os.path.isfile(file_name) 
    if not found: 
        f = open(file_name, 'w')	

    # Pythonic do-while loop 
    done = False
    while not done:
        response = receiveMessage(connectionSocket, MAX_BUFFER)
        if "@_done_@" in response:
            response = response.replace("@_done_@", "")
            done = True
        if not found: 
            f.write(response)        
   
    if found:
        print("DUPLICATE FILE: ", file_name, " not saved to disk.")
    else:
        print("File transfer complete")
        f.close()

'''
Function: intializeControlConnection

Initialize control connection with server by receiving "ready?" and sending "ready!"
If this fails, the program stops and no data transfer occurs. 
'''

def initializeControlConnection(CONTROL_SOCKET):
    sendMessage(CONTROL_SOCKET, "ready?")
    confirmation = receiveMessage(CONTROL_SOCKET, 6)
    if confirmation != "ready!":
        print("ERROR: establishing connection")
        return False
    
    return True

'''
Function: intializeDataConnection

Initialize data connection with server by sending "ready?" and receiving "ready!"
If this fails, the program stops and no data transfer occurs. 
'''

def initializeDataConnection(DATA_CONNECTION):
    initial_message = receiveMessage(DATA_CONNECTION, 6)
    if initial_message != "ready?":
        print("ERROR: establishing connection")
        return False

    sendMessage(DATA_CONNECTION, "ready!")
    return True

'''
Function: sendHandle

First send length of incoming handle, then send handle with 
host, command, data port, and file name. 
'''

def sendHandle(CONTROL_SOCKET, HOST, COMMAND, DATA_PORT, FILE_NAME):
    handle = createHandle(HOST, COMMAND, DATA_PORT, FILE_NAME)
    sendMessage(CONTROL_SOCKET, str(len(handle)))
    sendMessage(CONTROL_SOCKET, handle)

def handleDataTransfer(DATA_CONNECTION, FILE_NAME, TCP_IP, DATA_PORT):
    if FILE_NAME:
        writeFile(DATA_CONNECTION, FILE_NAME, TCP_IP, DATA_PORT)
    else:
        printDirectory(DATA_CONNECTION, TCP_IP, DATA_PORT)


'''
Function: main

This function receives the contents of the requested text file. 
First it receives a confirmation message from the server whether the file
exists. If it doesn't, no data transfer takes place and no file is created

Otherwise, it receives the file in MAX_BUFFER increments until receiving @_done_@
If the file is a duplicate (aleady exists in the client's directory), the data is
received, but not written to the file. No new file is created and an error message
is printed to the console.

Otherwise, it creates a new file with the received contents. 

@_done_@ marks the end of the directory listing. 
'''

def main(): 
    # End program if all parameters are not valid 
    error = validateParameters(sys.argv)
    if error: 
        print(error) 
        return

    # Get info from command line parameters. FILE_NAME is None unless specified
    TCP_IP = sys.argv[1]
    CONTROL_PORT = int(sys.argv[2])
    COMMAND = sys.argv[3]
    DATA_PORT = int(sys.argv[-1])
    FILE_NAME = sys.argv[4] if len(sys.argv) == 6 else None

    # Connect to the server (control connection). End program if fails
    try:
        CONTROL_SOCKET = socket(AF_INET, SOCK_STREAM)
        CONTROL_SOCKET.connect((TCP_IP, CONTROL_PORT))
        HOST = gethostname()
    except:
        print("Could not establish connection with", TCP_IP)
        return

    # Initialize connection with "ready!". End program if fails 
    if not initializeControlConnection(CONTROL_SOCKET): return 

    # Create handle, send its incoming length, then send the handle
    sendHandle(CONTROL_SOCKET, HOST, COMMAND, DATA_PORT, FILE_NAME)

    # Setup data connection 
    DATA_SOCKET = setupDataConnection(DATA_PORT)
    DATA_CONNECTION, addr = DATA_SOCKET.accept()

    # Initialize connection with "ready?". End program if fails
    if not initializeDataConnection(DATA_CONNECTION): return 
    
    # Receive either file names of file contents based on parameters
    handleDataTransfer(DATA_CONNECTION, FILE_NAME, TCP_IP, DATA_PORT)

    # Close all connections and sockets with each instance
    DATA_CONNECTION.close()
    DATA_SOCKET.close()
    CONTROL_SOCKET.close()



if __name__ == "__main__":
    main()
