#ifndef DEF_LOG
#define DEF_LOG

#include <string>
#include <iostream>
#include <fstream>

using namespace std;

class Log {
public:
  Log(string fileName);
  //Destructeur avec fclose(stderr)
  //virtual void printState() = 0;

  void writeToLog(string message);
private:
  const string currentDateTime();
  ofstream m_file;
};

#endif
