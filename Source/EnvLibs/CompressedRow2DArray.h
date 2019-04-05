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
/* CompressedRow2DArray.cpp
** Author: Tim Sheehan
** Creation Date: 2009.05.13
**
** This is an implementation of a compressed row array.
** basically, the idea is store a sparse array by having
** each row hold only the non-default value of the array
** cells.
**
** To build this array, cells can only be added in row/col
** sequential order.
**
** Data is stored by using four 1D arrays:
**  m_colNdx - holds the column number for each cell
**    stored in the sparse array
**  m_values - holds the data for each non-default
**    valued cell in the array
**  m_rowStartNdx - holds the index of where each sparse
**    array row starts in m_values and m_colNdx arrays
**  m_rowEndNdx - holds the index of a row's last element
**    in m_values and m_colNdx arrays
**
** If a row is empty, I store a -1 in m_rowStartNdx for that
** row.
**
** History
**   2009.05.13 - tjs - Wrote code, got to compile
**   2009.05.14 - tjs - Abandoning at the moment as
**     such a complex lookup may not be needed.
**   2009.06.29 - tjs - Implemented as templated class
**   2009.07.04 - tjs - Implemented and tested as suitable
** class for use with grid/poly poly/grid lookups in flammap envision
** extension. Changed error output from console based to messages.
**
**   2011.03.22 - tjs - changed anything with an exit() to an ASSERT,
** cleaned up indentation
**
** 2011.03.28 - tjs - impelemented and tested efficient transpose
** 2011.03.29 - tjs - added header block to change between MFC and
**   32 bit console versions
** 2011.04.01 - tjs - implementing a m_rowEndNdx vector for better
**   computational efficiency and simplicity in many of the methods
** 2011.04.02 - tjs - changed name of rowPtr to rowNdx
*/

#pragma once
#include "EnvLibs.h"

#include<iostream>
#include<cstdio>
#include <vector>

#include <atlstr.h>


// vvvvvvvvvvvvv Changes between console and MFC apps vvvvvvvvvvv

// This should be enabled for 32 bit console app
// and disabled for MFC version (that used with Envision)

#if 0
#include<ASSERT.h>
#define ASSERT assert
#define UINT size_t
#endif

// ^^^^^^^^^^^^^^ Changes between consloe and MFC apps ^^^^^^^^^^^^^

using namespace std;


template <class T>
class CompressedRow2DArray 
{
private:  
  int m_numRows;
  int m_numCols;
  int m_allocSz; // number of elements to allocate at a time
  int m_crntAddRow;
  int m_crntAddCol;

  T m_defaultValue;
  T m_nullValue; 

  int GetRowStartNdx(int elRow);
  int GetRowEndNdx(int elRow);

public:
   int m_rowColIndex;

   vector<int> m_colNdx; // index of column in the uncompreseed array
   vector<int> m_rowStartNdx; // index of the first element of uncompressed row[i]
   vector<int> m_rowEndNdx; // index of the last element of uncompressed row[i]
   vector<T> m_values; // the actual data value

  CompressedRow2DArray(int numRows,int numCols,int allocSz,T defaultVal,T m_nullValue);
  CompressedRow2DArray(FILE *iFile);
  ~CompressedRow2DArray();

  int SetElement(int elRow,int elCol,T elVal);
  T GetElement(int elRow,int elCol);
  void SetCrntRow(int elRow);
  void SetCrntCol(int elCol);

  int GetColCnt(const int &row) 
      {
      int strtNdx = GetRowStartNdx(row);
      return (strtNdx < 0 ? 0 : GetRowEndNdx(row) - strtNdx + 1);
      }

  T GetRowSum(int arrRow);
  T GetRowMin(int arrRow);
  T GetRowMax(int arrRow);

  int GetNdxOfRowMin(int arrRow);
  int GetNdxOfRowMax(int arrRow);

  // Create another array with the rows and cols reversed.
  CompressedRow2DArray *Transpose();
  CompressedRow2DArray *Duplicate();

  int GetRowNdxs(const int &row, int *ndxs);
  int GetRowValues(const int &row, T *m_values);

