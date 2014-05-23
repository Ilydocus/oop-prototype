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

#define MAX_EVENTS 10 //for EventLoop

using namespace std;

MmeEventHandler::MmeEventHandler(){
    

int status;
    struct addrinfo host_info;      
    struct addrinfo *host_info_list; 

    memset(&host_info, 0, sizeof host_info);

    std::cout << "Setting up the structs..."  << std::endl;

    host_info.ai_family = AF_UNSPEC;     
    host_info.ai_socktype = SOCK_STREAM; 
    host_info.ai_flags = AI_PASSIVE;    

     status = getaddrinfo(NULL, "43001", &host_info, &host_info_list);
    
     //if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;


    std::cout << "Creating a socket..."  << std::endl;
    int socketfd ; 
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,host_info_list->ai_protocol);
    //if (socketfd == -1)  std::cout << "socket error " ;

    std::cout << "Binding socket..."  << std::endl;
    // we use to make the setsockopt() function to make sure the port is not in use
    // by a previous execution of our code. (see man page for more information)
    int yes = 1;
    status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1)  std::cout << "bind error" << std::endl ;

    std::cout << "Listening for connections..."  << std::endl;
    status =  listen(socketfd, 5);//5 is the number of "on hold"
    if (status == -1)  std::cout << "listen error" << std::endl ;

    m_listenSocket = socketfd;
}

void MmeEventHandler::run () {    

  struct epoll_event ev, events[MAX_EVENTS];
  //int m_listenSocket, conn_sock, nfds, epollfd;
  int conn_sock, nfds, epollfd;

  /* Set up listening socket, 'm_listenSocket' (socket(),
              bind(), listen()) *///Done in main or other function?

  epollfd = epoll_create(10);//create an epoll instance
  if (epollfd == -1) {
    perror("epoll_create");
               exit(EXIT_FAILURE);
           }

           ev.events = EPOLLIN;
           ev.data.fd = m_listenSocket;
           if (epoll_ctl(epollfd, EPOLL_CTL_ADD, m_listenSocket, &ev) == -1) {
	     //add the socket to be watched to the epoll instance
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
		     //Accept the socket
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
				     &ev) == -1) {//add this socket to the list of to be checked sockets
                           perror("epoll_ctl: conn_sock");
                           exit(EXIT_FAILURE);
                       }
		       this->handleNewUe(conn_sock);
                   } else {
		    
		     //do_use_fd(events[n].data.fd);
		     //Read and deserialize the message
		     ssize_t bytes_recieved;
		     char incoming_data_buffer[1000];
		     bytes_recieved = recv(events[n].data.fd, incoming_data_buffer,1000, 0);
		      // If no data arrives, the program will just wait here until some data arrives.
		     if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl ;}
		       //return;}
		     if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl ;}//return;}
		     std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
		     if (bytes_recieved != -1 && bytes_recieved != 0){
		     incoming_data_buffer[bytes_recieved] = '\0';
		     //std::cout << incoming_data_buffer << std::endl;

		     //The message is serialized, we need to deserialize it
		     GOOGLE_PROTOBUF_VERIFY_VERSION;
    
		     S1Message s1Message;
		     //const string str_message;
		     string str_message(incoming_data_buffer, bytes_recieved);
		     s1Message.ParseFromString(str_message);
		     cout << "Message deserialized " << endl;

		     //Get the object handler in the map
		     map<int,UeContextMme>::iterator temp_it = m_ueContexts.find(events[n].data.fd);		    
		     UeContextMme ueContext = temp_it->second;
		     
		     //Switch on the message type
		     handleUeMessage(s1Message, ueContext);		     
		     
		     }
                   }
               }
           }
}

void MmeEventHandler::handleNewUe(int conn_sock){

  //Creation of an object to handle this UE
  UeContextMme *ueContext = new UeContextMme(conn_sock);
  //Store this object in a map
  m_ueContexts.insert(pair<int,UeContextMme>(conn_sock,*ueContext));
} 

void MmeEventHandler::handleUeMessage(S1Message s1Message, UeContextMme ueContext){
  switch (s1Message.messagetype()){
    case 0 : //S1ApInitialUeMessage
      {S1ApInitialUeMessage initialUeMessage;
      initialUeMessage = s1Message.messages1apiuem();
      ueContext.handleS1ApInitialUeMessage(initialUeMessage);
      cout << "Handling InitialUeMessage done " << endl;}
      break;
    case 2 : //S1ApInitialContextSetupResponse
      {S1ApInitialContextSetupResponse initialCSResponse;
      initialCSResponse = s1Message.messages1apicsresponse();
      ueContext.handleS1ApInitialContextSetupResponse(initialCSResponse);}
      break;
  }
}
