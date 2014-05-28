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
  UeContextUe *mUeContext;
  int mEnbSocket;
  int mUeId;

  void createRaPreamble (RaPreamble *rapreamble, int ueId);
  void sendRaPreamble (int socketfd, int ueId);
};

#endif
