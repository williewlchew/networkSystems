A simple UDP file transfer server and client by Willie Chew 

How to compile:
1. Navagate a terminal to the server directory
2. Enter 'make' in the terminal to compile the server
3. Navagate a terminal to the client directory
4. Enter 'make' in the terminal to compile the client


Running the server:
1. Navagate a terminal to the server directory
2. Enter the following command './server <port>'
3. Replace <port> with the desired port number
4. Enter 'hostname -i' to aquire the ip address that the server is operating out of


Running the client:
1. Navagate a terminal to the client directory
2. Enter the following command './client <ip> <port>'
3. Replace <ip> with the ip address of the server
4. Replace <port> with the port that the server is listening from
5. Press Cntrl+C to exit the client


Using the client:
The client supports the following commands:
1. get <filename>
	- puts an copy of the specified file in the local directory
2. put <filename>
	- puts an copy of the specified local file in the server's local directory
3. delete <filename>
	- deletes the specified file in the server's directory
4. ls
	- lists the files in the server's directory
5. exit
	- shuts down the server

Note: promts(not including <filename>) must be entered in lower case
Note: if the server does not return a response, there was an error with sending the message. Restart the client and try again


How it works:
Server 
	+ The server is set to listen on the specified port for udp packets
	+ Once a packet is detected, the packet is parsed for information on the action
	+ Server responds to the client according to the action
	+ Shuts down on the 'exit' command

Client 
	+ User enters an action from the promt above
	+ Client prepares the message based on the action requested
	+ Send off the pacekt
	+ Output confirmation packet