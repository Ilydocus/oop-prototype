#ifndef DEF_UEEVENTHANDLER
#define DEF_UEEVENTHANDLER

#include "UeContextUe.hh"
#include "EventHandler.hh"

class UeEventHandler: public EventHandler{
  public:
  UeEventHandler(int ueId, Log *log);
  ~UeEventHandler();
  void run();

  private:
  UeContextUe *m_ueContext;
  int m_enbSocket;
  Log *m_log;

  //void handleNewUe(int conn_sock);
  //void handleUeMessage(RrcMessage rrcMessage,UeContextEnb ueContext);
  void createRaPreamble (RaPreamble *rapreamble, int ueId);
  void sendRaPreamble (int socketfd, int ueId);

};


#endif
