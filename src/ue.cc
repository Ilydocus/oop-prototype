#include "UeEventHandler.hh"

int main () {
  const int nbOfUes = 100;

  UeEventHandler eventHandler;
  eventHandler.powerOnUes(nbOfUes);
  eventHandler.run();

  return 0;
}




