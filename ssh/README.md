# Project Description
Basic lightweight remote secure shell (ssh) implementation. Capable of sending and receiving files between server and client. Contains a basic text-based shell implementation for executing commands on remote server from the client. Only handles basic text and file commands (i.e. touch, cd, ls, cat, echo, mkdir, rm, dir,etc.)
# Running
Navigate to the root directory that contains the makefile and run `make`. View the `users.txt` file located in the root directory. These are all the valid username, password combinations that are allowed access to the server. Open two windows; one for server and one for client. These may be running on different machines. If working on the same machine, use a hostname of "localhost". Navigate to the `/bin` subdirectory.

1. Server: ```./server 3000``` , where the argument is any valid port number.
2. Client: ```./client user1@hostname -p 3000``` , where `user1` is one of the valid usernames listed in `users.txt`. Choose a port number as argument to `-p` flag. `Hostname` is "localhost" or any valid IP address within your local network. Enter the correct password when prompted.
3. Type "quit" to exit the shell and close the connection.

# Valid Commands

1. Custom commands available

   - `scp path_to_server_file path_to_client_directory` - "secure copy" - copies the specified file from the server to the specified directory on the client's machine. File size limited to < 50MiB
   - `ssend path_to_client_file path_to_server_directory` - "secure send" - copies the specified file from the client to the specified directory on the remote machine. File size limited to < 50MiB
   - `cd directory` - "change directory" - change to a different directory on the server machine. This command is typically built into most shell programs; re-implemented here.

2. Other commands

   - The basic shell implementation accepts all other user input and attempts to execute it on the remote machine. If the command is ill-formed, the proper error message will be returned from the remote machine
   - Only text-based command-lines are supported.(i.e. touch,cd, ls, cat, echo, mkdir, rm, dir, etc.)
   - If the client attempts to execute a command that is typically supported on the remote machine, but is not supported by this (very) simple shell implementation, most things will probably break. Such cases would be any program that requires graphical data or interactivity with the user, such as most text editors (gedit, nano, vim, emacs, etc.) as well as web browsers (chrome,firefox, etc.), and any other program that is not purely text-based single input, single output.

   # Limitations

   All limitations are currently tracked in the `improve.txt` file.