  void WriteToFile(FILE *oFile);
  void WriteToFile(const char *fName);
  void ToString( CString&, LPCTSTR format );
  void DisplayArray(const char *fname);

}; // class CompressedRow2DArray

// Method Definitions

template <class T>
CompressedRow2DArray<T>::CompressedRow2DArray( int _numRows, int _numCols, int _allocSz, T _defaultValue, T _nullValue )
: m_numRows( _numRows)
, m_numCols( _numCols )
, m_allocSz( _allocSz )
, m_defaultValue( _defaultValue )
, m_nullValue( _nullValue )
, m_rowColIndex( 0 )
, m_crntAddRow( -1 )
, m_crntAddCol( -1 )
   {
   // reserve the appropriate number of elements in the vector objects
   m_colNdx.reserve((size_t) m_allocSz);
   m_rowStartNdx.resize((size_t) m_numRows, -1);
   m_rowEndNdx.resize((size_t) m_numRows, -1);
   m_values.reserve((size_t) m_allocSz );
   }


template <class T>
CompressedRow2DArray<T>::CompressedRow2DArray(FILE *iFile) 
: m_numRows( -1)
, m_numCols( -1 )
, m_allocSz( -1 )
, m_defaultValue( (T) 0 )
, m_nullValue( (T) 0 )
, m_rowColIndex( 0 )
, m_crntAddRow( -1 )
, m_crntAddCol( -1 )
   {
   // This method is designed to be used in conjuction with the 
   // PolyGridLookUps
   // class. The idea is for a PolyGridLookups object to be able to 
   // pass an open file pointer to this method so that this method
   // can read its data from the same file that the PolyGridLookups
   // constructor is reading for its own data.

   fread((void*)& m_numRows, sizeof(int), 1, iFile);

   m_rowStartNdx.resize( m_numRows,-1);

   m_rowEndNdx.resize(m_numRows,-1);

   fread((void*) &m_numCols, sizeof(int), 1, iFile);
   fread((void*) &m_allocSz, sizeof(int), 1, iFile);
   fread((void*) &m_crntAddRow, sizeof(int), 1, iFile);
   fread((void*) &m_crntAddCol, sizeof(int), 1, iFile);

   // jpb = changed from size_t to UINT
   UINT numElements;
   fread((void *) &numElements, sizeof(UINT), 1, iFile);

   m_colNdx.resize( numElements, -1);
   m_values.resize( numElements, m_nullValue);
   
   // Reading vector class contents for row and col ndx's
   for(unsigned i=0; i < m_colNdx.size(); i++)
      fread((void*) &m_colNdx[i], sizeof(int), 1, iFile );

   for(unsigned i=0; i < m_rowStartNdx.size(); i++)
      fread((void*) &m_rowStartNdx[i], sizeof(int), 1, iFile );

   for(unsigned i=0; i < m_rowEndNdx.size(); i++) 
      fread((void *)&m_rowEndNdx[i], sizeof(int), 1, iFile);

   fread((void*) &m_defaultValue, sizeof(T), 1, iFile);

   fread((void*) &m_nullValue, sizeof(T), 1, iFile);

   for(unsigned i=0;i<m_values.size();i++) 
      fread((void *)&m_values[i], sizeof(T), 1, iFile);
   }


template <class T> CompressedRow2DArray<T>::~CompressedRow2DArray() { }
template <class T> void CompressedRow2DArray<T>::SetCrntRow(int elRow) { m_crntAddRow = elRow; }
template <class T> void CompressedRow2DArray<T>::SetCrntCol(int elCol) { m_crntAddCol = elCol; }

