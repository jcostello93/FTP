John Costello

1. Put the files in different directories or else you will always get the duplicate file error (as expected)
2. Run the server:   
   
   $ make
   $ ./ftserver $PORT

3. Run the client (using python3). If command is -g, use the first format. If -l, use the second. 
   
   $ chmod +x ftclient
   $ ./ftclient $SERVER_HOST $CONTROL_PORT $COMMAND $FILE_NAME $DATA_PORT
   $ ./ftclient $SERVER_HOST $CONTROL_PORT $COMMAND $DATA_PORT

Note: 

I first developed locally using Windows Visual Studio code and tested locally. 
When I uploaded the files to flip, I used Notepad++ to make edits. 
After I edit a file, I sometimes get an error compiling the C file or starting the Python file for the first
time only. 

If this happens, please just try to compile using the makefile and start the Python script again. There
should be no more errors. 

I think this was related to the file encoding for the C file and the indentation for the Python file. 

