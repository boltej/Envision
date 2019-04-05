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
** Data is stored by using three 1D arrays:
**  colNdx - holds the column number for each cell
**    stored in the sparse array
**  values - holds the data for each non-default
**    valued cell in the array
**  rowStartNdx - holds the index of where each sparse
**    array row starts in values and colNdx arrays
**  rowEndNdx - holds the index of a row's last element
**    in values and colNdx arrays
**
** If a row is empty, I store a -1 in rowStartNdx for that
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
** 2011.04.01 - tjs - implementing a rowEndNdx vector for better
**   computational efficiency and simplicity in many of the methods
** 2011.04.02 - tjs - changed name of rowPtr to rowNdx
** 2011.05.16 - tjs - changed certain variables to protected and 
**   made PolyGridLookups a friend.
** 2011.05.19 - tjs - 
**    Split from orginal for retooling to use long long instead
**    of int for indexes.
**    Retooling for large array sizes, making indexes 
**    long long instead of int. Using an #if 1 ... #else ... #endif to 
**    preserve original code
*/

//****************************************************************
//               START OF NEW CODE WITH long long 2011.05.19
//****************************************************************
#pragma once
#include<iostream>
#include<cstdio>
#include <vector>

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
class CompressedRow2DArray {
private:
  long long
    crntAddRow,
    crntAddCol;

  vector<long long> colNdx; // index of column in the uncompreseed array
  vector<long long> rowStartNdx; // index of the first element of uncompressed row[i]
  vector<long long> rowEndNdx; // index of the last element of uncompressed row[i]

  vector<T> values; // the actual data value

  long long GetRowStartNdx(long long elRow);
  long long GetRowEndNdx(long long elRow);

protected:

	friend class PolyGridLookups;

	long long
		numRows,
		numCols,
		allocSz;  // number of elements to allocate at a time

	T 
		defaultValue,
		nullValue; 

public:
  CompressedRow2DArray(
    long long numRows,
    long long numCols,
    long long allocSz,
    T defaultVal,
    T nullValue
  );

  CompressedRow2DArray(FILE *iFile);

  ~CompressedRow2DArray();

  int SetElement(
    long long elRow,
    long long elCol,
    T elVal
  );

  T GetElement(
    long long elRow,
    long long elCol
  );
  
  long long GetColCnt(const long long &row) {
    long long
      strtNdx,
      rtrn = 0;

    strtNdx = GetRowStartNdx(row);
    
    return (strtNdx < 0 ? 0 : GetRowEndNdx(row) - strtNdx + 1);

  }

  T GetRowSum(long long arrRow);
  T GetRowMin(long long arrRow);
  T GetRowMax(long long arrRow);

  long long GetNdxOfRowMin(long long arrRow);
  long long GetNdxOfRowMax(long long arrRow);

  // Create another array with the rows and cols reversed.
  CompressedRow2DArray *Transpose();

  // Replaced by Transpose()
  //CompressedRow2DArray *Invert();

  CompressedRow2DArray *Duplicate();

  long long GetRowNdxs(const long long &row, long long *ndxs);
  long long GetRowValues(const long long &row, T *values);

  void WriteToFile(FILE *oFile);
  void WriteToFile(const char *fName);
  void DisplayArray();
  void DisplayArray(const char *fname);

}; // class CompressedRow2DArray

// Method Definitions

template <class T>
CompressedRow2DArray<T>::CompressedRow2DArray(
  long long numRows,
  long long numCols,
  long long allocSz,
  T defaultValue,
  T nullValue
){

  // for working on array
  this->numRows = numRows;
  this->numCols = numCols;
  this->allocSz = allocSz;
  crntAddRow = -1;
  crntAddCol = -1;

  // for values in array
  this->defaultValue = defaultValue;
  this->nullValue =nullValue;

  // reserve the appropriate number of elements in the vector objects
  colNdx.reserve((size_t)allocSz);
  rowStartNdx.resize((size_t)numRows,-1);
  rowEndNdx.resize((size_t)numRows,-1);
  values.reserve((size_t)allocSz);

} // CompressedRow2DArray::CompressedRow2DArray

