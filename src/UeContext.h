#ifndef DEF_UECONTEXT
#define DEF_UECONTEXT

#include <iostream>
#include "RrcMessages.pb.h"

class UeContext 
{
 public:

  UeContext(int ueSocket, int mmeSocket);
  void handleRaPreamble(RaPreamble message);

 private:

  int m_ueSocket;
  int m_mmeSocket;
  int m_state;//To be expanded afterwards

};

#endif
