#include "MmeEventHandler.hh"
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <map>
#include <sys/epoll.h>
#include <cstdio>
#include <unistd.h>
#include <utility>
#include <sstream>

#define MAX_EVENTS 10 

using namespace std;

MmeEventHandler::MmeEventHandler(){
  int status;
  struct addrinfo hostInfo;      
  struct addrinfo *hostInfoList; 

  memset(&hostInfo, 0, sizeof hostInfo);

  hostInfo.ai_family = AF_UNSPEC;     
  hostInfo.ai_socktype = SOCK_STREAM; 
  hostInfo.ai_flags = AI_PASSIVE;    

  status = getaddrinfo(NULL, "43001", &hostInfo, &hostInfoList); 
  if (status != 0)  cout << "getaddrinfo error" << gai_strerror(status) << endl;

  int socketfd ; 
  socketfd = socket(hostInfoList->ai_family, hostInfoList->ai_socktype,hostInfoList->ai_protocol);
  if (socketfd == -1)  cout << "socket error " << endl;

  int yes = 1;
  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socketfd, hostInfoList->ai_addr, hostInfoList->ai_addrlen);
  if (status == -1)  cout << "bind error" << endl;

  status =  listen(socketfd, 5);//5 is the number of "on hold"
  if (status == -1)  cout << "listen error" << endl;
  freeaddrinfo(hostInfoList);

  mListenSocket = socketfd;
  mLog = new Log("MmeLog.txt");
  mNbMessages = 0;
}

void MmeEventHandler::run () {    
  struct epoll_event ev, events[MAX_EVENTS];
  int connSock, nfds, epollfd;

  epollfd = epoll_create(10);
  if (epollfd == -1) {
    perror("epoll_create");
    exit(EXIT_FAILURE);
  }
  ev.events = EPOLLIN;
  ev.data.fd = mListenSocket;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, mListenSocket, &ev) == -1) {
    perror("epoll_ctl: mListenSocket");
    exit(EXIT_FAILURE);
  }
  
  for (;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_pwait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == mListenSocket) {
	struct sockaddr_storage theirAddr;
	socklen_t addrSize = sizeof(theirAddr);
	connSock = accept(mListenSocket, (struct sockaddr *)&theirAddr, &addrSize);
	if (connSock == -1) {
	  perror("accept");
	  exit(EXIT_FAILURE);
	}
	makeSocketNonBlocking(connSock);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = connSock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connSock,
		      &ev) == -1) {
	  perror("epoll_ctl: connSock");
	  exit(EXIT_FAILURE);
	}
	handleNewUe(connSock);
      }
      else {
	ssize_t bytesRecieved;
	char incomingDataBuffer[1000];
	bytesRecieved = recv(events[n].data.fd, incomingDataBuffer,1000, 0);
	
	if (bytesRecieved == 0) {cout << "host shut down." << endl ;}
	if (bytesRecieved == -1){cout << "recieve error!" << endl ;}
	if (bytesRecieved != -1 && bytesRecieved != 0){
	  incomingDataBuffer[bytesRecieved] = '\0';
	  GOOGLE_PROTOBUF_VERIFY_VERSION;
    
	  S1Message s1Message;
	  string strMessage(incomingDataBuffer, bytesRecieved);
	  s1Message.ParseFromString(strMessage);
	  
	  map<int,UeContextMme>::iterator tempIt = mUeContexts.find(events[n].data.fd);		    
	  UeContextMme ueContext = tempIt->second;
	  
	  handleUeMessage(s1Message, ueContext);		       
	}
      }
    }
  }
}

void MmeEventHandler::handleNewUe(int connSock){
  UeContextMme *ueContext = new UeContextMme(connSock,mLog);
  mUeContexts.insert(pair<int,UeContextMme>(connSock,*ueContext));
} 

void MmeEventHandler::handleUeMessage(S1Message s1Message, UeContextMme ueContext){
  mNbMessages++;
  ostringstream messageLog;
  messageLog << "Total number of messages received in the MME: " << mNbMessages << endl;
  mLog->writeToLog(messageLog.str());
  switch (s1Message.messagetype()){
    case 0 : //S1ApInitialUeMessage
      {S1ApInitialUeMessage initialUeMessage;
      initialUeMessage = s1Message.messages1apiuem();
      ueContext.handleS1ApInitialUeMessage(initialUeMessage);}
      break;
    case 2 : //S1ApInitialContextSetupResponse
      {S1ApInitialContextSetupResponse initialCSResponse;
      initialCSResponse = s1Message.messages1apicsresponse();
      ueContext.handleS1ApInitialContextSetupResponse(initialCSResponse);}
      break;
    default: 
      cerr << "Unexpected message type in MME" << endl;
  }
}
