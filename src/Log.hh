#ifndef DEF_LOG
#define DEF_LOG

#include <string>
#include <iostream>
#include <fstream>

using namespace std;

class Log {
public:
  Log(string fileName);
  ~Log();
  void writeToLog(string message);
private:
  const string currentDateTime();
  ofstream mFile;
};

#endif
