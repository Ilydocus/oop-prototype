#include "UeContext.hh"

#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

void UeContext::genRandId(std::string * id,const int len){
  char temp[len];
  static const char num[11] = "0123456789";
  for (int i = 0; i<len; ++i){
    temp[i] = num[(int)((double)rand() / ((double)RAND_MAX + 1)*(sizeof(num)-1))];
  }
  temp[len]=0;
  
  std::string s(temp);
  *id = s;
}

void UeContext::sendRrcMessage(int socket, RrcMessage message){
  std::string outputMessage;
  message.SerializeToString(&outputMessage);

  int len;
  ssize_t bytesSent;
  bytesSent = send (socket, outputMessage.c_str(), 
		     outputMessage.length(), 0);
}

void UeContext::sendS1Message(int socket, S1Message message){
  std::string outputMessage;
  message.SerializeToString(&outputMessage);

  uint32_t nlength = htonl(outputMessage.length());
  ssize_t bytesSent;
  bytesSent = send (socket, &nlength, 4, 0);
  bytesSent = send (socket, outputMessage.c_str(), 
		     outputMessage.length(), 0);
}
 

