#include "Log.hh"

#include <cstdio>
#include <ctime>

Log::Log(std::string fileName){
  mFile.open(fileName.c_str());
}

Log::~Log(){
  mFile.close();
}

void Log::writeToLog(std::string message){
  mFile << currentDateTime() << message << std::endl;
}

const std::string Log::currentDateTime(){
  time_t now=time(0);
  struct tm tstruct;
  char buf[80];

  tstruct = *localtime(&now);
  strftime(buf,sizeof(buf),"%Y-%m-%d.%X : ",&tstruct);
  return buf;
}