template <class T>
int CompressedRow2DArray<T>::SetElement( int elRow, int elCol, T elVal )
   {

   //// Invalid row or column index
   //ASSERT(!(elRow >= m_numRows || 
   //   elCol >= m_numCols ||
   //   elRow < m_crntAddRow ||
   //   (elCol < m_crntAddCol && elRow <= m_crntAddRow)));

    // if current allocated space is too small, allocate
   // more space
   if (m_values.size() >= m_values.capacity()) 
      {
      // Max number of elements exceeded
      ASSERT(!(m_values.capacity() + m_allocSz > m_values.max_size() ||
      m_colNdx.capacity() + m_allocSz > m_colNdx.max_size()));
      m_values.reserve(m_values.capacity() + m_allocSz);
      m_colNdx.reserve(m_colNdx.capacity() + m_allocSz);
      } // if(crntElementNdx >= m_values.capacity())

   if (elVal != m_defaultValue) 
      {
      if(elRow > m_crntAddRow)
         // index where the new row starts
         m_rowStartNdx[elRow] = (int) m_values.size();

      m_values.push_back(elVal);
      m_colNdx.push_back(elCol);
      
      // index where the row ends
      m_rowEndNdx[elRow] = ((int)m_colNdx.size()-1);

      m_crntAddRow = elRow;
      m_crntAddCol = elCol;
      }

   return 1;
   }


template <class T>
T CompressedRow2DArray<T>::GetElement(int elRow, int elCol) 
   {
   int startRowNdx;
   int endRowNdx;
   T rtrnVal = m_defaultValue;

   // Element out of bounds
   ASSERT(!(elRow > m_numRows || elCol > m_numCols || elRow < 0 || elCol < 0));
  
   if((startRowNdx = GetRowStartNdx(elRow)) != -1) 
      {
      endRowNdx = GetRowEndNdx(elRow);

      for(int i=startRowNdx;i<=endRowNdx;i++) 
         {
         if(m_colNdx[i] == elCol)
            rtrnVal = m_values[i];
         }

      } // if((startRowNdx = GetRowStartNdx(elRow)) != -1)

   return rtrnVal;
   } // unsigned CompressedRow2DArray::GetElement(int elRow, int elCol)


template <class T> 
T CompressedRow2DArray<T>::GetRowSum(int arrRow) 
   {
   int startRowNdx;
   int endRowNdx;

   T sum = 0;

   // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);
 
   if((startRowNdx = GetRowStartNdx(arrRow)) != -1) 
      {
      endRowNdx = GetRowEndNdx(arrRow);
    
      for(int i=startRowNdx;i<=endRowNdx;i++) 
         sum += m_values[i];

      sum += (m_numRows - (endRowNdx - startRowNdx + 1)) * m_defaultValue;
      } 
   else
      sum = m_numRows * m_defaultValue;

   return sum;
   }


template <class T> 
T CompressedRow2DArray<T>::GetRowMin(int arrRow) 
   {
   int startRowNdx;
   int endRowNdx;

   T min = m_defaultValue;

   // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);
 
   if((startRowNdx = GetRowStartNdx(arrRow)) != -1) 
      {
      endRowNdx = GetRowEndNdx(arrRow);
    
      for(int i=startRowNdx;i<=endRowNdx;i++) 
         {
         if(min > m_values[i])
            min = m_values[i];
         }
      }
   return min;
   }

template <class T> 
T CompressedRow2DArray<T>::GetRowMax(int arrRow) {

   int
      startRowNdx,
      endRowNdx;
   T
      max = m_defaultValue;

     // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);

   if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
      endRowNdx = GetRowEndNdx(arrRow);
    
      for(int i=startRowNdx;i<=endRowNdx;i++) {
         if(max < m_values[i])
            max = m_values[i];
      }
   }

  return max;

} // T CompressedRow2DArray<T>::GetRowMax(int arrRow)

template <class T> 
int CompressedRow2DArray<T>::GetNdxOfRowMin(int arrRow) {

  int
    startRowNdx,
    endRowNdx,
    minNdx = 0;
  T
    min = m_defaultValue;

    // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);
 
   if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
      endRowNdx = GetRowEndNdx(arrRow);
    
      for(int i=startRowNdx;i<=endRowNdx;i++) {
         if(min > m_values[i]) {
            min = m_values[i];
            minNdx = m_colNdx[i];
         }
      }

   }

   return minNdx;

} // int CompressedRow2DArray<T>::GetNdxOfRowMin(int arrRow)

