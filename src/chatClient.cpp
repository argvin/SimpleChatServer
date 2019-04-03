/**
\file chatClient.cpp
\author Vincent Hornak
\details
    <br>Course:        CSC552</br>
    <br>Assignment:    p3</br>
    <br>Date:          12/10/18</br>
    <br>Purpose:       practice using sockets for IPC, shared memory,
                       and semaphores.</br>
\brief Chat client which connects to the lookup server via TCP/IP, 
and chats with other clients by entering a username followed by a message.
\details Clients send messages to eachother via UDP/IP. There is shared
memory set up so clients could see who is on locally.
*/
#include  <iostream>
#include  <fstream>
#include  <vector>
#include  <cmath>
#include 	<stdio.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include 	<string.h>
#include  <sys/socket.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/sem.h>
#include  <sys/shm.h>
#include  <netinet/in.h>
#include  <netdb.h>
#include  <arpa/inet.h>
#include  <errno.h>
#include  <sys/termios.h>
#include  "Msg.h"
#include  "CSInfo.h"

/**
 * @fn displayMenu()
 * @brief displays the menu describing valid user input 
 */
void displayMenu();
/**
 *\fn initClient 
  \param sfd - the server fd
  \param inbound - message buffer
  \param ss - stringstream to parse data
  \param shmid - shared memory id
  \param semid - semaphore id
  \brief reads in the shared memory id and semaphore id
  */
bool initClient(int sfd, char *inbound, stringstream &ss, int &shmid, int &semid);
/**
 * \fn registerClient 
 * \param uname - client username 
 * \param myidx - OUTPUT - idx in shmem
 * \brief adds the client to shared memory
 */
bool registerClient(char *uname,int &myidx);
/**
 * \fn list
 * \brief prints the clients in shared memory
 */
void list();

//globals
LOCAL_DIR *dir;///dir is shmem layout, see CSInfo.h

