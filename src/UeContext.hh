#ifndef DEF_UECONTEXT
#define DEF_UECONTEXT

#include <string>

using namespace std;

class UeContext {
protected:
  void genRandId(string * id,const int len);
  void sendMessage(int socket, string output_message);
};

#endif
