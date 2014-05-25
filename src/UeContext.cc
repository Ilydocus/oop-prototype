#include "UeContext.hh"

#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>

void UeContext::genRandId(string * id,const int len){
  char temp[len];
  static const char num[11] = "0123456789";
  for (int i = 0; i<len; ++i){
    temp[i] = num[(int)((double)rand() / ((double)RAND_MAX + 1)*(sizeof(num)-1))];
  }
  temp[len]=0;
  string s(temp);
  *id = s;
}

void UeContext::sendMessage(int socket, string output_message){
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (socket, output_message.c_str(), 
		     output_message.length(), 0);
}
