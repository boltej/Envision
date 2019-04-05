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

#include "EnvLibs.h"

#include "tinyxml.h"

#include "Typedefs.h"
#include "Maplayer.h"
#include <string>

struct XML_ATTR {
   LPCTSTR name;
   TYPE    type;
   void   *value;
   bool    isRequired;
   int     checkCol;
   };


//bool LIBSAPI TiXmlGetAttributes( TiXmlElement *pXml, XML_ATTR attrs[], LPCTSTR element, LPCTSTR filename, MapLayer *pLayer=NULL );  // pLayer only needed if CheckCol != 0 (DEPRECATED, use version defined below
bool LIBSAPI TiXmlGetAttributes( TiXmlElement *pXml, XML_ATTR attrs[], LPCTSTR filename, MapLayer *pLayer=NULL ); 


bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, int    &value,  int     defaultValue, bool isRequired );
bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, long   &value,  long    defaultValue, bool isRequired );
bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, float  &value,  float   defaultValue, bool isRequired );
bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, bool   &value,  bool    defaultValue, bool isRequired );
bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, LPTSTR  &value, LPCTSTR defaultValue, bool isRequired );
bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, CString &value, LPCTSTR defaultValue, bool isRequired);
bool LIBSAPI TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, std::string &value, LPCTSTR defaultValue, bool isRequired);
void LIBSAPI TiXmlError( LPCTSTR name, LPCTSTR nodeName );

bool LIBSAPI GetXmlStr( LPCTSTR input, CString &output );
