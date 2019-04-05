//-------------------------------------------------------------------
//  IMATRIX.HPP
//
//-- instantiations of float matrix class
//-------------------------------------------------------------------
#pragma once
#include "libs.h"


#if !defined _TMATRIX_HPP
#include "TMATRIX.HPP"
#endif


class IntMatrix : public TMatrix< int >
   {
   public:
      IntMatrix( UINT _numRows, UINT _numCols )
         : TMatrix< int >( _numRows, _numCols) { }

      IntMatrix( UINT _numRows, UINT _numCols, int initialValue )
         : TMatrix< int >( _numRows, _numCols, initialValue ) { }

      IntMatrix( void )
         : TMatrix< int >() { }

      //-- multiply a matrix by a vector.  Length of inVector must
      //   be equal to the number of column in the matrix.  
      //   Results are placed in outVector. Length of outVector
      //   must be equal to the number of rows in the matrix
      //void Multiply( int *inVector, int *outVector );
      //BOOL SolveLU( int *b );  // b must be a vector "rows=cols" long

   };

