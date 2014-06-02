#include "UeEventHandler.hh"
#include <iostream>
#include <cstring> 
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <sys/epoll.h>

#define MAX_EVENTS 10 

UeEventHandler::UeEventHandler()
  :mNbCreatedUes(0),mNbCompletedUes(0){
  mLog = new Log("UesLog.txt"); 
}

UeEventHandler::~UeEventHandler(){
  for (UeMap::iterator it = mUeContexts.begin(); it != mUeContexts.end(); ++it){
    UeContextUe *ueContext = it->second;
    delete ueContext;
  }
  delete mLog;
  google::protobuf::ShutdownProtobufLibrary();
}

void UeEventHandler::powerOnUes(int nbOfUes){
  mEpollfd = epoll_create(MAX_EVENTS);
  if (mEpollfd == -1) {
    perror("epoll_create");
    exit(EXIT_FAILURE);
  }
  
  for (int i = 0;i<nbOfUes;i++){
    int socketfd;
    int status;
    struct addrinfo hostInfo;
    struct addrinfo *hostInfoList;

    memset(&hostInfo, 0, sizeof hostInfo);
    hostInfo.ai_family = AF_UNSPEC;
    hostInfo.ai_socktype = SOCK_STREAM;

    status = getaddrinfo("127.0.0.1","43000",&hostInfo, &hostInfoList);
    if (status != 0) {
      perror("getaddrinfo \n");
      exit(EXIT_FAILURE);
    }
    socketfd = socket(hostInfoList->ai_family, hostInfoList->ai_socktype, hostInfoList->ai_protocol);
    if(socketfd == 1){
      perror("socket error \n");
      exit(EXIT_FAILURE);
    }
    
    status = connect (socketfd, hostInfoList->ai_addr, hostInfoList->ai_addrlen);
    if(status == -1) {
      perror("connect error \n");
      exit(EXIT_FAILURE);
    } 

    makeSocketNonBlocking(socketfd);
        
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLOUT; 
    ev.data.fd = socketfd;
    if (epoll_ctl (mEpollfd,EPOLL_CTL_ADD, socketfd, &ev) == -1){
      perror("epoll_ctl, adding socket \n");
      exit(EXIT_FAILURE);
    }
    handleNewUe(socketfd,i+1);
    freeaddrinfo(hostInfoList);
  }
  mNbCreatedUes = nbOfUes;
}

void UeEventHandler::run(){
  int nfds;
  epoll_event events[MAX_EVENTS];
  epoll_event eventMask;
  memset(&eventMask, 0, sizeof eventMask);


  for (;;) {
    nfds = epoll_wait(mEpollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_pwait");
      exit(EXIT_FAILURE);
    }
    
    for (int n = 0; n < nfds; ++n) {
      if (events[n].events & EPOLLOUT) {
	UeMap::iterator tempIt = mUeContexts.find(events[n].data.fd);		    
	UeContextUe *ueContext = tempIt->second;
	ueContext->sendRaPreamble();

	eventMask.events =  EPOLLIN | EPOLLET;
	eventMask.data.fd = events[n].data.fd;
	if (epoll_ctl(mEpollfd, EPOLL_CTL_MOD, events[n].data.fd, &eventMask) != 0){
	  perror("epoll_ctl, modify socket\n");
	  exit(EXIT_FAILURE);
	}
      }
      else {
        if (events[n].events & EPOLLIN) {
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
	     UeMap::iterator tempIt = mUeContexts.find(events[n].data.fd);		    
	     UeContextUe *ueContext = tempIt->second;
	
	     handleEnbMessage(rrcMessage, ueContext);	
	     if (mNbCreatedUes == mNbCompletedUes) return;
	   }
	}
      }
    }
  }
}

void UeEventHandler::handleNewUe(int connSock, int ueId){
  UeContextUe *tempUeContext = new UeContextUe(ueId,connSock,mLog);
  mUeContexts.insert(std::pair<int,UeContextUe*>(connSock,tempUeContext));
}

void UeEventHandler::handleEnbMessage(RrcMessage rrcMessage, UeContextUe *ueContext){
  switch (rrcMessage.messagetype()){
    case RrcMessage_MessageType_TypeRaR : 
      {RaResponse raR;
      raR = rrcMessage.messagerar();
      ueContext->handleRaResponse(raR);}
      break;
    case RrcMessage_MessageType_TypeRrcCS : 
      {RrcConnectionSetup rrcCS;
      rrcCS = rrcMessage.messagerrccs();
      ueContext->handleRrcConnectionSetup(rrcCS);}
      break;
    case RrcMessage_MessageType_TypeSecurityMCommand : 
      {SecurityModeCommand securityMCommand;
      securityMCommand = rrcMessage.messagesecuritymcommand();
      ueContext->handleSecurityModeCommand(securityMCommand);}
      break;
    case RrcMessage_MessageType_TypeUeCE :
      {UeCapabilityEnquiry ueCE;
      ueCE = rrcMessage.messageuece();
      ueContext->handleUeCapabilityEnquiry(ueCE);}
      break;
    case RrcMessage_MessageType_TypeRrcCReconfiguration : 
      {RrcConnectionReconfiguration rrcCReconfiguration;
      rrcCReconfiguration = rrcMessage.messagerrccreconfiguration();
      ueContext->handleRrcConnectionReconfiguration(rrcCReconfiguration);}
      break;
    case RrcMessage_MessageType_TypeRrcCReject : 
      {RrcConnectionReject rrcCReject;
      rrcCReject = rrcMessage.messagerrccreject();
      ueContext->handleRrcConnectionReject(rrcCReject);
      mNbCompletedUes++;}
      break;
    case RrcMessage_MessageType_TypeRrcCA : 
      {RrcConnectionAccept rrcCA;
      rrcCA = rrcMessage.messagerrcca();
      ueContext->handleRrcConnectionAccept(rrcCA);
      mNbCompletedUes++;}
      break;
    default: 
      std::cout << "Unexpected message type in Ue" << std::endl;
  };
}
