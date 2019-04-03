/**
\file chatServer.cpp
\author Vincent Hornak
\details
    <br>Course:        CSC552</br>
    <br>Assignment:    p3</br>
    <br>Date:          12/10/18</br>
    <br>Purpose:       practice using sockets for IPC, shared memory,
                       and semaphores.</br>
\brief Chat lookup server which forks child servers
to deal with individual clients.
\details Clients connect to the server with TCP/IP, and uses that 
socket connection to receive information about other users stored
in shared memory.
*/
#include  <iostream>
#include  <fstream>
#include  <vector>
#include  <cmath>
#include 	<stdio.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include  <sys/socket.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/sem.h>
#include  <sys/shm.h>
#include  <netinet/in.h>
#include  <netdb.h>
#include  <arpa/inet.h>
#include  <errno.h>
#include  "Msg.h"
#include  "CSInfo.h"


/**
 * \fn addClient
 * \param numClients - num clients in client dir
 * \param uname - username to add 
 * \param cliaddr - users sockaddr_in to add with username
 * \brief adds a clients to the shared memory segment
 */
bool addClient(int numClients,const char *uname, sockaddr_in &cliaddr);
/**
 * @fn registerClient
 * @param cfd - the cliend fd
 * @param ss - the string stream to parse input
 * @param cshm_id - client shared memory id
 * @param csem_id - client sem id
 * @param ssem_id - server sem id
 * @brief registers the client by retrieving and sending info 
 * and calling addClient() to add to shmem
 * @details 
 */
bool registerClient(int cfd, stringstream &ss, int cshm_id, int csem_id,
                    int ssem_id, int cliport, const char *ip);
/**
 * \fn printTheShm
 * \param numClients - numclients in the system
 * \brief prints the clients in shmem
 */
void printTheShm(int numClients);
/**
 * \fn childServer 
 * \param cfd - the client file descriptor
 * \param ss - string stream used to parse data
 * \clientNum - clients idx in shmem
 * \brief launches child server to handle client messages and lookups
 */
void childServer(int cfd, stringstream &ss, int clientNum);
/**
 * \fn removeClient
 * \brief TODO not implemented
 */
void removeClient(const char* uname);

//globals 
CLIENT_DIR *dir; //immutable
vector<int> iports;
int aports[MAX_CLIENTS];
char ips[10][16];
//bool free[MAX_CLIENTS];

int main(int argc, char *argv[]){

    dir = new CLIENT_DIR;
    initPorts(iports,15275,15299);

    //initIps(ips);
    map<string,pair<int,int> > ipMap;

    sockaddr_in serv_addr, cli_addr;
    semun semData;
    semun semInfo;
    // create the socket
    int sockfd = createMainSock(serv_addr,SERV_WKPORT,SERV_ADDR);
    
    // bind socket 
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        perror("Error bind failed: " + errno);

    //--listen can have queue limit set to 10
    if (listen(sockfd,5) == -1)
        perror("Error listen failed: " + errno);

    int shmid,semid=111;
    if((shmid = createShm<CLIENT_DIR>(getuid())) != -1){
        //if((semid = createSem(getuid()+4)) != -1){
        cerr << "\nCreating shmid.." << shmid << endl;
            attachShm<CLIENT_DIR>(shmid,dir);
            //initSem(semInfo,semData);
            //send shmid to server .. INIT_MSG
        //}
    }
    else{
        perror("\nError couldn't create socket: " + errno);
    }

    stringstream ss;
    memset(&cli_addr,0,sizeof(cli_addr));
    
    int cli_addr_len = sizeof(cli_addr);
    char *inbound = new char[80];
    char *outbound = new char[80];
    int cliport;
    int cfd;
    int msgsize;
    time_t thePresent;
    pair<int,int> shmsem;
    int child;

    while(1){
        cfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&cli_addr_len);
        string ip(inet_ntoa(cli_addr.sin_addr));

        //check if shmsem already associated with ip
        if(!need2RegisterIp(ipMap,ip)){
            shmsem = ipMap[ip];
            
        } else{//first client on x machina
            //client will send shmsem
            getShmSem(cfd,ss,inbound,shmsem.first,shmsem.second);
            ipMapInsert(ipMap,ip,shmsem);
        }
        if(!registerClient(cfd,ss,shmsem.first,shmsem.second,semid,
                          getPort(dir,iports,aports), ip.c_str())){
            //send error message to client 
            continue;
        }

        if(cfd == 1)
            continue;

        child = fork();
        if(child == 0){
            childServer(cfd, ss, dir->numClients-1);
        }
    }
    wait();
    delete [] inbound, outbound;
    cleanExit(dir, shmid, semid);
} 

void removeClient(const char* uname){
    int rmIdx;
    search(dir,uname,rmIdx);
    //iports.push_back(aports.pop_back());
    //free the port
    //swap to last idx
    //remove 
    memset(&dir->clientInfo[dir->numClients-1],0,sizeof(CLIENT_INFO));
}
    

