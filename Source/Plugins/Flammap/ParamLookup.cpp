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
/* ParamLookup.cpp - See .h file for more info
**
** 2011.03.23 - tjs - Added error handling
*/

#include "stdafx.h"
#pragma hdrstop

#include "ParamLookup.h"
#include <MapLayer.h>
#include <stdio.h>
#include <cstring>
#include <cctype>

CString ParamLookup::GetStrParam(const CString str)
   {
   CString rtrn("");

   m_Status = true;

   if(m_StringParams.find(str) == m_StringParams.end()) 
      {
      msg.Format(_T("Missing string parameter \"%s\""),str.GetString());
      Report::InfoMsg(msg);
      m_Status = false;
      }
   else
      {
      rtrn = m_StringParams.find(str)->second;
      }

   return rtrn;

} // CString ParamLookup::GetStrParam(const CString str)

int ParamLookup::GetIntParam(const CString str) 
   {
   int rtrn = 0;

   m_Status = true;

   if(m_IntParams.find(str) == m_IntParams.end()) 
      {
      msg.Format(_T("Missing int parameter \"%s\""),str.GetString());
      Report::InfoMsg(msg);
      m_Status = false;
      }
   else 
      {
      rtrn = m_IntParams.find(str)->second;
      }

   return rtrn;

} // int ParamLookup::GetIntParam(const CString str)

bool ParamLookup::GetBoolParam(const CString str) 
   {
   bool rtrn = false;

   m_Status = true;

   if(m_BoolParams.find(str) == m_BoolParams.end()) 
      {
      msg.Format(_T("Missing Bool parameter \"%s\""),str.GetString());
      Report::InfoMsg(msg);
      m_Status = false;
      }
   else
      {
      rtrn = m_BoolParams.find(str)->second;
      }

   return rtrn;

} // bool ParamLookup::GetBoolParam(const CString str)

float ParamLookup::GetFloatParam(const CString str) 
   {
   float rtrn = 0.0;

   m_Status = true;

   if(m_FloatParams.find(str) == m_FloatParams.end()) 
      {
      msg.Format(_T("Missing float parameter \"%s\""),str.GetString());
      Report::InfoMsg(msg);
      m_Status = false;
      }
   else
      {
      rtrn = m_FloatParams.find(str)->second;
      }

   return rtrn;
   } // float ParamLookup::GetFloatParam(const CString str)

bool ParamLookup::CheckAllReqParams() 
   {
   bool rtrn = true;

   m_Status = true;

   map<CString, int>::iterator itReq;

   for(itReq=m_StringReqParams.begin(); itReq!=m_StringReqParams.end(); itReq++) 
      {
      if(m_StringParams.find(itReq->first) == m_StringParams.end()) 
         {
         msg.Format(_T("m_StringParams missing required parameter: %s"), itReq->first);
         Report::ErrorMsg(msg);
         rtrn = false;
         }
      } // for(itReq=m_ReqStringParams.begin(); itReq!=m_ReqStringParams.end(); itReq++) {

   for(itReq=m_IntReqParams.begin(); itReq!=m_IntReqParams.end(); itReq++) 
      {
      if(m_IntParams.find(itReq->first) == m_IntParams.end()) 
         {
         msg.Format(_T("m_IntParams missing required parameter: %s"), itReq->first);
         Report::ErrorMsg(msg);
         rtrn = false;
         }
      } // for(itReq=m_ReqIntParams.begin(); itReq!=m_ReqIntParams.end(); itReq++) {

   for(itReq=m_BoolReqParams.begin(); itReq!=m_BoolReqParams.end(); itReq++) 
      {
      if(m_BoolParams.find(itReq->first) == m_BoolParams.end()) 
         {
         msg.Format(_T("m_BoolParams missing required parameter: %s"), itReq->first);
         Report::ErrorMsg(msg);
         rtrn = false;
         }
      } // for(itReq=m_ReqBoolParams.begin(); itReq!=m_ReqBoolParams.end(); itReq++) {

   for(itReq=m_FloatReqParams.begin(); itReq!=m_FloatReqParams.end(); itReq++) 
      {
      if(m_FloatParams.find(itReq->first) == m_FloatParams.end()) 
         {
         msg.Format(_T("m_FloatParams missing required parameter: %s"), itReq->first);
         Report::ErrorMsg(msg);
         rtrn = false;
         }
      } // for(itReq=m_ReqFloatParams.begin(); itReq!=m_ReqFloatParams.end(); itReq++) {

   return rtrn;
   } // bool ParamLookup::CheckAllReqParams();

