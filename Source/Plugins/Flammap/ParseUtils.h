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


class ParseUtils {
public:

  static char * GetFirstField(char *pcInBuf) {
    return strtok(pcInBuf," \t\n");
  } // char * ParseUtils::GetFirstField(char *pcInBuf)

  static char * GetNextField() {
    return strtok(NULL," \t\n");
  } // char * ParseUtils::GetNextField()

  static char * ParseUtils::GetRestOfLine() {
    return strtok(NULL,"\t\n");
  } // char * ParseUtils::GetRestOfLine()

  static void TrimSpaces(char *);
}; // class ParseUtils
