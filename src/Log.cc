#include "Log.hh"

#include <cstdio>
#include <ctime>

using namespace std;

Log::Log(string fileName){
  mFile.open(fileName.c_str());
}

Log::~Log(){
  mFile.close();
}

void Log::writeToLog(string message){
  mFile << currentDateTime() << message << endl;
}

const string Log::currentDateTime(){
  time_t now=time(0);
  struct tm tstruct;
  char buf[80];

  tstruct = *localtime(&now);
  strftime(buf,sizeof(buf),"%Y-%m-%d.%X : ",&tstruct);
  return buf;
}
