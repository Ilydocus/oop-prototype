#ifndef DEF_MMEEVENTHANDLER
#define DEF_MMEEVENTHANDLER

#include "UeContextMme.hh"
#include "EventHandler.hh"

class MmeEventHandler: public EventHandler{
  public:
  MmeEventHandler();
  void run();

  private:
  map<int,UeContextMme> m_ueContexts;
  int m_listenSocket;

  void handleNewUe(int conn_sock);
  void handleUeMessage(S1Message rrcMessage,UeContextMme ueContext);

};


#endif