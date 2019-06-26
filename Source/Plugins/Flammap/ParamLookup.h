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
/* ParamLookup.h - class for accessing and maintaining parameters.
**
**  To maintain a set of variables needed over
** the scope of multiple classes, something akin to a namespace, but
** providing a way to create variables on the fly.
**
** History:
** 2010.04.22 - tjs - Started coding
** 2011.03.23 - tjs - Added status for use as a flag when
**   a variable is not found.
*/


#pragma once
#include <afxtempl.h>
#include <iostream>
#include <cstdio>
#include <map>
#include "ParseUtils.h"

// for using map
using namespace std;
class ParamLookup {
private:
	bool
		m_Status;

	// Initialization Parameters
	map<CString, CString>
		m_StringParams;
	map<CString, int>
		m_IntParams;
	map<CString, bool>
		m_BoolParams;
	map<CString, float>
		m_FloatParams;
	map<CString, int>
		m_StringReqParams,
		m_IntReqParams,
		m_BoolReqParams,
		m_FloatReqParams;

	CString
		msg,
		m_TmpCStr;

	// Parsing routines

	char * GetFirstField(char *pcInBuf) {
		return strtok(pcInBuf,": \t\n");
	}
  
	char * GetRestOfLine() {
		char
			*rtrn = NULL;
		char * pcStr = strtok(NULL,"\n");
		if( pcStr != NULL) {
			while(isspace((int)*pcStr))
			pcStr++;
			rtrn = pcStr;
		}
		return rtrn;
	}

	bool StrIsAll(const char *str, const char *tokens);
	bool StrIsInt(const char *str) {
		return StrIsAll(str, "0123456789\n");
	}
	bool StrIsFloat(const char *str) {
		return StrIsAll(str, "0123456789.\n");
	}
	bool StrIsBoolTrue(const char *str);
	bool StrIsBoolFalse(const char *str);

public:
  ParamLookup() {m_Status = true;}
  ParamLookup(CString fName) { ProcessInitFile(fName.GetString()); }
  ParamLookup(const char *fName) { ProcessInitFile(fName); }

  BOOL ProcessInitFile(CString fName)  { return ProcessInitFile(fName.GetString()); }
  BOOL ProcessInitFile(const char *fName);

  void SetStrParam(const char *str, const CString param)
  {CString myNdxCstr(str); SetStrParam(myNdxCstr, param);}

  void SetStrParam(const CString str, const char *param)
  {CString myParamCstr(param); SetStrParam(str, param);}

  void SetStrParam(const char *str, const char *param)
  {CString myNdxCstr(str), myParamCstr(param); SetStrParam(myNdxCstr, myParamCstr);}

  void SetIntParam(const char *str, const int param)
  {CString myNdxCstr(str); SetIntParam(myNdxCstr, param);}

  void SetBoolParam(const char *str, const bool param)
  {CString myNdxCstr(str); SetBoolParam(myNdxCstr, param);}

  void SetFloatParam(const char *str, const float param)
  {CString myNdxCstr(str); SetFloatParam(myNdxCstr, param);}
  
  // The "real" Set routines
  void SetStrParam(const CString str, const CString param)
  {m_Status = true;m_StringParams[str] = param;}

  void SetIntParam(const CString str, const int param)
  {m_Status = true;m_IntParams[str] = param;}

  void SetBoolParam(const CString str, const bool param)
  {m_Status = true;m_BoolParams[str] = param;}

  void SetFloatParam(const CString str, const float param)
  {m_Status = true;m_FloatParams[str] = param;}

  CString GetStrParam(const char *str)
  {CString myNdxCstr(str); return GetStrParam(myNdxCstr);}
  int GetIntParam(const char *str)
  {CString myNdxCstr(str); return GetIntParam(myNdxCstr);}
  bool GetBoolParam(const char *str)
  {CString myNdxCstr(str); return GetBoolParam(myNdxCstr);}
  float GetFloatParam(const char *str)
  {CString myNdxCstr(str); return GetFloatParam(myNdxCstr);}

  CString GetStrParam(const CString str);
  int GetIntParam(const CString str);
  bool GetBoolParam(const CString str);
  float GetFloatParam(const CString str);

  bool IsSetStrParam(const CString str)
  {return (m_StringParams.find(str) != m_StringParams.end());}
  bool IsSetIntParam(const CString str)
  {return (m_IntParams.find(str) != m_IntParams.end());}
  bool IsSetBoolParam(const CString str)
  {return (m_BoolParams.find(str) != m_BoolParams.end());}
  bool IsSetFloatParam(const CString str)
  {return (m_FloatParams.find(str) != m_FloatParams.end());}

  void AddReqStrParam(const CString str)
  {m_StringReqParams[str] = 1;}
  void AddReqIntParam(const CString str)
  {m_IntReqParams[str] = 1;}
  void AddReqBoolParam(const CString str)
  {m_BoolReqParams[str] = 1;}
  void AddReqFloatParam(const CString str)
  {m_FloatReqParams[str] = 1;}

  bool CheckAllReqParams();

  void OutputParams(const char *fName);

  void ResetStatus() {m_Status = true;}

};

