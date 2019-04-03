/**
\file Msg.h
\author Vincent Hornak
\details
    <br>Course:        CSC552</br>
    <br>Assignment:    p3</br>
    <br>Date:          12/10/18</br>
\brief contains message related constants and functions
 */
#ifndef MSG_H
#define MSG_H

#include <string.h>
#include <string>
#include <sstream>
using namespace std;

#define MAX_SIZE 4096


/**
 * \fn resetSS
 * \param ss - stringstream with stuff in it
 * \brief resets the stringstream
 */
void resetSS(stringstream &ss){
    ss.clear();
    ss.str("");
}

/**
 * \fn sendMsg
 * \param fd - file descriptor of msg receiver
 * \param msg - the msg 
 * \brief sends msg to fd, returns number of bytes sent
 */
int sendMsg(int fd,const char *msg){
    int n;
    //TODO figure out if sending the full buffer size is better
    if((n = send(fd,msg,strlen(msg),0)) == -1){
        perror("Error sending message" + errno);
        exit(-1);
    }
    return n;
}

/**
 * \fn recvMsg
 * \param fd - file descriptor of msg receiver
 * \param msg - the msg buffer
 * \brief recv's msg and returns number of bytes read
 */
int recvMsg(int fd, char *msg, int size){
    int n;
    if((n = recv(fd,msg,size,0)) == -1){
        perror("Error receiving message" + errno);
        exit(-1);
    }
    return n;
}


/**
 * \fn unpackMsg
 * \param msg - filled msg buffer
 * \param ss - stringstream used for parsing,
 * \param i1 - item to be packed with data from ss
 * \brief unpacks message into item
 */
template<class T>
int unpackMsg(char *msg, stringstream &ss, T &i1){
    ss << msg;
    if(ss >> i1){
        resetSS(ss);
        memset(msg,NULL,strlen(msg));
        return 1;
    }
    cerr << "Error couldn't unpack message " << endl;
    return -1;
    //TODO send signal on err .. if it sets errno 
}

/**
 * \fn unpackMsg
 * \param msg - filled msg buffer
 * \param ss - stringstream used for parsing,
 * \param i1 - item to be packed with data from ss
 * \param i2 - item to be packed with data from ss
 * \brief unpacks message into 2 items
 */
template<class T,class U>
int unpackMsg(char *msg, stringstream &ss, T &i1, U &i2){
    ss << msg;
    if(ss >> i1 >> i2){
        resetSS(ss);
        memset(msg,NULL,strlen(msg));
        return 2;
    }
    cerr << "Error couldn't unpack message " << endl;
    return -1;
    //TODO send signal on err .. if it sets errno 
}

/**
 * \fn unpackMsg
 * \param msg - filled msg buffer
 * \param ss - stringstream used for parsing,
 * \param i1 - item to be packed with data from ss
 * \param i2 - item to be packed with data from ss
 * \param i3 - item to be packed with data from ss
 * \brief unpacks message into 3 items
 */
template<class T,class U, class V>
int unpackMsg(char *msg, stringstream &ss, T &i1, U &i2, V &i3){
    ss << msg;
    if(ss >> i1 >> i2 >> i3){
        resetSS(ss);
        memset(msg,NULL,strlen(msg));
        return 2;
    }
    cerr << "Error couldn't unpack message " << endl;
    return -1;
    //TODO send signal on err .. if it sets errno 
}

/**
 * \fn getShmSem
 * \param cfd - client fd receiving the shmid and semid
 * \param ss - string stream for unpack()
 * \param inbound - the message buffer to recv data
 * \brief gets the shmid and semid
 */
void getShmSem(int cfd, stringstream &ss, char *inbound,
               int &shmid, int &semid){
        recvMsg(cfd, inbound,40); 
        cout << "getShmSem: " << inbound << endl;
        int len = unpackMsg<int,int>(inbound, ss,shmid,semid);
        memset(inbound,0,len);
}

#endif //MSG_H
