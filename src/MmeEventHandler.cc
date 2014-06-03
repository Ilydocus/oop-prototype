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
#include <cerrno>

#define MAX_EVENTS 10 

MmeEventHandler::MmeEventHandler(){
  int status;
  struct addrinfo hostInfo;      
  struct addrinfo *hostInfoList; 

  memset(&hostInfo, 0, sizeof hostInfo);

  hostInfo.ai_family = AF_UNSPEC;     
  hostInfo.ai_socktype = SOCK_STREAM; 
  hostInfo.ai_flags = AI_PASSIVE;    

  status = getaddrinfo(NULL, "43001", &hostInfo, &hostInfoList); 
  if (status != 0)  std::cerr << "getaddrinfo error" << gai_strerror(status) << std::endl;

  int socketfd ; 
  socketfd = socket(hostInfoList->ai_family, hostInfoList->ai_socktype,hostInfoList->ai_protocol);
  if (socketfd == -1)  std::cerr << "socket error " << std::endl;

  int yes = 1;
  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socketfd, hostInfoList->ai_addr, hostInfoList->ai_addrlen);
  if (status == -1)  std::cerr << "bind error" << std::endl;

  status =  listen(socketfd, 5);//5 is the number of "on hold"
  if (status == -1)  std::cerr << "listen error" << std::endl;
  freeaddrinfo(hostInfoList);

  mListenSocket = socketfd;
  mLog = new Log("MmeLog.txt");
  mNbMessages = 0;
}

MmeEventHandler::~MmeEventHandler(){
  for (UeMap::iterator it = mUeContexts.begin(); it != mUeContexts.end(); ++it){
    UeContextMme *ueContext = it->second;
    delete ueContext;
  }
  delete mLog;
  google::protobuf::ShutdownProtobufLibrary();
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

  ev.events = EPOLLIN;
  ev.data.fd = STDIN_FILENO;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
    perror("epoll_ctl: std::stdin");
    exit(EXIT_FAILURE);
  }
  uint32_t length;
  length = 0;
  for (;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_pwait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == STDIN_FILENO) {
	char buf[15];
	ssize_t sz = read(STDIN_FILENO,buf,15);
	buf[sz-1]= '\0';
	std::cout << "Exited on reading " << buf << std::endl;
	return;
      }
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
	for(;;){
	  ssize_t bytesReceived;
	  if (length == 0){
	    uint32_t  nlength;
	    bytesReceived = recv(events[n].data.fd, &nlength, 4, 0);
	    if (bytesReceived == 0) {std::cout << "host shut down." << std::endl ;break;}
	    if (bytesReceived == -1){
	    if (errno == EAGAIN) {
	      //End of data available
	      break;
	    }
	    else {
	      std::cerr << "recieve error!" <<std::endl;
	      break;
	    }
	  }
	    length = ntohl(nlength);
	  }
	  
	  char incomingDataBuffer[length];
	  bytesReceived = recv(events[n].data.fd, incomingDataBuffer,length, 0);
	  if (bytesReceived == 0) {std::cout << "host shut down." << std::endl;break;}
	  if (bytesReceived == -1){
	    if (errno == EAGAIN) {
	      //End of data available
	      break;
	    }
	    else {
	      std::cerr << "recieve error!" <<std::endl;
	      break;
	    }
	  }
	  if (bytesReceived != -1 && bytesReceived != 0){
	    incomingDataBuffer[bytesReceived] = '\0';
	    GOOGLE_PROTOBUF_VERIFY_VERSION;
	    
	    S1Message s1Message;
	    std::string strMessage(incomingDataBuffer, bytesReceived);
	    s1Message.ParseFromString(strMessage);
	    
	    UeMap::iterator tempIt = mUeContexts.find(events[n].data.fd);		    
	    UeContextMme *ueContext = tempIt->second;
	    
	    handleEnbMessage(s1Message, ueContext);
	    
	    length = 0;
	  }
	}
      }
    }
  }
}

void MmeEventHandler::handleNewUe(int connSock){
  UeContextMme *ueContext = new UeContextMme(connSock,mLog);
  mUeContexts.insert(std::pair<int,UeContextMme*>(connSock,ueContext));
} 

void MmeEventHandler::handleEnbMessage(S1Message s1Message, UeContextMme *ueContext){
  mNbMessages++;
  std::ostringstream messageLog;
  messageLog << "Total number of messages received in the MME: " << mNbMessages << std::endl;
  mLog->writeToLog(messageLog.str());
  switch (s1Message.messagetype()){
    case S1Message_MessageType_TypeS1ApIUeM : 
      {S1ApInitialUeMessage initialUeMessage;
      initialUeMessage = s1Message.messages1apiuem();
      ueContext->handleS1ApInitialUeMessage(initialUeMessage);}
      break;
    case  S1Message_MessageType_TypeS1ApICSResponse : 
      {S1ApInitialContextSetupResponse initialCSResponse;
      initialCSResponse = s1Message.messages1apicsresponse();
      ueContext->handleS1ApInitialContextSetupResponse(initialCSResponse);}
      break;
    default: 
      std::cerr << "Unexpected message type in MME" << std::endl;
  }
}
