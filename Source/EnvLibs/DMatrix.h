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
//-------------------------------------------------------------------
//  FMATRIX.HPP
//
//-- instantiations of double matrix class
//-------------------------------------------------------------------

#pragma once
#include "EnvLibs.h"

#include "TMATRIX.HPP"


class LIBSAPI DoubleMatrix : public TMatrix< double >
   {
   public:
   typedef double type_data;
      DoubleMatrix( UINT _numRows, UINT _numCols)
         : TMatrix< double >( _numRows, _numCols) { }

      DoubleMatrix( UINT _numRows, UINT _numCols, double initialValue )
         : TMatrix< double >( _numRows, _numCols, initialValue ) { }

      DoubleMatrix( void ) : TMatrix< double >() { }

      DoubleMatrix( const type_data *data, UINT _numRows, UINT _numCols)
         : TMatrix< type_data >( data,  _numRows, _numCols ) { }

      //-- multiply a matrix by a vector.  Length of inVector must
      //   be equal to the number of column in the matrix.  
      //   Results are placed in outVector. Length of outVector
      //   must be equal to the number of rows in the matrix
      void Multiply( double *inVector, double *outVector );
      BOOL SolveLU( double *b );  // b must be a vector "rows=cols" long
      void Add(int rows, int cols, double Value);

   };

