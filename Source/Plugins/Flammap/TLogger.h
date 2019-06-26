/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#pragma once


#include <stdio.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <ctime>

class TLogger {

private:
  FILE
    *LogFile;
  time_t
    rawtime;
  struct tm 
    *timeinfo;
  char
    timeStr[256];
  bool
	  doLogging;

public:
  TLogger(){
    LogFile = NULL;
	doLogging = true;
  }
  TLogger(const char *fName) {
    LogFile = fopen(fName,"w");
	doLogging = true;
  }
  ~TLogger() {
    if(LogFile != NULL)
      fclose(LogFile);
  }

  void SetFile(const char *fName) {
    if(LogFile != NULL)
      fclose(LogFile);
    LogFile = fopen(fName,"w");
  }
  void LogDialogue(const CString &logStr) {
		Log(logStr.GetString());
  }
  void Log(const CString &logStr) {
    Log(logStr.GetString());
  }
  void Log(const char *logStr) {
    if(LogFile != NULL && doLogging) {
      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      strcpy(timeStr, asctime(timeinfo));
      timeStr[strlen(timeStr) - 1] = '\0';
      fprintf(LogFile, "%s: %s\n", timeStr, logStr);
      fflush(LogFile);
    }
  }
  void LogDialogue(const char *logStr) {
    char
      outStr[2048];
      sprintf(outStr, "%s: %s\n", timeStr, logStr);
    if(LogFile != NULL && doLogging) {
      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      strcpy(timeStr, asctime(timeinfo));
      timeStr[strlen(timeStr) - 1] = '\0';
      fprintf(LogFile,outStr);
      fflush(LogFile);
	  MessageBox(NULL, outStr ,"Envision", MB_ICONINFORMATION);
	}

  }

  void DoLogging(bool logOrNot) {
	  doLogging = logOrNot;
  }
  
  void LoggingOn() {
	  doLogging = true;
  }

  void LoggingOff() {
	  doLogging = false;
  }

}; // class TLogger