int main(int argc, char *argv[]){
    if(argc != 2){
        cout << "\nError incorrect usage:  ./client <username>\n";
        cout << "Username cannot be a keyword: LIST ALL EXIT (case insensitive)\n";
        exit(-1);
    }
    char *uname = new char[16];
    strncpy(uname,argv[1],16);
    uname[16] = '\0';
    if(upcmp(uname,"LIST") || upcmp(uname,"ALL") || upcmp(uname,"EXIT")){
        cerr << "Error: username cannot be keyword\n" ;
        exit(-1);
    }


    dir  = new LOCAL_DIR;
    sockaddr_in serv_addr, cli_addr;
    semun semData;
    semun semInfo;

    int sfd = createMainSock(serv_addr,0,SERV_ADDR);//inet_ntoa(addr)

    if(connect(sfd, (sockaddr *) &serv_addr,(socklen_t)sizeof(serv_addr))== -1)
          { perror("client: cannot connect to server"); exit(2);  }


    stringstream ss;
    int shmid = 0;
    int semid = 0;
    int ssemid = 0;
    char *inbound = new char[400]; 
    char *outbound = new char[400];
    if((shmid = createShm<LOCAL_DIR>(getuid()+2)) != -1){
        //TODO implement sems
        //if((semid = createSem(getuid()+4)) != -1){
            //attachShm<LOCAL_DIR>(shmid,dir);
            //initSem(semInfo,semData);
            sprintf(inbound,"%d %d", shmid, semid);
            sendMsg(sfd,inbound);
            initClient(sfd,inbound, ss, shmid, semid);
            memset(dir,0,sizeof(LOCAL_DIR));
        //}
    }
    else{
        //hmm what if the server is not active?
        memset(inbound,NULL,40);
        if(!initClient(sfd,inbound, ss, shmid, semid)){
           cerr << "Error: no more room left on server\n";
           cleanExit(dir,shmid,semid);
        }
    }

    sendMsg(sfd,uname);

    int port;
    char *ip = new char[16];
    int clisock;
    int myidx;
    memset(inbound, NULL, 80);
    recvMsg(sfd,inbound,32); //get the port & ip
    unpackMsg<int,char*>(inbound,ss,port,ip);
    if(port == -1){
        cerr << "Error: username taken\n";
        cleanExit(dir,shmid,semid);
    }
    cli_addr = createUDPSock(port,ip, clisock);
    registerClient(uname,myidx);

    sockaddr_in destAddr;
    sockaddr_in returnAddr;

    int sockaddrlen = sizeof(destAddr);
    int msize; 
    //vars for select()
    fd_set fdReads; ///set of fds for reading 
    struct termios term, termsave ;
    bool done = false;
    FILE *fp = fopen(ctermid(NULL), "r+") ;
    setbuf(fp, NULL) ;      // No buffering of input
    int UserRead = fileno(fp) ;   // UserRead will be the terminal's fd
    //buffers
    char *servMsg = new char[16];
    char *sfdbuff = new char[100];
    char *cliMsg = new char [100];
    
    //select and terminal attributes adapted from Dr. Spiegel's example termdemo.c
    while(!done)
    {
        memset(uname,0,16);
        memset(inbound,0, 100);
        memset(outbound,0, 100);
        memset(sfdbuff,0,100);
        memset(servMsg,0,16);
        resetSS(ss);

        FD_ZERO(&fdReads) ;
        FD_SET(UserRead, &fdReads) ;
        FD_SET(sfd, &fdReads) ;
        FD_SET(clisock, &fdReads) ;

        //Get parameters of terminal; place into itermios struct termsave
        tcgetattr(UserRead, &termsave) ;
        // want to save initial condition, so will manipulate term
        term = termsave ;
        // echo characters. Allow any size input. See termios man page
        term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON) ;
        // Record changes to UserRead made by changing the c_lflag
        //tcsetattr(UserRead, TCSAFLUSH, &term) ;
        tcsetattr(UserRead, TCSAFLUSH ,&termsave);
        // Set up the select to listen; only using read fds (others are null)
        select(32, &fdReads,(fd_set *)0, (fd_set *)0, NULL ) ;
        // Restore UserRead to default after output
        tcsetattr(UserRead,  TCSADRAIN, &termsave);
        //stdin 
        if(FD_ISSET(UserRead, &fdReads))
        { 
            //get the input --- 1
            msize = read(UserRead,outbound,100);//leave room for uname
            ss << outbound;
            ss >> servMsg;
            strcpy(cliMsg,outbound+strlen(servMsg));//extract the message
            if(upcmp(servMsg,"LIST")){
               list();
               continue;
            }
            else if(upcmp(servMsg,"ALL")){
                sendMsg(sfd,servMsg);
                memset(servMsg,0,16);
                cout << "All clients on the system.." << endl;
                while(recvMsg(sfd,servMsg,16)){
                    if(!strcmp(servMsg,"EXIT"))
                        break;
                   cout << '\t' << servMsg << endl; 
                   memset(servMsg,0,16);
                }
                continue;
            }
            else if(upcmp(servMsg,"EXIT")){
                sendMsg(sfd,servMsg);
                //swap thing being removed with the last one
                //if you're the last client you also have to delete shm
                break; //cleanExit
            }
            //get where it's going from the server
            sendMsg(sfd,servMsg);
            memset(servMsg,0,16);
            recv(sfd,(void*)&destAddr,sizeof(sockaddr_in),0); //sockaddr incoming ..
            //send something to our new friend
            if(destAddr.sin_port != 0){
                sendMsg(sfd,dir->localInfo[myidx].name);
                recvMsg(sfd,servMsg,16);
                //send my username
                sendto(clisock,servMsg,16,0,(const sockaddr*)&destAddr, (socklen_t)sockaddrlen);
                //send the message
                sendto(clisock,cliMsg,100,0,(const sockaddr*)&destAddr, (socklen_t)sockaddrlen);
                dir->totalMsgs++;
                //maintain spot in local dir and increment numMsg, lastMsgTime
            }
            else{
                cout << "Error: user not found" << endl;
                memset(outbound,0,strlen(outbound));
            }
        }
        if(FD_ISSET(sfd, &fdReads)){
            //recv msg from server
            cout << recvMsg(sfd,sfdbuff,100);
            cout << "sfd message: " << sfdbuff << endl;
        }
        if(FD_ISSET(clisock, &fdReads))
        {
            //recv msg from another client
            recvfrom(clisock,uname,16,0,(sockaddr*)&returnAddr,(socklen_t*)&sockaddrlen);
            cout << "Msg from " << uname << ": ";
            recvfrom(clisock,inbound,100,0,(sockaddr*)&returnAddr,(socklen_t*)&sockaddrlen);
            cout << inbound << endl;
        }
    }

    delete [] ip,inbound,outbound,cliMsg,servMsg, uname;
    cleanExit(dir,shmid,semid);
} 

bool initClient(int sfd, char *inbound, stringstream &ss, int &shmid, int &semid){
    //get sem and shm back
    recvMsg(sfd,inbound,40);
    unpackMsg<int,int>(inbound,ss,shmid,semid);
    cout << "shmsem: " << shmid << " " << semid << endl;
    if(semid == -1)
        return false;
    attachShm(shmid,dir);
    return true;
}

bool registerClient(char *uname,int &myidx){
    time_t thePresent;
    time(&thePresent);
    LOCAL_INFO *inf = dir->localInfo;
    strcpy(inf[dir->numClients].name,uname);
    inf[dir->numClients].startTime = thePresent ;
    inf[dir->numClients].lastMsgTime = thePresent;
    inf[dir->numClients].numMsg = 0;
    inf[dir->numClients].pid = getpid();
    myidx = dir->numClients;
    dir->numClients++;
    cout << "numClients: " << dir->numClients << endl;
    list();
    return true;
}

void list(){ 
    cout << endl;
    LOCAL_INFO *inf = dir->localInfo;
    char *t = new char[16];
    for(int i = 0; i < dir->numClients; i++){
        cout << "client: " << i << endl;
        cout << "  name: " <<  inf[i].name << endl;
        memset(t,0,16);
        strftime(t,16,"%r", localtime(&inf[i].startTime));
        cout << "  startTime: " << t << endl;
        memset(t,0,16);
        strftime(t,16,"%r", localtime(&inf[i].lastMsgTime));
        cout << "  lastMsgTime: " << t << endl;
        cout << "  numMsg: " << inf[i].numMsg<< endl;
        cout << "  pid: " << inf[i].pid<< endl;
    }
    cout << endl;
    delete [] t;
}


