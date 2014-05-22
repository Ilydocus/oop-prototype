#ifndef DEF_ENBEVENTHANDLER
#define DEF_ENBEVENTHANDLER

#include "UeContextEnb.hh"
#include "EventHandler.hh"

class EnbEventHandler: public EventHandler{
  public:
  EnbEventHandler();
  void run();

  private:
  map<int,UeContextEnb> m_ueContexts;
  int m_listenSocket;
  int m_mmeSocket;

  void handleNewUe(int conn_sock);
  void handleUeMessage(RrcMessage rrcMessage,UeContextEnb ueContext);

};


#endif
