#ifndef DEF_LOG
#define DEF_LOG

#include <string>
#include <iostream>
#include <fstream>

class Log {
public:
  Log(std::string fileName);
  ~Log();
  void writeToLog(std::string message);
private:
  const std::string currentDateTime();
  std::ofstream mFile;
};

#endif