template <class T>
CompressedRow2DArray<T>::CompressedRow2DArray(FILE *iFile) {

  // This method is designed to be used in conjuction with the 
  // PolyGridLookUps
  // class. The idea is for a PolyGridLookups object to be able to 
  // pass an open file pointer to this method so that this method
  // can read its data from the same file that the PolyGridLookups
  // constructor is reading for its own data.

  ASSERT(fread((void *)&numRows, 1, sizeof(long long), iFile) == sizeof(long long));

  rowStartNdx.resize(numRows,-1);

  rowEndNdx.resize(numRows,-1);

  ASSERT(fread((void *)&numCols, 1, sizeof(long long), iFile) == sizeof(long long));

  ASSERT(fread((void *)&allocSz, 1, sizeof(long long), iFile) == sizeof(long long));

  ASSERT(fread((void *)&crntAddRow, 1, sizeof(long long), iFile) == sizeof(long long));

  ASSERT(fread((void *)&crntAddCol, 1, sizeof(long long), iFile) == sizeof(long long));

  // jpb = changed from size_t to UINT
  UINT numElements;
  ASSERT(fread((void *)&numElements, 1, sizeof(UINT), iFile) == sizeof(UINT));

  colNdx.resize(numElements, -1);
  values.resize(numElements, nullValue);
  
  // Reading vector class contents for row and col ndx's
  for(unsigned i=0;i<colNdx.size();i++) {
    ASSERT(fread((void *)&colNdx[i], 1, sizeof(long long), iFile) == sizeof(long long));
  }

  for(unsigned i=0;i<rowStartNdx.size();i++) {
    ASSERT(fread((void *)&rowStartNdx[i], 1, sizeof(long long), iFile) == sizeof(long long));
  }

  for(unsigned i=0;i<rowEndNdx.size();i++) {
    ASSERT(fread((void *)&rowEndNdx[i], 1, sizeof(long long), iFile) == sizeof(long long));
  }

  ASSERT(fread((void *)&defaultValue, 1, sizeof(T), iFile) == sizeof(T));

  ASSERT(fread((void *)&nullValue, 1, sizeof(T), iFile) == sizeof(T));

  for(unsigned i=0;i<values.size();i++) {
    ASSERT(fread((void *)&values[i], 1, sizeof(T), iFile) == sizeof(T));
  }

} // CompressedRow2DArray<T>::CompressedRow2DArray(FILE *iFile) {

template <class T>
CompressedRow2DArray<T>::~CompressedRow2DArray() {
} // CompressedRow2DArray::~CompressedRow2DArray() {

template <class T>
int CompressedRow2DArray<T>::SetElement(
  long long elRow, 
  long long elCol, 
  T elVal
) {

	int 
		rtrnVal = 0; // default is error

	// Invalid row or column index
	ASSERT(!(elRow >= numRows || 
		elCol >= numCols ||
		elRow < crntAddRow ||
		(elCol < crntAddCol && elRow <= crntAddRow)));

	 // if current allocated space is too small, allocate
	// more space
	if(values.size() >= values.capacity()) {

		// Max number of elements exceeded
		ASSERT(!(values.capacity() + allocSz > values.max_size() ||
		colNdx.capacity() + allocSz > colNdx.max_size()));
		values.reserve(values.capacity() + allocSz);
		colNdx.reserve(colNdx.capacity() + allocSz);
	 } // if(crntElementNdx >= values.capacity())

	if(elVal != defaultValue) {
		if(elRow > crntAddRow)
			// index where the new row starts
			rowStartNdx[elRow] = (long long) values.size();

		values.push_back(elVal);
		colNdx.push_back(elCol);

		// index where the row ends
		rowEndNdx[elRow] = ((long long)colNdx.size()-1);

		crntAddRow = elRow;
		crntAddCol = elCol;
	}

	rtrnVal = 1;

	return rtrnVal;
} // int CompressedRow2DArray::SetElement(

template <class T>
T CompressedRow2DArray<T>::GetElement(long long elRow, long long elCol) {

	long long
		startRowNdx,
		endRowNdx;
	T
		rtrnVal = defaultValue;

	// Element out of bounds
	ASSERT(!(elRow > numRows || elCol > numCols || elRow < 0 || elCol < 0));
  
	if((startRowNdx = GetRowStartNdx(elRow)) != -1) {
		endRowNdx = GetRowEndNdx(elRow);

		for(long long i=startRowNdx;i<=endRowNdx;i++) {
			if(colNdx[i] == elCol)
				rtrnVal = values[i];
		}

	} // if((startRowNdx = GetRowStartNdx(elRow)) != -1)

  return rtrnVal;

} // unsigned CompressedRow2DArray::GetElement(long long elRow, long long elCol)

