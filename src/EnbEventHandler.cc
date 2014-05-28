#include "EnbEventHandler.hh"
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

EnbEventHandler::EnbEventHandler(){
  int status;
  struct addrinfo hostInfo;      
  struct addrinfo *hostInfoList; 

  memset(&hostInfo, 0, sizeof hostInfo);

  hostInfo.ai_family = AF_UNSPEC;     
  hostInfo.ai_socktype = SOCK_STREAM; 
  hostInfo.ai_flags = AI_PASSIVE;    

  status = getaddrinfo(NULL, "43000", &hostInfo, &hostInfoList);
  if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) << std::endl;

  int socketfd ; 
  socketfd = socket(hostInfoList->ai_family,hostInfoList->ai_socktype,hostInfoList->ai_protocol);
  if (socketfd == -1)  std::cout << "socket error " << std::endl;

  int yes = 1;
  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socketfd, hostInfoList->ai_addr, hostInfoList->ai_addrlen);
  if (status == -1)  std::cout << "bind error" << std::endl ;

  status =  listen(socketfd, 5);//5 is the number of "on hold"
  if (status == -1)  std::cout << "listen error" << std::endl;
  freeaddrinfo(hostInfoList);

  mListenSocket = socketfd;

  int statusMme;
  struct addrinfo hostInfoMme;
  struct addrinfo *hostInfoListMme;

  memset(&hostInfoMme, 0, sizeof hostInfoMme);
  hostInfoMme.ai_family = AF_UNSPEC;
  hostInfoMme.ai_socktype = SOCK_STREAM;

  statusMme = getaddrinfo("127.0.0.1","43001",&hostInfoMme, &hostInfoListMme);
  if (statusMme != 0) std::cout << "getaddrinfo error" << std::endl;
  int socketMme;
  socketMme = socket(hostInfoListMme->ai_family, hostInfoListMme->ai_socktype, hostInfoListMme->ai_protocol);
  if(socketMme == 1) std::cout << "socket error" << std::endl;

  statusMme = connect (socketMme, hostInfoListMme->ai_addr, hostInfoListMme->ai_addrlen);
  freeaddrinfo(hostInfoListMme);

  mMmeSocket = socketMme;
  mLog = new Log("EnbLog.txt");
  mNbMessages = 0;
}

void EnbEventHandler::run () {    
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
	if (bytesRecieved == 0) {std::cout << "host shut down." << std::endl;}
	if (bytesRecieved == -1){std::cout << "recieve error!" << std::endl;}
	if (bytesRecieved != -1 && bytesRecieved != 0){
	  incomingDataBuffer[bytesRecieved] = '\0';
	  GOOGLE_PROTOBUF_VERIFY_VERSION;
    
	  RrcMessage rrcMessage;
	  std::string strMessage(incomingDataBuffer, bytesRecieved);
	  rrcMessage.ParseFromString(strMessage);
	  std::map<int,UeContextEnb>::iterator tempIt = mUeContexts.find(events[n].data.fd);		    
	  UeContextEnb ueContext = tempIt->second;
	
	  handleUeMessage(rrcMessage, ueContext);		     
	  
	}
      }
    }
  }
}

void EnbEventHandler::handleNewUe(int connSock){
  UeContextEnb *ueContext = new UeContextEnb(connSock,mMmeSocket,mLog);
  mUeContexts.insert(std::pair<int,UeContextEnb>(connSock,*ueContext));
} 

void EnbEventHandler::handleUeMessage(RrcMessage rrcMessage, UeContextEnb ueContext){
  mNbMessages++;
  std::ostringstream messageLog;
  messageLog << "Total number of messages received in the eNodeB: " << mNbMessages << std::endl;
  mLog->writeToLog(messageLog.str());
  switch (rrcMessage.messagetype()){
    case 0 : //RaPreamble
      {RaPreamble rapreamble;
      rapreamble = rrcMessage.messagerap();
      ueContext.handleRaPreamble(rapreamble);}
      break;
    case 2 : //RrcConnectionRequest
      {RrcConnectionRequest rrcCRequest;
      rrcCRequest = rrcMessage.messagerrccrequest();
      ueContext.handleRrcConnectionRequest(rrcCRequest);}
      break;
    case 4 : //RrcConnectionSetupComplete
      {RrcConnectionSetupComplete rrcConnectionSetupComplete;
      rrcConnectionSetupComplete = rrcMessage.messagerrccsc();
      ueContext.handleRrcConnectionSetupComplete(rrcConnectionSetupComplete);}
      break;
    case 6 : //SecurityModeComplete
      {SecurityModeComplete securityModeComplete;
      securityModeComplete = rrcMessage.messagesecuritymcomplete();
      ueContext.handleSecurityModeComplete(securityModeComplete);}
      break;
    case 8 : //UeCapabilityInformation
      {UeCapabilityInformation ueCI;
      ueCI = rrcMessage.messageueci();
      ueContext.handleUeCapabilityInformation(ueCI);}
      break;
    case 10 : //RrcConnectionReconfigurationComplete
      {RrcConnectionReconfigurationComplete rrcCRC;
      rrcCRC = rrcMessage.messagerrccrc();
      ueContext.handleRrcConnectionReconfigurationComplete(rrcCRC);}
      break;
    default: 
      std::cerr << "Unexpected message type in eNodeB" << std::endl;
  }
}
