#include "EventHandler.hh"

#include <fcntl.h>
#include <cstdio>

int EventHandler::makeSocketNonBlocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      std::cerr << "error in fcntl" << std::endl;
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      std::cerr << "error in fcntl" << std::endl;
      return -1;
    }

  return 0;
}
