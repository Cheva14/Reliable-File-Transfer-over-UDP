# Reliable-File-Transfer-over-UDP
The program opens a connection between a client and a server. The client sends a filename to the server, the server reads the file and sends it back to the client using sliding window protocol to ensure reliability in any packet loss, reordering and delay, and then the client writes the data into a new file.

Documentation
Reliable File Transfer over UDP
 
Program Overview:
The program contains two main files (client.cpp and server.cpp), helper files and a makefile for compiler purposes. The client sends to the server a request to initiate a transfer of a specific file. The program works with all file types and handles packet loss, packet delay, and packet reordering. The program was written in C++ using UDP datagram sockets.
When the server starts, it waits for a client connection. The client also starts and establishes connection with the server. Upon setting up the connection, the server reads the requested file, if the file is found within the directory, the server sends the file to the client and the client then writes the file locally. It uses UDP (datagram) sockets and sends the data with a sliding window protocol in chunks of 5 frames. The client and server print to the screen each time they send or receive a packet.

Run the program

To be able to run the program, run $ make in a terminal window to compile the files.
Start the server by running $ ./server <port> and the server will start waiting for the client to send a file request.
In a new terminal window start the client by running $ ./client <filename> <ip> <port> and the program will start the file transfer.


Program walkthrough:
The client first creates the connection with the server using UDP sockets and the ip and port provided by the user. The server also creates a socket to connect with the client using the port number to make the connection with the client. After having the UDP Socket part set up and having a connection between the client and server, the client sends the filename to the server. The client will also use this filename to create an output file with the right extension. The server will receive the filename and will check in the directory for a file with this name. If the file doesn’t exist in the directory, it will print this error and exit the program. If the file exists in the directory, it will continue with the program.

Server side:
The server will read in chunks of 1024 bits from the file and store them in packets. Then the server will use the sliding window protocol to send the packets to the client. The server creates two threads to handle the data packets and the acknowledge packets at the same time. After sending a packet, the server will also be able to receive acknowledge packets to be able to move on with the protocol.
 
Client side:
The client will create a new file to write all the packets it gets from the server. The server will check if the packet is in the right order and write it in the final file and then send a acknowledge packet to the client. The client will keep waiting for incoming packets. After 3 seconds of not receiving anything, it will stop the program.

Sliding Window Protocol

The program uses a sliding window protocol to make the transfer reliable with a 5 packet length to transfer the files. This protocol allows you to send more than one packet at a time, and receive them out of order. It also controls the errors by sending a packet with acknowledgement to know if the transfer was successful. If we don’t receive the correct acknowledgement packet, the program will re-transmit the packets.
The packets will have a sequence number to be able to track what packets are being sent and received and handle any reordering and delay in the packets being sent.
The server creates a window that holds 5 packets which will be sent to the client. The server then will wait for acknowledgement of these packets, once the server receives the acknowledgement of the first packet in the window, it will shift and send the next packet. The window can only shift when the packet in the first place is acknowledged. If the server doesn’t receive an acknowledgement or receives a negative acknowledgment, it will re-transmit the packet.
The client will be receiving packets and for every packet that receives, it will send its acknowledgment for the corresponding packet back, and will also shift its window when the packet in the first position of the window is received.