template <class T> 
int CompressedRow2DArray<T>::GetNdxOfRowMax(int arrRow) {

   int
      startRowNdx,
      endRowNdx,
      maxNdx = 0;
   T
      max = m_defaultValue;
     
   // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);

   if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
      endRowNdx = GetRowEndNdx(arrRow);
    
      for(int i=startRowNdx;i<=endRowNdx;i++) {
         if(max < m_values[i]) {
            max = m_values[i];
            maxNdx = m_colNdx[i];
         }
      }

   }

   return maxNdx;

} // int CompressedRow2DArray<T>::GetNdxOfRowMax(int arrRow)

template <class T>
CompressedRow2DArray<T> *CompressedRow2DArray<T>::Duplicate(void) {
  
   CompressedRow2DArray
      *newArray;

   newArray = new CompressedRow2DArray(
      m_numRows,
      m_numCols,
      m_allocSz,
      m_defaultValue,
      m_nullValue);

   T
      elVal;

   // Copy the m_values

   for(int i=0; i<=m_crntAddRow; i++) {
      for(int j=0; j<=m_crntAddCol; j++) {
         if((elVal = GetElement(i,j)) != m_defaultValue) {
            newArray->SetElement(i,j,elVal);
         }
      }
   }

   return newArray;
} // CompressedRow2DArray * CompressedRow2DArray::Duplicate(void) {

template <class T>
CompressedRow2DArray<T> *CompressedRow2DArray<T>::Transpose(void) 
   {
   // An efficient way to create a transpose matrix by directly
   // manipulating the contents of the row, column, and data vectors.
  
   CompressedRow2DArray *newArray = NULL;
   CompressedRow2DArray *oldArray = this;
   
   newArray = new CompressedRow2DArray( m_numCols, m_numRows, m_allocSz, m_defaultValue, m_nullValue);

	// No need to transpose an empty array
	if (m_values.size() == 0 )
		return newArray;

   // This may be confusing, but here goes: the first step is to
   // get the number of col m_values for each row in the new array,
   // this is the number of row m_values in each col of the starting
   // array.

   vector<int> tmpColCounter;
   tmpColCounter.assign(newArray->m_numRows, 0);

   int maxColNdx = 0;
   for (int ndx = 0; ndx < (int) oldArray->m_colNdx.size(); ndx++) 
      {
      tmpColCounter[oldArray->m_colNdx[ndx]]++;
      if (oldArray->m_colNdx[ndx] > maxColNdx)
         maxColNdx = oldArray->m_colNdx[ndx];
      //printf("ndx, value: %3d  %3d\n",ndx,oldArray->m_colNdx[ndx]);
      }

   // Translate those counts of cols in each row to the
   // starting index and ending index of each row in the new array

   int lastNdx = 0;
   for(int ndx = 0; ndx < newArray->m_numRows; ndx ++) 
      {
      if ( tmpColCounter[ndx] == 0 ) 
         {
         newArray->m_rowStartNdx[ndx] = -1;
         newArray->m_rowEndNdx[ndx] = -1;
         }
      else 
         {
         newArray->m_rowStartNdx[ndx] = lastNdx;
         newArray->m_rowEndNdx[ndx] = lastNdx + tmpColCounter[ndx] - 1;
         }

      lastNdx += tmpColCounter[ndx];
      }

   // initialize empty vectors for the m_values and columns in the
   // newArray

   newArray->m_colNdx.assign(oldArray->m_colNdx.size(),-1);
   newArray->m_values.assign(oldArray->m_values.size(),m_nullValue);

   // Now traverse the data in the oldArray and place it in
   // in the newArray

   // A vector to keep track of the offset for each row in
   // the newArray
   vector<int> newArrayRowOffset;
   newArrayRowOffset.assign(newArray->m_numRows,0);

   int stopColNdx = -1;
   int maxColArrNdx = (int)oldArray->m_colNdx.size() - 1;

   for (int oldRowNdx = 0; oldRowNdx < oldArray->m_numRows; oldRowNdx++) 
      {
      // skip an empty row
      if( oldArray->m_rowStartNdx[oldRowNdx] < 0 )
         continue;

      // get the column to stop at for the row
      int nextRowNdx = oldRowNdx+1;
      while(nextRowNdx < oldArray->m_numRows) 
         {
         if ( (stopColNdx = oldArray->m_rowStartNdx[nextRowNdx]) >= 0 )
            break;

         nextRowNdx++;
         } // end of: while(nextRowNdx < oldArray->m_colNdx.size())
      
      if ( nextRowNdx >= oldArray->m_numRows )
         stopColNdx = (int)oldArray->m_colNdx.size();
      
      // drop the elements from the row of the oldArray
      // into the proper place of the newArray
      for(int oldArrayNdx = oldArray->m_rowStartNdx[oldRowNdx]; oldArrayNdx < stopColNdx; oldArrayNdx++) 
         {         
         int newArrayNdx = 
            newArray->m_rowStartNdx[oldArray->m_colNdx[oldArrayNdx]] +
            newArrayRowOffset[oldArray->m_colNdx[oldArrayNdx]];

         newArray->m_colNdx[newArrayNdx] = oldRowNdx;
         newArray->m_values[newArrayNdx] = oldArray->m_values[oldArrayNdx];

         //printf("  Dropping: value oldArrayNdx, newArrayNdx, offset: %ld %d %d %d %d\n",
         //   oldArray->m_values[oldArrayNdx],
         //   oldRowNdx,
         //   oldArrayNdx, 
         //   newArrayNdx, 
         //   newArrayRowOffset[oldArray->m_colNdx[oldArrayNdx]]);

         newArrayRowOffset[oldArray->m_colNdx[oldArrayNdx]]++;
         } // end of: for(int oldArrayNdx ...
      } // end of: for(int oldRowNdx = 0; oldRowNdx < oldArray->m_numRows; oldRowNdx++)

   // set the m_crntAddRow and m_crntAddCol
   newArray->m_crntAddRow = (int)newArray->m_rowStartNdx.size();

   do 
      {
      newArray->m_crntAddRow--;
      } while(newArray->m_rowStartNdx[newArray->m_crntAddRow] == -1 && newArray->m_crntAddRow != 0);

   newArray->m_crntAddCol = newArray->m_colNdx.back();
   if(newArray->m_crntAddCol == newArray->m_numCols) 
      {
      newArray->m_crntAddCol = 0;
      newArray->m_crntAddRow++;
      }

   return newArray;
   } // CompressedRow2DArray * CompressedRow2DArray::Transpose(void) {


