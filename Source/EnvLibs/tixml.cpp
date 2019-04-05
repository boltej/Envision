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
#include "EnvLibs.h"
#pragma hdrstop

#include "tixml.h"

#include "Report.h"

#include <string>

void TiXmlError( LPCTSTR name, LPCTSTR nodeName )
   {
#ifndef NO_MFC
   CString msg( "XML Error parsing " );
   msg += name;
   msg += ":";
   msg += nodeName;
   msg += " Node doesn't exist.";
   Report::LogError( msg );
#endif
   return;
   }


bool TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, long &value, long defaultValue, bool isRequired )
   {
   LPCTSTR strVal = pElement->Attribute( attrName );
   
   if ( strVal == NULL )
      {
      value = defaultValue;   
      if ( isRequired )
         {
         TiXmlError( pElement->Value(), attrName );
         return false;
         }
      else
         return true;
      }

   value = atol( strVal );
   return true;
   }



bool GetXmlStr( LPCTSTR input, CString &output )
   {
   output.Empty();

   TCHAR *p = (TCHAR*) input;
   while ( *p != NULL )
      {
      switch( *p )
         {
         case '"':
            output.Append( _T( "&quot;" ) );
            break;

         case '\'':
            output.Append( _T( "&apos;" ) );
            break;

         case '&':
            output.Append( _T("&amp;") );
            break;

         case '<':
         case '>':
            {
            bool padFront = false;
            bool padRear = false;
            if ( p == input ) // first char?
               padFront = true;
            else if ( *(p-1) != ' ' )
               padFront = true;

            if ( *(p+1) != ' ' && *(p+1) != '=' )
               padRear = true;

            if ( padFront )
               output.Append( _T(" ") );

            if ( *p == '<' )
               output.Append( _T( "&lt;" ) );
            else
               output.Append( _T( "&gt;" ) );

            if ( padRear )
               output.Append( _T(" ") );
            }
            break;

         default:
            output += *p;  //.AppendChar( *p );
         }

      ++p;
      }
   return true;
   }



bool TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, int &value, int defaultValue, bool isRequired )
   {
   LPCTSTR strVal = pElement->Attribute( attrName );
   
   if ( strVal == NULL )
      {
      value = defaultValue;   
      if ( isRequired )
         {
         TiXmlError( pElement->Value(), attrName );
         return false;
         }
      else
         return true;
      }

   value = atoi( strVal );
   return true;
   }

bool TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, float &value, float defaultValue, bool isRequired )
   {
   LPCTSTR strVal = pElement->Attribute( attrName );
   
   if ( strVal == NULL )
      {
      value = defaultValue;   
      if ( isRequired )
         {
         TiXmlError( pElement->Value(), attrName );
         return false;
         }
      else
         return true;
      }

   value = (float) atof( strVal );
   return true;
   }


bool TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, bool &value, bool defaultValue, bool isRequired )
   {
   LPCTSTR strVal = pElement->Attribute( attrName );
   
   if ( strVal == NULL )
      {
      value = defaultValue;   
      if ( isRequired )
         {
         TiXmlError( pElement->Value(), attrName );
         return false;
         }
      else
         return true;
      }

   switch( *strVal )
      {
      case 't':
      case 'T':
      case '1':
      case 'y':
      case 'Y':
         value = true;
         break;

      default:
         value = false;
      }
   
   return true;
   }


bool TiXmlGetAttr( TiXmlElement *pElement, LPCTSTR attrName, LPTSTR &value, LPCTSTR defaultValue, bool isRequired )
   {
   LPCTSTR attrValue = pElement->Attribute( attrName );
   if ( attrValue == NULL )
      {
      value = (LPTSTR) defaultValue;   
      if ( isRequired )
         {
         TiXmlError( pElement->Value(), attrName );
         return false;
         }
      else
         return true;
      }

   else
      value = (LPTSTR) attrValue;
   
   return true;
   }


bool TiXmlGetAttr(TiXmlElement *pElement, LPCTSTR attrName, CString &value, LPCTSTR defaultValue, bool isRequired)
   {
   LPCTSTR strVal = pElement->Attribute(attrName);

   if (strVal == NULL)
      {
      value = defaultValue;
      if (isRequired)
         {
         TiXmlError(pElement->Value(), attrName);
         return false;
         }
      else
         return true;
      }

   value = strVal;
   return true;
   }


bool TiXmlGetAttr(TiXmlElement *pElement, LPCTSTR attrName, std::string &value, LPCTSTR defaultValue, bool isRequired)
   {
   LPCTSTR strVal = pElement->Attribute(attrName);

   if (strVal == NULL)
      {
      value = defaultValue;
      if (isRequired)
         {
         TiXmlError(pElement->Value(), attrName);
         return false;
         }
      else
         return true;
      }

   value = strVal;
   return true;
   }


