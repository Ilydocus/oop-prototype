#ifndef DEF_UEEVENTHANDLER
#define DEF_UEEVENTHANDLER

#include "UeContextUe.hh"
#include "EventHandler.hh"

class UeEventHandler: public EventHandler{
  public:
  UeEventHandler(int ueId);
  ~UeEventHandler();
  void run();

  private:
  UeContextUe *m_ueContext;
  int m_enbSocket;
  //int m_mmeSocket;

  //void handleNewUe(int conn_sock);
  //void handleUeMessage(RrcMessage rrcMessage,UeContextEnb ueContext);
  void createRaPreamble (RaPreamble *rapreamble, int ueId);
  void sendRaPreamble (int socketfd, int ueId);

};


#endif
