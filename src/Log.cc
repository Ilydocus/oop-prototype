#include "Log.hh"

#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <sstream>
#include <iomanip>

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
  timeval tv;
  tm *tm;
  int rc;

  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);
  
  std::ostringstream timestamp;
  timestamp << 1900 + tm->tm_year << "-" << std::setfill('0') 
	    << std::setw(2) << 1 + tm->tm_mon << "-" << std::setfill('0') 
	    << std::setw(2) << tm->tm_mday << "." << std::setfill('0') 
	    << std::setw(2) <<tm->tm_hour << ":" << std::setfill('0') 
	    << std::setw(2) << tm->tm_min << ":" << std::setfill('0') 
	    << std::setw(2) << tm->tm_sec << ":" << tv.tv_usec << std::endl; 
  return timestamp.str();
}