void childServer(int cfd, stringstream &ss, int clientNum){
    int *numClients = &(dir->numClients);
    sockaddr_in dest;
    memset(&dest,0,sizeof(sockaddr_in));
    sockaddr_in me = dir->clientInfo[clientNum].serverAddr;
    char *inbound = new char[16];
    memset(inbound,0,16);
    char *from = new char[16];
    memset(from,0,16);
    int addrsize = sizeof(sockaddr_in);
    int msgsize = 0;
    int idx;
    
    //do we need select?
    while(1){
        //listen for message
        msgsize = recvMsg(cfd,inbound,16); //to usr2 (if its a uname)
        cout << "recved msg: " << inbound <<  endl;
        if(upcmp(inbound,"ALL")){
            cout <<"numCli " <<  dir->numClients << endl;
            for(int i=0; i < dir->numClients; i++){
                send(cfd,dir->clientInfo[i].name,16,0);
            }
            cout << "sent to close: " << sendMsg(cfd,"EXIT") << endl;
        }
        else if(upcmp(inbound,"EXIT")){
            //remove and reorder shmem
            exit(1);
        }
        //will receive uname, ALL or exit...
        else if(search(dir,inbound,idx)){ 
            cout << "sending sockaddr to " << cfd <<endl;
            cout << "the port: " << dir->clientInfo[idx].serverAddr.sin_port << endl;
            cout << "the ip: " << inet_ntoa(dir->clientInfo[idx].serverAddr.sin_addr)<< endl;
            send(cfd, (void*)&dir->clientInfo[idx].serverAddr,addrsize,0);//to inbound 
            recvMsg(cfd,from,16);//from sender
            //send destination addr to client
            sendMsg(cfd, from); 
        }
        else{
            //send error sockaddr
            send(cfd,(void*)&dest,addrsize,0);
        }

        resetSS(ss);
        memset(&dest,0,addrsize);
        memset(inbound,0,16);
        memset(from, 0, 16);
    }
    delete [] inbound, from;
    delete numClients;
    exit(1);
}

bool registerClient(int cfd, stringstream &ss, int cshm_id, int csem_id, 
                    int ssem_id, int cliport, const char *ip){
    char *uname = new char[16];
    char *buf = new char[80];
    int idx;

    //TODO reset MAXCLIENTS to 10
    if(dir->numClients == MAX_CLIENTS){
        sprintf(buf, "-1 -1");
        sendMsg(cfd,buf);
        delete [] uname, buf;
        return false;
    }

    cout << "sending client shm and sem.." << endl;
    //send client shmid and client semid 
    sprintf(buf,"%d %d", cshm_id,csem_id);
    int msgsize = sendMsg(cfd,buf);
    memset(buf,0,msgsize);

    //recv uname
    msgsize = recvMsg(cfd,buf,16);
    unpackMsg<char*>(buf,ss,uname);
    //search for uname in dir
    sockaddr_in cliaddr;
    memset(&cliaddr,0,sizeof(sockaddr_in));
    //check if uname exists
    if(!search(dir,uname,idx)){
        initSockAddr(cliport,ip,cliaddr);
        memset(buf,0,msgsize);
        sprintf(buf,"%d %s", cliaddr.sin_port, ip);
        //send the port and ip client should bind to
        msgsize = sendMsg(cfd, buf);
        memset(buf,0,msgsize);
    } else{
        memset(buf,0,msgsize);
        sprintf(buf,"%d",-1);
        //send -1 if uname found
        msgsize = sendMsg(cfd, buf);
        memset(buf,0,msgsize);
        return false;
    }

    bool res = addClient(dir->numClients, uname, cliaddr);
    delete [] uname, buf;
    return res;
}

bool addClient(int numClients,const char *uname, sockaddr_in &cliaddr){
    time_t thePresent;
    time(&thePresent);
    CLIENT_INFO *clientToAdd = dir->clientInfo;
    strcpy(clientToAdd[numClients].name,uname);
    clientToAdd[numClients].startTime= thePresent; 
    clientToAdd[numClients].lastLookupTime = thePresent;
    clientToAdd[numClients].serverAddr = cliaddr;
    dir->numClients++;
    printTheShm(dir->numClients);
    return true;
}

void printTheShm(int numClients){
    cout << endl;
    CLIENT_INFO *clients = dir->clientInfo;
    char *t = new char[16];
    for(int i = 0; i < numClients; i++){
        cout << "client: " << i << endl;
        cout << "  name: " <<  clients[i].name << endl;
        memset(t,0,16);
        strftime(t,16,"%r", localtime(&clients[i].startTime));
        cout << "  startTime: " << t << endl;
        memset(t,0,16);
        strftime(t,16,"%r", localtime(&clients[i].lastLookupTime));
        cout << "  lastLookupTime: " << t << endl;
        cout << '\t' << "sa port: " << clients[i].serverAddr.sin_port << endl;
        cout << '\t' << "sa addr: " << inet_ntoa(clients[i].serverAddr.sin_addr) << endl;
    }
    delete [] t;
}


