To compile these, there isn't a Makefile, so just say:
gcc server.c -o server

and then:
gcc client.c -o client


To run the server (always run the server FIRST):
./server

Without a second argument on the command line, the server will automatically run on port 3490.  To specify a different port:
./server <port>

For example:
./server 9999

Will run the server and listen on port 9999.



To run the client:
./client hostname

Without a third argument on the command line, the client will, by default, use the port 3490.  If you are running the server and client on the same machine, you can use localhost for the hostname.  If the server is on a different machine, you can use the hostname of the server (which the server program prints out when first run, along with its IP address) or the IP address of the server.

For example:
./client localhost

Will run the client, use the default port of 3490 to connect to the server running on the same machine.  

To specify a port, run the client as in:
./client hostname <server_port>

Where server_port is the port number the server is listening on.

If you wish to run the client on the local machine and the server is listening on port 9999:
./client localhost 9999