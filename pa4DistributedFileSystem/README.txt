Simple DFS and DFC  by Willie Chew

What is provided and how to use
----------------
The following directory contains a directory for a DFC and a DFS program. Each of the directories
have a make file to compile the respective server or client. Config files are also in the directory. Modify 
the config files in the structure below for the server and clients to work. 

Config file structures
-----------------------------
DFC:
<address of server 1>:<port of server 1>
<address of server 2>:<port of server 2>
<address of server 3>:<port of server 3>
<address of server 4>:<port of server 4>
<username>
<password>

DFS:
<username1><password1>
<username2><password2>
<username3><password3>


How to compile
--------------
- Open up a terminal and navigate it to the directory of DFC 
- Type 'make' into the terminal to trigger the makefile
	+ This should compile the program 
- Do the same for DFS


Using the DFS
---------------
The program requires both the servers and the client to be connected to the same network

Make sure not to change the file structure of the dfs directory.
Host the servers with the following command: ./dfs DFS<server number> <port number>
Make sure the server addresses and ports match with the dfc config

Run the dfc with: ./dfc
The commands that the DFC accepts are:
- get <filename>
- put <filename>
- exit