import socket
import sys

def server_program():
    # get the hostname
    host = socket.gethostname()
    print("Host name: " + str(host))

    # host = 'localhost'

    if(len(sys.argv) != 2):
        print("Usage: python server.py <port_number>")
        sys.exit()

    port = int(sys.argv[1])

    # create the socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    #bind
    server_socket.bind((host, port))

    #listen
    server_socket.listen(5)

    #accept
    while True:
        conn, address = server_socket.accept()

        print("Connection from: " + str(address))

        data = ""
        while str(data).strip() != '.':
            data = conn.recv(1024).decode()
            if not data:
                break

            print("Data from user: " + str(data))

        conn.close

if __name__ == '__main__':
    server_program()
