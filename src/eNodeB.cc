#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <map>
#include <sys/epoll.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <utility>
#include "RrcMessages.pb.h"
#include "UeContext.h"

#define MAX_EVENTS 10 //for EventLoop

using namespace std;

static int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}

//Event loop
void  eventLoop (int listen_sock) {    

  struct epoll_event ev, events[MAX_EVENTS];
  //int listen_sock, conn_sock, nfds, epollfd;
  int conn_sock, nfds, epollfd;

  //"database" for the UeContext
  map<int,UeContext> ueContexts;

  /* Set up listening socket, 'listen_sock' (socket(),
              bind(), listen()) *///Done in main or other function?

  epollfd = epoll_create(10);//create an epoll instance
  if (epollfd == -1) {
    perror("epoll_create");
               exit(EXIT_FAILURE);
           }

           ev.events = EPOLLIN;
           ev.data.fd = listen_sock;
           if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
	     //add the socket to be watched to the epoll instance
               perror("epoll_ctl: listen_sock");
               exit(EXIT_FAILURE);
           }

           for (;;) {
               nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
               if (nfds == -1) {
                   perror("epoll_pwait");
                   exit(EXIT_FAILURE);
               }

	       for (int n = 0; n < nfds; ++n) {
                   if (events[n].data.fd == listen_sock) {
		     //Accept the socket
		     int new_sd;
		     struct sockaddr_storage their_addr;
		     socklen_t addr_size = sizeof(their_addr);
		     conn_sock = accept(listen_sock, (struct sockaddr *)&their_addr, &addr_size);
		     //conn_sock = accept(listen_sock,
		     //                (struct sockaddr *) &local, &addrlen);
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
		       //Connection to the MME
		       // TO BE ADDED
		       int mme_sock;
		       mme_sock = 0;
		       //Creation of an object to handle this UE
		       UeContext *ueContext = new UeContext(conn_sock,mme_sock);
		       //Store this object in a map
		       ueContexts.insert(pair<int,UeContext>(conn_sock,*ueContext));
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
    
		     RrcMessage rrcMessage;
		     //const string str_message;
		     string str_message(incoming_data_buffer, bytes_recieved);
		     rrcMessage.ParseFromString(str_message);
		     cout << "Message deserialized " << endl;

		     //Get the object handler in the map
		     map<int,UeContext>::iterator temp_it = ueContexts.find(events[n].data.fd);		    
		     UeContext ueContext = temp_it->second;
		     
		     //Switch on the message type
		     switch (rrcMessage.messagetype()){
		     case 0 : //RaPreamble
		       RaPreamble rapreamble;
		       rapreamble = rrcMessage.messagerap();
		       ueContext.handleRaPreamble(rrcMessage.messagerap());
		       cout << "Handling RaP done " << endl;
		     }
		     		     
		     
		     }
                   }
               }
           }
}



int main () {
  

int status;
    struct addrinfo host_info;      
    struct addrinfo *host_info_list; 

    memset(&host_info, 0, sizeof host_info);

    std::cout << "Setting up the structs..."  << std::endl;

    host_info.ai_family = AF_UNSPEC;     
    host_info.ai_socktype = SOCK_STREAM; 
    host_info.ai_flags = AI_PASSIVE;    

     status = getaddrinfo(NULL, "43000", &host_info, &host_info_list);
    
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

    std::cout << "Listen()ing for connections..."  << std::endl;
    status =  listen(socketfd, 5);//5 is the number of "on hold"
    if (status == -1)  std::cout << "listen error" << std::endl ;

    eventLoop(socketfd);


    /*int new_sd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    new_sd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_sd == -1)
    {
        std::cout << "listen error" << std::endl ;
    }
    else
    {
        std::cout << "Connection accepted. Using new socketfd : "  <<  new_sd << std::endl;
	}*/


    /*std::cout << "Waiting to recieve data..."  << std::endl;
    ssize_t bytes_recieved;
    char incoming_data_buffer[1000];
    bytes_recieved = recv(new_sd, incoming_data_buffer,1000, 0);
    // If no data arrives, the program will just wait here until some data arrives.
    if (bytes_recieved == 0) std::cout << "host shut down." << std::endl ;
    if (bytes_recieved == -1)std::cout << "recieve error!" << std::endl ;
    std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
    incoming_data_buffer[bytes_recieved] = '\0';
    //std::cout << incoming_data_buffer << std::endl;

    //The message is serialized, we need to deserialize it
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    RaPreamble rapreamble;
    //const string str_message;
    string str_message(incoming_data_buffer, bytes_recieved);
    std::cout << "Length: " << str_message.length() 
	      << std::endl;
    rapreamble.ParseFromString(str_message);
    std::cout << "Ra_rnti is : " << rapreamble.ueidrntivalue() << std::endl;
    std::cout << "Type of rnti is : " << rapreamble.ueidrntitype() << std::endl;*/


    /*std::cout << "send()ing back a message..."  << std::endl;
    char *msg = "thank you.";
    int len;
    ssize_t bytes_sent;
    len = strlen(msg);
    bytes_sent = send(new_sd, msg, len, 0);

    std::cout << "Stopping server..." << std::endl;
    freeaddrinfo(host_info_list);
    close(new_sd);
    close(socketfd);*/

  



  return 0;
}



