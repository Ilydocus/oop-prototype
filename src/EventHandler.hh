#ifndef DEF_EVENTHANDLER
#define DEF_EVENTHANDLER

#include "Log.hh"

class EventHandler{
protected:
  Log *m_log;
  int make_socket_non_blocking (int sfd);  
};

#endif