template <class T>
int CompressedRow2DArray<T>::GetRowStartNdx(int elRow) 
   {
   int rtrnVal = -1;

   if(elRow <= m_crntAddRow)
      rtrnVal = m_rowStartNdx[elRow];

   return rtrnVal;
   }


template<class T>
int CompressedRow2DArray<T>::GetRowEndNdx(int elRow) 
   {
   // get the index for the last col in a row

   int rtrnVal = -1;

   if(elRow <= m_crntAddRow)
      rtrnVal =  m_rowEndNdx[elRow];

   return rtrnVal;
   }


template<class T>
int CompressedRow2DArray<T>::GetRowNdxs(const int &arrRow, int *ndxs) 
   {
   // Get the col ndxs for a row
   int colCnt = 0;

     // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);
 
   int startRowNdx = GetRowStartNdx(arrRow);
   
   if (startRowNdx != -1) 
      {
      int endRowNdx = GetRowEndNdx(arrRow);
      colCnt = endRowNdx - startRowNdx + 1;
    
      for(int i=startRowNdx;i<=endRowNdx;i++) 
         ndxs[i-startRowNdx] = m_colNdx[i];
      }

   return colCnt;
   } // int CompressedRow2DArray<T>::GetRowNdxs(const int &row, int *ndxs) {


template<class T>
int CompressedRow2DArray<T>::GetRowValues(const int &arrRow, T *m_values) 
   {
   int startRowNdx = -1;
   int endRowNdx = -1;
   int colCnt = 0;

     // Row out of bounds
   ASSERT(arrRow <= m_numRows && arrRow >= 0);
 
   if((startRowNdx = GetRowStartNdx(arrRow)) != -1) 
      {
      endRowNdx = GetRowEndNdx(arrRow);
      colCnt = endRowNdx - startRowNdx + 1;
      for(int i=startRowNdx;i<=endRowNdx;i++)
         m_values[i-startRowNdx] = this->m_values[i];
      }
   return colCnt;
   }


