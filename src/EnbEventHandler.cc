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
#include <cerrno>

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
  if (status != 0)  std::cerr << "getaddrinfo error" << gai_strerror(status) << std::endl;

  int socketfd ; 
  socketfd = socket(hostInfoList->ai_family,hostInfoList->ai_socktype,hostInfoList->ai_protocol);
  if (socketfd == -1)  std::cerr << "socket error " << std::endl;

  int yes = 1;
  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socketfd, hostInfoList->ai_addr, hostInfoList->ai_addrlen);
  if (status == -1)  std::cerr << "bind error" << std::endl ;

  status =  listen(socketfd, 5);//5 is the number of "on hold"
  if (status == -1)  std::cerr << "listen error" << std::endl;
  freeaddrinfo(hostInfoList);

  mListenSocket = socketfd;

  int statusMme;
  struct addrinfo hostInfoMme;
  struct addrinfo *hostInfoListMme;

  memset(&hostInfoMme, 0, sizeof hostInfoMme);
  hostInfoMme.ai_family = AF_UNSPEC;
  hostInfoMme.ai_socktype = SOCK_STREAM;

  statusMme = getaddrinfo("127.0.0.1","43001",&hostInfoMme, &hostInfoListMme);
  if (statusMme != 0) std::cerr << "getaddrinfo error" << std::endl;
  int socketMme;
  socketMme = socket(hostInfoListMme->ai_family, hostInfoListMme->ai_socktype, hostInfoListMme->ai_protocol);
  if(socketMme == 1) std::cerr << "socket error" << std::endl;

  statusMme = connect (socketMme, hostInfoListMme->ai_addr, hostInfoListMme->ai_addrlen);
  makeSocketNonBlocking(socketMme);
  freeaddrinfo(hostInfoListMme);

  mMmeSocket = socketMme;
  mLog = new Log("EnbLog.txt");
  mNbMessages = 0;
}

EnbEventHandler::~EnbEventHandler(){
  for (UeMapUe::iterator it = mUeContextsUe.begin(); it != mUeContextsUe.end(); ++it){
    UeContextEnb *ueContext = it->second;
    delete ueContext;
  }
  delete mLog;
  google::protobuf::ShutdownProtobufLibrary();
}

void EnbEventHandler::run () {    
  struct epoll_event ev, events[MAX_EVENTS];
  int connSock, nfds, epollfd;

  epollfd = epoll_create(MAX_EVENTS);
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

  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = mMmeSocket;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, mMmeSocket, &ev) == -1) {
    perror("epoll_ctl: mMmeSocket");
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
      
      if (events[n].data.fd == mMmeSocket) {
	for(;;){
	  ssize_t bytesReceived;
	  if (length == 0){
	    uint32_t  nlength;
	    bytesReceived = recv(events[n].data.fd, &nlength, 4, 0);
	    if (bytesReceived == 0) {std::cout << "host shut down." << std::endl ;break;}
	    if (bytesReceived == -1){
	    if (errno == EAGAIN) {
	      //End of available data
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
	  if (bytesReceived == 0) {std::cout << "host shut down." << std::endl ;break;}
	  if (bytesReceived == -1){
	    if (errno == EAGAIN) {
	      //End of available data  
	      break;
	    }
	    else {
	      std::cerr << "recieve error!" <<std::endl ;
	      break;
	    }
	  }
	  
	  if (bytesReceived != -1 && bytesReceived != 0){
	    incomingDataBuffer[bytesReceived] = '\0';
	    GOOGLE_PROTOBUF_VERIFY_VERSION;
   
	    S1Message s1Message;
	    std::string strMessage(incomingDataBuffer, bytesReceived);
	    s1Message.ParseFromString(strMessage);
	        
	    handleMmeMessage(s1Message);
	    
	    length = 0;
	  }
	}
      }
      else{
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
	  ssize_t bytesReceived;
	  char incomingDataBuffer[1000];
	  memset(incomingDataBuffer,0,1000);
	  bytesReceived = recv(events[n].data.fd, incomingDataBuffer,1000, 0);
	  if (bytesReceived == 0) {std::cout << "host shut down." << std::endl;}
	  if (bytesReceived == -1){std::cerr << "recieve error!" << std::endl;}
	  if (bytesReceived != -1 && bytesReceived != 0){
	    incomingDataBuffer[bytesReceived] = '\0';
	    GOOGLE_PROTOBUF_VERIFY_VERSION;
	    std::string strMessage(incomingDataBuffer, bytesReceived);
	    RrcMessage rrcMessage;
	    rrcMessage.ParseFromString(strMessage);
	    
	    UeMapUe::iterator tempIt = mUeContextsUe.find(events[n].data.fd);
	    UeContextEnb *ueContext = tempIt->second;
	    handleUeMessage(rrcMessage, ueContext);
	  } 
	}
      }
    }
  }
}