template <class T> 
T CompressedRow2DArray<T>::GetRowSum(long long arrRow) {

	long long
		startRowNdx,
		endRowNdx;

	T
		sum = 0;

	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);
 
	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
    
		for(long long i=startRowNdx;i<=endRowNdx;i++) {
			sum += values[i];
		}

		sum += (numRows - (endRowNdx - startRowNdx + 1)) * defaultValue;

	} else {

		sum = numRows * defaultValue;

	} // if((startRowNdx = GetRowStartNdx(elRow)) != -1)

  return sum;

} // unsigned CompressedRow2DArray::GetRowSum(long long elRow)

template <class T> 
T CompressedRow2DArray<T>::GetRowMin(long long arrRow) {

	long long
		startRowNdx,
		endRowNdx;

	T
		min = defaultValue;

	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);
 
	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
    
		for(long long i=startRowNdx;i<=endRowNdx;i++) {
			if(min > values[i])
				min = values[i];
		}

	}

	return min;

} // T CompressedRow2DArray<T>::GetRowMin(long long arrRow)

template <class T> 
T CompressedRow2DArray<T>::GetRowMax(long long arrRow) {

	long long
		startRowNdx,
		endRowNdx;
	T
		max = defaultValue;

  	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);

	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
    
		for(long long i=startRowNdx;i<=endRowNdx;i++) {
			if(max < values[i])
				max = values[i];
		}
	}

  return max;

} // T CompressedRow2DArray<T>::GetRowMax(long long arrRow)

template <class T> 
long long CompressedRow2DArray<T>::GetNdxOfRowMin(long long arrRow) {

  long long
    startRowNdx,
    endRowNdx,
    minNdx = 0;
  T
    min = defaultValue;

    // Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);
 
	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
    
		for(long long i=startRowNdx;i<=endRowNdx;i++) {
			if(min > values[i]) {
				min = values[i];
				minNdx = colNdx[i];
			}
		}

	}

	return minNdx;

} // long long CompressedRow2DArray<T>::GetNdxOfRowMin(long long arrRow)

template <class T> 
long long CompressedRow2DArray<T>::GetNdxOfRowMax(long long arrRow) {

	long long
		startRowNdx,
		endRowNdx,
		maxNdx = 0;
	T
		max = defaultValue;
     
	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);

	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
    
		for(long long i=startRowNdx;i<=endRowNdx;i++) {
			if(max < values[i]) {
				max = values[i];
				maxNdx = colNdx[i];
			}
		}

	}

	return maxNdx;

} // long long CompressedRow2DArray<T>::GetNdxOfRowMax(long long arrRow)

template <class T>
CompressedRow2DArray<T> *CompressedRow2DArray<T>::Duplicate(void) {
  
	CompressedRow2DArray
		*newArray;

	newArray = new CompressedRow2DArray(
		numRows,
		numCols,
		allocSz,
		defaultValue,
		nullValue);

	T
		elVal;

	// Copy the values

	for(long long i=0; i<=crntAddRow; i++) {
		for(long long j=0; j<=crntAddCol; j++) {
			if((elVal = GetElement(i,j)) != defaultValue) {
				newArray->SetElement(i,j,elVal);
			}
		}
	}

	return newArray;
} // CompressedRow2DArray * CompressedRow2DArray::Duplicate(void) {


// Invert() has bee replaced by Transpose(). It is left in here for
// reference and possible debugging testing
#if 0
template <class T>
CompressedRow2DArray<T> *CompressedRow2DArray<T>::Invert(void) {
  
	CompressedRow2DArray
		*newArray;

	newArray = new CompressedRow2DArray(
		numCols,
		numRows,
		allocSz,
		defaultValue,
		nullValue);

	T
		elVal;

	// For each column, get the row values
	// and add them to the inverted array
	// with row and column indexes reversed

	for(long long i=0; i<numCols; i++) { // all the cols
		for(long long j=0; j<=crntAddRow; j++) { // all the rows so far
			if((elVal = GetElement(j,i)) != defaultValue) {
				newArray->SetElement(i,j,elVal);
			}
		}
	}

	return newArray;
} // CompressedRow2DArray * CompressedRow2DArray::Invert(void) {
#endif

