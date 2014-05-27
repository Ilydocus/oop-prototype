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

using namespace std;

EnbEventHandler::EnbEventHandler(){
  int status;
  struct addrinfo host_info;      
  struct addrinfo *host_info_list; 

  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC;     
  host_info.ai_socktype = SOCK_STREAM; 
  host_info.ai_flags = AI_PASSIVE;    

  status = getaddrinfo(NULL, "43000", &host_info, &host_info_list);
  if (status != 0)  cout << "getaddrinfo error" << gai_strerror(status) << endl;

  int socketfd ; 
  socketfd = socket(host_info_list->ai_family,host_info_list->ai_socktype,host_info_list->ai_protocol);
  if (socketfd == -1)  cout << "socket error " << endl;

  int yes = 1;
  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)  cout << "bind error" << endl ;

  status =  listen(socketfd, 5);//5 is the number of "on hold"
  if (status == -1)  cout << "listen error" << endl;
  freeaddrinfo(host_info_list);

  m_listenSocket = socketfd;

  int status_mme;
  struct addrinfo host_info_mme;
  struct addrinfo *host_info_list_mme;

  memset(&host_info_mme, 0, sizeof host_info_mme);
  host_info_mme.ai_family = AF_UNSPEC;
  host_info_mme.ai_socktype = SOCK_STREAM;

  status_mme = getaddrinfo("127.0.0.1","43001",&host_info_mme, &host_info_list_mme);
  if (status_mme != 0) cout << "getaddrinfo error" << endl;
  int socketMme;
  socketMme = socket(host_info_list_mme->ai_family, host_info_list_mme->ai_socktype, host_info_list_mme->ai_protocol);
  if(socketMme == 1) cout << "socket error" << endl;

  status_mme = connect (socketMme, host_info_list_mme->ai_addr, host_info_list_mme->ai_addrlen);
  freeaddrinfo(host_info_list_mme);

  m_mmeSocket = socketMme;
  m_log = new Log("EnbLog.txt");
  m_nbMessages = 0;
}

void EnbEventHandler::run () {    
  struct epoll_event ev, events[MAX_EVENTS];
  int conn_sock, nfds, epollfd;

  epollfd = epoll_create(10);
  if (epollfd == -1) {
    perror("epoll_create");
    exit(EXIT_FAILURE);
  }
  ev.events = EPOLLIN;
  ev.data.fd = m_listenSocket;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, m_listenSocket, &ev) == -1) {
    perror("epoll_ctl: m_listenSocket");
    exit(EXIT_FAILURE);
  }
  
  for (;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_pwait");
      exit(EXIT_FAILURE);
    }
    
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == m_listenSocket) {
	int new_sd;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);
	conn_sock = accept(m_listenSocket, (struct sockaddr *)&their_addr, &addr_size);
	if (conn_sock == -1) {
	  perror("accept");
	  exit(EXIT_FAILURE);
	}
	make_socket_non_blocking(conn_sock);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = conn_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
		      &ev) == -1) {
	  perror("epoll_ctl: conn_sock");
	  exit(EXIT_FAILURE);
	}
	handleNewUe(conn_sock);
      }
      else {
	ssize_t bytes_recieved;
	char incoming_data_buffer[1000];
	bytes_recieved = recv(events[n].data.fd, incoming_data_buffer,1000, 0);
	if (bytes_recieved == 0) {cout << "host shut down." << endl;}
	if (bytes_recieved == -1){cout << "recieve error!" << endl;}
	if (bytes_recieved != -1 && bytes_recieved != 0){
	  incoming_data_buffer[bytes_recieved] = '\0';
	  GOOGLE_PROTOBUF_VERIFY_VERSION;
    
	  RrcMessage rrcMessage;
	  string str_message(incoming_data_buffer, bytes_recieved);
	  rrcMessage.ParseFromString(str_message);
	  map<int,UeContextEnb>::iterator temp_it = m_ueContexts.find(events[n].data.fd);		    
	  UeContextEnb ueContext = temp_it->second;
	
	  handleUeMessage(rrcMessage, ueContext);		     
	  
	}
      }
    }
  }
}

void EnbEventHandler::handleNewUe(int conn_sock){
  UeContextEnb *ueContext = new UeContextEnb(conn_sock,m_mmeSocket,m_log);
  m_ueContexts.insert(pair<int,UeContextEnb>(conn_sock,*ueContext));
} 

void EnbEventHandler::handleUeMessage(RrcMessage rrcMessage, UeContextEnb ueContext){
  m_nbMessages++;
  ostringstream message_log;
  message_log << "Total number of messages received in the eNodeB: " << m_nbMessages << endl;
  m_log->writeToLog(message_log.str());
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
      cerr << "Unexpected message type in eNodeB" << endl;
  }
}
