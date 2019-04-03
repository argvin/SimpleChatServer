/**
\file CSInfo.h
\author Vincent Hornak
\details
    <br>Course:        CSC552</br>
    <br>Assignment:    p3</br>
    <br>Date:          12/10/18</br>
\brief contains structs, constants, and utility functions
       shared among client.cpp and server.cpp
*/
#ifndef CSINFO_H
#define CSINFO_H

#include  <time.h>
#include  <map>
#include  <utility>

#define SERV_ADDR "ADD SERV IP HERE"
#define SERV_WKPORT 15080
#define MAX_CLIENTS 10

///\brief info stored on the local clients shmem
struct LOCAL_INFO{
    char name[16];
    time_t startTime;
    time_t lastMsgTime; //SEMOP will tell you last time in semInfo
    int numMsg;
    pid_t pid;
    //time of most recent lookup
};

///\brief the local clients shmem layout
struct LOCAL_DIR{
    LOCAL_INFO localInfo[MAX_CLIENTS];
    int numClients;
    int totalMsgs;
};

///\brief info stored on the server's shmem
struct CLIENT_INFO{
    char name[16];
    struct sockaddr_in serverAddr;
    time_t startTime;
    //time of most recent lookup
    time_t lastLookupTime;
};

///\brief layout of the server shmem
struct CLIENT_DIR{
    CLIENT_INFO clientInfo[MAX_CLIENTS];
    int numClients;
};
///\brief struct used for sems
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};


/********* SEM AND SHM ***********/
/**
 * \fn createSem
 * \param key - key for the sem set 
 * \brief creates the semaphore set
 */
int createSem(int key){
    //Allocate shmem needed
    int semid=semget(key,1,IPC_CREAT|IPC_EXCL|0777);
    if(semid == -1){
        if(errno == EEXIST)
            return -1;
        perror("\nError: shm not created: ");
        exit(-1);
    }
    return semid;
}

/**
 * \fn createShm
 * \param key - key for the sem set 
 * \brief creates the semaphore set
 */
template <class T>
int createShm(int key){
    //Allocate shmem needed
    int shmid;
    shmid=shmget(key,sizeof(T)+1,IPC_CREAT|IPC_EXCL|0777);
    if(shmid == -1){
        if(errno == EEXIST)
            return -1;
        perror("\nError: shm not created: " );
        exit(-1);
    }
    return shmid;
}

/**
 * \fn initSem
 * \param semid - semaphore id
 * \param semun - struct with sem inf
 * \param semData - layout of the sem
 * \brief initializes a semset -- TODO INCOMPLETE
 *  add param for numsems
 */
void initSem(int semid, semun &semInfo, semun &semData){
    semctl(semid,0,IPC_STAT,&semInfo);
    //set all to 1 at once
    memset(&semData, 1, sizeof(semData));
    semctl(semid,NULL,SETALL,semData);
}


/**
 * \fn initSem
 * \param id - shmem id
 * \param dir - CLIENT_DIR shmem layout
 * \brief attaches shared memory to struct
 */
template <class T> 
void attachShm(int id, T *&dir){
    dir = (T*)shmat(id,0,0);
}

/********* SYSTEM ***********/
/**
 * \fn cleanExit
 * \param shmid - the shmem id
 * \param semid - the semaphore id
 * \brief deallocates resources and exits process
 */
template <class T>
void cleanExit(T *shmptr, int shmid, int semid){
    // Detach memory
    shmdt(shmptr);
    // Remove shmem
    shmctl(shmid,IPC_RMID,0);
    //printf("And, remove the set......\n");
    semctl(semid,0,IPC_RMID,0);
    exit(0);
}

//15275 - 15299 , client ports 
/**
 * \fn initPorts
 * \param iports - inactive ports
 * \param start - lower bound
 * \param end - upper bound inclusive 
 * \brief initializes the inactive port vector which
 * maintains inactive assigned ports
 */
void initPorts(vector<int> &iports,int start, int end){
    for(int i = start; i <= end; i++){
        iports.push_back(i);
    }
}