template <class T>
CompressedRow2DArray<T> *CompressedRow2DArray<T>::Transpose(void) {
	// An efficient way to create a transpose matrix by directly
	// manipulating the contents of the row, column, and data vectors.
  
	CompressedRow2DArray
		*newArray,
		*oldArray = this;
	
	newArray = new CompressedRow2DArray(
		numCols,
		numRows,
		allocSz,
		defaultValue,
		nullValue);


	// This may be confusing, but here goes: the first step is to
	// get the number of col values for each row in the new array,
	// this is the number of row values in each col of the starting
	// array.

	vector<long long>
		tmpColCounter;

	tmpColCounter.assign(newArray->numRows,0);

	long long maxColNdx = 0;
	for(long long ndx = 0; ndx < oldArray->colNdx.size(); ndx++) {
		tmpColCounter[oldArray->colNdx[ndx]]++;
		if (oldArray->colNdx[ndx] > maxColNdx)
			maxColNdx = oldArray->colNdx[ndx];
		//printf("ndx, value: %3d  %3d\n",ndx,oldArray->colNdx[ndx]);
	}

	// Translate those counts of cols in each row to the
	// starting index and ending index of each row in the new array

	long long lastNdx = 0;
	for(long long ndx = 0; ndx < newArray->numRows; ndx ++) {
		if(tmpColCounter[ndx] == 0) {
			newArray->rowStartNdx[ndx] = -1;
			newArray->rowEndNdx[ndx] = -1;
		} else {
			newArray->rowStartNdx[ndx] = lastNdx;
			newArray->rowEndNdx[ndx] = lastNdx + tmpColCounter[ndx] - 1;
		}
		lastNdx += tmpColCounter[ndx];
	}

	// initialize empty vectors for the values and columns in the
	// newArray

	newArray->colNdx.assign(oldArray->colNdx.size(),-1);
	newArray->values.assign(oldArray->values.size(),nullValue);

	// Now traverse the data in the oldArray and place it in
	// in the newArray

	// A vector to keep track of the offset for each row in
	// the newArray

	vector<long long>
		newArrayRowOffset;
	newArrayRowOffset.assign(newArray->numRows,0);
	long long
		stopColNdx,
		maxColArrNdx = (long long)oldArray->colNdx.size() - 1;

	for(long long oldRowNdx = 0;
		oldRowNdx < oldArray->numRows; 
		oldRowNdx++) 
	{

		// skip an empty row
		if(oldArray->rowStartNdx[oldRowNdx] < 0)
			continue;

		// get the column to stop at for the row
		long long nextRowNdx = oldRowNdx+1;
		while(nextRowNdx < oldArray->numRows) {

			if((stopColNdx = oldArray->rowStartNdx[nextRowNdx]) >= 0)
				break;

			nextRowNdx++;

		} // while(nextRowNdx < oldArray->colNdx.size())
		
		if(nextRowNdx >= oldArray->numRows)
			stopColNdx = (long long)oldArray->colNdx.size();
		
		// drop the elements from the row of the oldArray
		// into the proper place of the newArray
		for(long long oldArrayNdx = oldArray->rowStartNdx[oldRowNdx];
			oldArrayNdx < stopColNdx;
			oldArrayNdx++
		) {
			
			long long newArrayNdx = 
				newArray->rowStartNdx[oldArray->colNdx[oldArrayNdx]] +
				newArrayRowOffset[oldArray->colNdx[oldArrayNdx]];

			newArray->colNdx[newArrayNdx] = oldRowNdx;
			newArray->values[newArrayNdx] = oldArray->values[oldArrayNdx];

			printf("  Dropping: value oldArrayNdx, newArrayNdx, offset: %ld %d %d %d %d\n",
				oldArray->values[oldArrayNdx],
				oldRowNdx,
				oldArrayNdx, 
				newArrayNdx, 
				newArrayRowOffset[oldArray->colNdx[oldArrayNdx]]);

			newArrayRowOffset[oldArray->colNdx[oldArrayNdx]]++;
		} // for(long long oldArrayNdx ...
	} // for(long long oldRowNdx = 0; oldRowNdx < oldArray->numRows; oldRowNdx++)

	// set the crntAddRow and crntAddCol

	newArray->crntAddRow = (long long)newArray->rowStartNdx.size();

	do {
		newArray->crntAddRow--;
	} while(newArray->rowStartNdx[newArray->crntAddRow] == -1 && newArray->crntAddRow != 0);

	// newArray->crntAddCol = newArray->colNdx.back();
	newArray->crntAddCol = newArray->colNdx.size()==0 ? 0 : newArray->colNdx.back();
	if(newArray->crntAddCol == newArray->numCols) {
		newArray->crntAddCol = 0;
		newArray->crntAddRow++;

	}

	return newArray;
} // CompressedRow2DArray * CompressedRow2DArray::Transpose(void) {