void ParamLookup::OutputParams(const char *fName) 
   {
   m_Status = true;

   FILE *outFile = fopen(fName, "w");
   
   if ( outFile == NULL)
      {
      CString msg( "Flammap: Error opening file " );
      msg += fName;
      Report::ErrorMsg( msg );
      }

   map<CString, int>::iterator itReq;

   fprintf(outFile, "Required String Parameters:\n\n");

   for(itReq=m_StringReqParams.begin(); itReq!=m_StringReqParams.end(); itReq++) 
      fprintf(outFile, "%s\n", (LPCTSTR) itReq->first);

   fprintf(outFile, "\n\nString Parameters:\n\n");

   map<CString, CString>::iterator itString;

   for(itString=m_StringParams.begin(); itString!=m_StringParams.end(); itString++)
      fprintf(outFile, "%s : %s\n", (LPCTSTR) itString->first, (LPCTSTR)itString->second);

   // Integer Parameters
   fprintf(outFile, "\n\nRequired Int Parameters:\n\n");

   for(itReq=m_IntReqParams.begin(); itReq!=m_IntReqParams.end(); itReq++) 
      fprintf(outFile, "%s\n", (LPCTSTR) itReq->first);

   fprintf(outFile, "\n\nInt Parameters:\n\n");

   map<CString, int>::iterator itInt;

   for(itInt=m_IntParams.begin(); itInt!=m_IntParams.end(); itInt++)
      fprintf(outFile, "%s : %d\n", (LPCTSTR)itInt->first, itInt->second);

   // Boolean Parameters
   fprintf(outFile, "\n\nRequired Bool Parameters:\n\n");

   for(itReq=m_BoolReqParams.begin(); itReq!=m_BoolReqParams.end(); itReq++)
      fprintf(outFile, "%s\n", (LPCTSTR)itReq->first);

   fprintf(outFile, "\n\nBool Parameters:\n\n");

   map<CString, bool>::iterator itBool;

   for(itBool=m_BoolParams.begin(); itBool!=m_BoolParams.end(); itBool++) 
      fprintf(outFile, "%s : %s\n", (LPCTSTR)itBool->first, (itBool->second)?"true":"false");

   // Float Parameters
   fprintf(outFile, "\n\nRequired Float Parameters:\n\n");

   for(itReq=m_FloatReqParams.begin(); itReq!=m_FloatReqParams.end(); itReq++) 
      fprintf(outFile, "%s\n", (LPCTSTR) itReq->first);

   fprintf(outFile, "\n\nFloat Parameters:\n\n");

   map<CString, float>::iterator itFloat;

   for(itFloat=m_FloatParams.begin(); itFloat!=m_FloatParams.end(); itFloat++)
      fprintf(outFile, "%s : %f\n", (LPCTSTR) itFloat->first, itFloat->second);

   fclose(outFile);

   } // void ParamLookup::OutputParams(const char *fName)

// Private member functions

bool ParamLookup::StrIsAll(const char *str, const char *tokens) {
  // checks to see if all the characters in str are
  // tokens in tokens
  unsigned
    strNdx,
    tokenNdx;
  bool
    rtrn = false;

  for(strNdx=0;strNdx<strlen(str);strNdx++) {
    rtrn = false;
    for(tokenNdx=0;tokenNdx<strlen(tokens);tokenNdx++) {
      if(str[strNdx] == tokens[tokenNdx]) {
        rtrn = true;
        break;
      }
    } // for(tokenNdx=0;tokenNdx<strlen(tokens);tokenNdx++) {
      if(rtrn == false)
        break;
  } //   for(strNdx=0;strNdx<strlen(str);strNdx++) {

  return rtrn;
} // bool ParamLookup::StrIsAll(const char *str, const char *tokens) {

bool ParamLookup::StrIsBoolTrue(const char *str) {
  char
    cmpBuf[5];
  bool
    rtrn = false;

  if(strlen(str) == 4 ||
    (strlen(str) == 5 && str[4] == '\n')) {
      for(int i=0; i<=3; i++) {
        cmpBuf[i] = toupper(str[i]);
      }
      cmpBuf[4] = '\0';
      if(!strcmp(cmpBuf,"TRUE"))
        rtrn = true;
  } // if(strlen(str)...

  return rtrn;
} // bool StrIsBoolTrue(const char *str)

bool ParamLookup::StrIsBoolFalse(const char *str) {
  char
    cmpBuf[6];
  bool
    rtrn = false;

  if(strlen(str) == 5 ||
    (strlen(str) == 6 && str[5] == '\n')) {
      for(int i=0; i<=4; i++) {
        cmpBuf[i] = toupper(str[i]);
      }
      cmpBuf[5] = '\0';
      if(!strcmp(cmpBuf,"FALSE"))
        rtrn = true;
  } // if(strlen(str)...

  return rtrn;
} // bool StrIsBoolFalse(const char *str)

BOOL ParamLookup::ProcessInitFile(const char *fName) {
   FILE
      *inFile;
   CString
      paramKey,
      failureID = (_T("Error: ParamLookup::ProcessInitFile Failed: "));
   char
      *paramName,
      *paramValue,
      inBuf[1024],
      holdBuf[1024];
   bool
      errorFlag = false;

   m_Status = true;

   // Read and parse initialization file

   if((inFile = fopen(fName,"r")) == NULL) {
      msg.Format(_T("unable to open file: %s!\n"),
         fName);
      msg = failureID + msg;
      Report::InfoMsg(msg);
      m_Status = false;
      return false;
   }

   while(fgets(inBuf, (int)sizeof(inBuf), inFile)) {
 
      // keep a copy of inbuf for error message
      strcpy(holdBuf,inBuf);

      paramName = GetFirstField(inBuf);

      // skip a blank line or comment line
      if(paramName == NULL || paramName[0] == '#')
         continue;

      // Get the field value and insure it is not null
      if((paramValue = GetRestOfLine()) == NULL) {
         msg.Format(_T(" Bad input line in file: %s\nline: %s"),
         fName, holdBuf);
         msg = failureID + msg;
         Report::InfoMsg(msg);
         m_Status = false;
         return false;
      }

      paramKey = paramName;

      if(StrIsInt(paramValue)) {
         m_IntParams[paramKey] = atoi(paramValue);
      } else if(StrIsFloat(paramValue)) {
         m_FloatParams[paramKey] = (float) atof(paramValue);
      } else if(StrIsBoolFalse(paramValue)) {
         m_BoolParams[paramKey] = false;
      } else if(StrIsBoolTrue(paramValue)) {
         m_BoolParams[paramKey] = true;
      } else {
         m_StringParams[paramKey] = paramValue;
      }

   } // end while(fgets(pcInBuf, (int)sizeof(pcInBuf), pInFile))

   return TRUE;
} // void ParamLookup::ProcessInitFile(char *fName)