/**
 * \fn getPort
 * \param dir - shmem which has numClients
 * \param iports - list of inactive ports
 * \param aports - list of active ports
 * \precondition will only be called if numClients < MAXCLIENTS
 * \brief - returns a port for the incoming client.  
 * aports will be parallel with the clientInfo in dir (shmem)
 */
int getPort(CLIENT_DIR* dir,vector<int> &iports,int aports[]){
    int thePort;
    if((thePort = iports.back()) != 0){
        iports.pop_back();
        aports[dir->numClients] = thePort;
        return thePort;
    }
    return NULL;
}

/********** UTIL ***********/
/**
 * \fn search 
 * \param dir - shmem, contains clientInfo (clients active on system)
 * \param uname - username to search 
 * \param spot - OUTPUT - index in shmem if it is found -1 otherwise
 * \brief searches shared memory for uname, if it exits spot will 
 * be assigned the index
 */
bool search(CLIENT_DIR *dir,const char *uname, int &spot){
    for(int i = 0; i < dir->numClients; i++){
        if( strcmp(dir->clientInfo[i].name,uname) == 0){
            spot = i;
            return true;
        }
    }
    spot =-1;
    return false;
}

/**
 * \fn
 * \param s_addr - OUTPUT - sock info
 * \param port - port for socket
 * \param ip - cstr ip for socket
 * \brief creates the main TCP socket to communicate with the server
 */
int createMainSock(sockaddr_in &s_addr, int port, const char *ip){
    // create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM,0);
    // clear memory
    s_addr.sin_family = AF_INET;  /* address family: Internet */
    s_addr.sin_addr.s_addr = inet_addr(ip);
    s_addr.sin_port = htons(SERV_WKPORT);//port;
    return sockfd;
}

/**
 * \fn initSockAddr
 * \param port - port for socket
 * \param ip - cstr ip for socket
 * \param saddr - OUTPUT - sock info
 * \brief creates the main TCP socket to communicate with the server
 */
void initSockAddr(int port, const char *ip, sockaddr_in &saddr){
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(ip);
    saddr.sin_port = port;
}

/**
 * \fn createUDPSock
 * \param cliport - port to bind to
 * \param ip - cstr ip for socket 
 * \param sockfd - OUTPUT - socket fd
 * \brief creates the UDP socket used for client msgs 
 */
sockaddr_in createUDPSock(int cliport, const char *ip, int &sockfd){
    sockaddr_in miniserv; 
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    initSockAddr(cliport, ip, miniserv);
    // bind socket 
    if (bind(sockfd, (struct sockaddr *) &miniserv, sizeof(miniserv)) == -1)
        perror("Error bind failed for miniserver: " + errno);

    return miniserv; 
}

/**
 * \fn upcmp
 * \param in - word for comparison
 * \param cmp - word to compare against 
 *             can also be str literal
 * \brief compares words case insensitively
 */
bool upcmp(char *in, const char *cmp){
    char U,T;
    int i=0,j=0;
    for(; in[i]; i++,j++){
        U = toupper(in[i]);
        T = toupper(cmp[j]);
        if(j != i || U != T) return false;
    }
    if(!in[i] && !cmp[j])
        return true;
    return false;
}

/**
 * \fn ipMapInsert
 * \param ip - ip addr to be inserted 
 * \param shmsem - shmemid and semid to be mapped to ip
 * \brief maps shmsem to ip if ip is not in map
 */
bool ipMapInsert(map<string,pair<int,int> > &ipMap, string ip,
                 pair<int,int> shmsem){
    pair<map<string,pair<int,int> >::iterator,bool> ret;
    ret = ipMap.insert(pair<string,pair<int,int> >(ip,
                    pair<int,int>(shmsem.first,shmsem.second)));
    if (ret.second==false) {
        return false;
    }
    return true;
}

/**
 * \fn need2RegisterIp
 * \param ipMap - map of ips to shmsem
 * \param ip - ip to find in map (string)
 *          not cstr because cpp doens't like
 *          cstr for map key 
 * \brief determines if IP/shmsem needs to be added to the map 
 */
bool need2RegisterIp(map<string,pair<int,int> > &ipMap, string ip){
    map<string, pair<int,int> >::iterator it;
    it = ipMap.find(ip);
    if(it != ipMap.end())
        return false;
    return true;
}


#endif //CSINFO_H