template <class T>
long long CompressedRow2DArray<T>::GetRowStartNdx(long long elRow) {
	long long
		rtrnVal = -1;

	if(elRow <= crntAddRow)
		rtrnVal = rowStartNdx[elRow];

	return rtrnVal;
} // CompressedRow2DArray::GetRowStartNdx(long long elRow) {

template<class T>
long long CompressedRow2DArray<T>::GetRowEndNdx(long long elRow) {
	// get the index for the last col in a row

		long long
		rtrnVal = -1;

	if(elRow <= crntAddRow)
		rtrnVal =  rowEndNdx[elRow];

	return rtrnVal;
} // CompressedRow2DArray::GetRowEndNdx(long long elRow) {

/*
template<class T>
int CompressedRow2DArray<T>::GetRowNdxs(const int &arrRow, int *ndxs) {
	// Get the col ndxs for a row

	int
		startRowNdx,
		endRowNdx,
		colCnt = 0;

  	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);
 
	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
		colCnt = endRowNdx - startRowNdx + 1;
    
		for(int i=startRowNdx;i<=endRowNdx;i++)
			ndxs[i-startRowNdx] = colNdx[i];
	}

	return colCnt;
} // int CompressedRow2DArray<T>::GetRowNdxs(const int &row, int *ndxs) {
*/

template<class T>
long long CompressedRow2DArray<T>::GetRowNdxs(const long long &arrRow, long long *ndxs) {
	// Get the col ndxs for a row

	long long
		startRowNdx,
		endRowNdx,
		colCnt = 0;

  	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);
 
	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
		colCnt = endRowNdx - startRowNdx + 1;
    
		for(long long i=startRowNdx;i<=endRowNdx;i++)
			ndxs[i-startRowNdx] = colNdx[i];
	}

	return colCnt;
} // long long CompressedRow2DArray<T>::GetRowNdxs(const long long &row, long long *ndxs) {

template<class T>
long long CompressedRow2DArray<T>::GetRowValues(const long long &arrRow, T *values) {
	long long
		startRowNdx,
		endRowNdx,
		colCnt = 0;

  	// Row out of bounds
	ASSERT(arrRow <= numRows && arrRow >= 0);
 
	if((startRowNdx = GetRowStartNdx(arrRow)) != -1) {
		endRowNdx = GetRowEndNdx(arrRow);
		colCnt = endRowNdx - startRowNdx + 1;
		for(long long i=startRowNdx;i<=endRowNdx;i++)
			values[i-startRowNdx] = this->values[i];
	}
	return colCnt;
} // long long CompressedRow2DArray<T>::GetRowValues(const long long &row, T *values) 

template<class T>
void CompressedRow2DArray<T>::DisplayArray(void) {

	// This only works with a console. Also,
	// For outputting the data, you will have to adjust the
	// output format to fit the data type

	printf("Rows: %ld\n", numRows);
	printf("Cols: %ld\n", numCols);

	printf("CrntAddRow: %ld\n", crntAddRow);
	printf("CrntAddCol: %ld\n", crntAddCol);

	for(long long i=0;i<numRows;i++){
		printf("Row %4ld: ",i);
		for(long long j=0;j<numCols;j++) {
			if(GetElement(i,j) != defaultValue)
				printf("%ld:%ld, ", j, GetElement(i,j));
		}
		printf(" : %ld\n", GetRowSum(i));
	}

	printf("\n");

	printf("ElementValues: \n");
	for(unsigned i=0;i<values.size();i++)
		printf("%ld ",values[i]);

	printf("\n\n Rows: \n");
	for(long long i=0;i<numRows;i++)
		printf("%d ", rowStartNdx[i]);

	printf("\n\nColumns: \n");

	for(unsigned i=0;i<values.size();i++)
		printf("%d ", colNdx[i]);

	printf("\n\n");

} // void CompressedRow2DArray::DisplayArray()

