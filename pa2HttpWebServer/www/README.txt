Http Server - A simple TCP web server with threads by Willie Chew

What is provided
----------------
The following directory contains a TCP web server(httpServer.c), a make file to compile the web server, 
and various files and directories that provide resources for the front end of the web application. 
The httpServer was written in c and was intended to run on a linux environment. 


How it works
------------
The web server works by listening to a desired port for http requests. Once a request is recieved on the 
port, a new thread is created to handle the request. The new thread processes the request and is closed.


How to compile
--------------
- Open up a terminal and navigate it to the directory of httpServer 
- Type 'make' into the terminal to trigger the makefile
	+ This should compile the program 


How to run
----------
- Open up a terminal and navigate it to the directory of httpServer 
- Make sure the program is complied with the intrustions above 
- Enter the following command in the terminal: './httpServer <port>'
	+ replace <port> with the desired port number for the application to listen to


Using the server
----------------
The methods below require both the server and the clients to be connected to the same network

Using a Web Browser:
- open up a prefered web browser(tested with chrome and firefox)
- enter the address and port of the webserver into the seach bar
	+ enter 'ip addr' on the machine that is running the web server to find the address

Using Telnet:
- enter 'telnet <address> <port>' into your commandline to connect to the server via telnet
	+ replace <address> with the web server's address
	+ replace <port> with the chosen port that the webserver is listening on
- once connected, try the following commands:
	+ GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: Keep-alive\r\n\r\n
	+ HEAD /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: Keep-alive\r\n\r\n
	+ POST /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: Keep-alive\r\n\r\n