void EnbEventHandler::handleNewUe(int connSock){
  UeContextEnb *ueContext = new UeContextEnb(connSock,mMmeSocket,mLog);
  mUeContextsUe.insert(std::pair<int,UeContextEnb*>(connSock,ueContext));
} 

void EnbEventHandler::handleUeMessage(RrcMessage rrcMessage, UeContextEnb *ueContext){
  mNbMessages++;
  std::ostringstream messageLog;
  messageLog << "Total number of messages received in the eNodeB: " << mNbMessages << std::endl;
  mLog->writeToLog(messageLog.str());
  switch (rrcMessage.messagetype()){
    case RrcMessage_MessageType_TypeRaP : 
      {RaPreamble raP;
      raP = rrcMessage.messagerap();
      ueContext->handleRaPreamble(raP);}
      break;
    case RrcMessage_MessageType_TypeRrcCRequest : 
      {RrcConnectionRequest rrcCRequest;
      rrcCRequest = rrcMessage.messagerrccrequest();
      ueContext->handleRrcConnectionRequest(rrcCRequest);}
      break;
    case RrcMessage_MessageType_TypeRrcCSC : 
      {RrcConnectionSetupComplete rrcCSC;
      rrcCSC = rrcMessage.messagerrccsc();
      mUeContextsMme.insert(std::pair<int,UeContextEnb*>(rrcCSC.uecrnti(),ueContext));
      ueContext->handleRrcConnectionSetupComplete(rrcCSC);}
      break;
    case RrcMessage_MessageType_TypeSecurityMComplete : 
      {SecurityModeComplete securityModeComplete;
      securityModeComplete = rrcMessage.messagesecuritymcomplete();
      ueContext->handleSecurityModeComplete(securityModeComplete);}
      break;
    case RrcMessage_MessageType_TypeUeCI :
      {UeCapabilityInformation ueCI;
      ueCI = rrcMessage.messageueci();
      ueContext->handleUeCapabilityInformation(ueCI);}
      break;
    case RrcMessage_MessageType_TypeRrcCRC : 
      {RrcConnectionReconfigurationComplete rrcCRC;
      rrcCRC = rrcMessage.messagerrccrc();
      ueContext->handleRrcConnectionReconfigurationComplete(rrcCRC);}
      break;
    default: 
      std::cerr << "Unexpected message type from Ue in eNodeB" << std::endl;
  }
}

void EnbEventHandler::handleMmeMessage(S1Message s1Message){
  mNbMessages++;
  std::ostringstream messageLog;
  messageLog << "Total number of messages received in the eNodeB: " << mNbMessages << std::endl;
  mLog->writeToLog(messageLog.str());
  switch (s1Message.messagetype()){
    case S1Message_MessageType_TypeS1ApICSRequest : 
      {S1ApInitialContextSetupRequest s1ApICSRequest;
      s1ApICSRequest = s1Message.messages1apicsrequest();
      UeMapMme::iterator tempIt = mUeContextsMme.find(s1ApICSRequest.enb_ue_s1ap_id()/17);
      if (tempIt == mUeContextsMme.end()) std::cerr << "Could not find context" <<std::endl;
      UeContextEnb *ueContext = tempIt->second;
      ueContext->handleS1ApInitialContextSetupRequest(s1ApICSRequest);}
      break;
    default: 
      std::cerr << "Unexpected message type from Mme in eNodeB" << s1Message.messagetype() <<std::endl;
  }
}
