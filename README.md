 
 Overview:
 This is a lookup chat client/server written in c++ for project 3 in
 Advanced Unix Programming(CSC552).  It demonstrates the use of shared memory,
 semaphores (TODO), sockets (TCP & UDP), and signal handling (TODO).
 Currently the program's base functionality is in place and it is 
 working with the max amount of clients as 10.  
 
 To use the program launch the server, and then launch a client with a 
 username as a commandline argument.  To send a message to a user 
 type his username followed by the message.
 
 In order to run this program the IP and well known port 
 must be specified in SERV_ADDR and SERV_WKPORT in CSInfo.h.
 
 Features:
    client communication
    server shared memory set up on server (without removal)
    local shared memory set up whenever client comes from new machine
        the list command will work with clients on different machines
        (at least on mcg and acad)
    local shared memory works on client
    commands work

 Features needed:
    semaphore synchronization
        shared memory 
        logging in
        msg send and rec on client 
    removal from shared memory
    exiting on the server
        removal of shmem if last client logging off of machine x
    signal handler 
 
 Assigned Ports:
    15080 - 15099
    15275 - 15299
 
 
