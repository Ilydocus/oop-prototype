#include "UeEventHandler.hh"
#include <pthread.h>
#include <iostream>
#include "UeEventHandler.hh"
#include <cstring> //for memset
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

UeEventHandler::UeEventHandler(int ueId){
  //Create a socket and connect it
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;

  memset(&host_info, 0, sizeof host_info);
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo("127.0.0.1","43000",&host_info, &host_info_list);
  //status != 0 -error
  int socketfd;
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  //use first one in the list
  //socket == 1 -error

  status = connect (socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);

  //Initialize the UE state
  m_enbSocket = socketfd;
  
  m_ueContext = new UeContextUe(ueId, m_enbSocket);


  freeaddrinfo(host_info_list);
}

void UeEventHandler::run(){

  m_ueContext->sendRaPreamble();
  m_ueContext->handleRaResponse();
  bool reject = m_ueContext->handleRrcConnectionSetup();
  if (!reject){
    m_ueContext->handleSecurityModeCommand();
    bool reject_2 = m_ueContext->handleUeCapabilityEnquiry();
    if (!reject_2){
      m_ueContext->handleRrcConnectionReconfiguration();
      //see if need of a RrcConnectionAccept
    }
  }
}

UeEventHandler::~UeEventHandler(){
  //Termination of the procedure
  google::protobuf::ShutdownProtobufLibrary();
  std::cout << "Stopping server..." << std::endl;
  //freeaddrinfo(host_info_list);
  close(m_enbSocket);
  
  delete m_ueContext;
  
  pthread_exit(NULL);
}