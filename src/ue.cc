#include "UeEventHandler.hh"

int main () {
  Log *log = new Log("UesLog.txt");
  const int nbOfUes = 100;

  UeEventHandler *eventHandler = new UeEventHandler(log);
  eventHandler->powerOnUes(nbOfUes);
  eventHandler->run();

  delete log;
  delete eventHandler;
  return 0;
}