template<class T>
void CompressedRow2DArray<T>::DisplayArray(const char *fname) {
	FILE
		*outFile;
  
	// For outputting the data, you will have to adjust the
	// output format to fit the data type

	outFile = fopen(fname, "w");
	fprintf(outFile, "Rows: %ld\n", numRows);
	fprintf(outFile, "Cols: %ld\n", numCols);

	fprintf(outFile, "crntAddRow: %ld\n", crntAddRow);
	fprintf(outFile, "crntAddCol: %ld\n", crntAddCol);

	fprintf(outFile, "allocSz: %ld\n", allocSz);

	for(long long i=0;i<numRows;i++){
		int tmpSum = GetRowSum(i);
		if(tmpSum != 0) {
			fprintf(outFile, "Row %4ld: ",i);

			for(long long j=0;j<numCols;j++) {
				if(GetElement(i,j) != defaultValue)
					fprintf(outFile, "%ld:%ld, ", j, GetElement(i,j));
			}
			fprintf(outFile, " : %ld\n", tmpSum);
		}
	}

	fprintf(outFile, "\n");

	fprintf(outFile, "ElementValues: \n");
	for(long long i=0;i<values.size();i++) {
		if(i % 10 == 0)
			fprintf(outFile,"\nVals %8d: ",i);
		fprintf(outFile, "%ld ",values[i]);
	}

	fprintf(outFile, "\n\nColumns: \n");
	for(long long i=0;i<values.size();i++) {
		if(i % 10 == 0)
			fprintf(outFile,"\nCols %8d: ",i);
		fprintf(outFile, "%d ", colNdx[i]);	
	}
	
	
	fprintf(outFile, "\n\n Rows: \n");
	for(long long i=0;i<numRows;i++) {
		if(i % 10 == 0)
			fprintf(outFile,"\nRows %8d:",i);
		fprintf(outFile, "%d ", rowStartNdx[i]);
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

	ASSERT(fwrite((void *)&numRows, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&numCols, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&allocSz, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&crntAddRow, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&crntAddCol, 1, sizeof(long long), oFile) == sizeof(long long));

	// size_t --> UINT
	UINT numElements = static_cast<UINT>(colNdx.size());
	ASSERT(fwrite((void *)&numElements, 1, sizeof(UINT), oFile) == sizeof(UINT));
  
	// Writing out the vector class contents for row and col ndx's
	for(unsigned i=0;i<colNdx.size();i++) {
		ASSERT(fwrite((void *)&colNdx[i], 1, sizeof(long long), oFile) == sizeof(long long));
	}

	for(unsigned i=0;i<rowStartNdx.size();i++) {
		ASSERT(fwrite((void *)&rowStartNdx[i], 1, sizeof(long long), oFile) == sizeof(long long));
	}

	for(unsigned i=0;i<rowEndNdx.size();i++) {
		ASSERT(fwrite((void *)&rowEndNdx[i], 1, sizeof(long long), oFile) == sizeof(long long));
	}

	// Writing out values associated with data

	ASSERT(fwrite((void *)&defaultValue, 1, sizeof(T), oFile) == sizeof(T));

	ASSERT(fwrite((void *)&nullValue, 1, sizeof(T), oFile) == sizeof(T));

	for(unsigned i=0;i<values.size();i++) {
		ASSERT(fwrite((void *)&values[i], 1, sizeof(T), oFile) == sizeof(T));
	}

} // CompressedRow2DArray<T>::WriteToFile(FILE *oFile) {

template<class T>
void CompressedRow2DArray<T>::WriteToFile(const char *fName) {
	FILE 
		*oFile;

	ASSERT((oFile = fopen(fName,"wb")) != NULL);

	ASSERT(fwrite((void *)&numRows, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&numCols, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&allocSz, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&crntAddRow, 1, sizeof(long long), oFile) == sizeof(long long));

	ASSERT(fwrite((void *)&crntAddCol, 1, sizeof(long long), oFile) == sizeof(long long));

	// size_t --> UINT
	UINT numElements = static_cast<UINT>(colNdx.size());
	ASSERT(fwrite((void *)&numElements, 1, sizeof(UINT), oFile) == sizeof(UINT));
  
	// Writing out the vector class contents for row and col ndx's
	for(unsigned i=0;i<colNdx.size();i++) {
		ASSERT(fwrite((void *)&colNdx[i], 1, sizeof(long long), oFile) == sizeof(long long));
	}

	//ASSERT(0);
	for(unsigned i=0;i<rowStartNdx.size();i++) {
		ASSERT(fwrite((void *)&rowStartNdx[i], 1, sizeof(long long), oFile) == sizeof(long long));
	}

	// Writing out values associated with data

	ASSERT(fwrite((void *)&defaultValue, 1, sizeof(T), oFile) == sizeof(T));

	ASSERT(fwrite((void *)&nullValue, 1, sizeof(T), oFile) == sizeof(T));

	for(unsigned i=0;i<values.size();i++) {
		ASSERT(fwrite((void *)&values[i], 1, sizeof(T), oFile) == sizeof(T));
	}

	fclose(oFile);

} // CompressedRow2DArray<T>::WriteToFile(const char *fName) {

