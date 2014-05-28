#include "UeEventHandler.hh"
#include <pthread.h>
#include <iostream>
#include "UeEventHandler.hh"
#include <cstring> 
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

UeEventHandler::UeEventHandler(int ueId,Log *log){
  int status;
  struct addrinfo hostInfo;
  struct addrinfo *hostInfoList;

  memset(&hostInfo, 0, sizeof hostInfo);
  hostInfo.ai_family = AF_UNSPEC;
  hostInfo.ai_socktype = SOCK_STREAM;

  status = getaddrinfo("127.0.0.1","43000",&hostInfo, &hostInfoList);
  if (status != 0) std::cout << "getaddrinfo error" << std::endl; 
  int socketfd;
  socketfd = socket(hostInfoList->ai_family, hostInfoList->ai_socktype, hostInfoList->ai_protocol);
  if(socketfd == 1) std::cout << "socket error" << std::endl; 

  status = connect (socketfd, hostInfoList->ai_addr, hostInfoList->ai_addrlen);

  mEnbSocket = socketfd;
  mUeContext = new UeContextUe(ueId, mEnbSocket,log);
  mLog = log;
  mUeId = ueId;

  freeaddrinfo(hostInfoList);
}

void UeEventHandler::run(){
  mUeContext->sendRaPreamble();
  mUeContext->handleRaResponse();
  bool reject = mUeContext->handleRrcConnectionSetup();
  if (!reject){
    mUeContext->handleSecurityModeCommand();
    bool reject2 = mUeContext->handleUeCapabilityEnquiry();
    if (!reject2){
      mUeContext->handleRrcConnectionReconfiguration();
    }
  }
}

UeEventHandler::~UeEventHandler(){
  google::protobuf::ShutdownProtobufLibrary();
  std::cout << "UE "<< mUeId << " switched off..." << std::endl;
  close(mEnbSocket); 
  delete mUeContext; 
  pthread_exit(NULL);
}