bool TiXmlGetAttributes( TiXmlElement *pXml, XML_ATTR attrs[], LPCTSTR filename, MapLayer *pLayer /*=NULL*/ ) 
   {
   LPCTSTR element = pXml->Value();

   // count number of elements in XM_ARRAY
   int arraySize = 0;
   while ( attrs[ arraySize ].name != NULL ) arraySize++;

   // allocate flags for found
   bool *foundArray = new bool[ arraySize ];
   memset( foundArray, 0, arraySize * sizeof( bool ) );

   // iterate through xml attributes associated with this element
   TiXmlAttribute *pAttr = pXml->FirstAttribute();

   while( pAttr != NULL )
      {
      LPCTSTR name = pAttr->Name();

      bool found = false;

      // iterate through the XML_ATTRs to try to find an appropriate entry
      for ( int i=0; i < arraySize; i++ )
         {
         if ( lstrcmp( name, attrs[ i ].name ) == 0 )   // does this XML_ATTR match?
            {
            found = true;

            if ( foundArray[ i ] == true )
               {
               CString msg;
               msg.Format( "Warning: duplicate XML attribute '%s' found reading tag <%s> in file %s - the duplicate will be ignored",
                              name, element, filename );
               Report::LogWarning( msg );
               break;
               }

            foundArray[ i ] = true;

            switch( attrs[ i ].type )
               {
               case TYPE_BOOL:
                  {
                  TCHAR *value = (TCHAR*) pAttr->Value();   // "true" or "false"

                  if ( value == NULL )
                     break;

                  if ( value && ( *value == 't' || *value == 'T' || *value == '1' ))
                     *((bool*)attrs[ i ].value) = true;
                  else
                     *((bool*)attrs[ i ].value) = false;
                  }
                  break;

               case TYPE_CHAR:
                  {
                  TCHAR *value = (TCHAR*) pAttr->Value();   // "true" or "false"

                  if ( value == NULL )
                     break;

                  *((TCHAR*)attrs[ i ].value) = *value;
                  }
                  break;

               case TYPE_INT:
                  {
                  int value;
                  if ( pAttr->QueryIntValue( &value ) != 0 )
                     {
                     CString msg;
                     msg.Format( "Error reading attribute %s value='%s' when parsing XML element <%s> in file %s.  This attribute will be ignored", name, pAttr->Value(), element, filename );
                      Report::LogError( msg );
                     break;
                     }

                  *((int*)(attrs[ i ].value)) = value;
                  break;
                  }

               case TYPE_UINT:
                  {
                  int value;
                  if ( pAttr->QueryIntValue( &value ) != 0 )
                     {
                     found = false;
                     CString msg;
                     msg.Format( "Error reading attribute %s value='%s' when parsing XML element <%s> in file %s.  This attribute will be ignored", name, pAttr->Value(), element, filename );
                      Report::LogError( msg );
                     break;
                     }

                  *((UINT*)(attrs[ i ].value)) = (UINT) value;
                  break;
                  }

               case TYPE_LONG:
                  {
                  int value;
                  if ( pAttr->QueryIntValue( &value ) != 0 )
                     {
                     CString msg;
                     msg.Format( "Error reading attribute %s value='%s' when parsing XML element <%s> in file %s.  This attribute will be ignored", name, pAttr->Value(), element, filename );
                      Report::LogError( msg );
                     break;
                     }

                  *((long*)(attrs[ i ].value)) = (long) value;
                  break;
                  }

               case TYPE_ULONG:
                  {
                  int value;
                  if ( pAttr->QueryIntValue( &value ) != 0 )
                     {
                     CString msg;
                     msg.Format( "Error reading attribute %s value='%s' when parsing XML element <%s> in file %s.  This attribute will be ignored", name, pAttr->Value(), element, filename );
                      Report::LogError( msg );
                     break;
                     }

                  *((ULONG*)(attrs[ i ].value)) = (ULONG) value;
                  break;
                  }

               case TYPE_FLOAT:
                  {
                  double value;
                  if ( pAttr->QueryDoubleValue( &value ) != 0 )
                     {
                     CString msg;
                     msg.Format( "Error reading attribute %s value='%s' when parsing XML element <%s> in file %s.  This attribute will be ignored", name, pAttr->Value(), element, filename );
                      Report::LogError( msg );
                     break;
                     }

                  *((float*)(attrs[ i ].value)) = (float) value;
                  break;
                  }

               case TYPE_DOUBLE:
                  {
                  double value;
                  if ( pAttr->QueryDoubleValue( &value ) != 0 )
                     {
                     CString msg;
                     msg.Format( "Error reading attribute %s value='%s' when parsing XML element <%s> in file %s.  This attribute will be ignored", name, pAttr->Value(), element, filename );
                      Report::LogError( msg );
                     break;
                     }

                  *((double*)(attrs[ i ].value)) = (double) value;
                  break;
                  }

               case TYPE_STRING:
               case TYPE_DSTRING:   // both of these only deal with the pointer, not the full string
                  {
                  TCHAR *value = (TCHAR*) pAttr->Value();   // ptr to string held by parser
                  *((TCHAR**)(attrs[ i ].value)) = value;   // .value is set to the address of the char ptr
                  }
                  break;

               case TYPE_CSTRING:  // attrs[index].value holds the address of a pointer to a CString
                  {
                  TCHAR *value = (TCHAR*) pAttr->Value();   // ptr to string held by parser
                  *((CString*)(attrs[ i ].value)) = value;
                  }
                  break;
               
               case TYPE_STLSTRING:   // attrs[index].value holds the address of a pointer to an std::string
                  {
                  TCHAR *value = (TCHAR*) pAttr->Value();   // ptr to string held by parser
                  *((std::string*)(attrs[ i ].value))= value;
                  }
                  break;

               default:
                  attrs[ i ].value = NULL;
                  TRACE( "Invalid type encountered in GetXmlAttributes()\n" );
                  break;
               }  // end of switch( type )
            break;   // out of while statement, found=true;
            }  // end of: if name match
         }  // end of: for ( i < arraySize ) - finished looking through the ATTR array

      // done with looking through the  XML_ATTR array.  If we didn't find the XML attribute in the array,
      // generate a warning and continue 
      if ( ! found )
         {
         CString msg;
         msg.Format( "The attribute '%s' referenced by element <%s> in Xml input file %s is not recognized and will be ignored", name, element, filename );
         Report::LogWarning( msg );
         }
      else    // it was found, no other action required 
         {
         }
      
      pAttr = pAttr->Next();
      }  // end of: while( pAttr != NULL )


   // at this point, we've iterated through all the attribute of the element, check on what we found
   for ( int i=0; i < arraySize; i++ )
      {
      if ( foundArray[ i ] == false && attrs[ i ].isRequired )
         {
         CString msg;
         msg.Format( "Error: Xml element <%s> is missing a required attribute '%s' when parsing file %s.  This attribute will be ignored", element, attrs[ i ].name, filename );
         //attrs[i].value = NULL;
         Report::LogError( msg );
         }

      int checkCol = attrs[ i ].checkCol;
      if ( checkCol > 0 && ( attrs[i].isRequired || ( ! attrs[i].isRequired && ( *((TCHAR**) attrs[ i ].value) != NULL && *(*((TCHAR**) attrs[ i ].value)) != NULL ) ) ) )
         {
         if ( pLayer == NULL )
            {
            CString msg;
            msg.Format( "Error reading XML element <%s>, attribute '%s':  No Map Layer specified for checking column info.  Column checking wil be disabled",
               element, attrs[ i ].name );
            Report::LogWarning( msg );
            }

         TCHAR *field = *((TCHAR**) attrs[ i ].value);

         attrs[ i ].checkCol = pLayer->GetFieldCol( field );
         if ( attrs[ i ].checkCol < 0 )
            {
            if ( ( checkCol & CC_AUTOADD ) && ( field != NULL && field[ 0 ] != NULL ) )    // autoadd?
               {
               TYPE typeToAdd = (TYPE) ( checkCol & (~CC_AUTOADD ) );     // automatically adds column if not present without prompting

               if ( typeToAdd == TYPE_NULL )
                  typeToAdd = TYPE_LONG;

               int width, decimals;
               GetTypeParams( typeToAdd, width, decimals );
               attrs[ i ].checkCol = pLayer->m_pDbTable->AddField( field, typeToAdd, width, decimals, true );
               }
            else if ( checkCol & CC_MUST_EXIST )
               {
#ifndef NO_MFC
               CString msg;
               msg.Format( "The coverage is missing the field [%s] referenced in input file %s. This is a required column. It can be automatically added, but it will not be populated with any data.  Would you like to add it anyway?", field, filename );
               int result = Report::WarningMsg( msg, "Warning - missing column", MB_YESNO );

               if ( result == IDYES )
                  {
                  TYPE typeToAdd = (TYPE)(checkCol & (~CC_MUST_EXIST));     // automatically adds column if not present without prompting

                  if ( typeToAdd == TYPE_NULL )
                     typeToAdd = TYPE_LONG;
      
                  int width, decimals;
                  GetTypeParams( typeToAdd, width, decimals );
                  attrs[ i ].checkCol = pLayer->m_pDbTable->AddField( field, typeToAdd, width, decimals, true );
                  }
#endif
               }
            }
         }  // end of: is ( checkCol > 0 )
      }  // end of: for ( i < arraySize

   delete [] foundArray;

   return true;
   }


//bool TiXmlGetAttributes( TiXmlElement *pXml, XML_ATTR attrs[], LPCTSTR filename, MapLayer *pLayer /*=NULL*/ ) 
//   {
//   LPCTSTR element = pXml->Value();
//   return TiXmlGetAttributes( pXml, attrs, element, filename, pLayer );
//   }





