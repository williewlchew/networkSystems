Simple Web Proxy - A simple web proxy with threads by Willie Chew

What is provided
----------------
The following directory contains a web proxy program(webproxy.c), a make 
file to compile the web server, and an empty file named 'blacklist'.
The httpServer was written in c and was intended to run on a linux environment. 


How it works
------------
When started, the program reads off the names from the 'blacklist' file. Each 
name has their hashes calculated through a hashing function and recorded in a 
hash table that records the blacklisted names. The program listens to the user 
set port. When a client tries to connect to the program via the port, a 
connection is established and a new thread of the program is spun up. The new 
tread is then passed the socket and will be responsible for handling the 
connected user. The thread first checks if the name or address of the connected 
client is in the blacklist. If the client is in the blacklist, a 403 response is 
sent back and the connection is terminated. If not, the program will then accept 
requests from the client. When a request is recieved from the client, the thread 
will parse out the request for a hostnmame, path, and body. The hostname is 
first checked in the blacklist, a 403 response is sent back and the connection 
terminated. If not in the blacklist, the program will then check if the contents 
of the desired address was cached by the program in an earlier search. Records 
of cached content by the program are stored in a hash table. The program takes 
the hash code of the host and path and searches the hash table using the code. 
If a cache hit occurs and the cached object is not stale, the cached contents 
are returned to the user. If a cache hit does not occur, the user inputted 
hostname is checked for an address via a dns lookup. If a name is not found, a 
404 is returned. If found, the program will then check if the request is a valid 
GET request. Any requests that are not a GET are sent a 403 respone. When all 
the checks are passed, the program will then a TCP connection to the desired 
address will be attempted. If the connection failed, a 400 is returned. If 
successful, the returned contents are forwarded back to the client. The contents 
are then cahced by being written in to a file. A new entry is made for the 
returned contents in the hash table and stored in the caching hash table under 
the calculated hash code of the host and path. 


How to compile
--------------
- Open up a terminal and navigate it to the directory of httpServer 
- Type 'make' into the terminal to trigger the makefile
	+ This should compile the program 


How to run/use
--------------
- Fill the 'blacklist' file with names or ip's that you want to block
	- each address should be separated by an endline
	- example:
		www.facebook.com
		www.cnn.com
		127.0.0.1
	- can be left blank if no addresses need to be blocked
- Open up a terminal and navigate it to the directory of webproxy.c 
- Make sure the program is complied with the intrustions above 
- Enter the following command in the terminal: './webproxy <port> <expiration>'
	+ replace <port> with the desired port number for the application to listen to
	+ replace <expiration> with the desired ammount of seconds for cached pages to be active
		+ this argument is optional and will default to 60 if left blank


Using the proxy
---------------
The methods below require both the server and the clients to be connected to the same network

Using a Web Browser:
- open up a prefered web browser(tested with chrome and firefox)
- configure the browser to connect with the proxy
	- each browser has its own process to achieve this
- enter the desired address into the search bar

Using Telnet:
- enter 'telnet <address> <port>' into your commandline to connect to the proxy via telnet
	+ replace <address> with the web proxy's address
	+ replace <port> with the chosen port that the proxy is listening on
- the proxy only works with GET requsets
- try the following example command:
	+ GET http://www.google.com HTTP/1.1