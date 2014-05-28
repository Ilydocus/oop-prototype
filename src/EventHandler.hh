#ifndef DEF_EVENTHANDLER
#define DEF_EVENTHANDLER

#include "Log.hh"

class EventHandler{
protected:
  Log *mLog;
  int makeSocketNonBlocking (int sfd);  
};

#endif