template<class T>
void CompressedRow2DArray<T>::ToString( CString &str, LPCTSTR format  )
   {
   // For outputting the data, you will have to adjust the
   // output format to fit the data type
   str.Empty();

   CString tmp;
   tmp.Format( "Rows: %i, Cols: %i; ", m_numRows, m_numCols);
   str.Append( tmp );

   tmp.Format("CrntAddRow: %i, CrntAddCol: %i\r\n", m_crntAddRow, m_crntAddCol);
   str.Append( tmp );

   /*for(int i=0; i < m_numRows; i++)
      {
      CString elemStr;
      int count = 0;
      for( int j=0; j < m_numCols; j++) 
         {
         if ( GetElement(i,j) != m_defaultValue )
            {
            tmp.Format( "%i:", j );
            elemStr.Append( tmp );
            tmp.Format( format, GetElement(i,j) );
            elemStr.Append( tmp );
            elemStr.Append( ",");
            count++;
            }
         }

      if ( count > 0 )
         {
         tmp.Format( "Row %5i: ", i );
         str.Append( tmp );
         str.Append( elemStr );
         str.Append( " : " );
         tmp.Format( format, GetRowSum( i ) );
         str.Append( "\r\n");
         }
      }*/
   
   str.Append( "\r\n Row Start Index: \r\n");
   for(int i=0;i<m_numRows;i++)
      {
      tmp.Format( "%d ", m_rowStartNdx[i] );
      str.Append( tmp );
      }

   str.Append("\r\n Row End Index: \r\n");
   for (int i = 0; i<m_numRows; i++)
      {
	   tmp.Format("%d ", m_rowEndNdx[i]);
	   str.Append(tmp);
      }

   str.Append( "\r\nColumns: \r\n");
   for(unsigned i=0;i<m_values.size();i++)
      {
      tmp.Format("%d ", m_colNdx[i]);
      str.Append( tmp );
      }

   str.Append("\r\n");
   str.Append("ElementValues: \r\n");
   for (unsigned i = 0; i<m_values.size(); i++)
      {
	   tmp.Format(format, m_values[i]);
	   str.Append(tmp);
	   str.Append(" ");
      }
   }

template<class T>
void CompressedRow2DArray<T>::DisplayArray(const char *fname) 
   {
   FILE *outFile = NULL;
  
   // For outputting the data, you will have to adjust the
   // output format to fit the data type
   int errNo = fopen_s( &outFile, fname, "w" );

   //outFile = fopen(fname, "w");
   fprintf(outFile, "Rows: %ld\n", m_numRows);
   fprintf(outFile, "Cols: %ld\n", m_numCols);

   fprintf(outFile, "m_crntAddRow: %ld\n", m_crntAddRow);
   fprintf(outFile, "m_crntAddCol: %ld\n", m_crntAddCol);

   fprintf(outFile, "m_allocSz: %ld\n", m_allocSz);

   for(int i=0;i<m_numRows;i++){
      int tmpSum = GetRowSum(i);
      if(tmpSum != 0) {
         fprintf(outFile, "Row %4ld: ",i);

         for(int j=0;j<m_numCols;j++) {
            if(GetElement(i,j) != m_defaultValue)
               fprintf(outFile, "%ld:%ld, ", j, GetElement(i,j));
         }
         fprintf(outFile, " : %ld\n", tmpSum);
      }
   }

   fprintf(outFile, "\n");

   fprintf(outFile, "ElementValues: \n");
   for(unsigned i=0;i<m_values.size();i++) {
      if(i % 10 == 0)
         fprintf(outFile,"\nVals %8d: ",i);
      fprintf(outFile, "%ld ",m_values[i]);
   }

   fprintf(outFile, "\n\nColumns: \n");
   for(unsigned i=0;i<m_values.size();i++) {
      if(i % 10 == 0)
         fprintf(outFile,"\nCols %8d: ",i);
      fprintf(outFile, "%d ", m_colNdx[i]);   
   }
   
   
   fprintf(outFile, "\n\n Rows: \n");
   for(int i=0;i<m_numRows;i++) {
      if(i % 10 == 0)
         fprintf(outFile,"\nRows %8d:",i);
      fprintf(outFile, "%d ", m_rowStartNdx[i]);
   }

   fprintf(outFile,"\n\n");
   fclose(outFile);
} // void CompressedRow2DArray::DisplayArray 

template <class T>
void CompressedRow2DArray<T>::WriteToFile(FILE *oFile) {

   // This method is designed to be used in conjuction with the PolyGridLookUps
   // class. The idea is for a PolyGridLookups object to be able to 
   // pass an open file pointer to this method so that this method
   // can write its data to the same file that the PolyGridLookups
   // WriteToFile method is writing its own data to.

	fwrite((void*)&m_numRows,sizeof(int), 1, oFile);
	fwrite((void*)&m_numCols, sizeof(int), 1, oFile);
	fwrite((void*)&m_allocSz, sizeof(int), 1, oFile);
	fwrite((void*)&m_crntAddRow, sizeof(int), 1, oFile);
	fwrite((void*)&m_crntAddCol, sizeof(int), 1, oFile);

   // size_t --> UINT
   UINT numElements = static_cast<UINT>(m_colNdx.size());
   fwrite((void *)&numElements, sizeof(UINT), 1, oFile);
  
   // Writing out the vector class contents for row and col ndx's
   for(unsigned i=0;i<m_colNdx.size();i++) 
      fwrite((void *)&m_colNdx[i], sizeof(int), 1, oFile);

   for(unsigned i=0;i<m_rowStartNdx.size();i++)
      fwrite((void *)&m_rowStartNdx[i], sizeof(int), 1, oFile);

   for(unsigned i=0;i<m_rowEndNdx.size();i++)
      fwrite((void *)&m_rowEndNdx[i], sizeof(int), 1, oFile);

   // Writing out m_values associated with data
   fwrite((void *)&m_defaultValue, sizeof(T), 1,  oFile);
   fwrite((void *)&m_nullValue, sizeof(T), 1, oFile);

   for(unsigned i=0;i<m_values.size();i++) 
      fwrite((void *)&m_values[i], sizeof(T), 1, oFile);

   } // CompressedRow2DArray<T>::WriteToFile(FILE *oFile) {

template<class T>
void CompressedRow2DArray<T>::WriteToFile(const char *fName)
   {
   FILE *oFile = fopen(fName,"wb");

   fwrite((void *)&m_numRows, sizeof(int), 1, oFile);
   fwrite((void *)&m_numCols, sizeof(int), 1, oFile);
   fwrite((void *)&m_allocSz, sizeof(int), 1, oFile);
   fwrite((void *)&m_crntAddRow, sizeof(int), 1, oFile);
   fwrite((void *)&m_crntAddCol, sizeof(int), 1, oFile);

   // size_t --> UINT
   UINT numElements = static_cast<UINT>(m_colNdx.size());
   fwrite((void *)&numElements, sizeof(UINT), 1, oFile);
  
   // Writing out the vector class contents for row and col ndx's
   for(unsigned i=0;i<m_colNdx.size();i++)
      fwrite((void *)&m_colNdx[i], sizeof(int), 1, oFile);

   //ASSERT(0);
   for(unsigned i=0;i<m_rowStartNdx.size();i++)
      fwrite((void *)&m_rowStartNdx[i], sizeof(int), 1, oFile);

   // Writing out m_values associated with data

   fwrite((void*) &m_defaultValue, sizeof(T), 1, oFile);
   fwrite((void*) &m_nullValue, sizeof(T), 1, oFile);

   for(unsigned i=0;i<m_values.size();i++)
      fwrite((void *)&m_values[i], sizeof(T), 1, oFile) == sizeof(T);

   fclose(oFile);
   } // CompressedRow2DArray<T>::WriteToFile(const char *fName) {
