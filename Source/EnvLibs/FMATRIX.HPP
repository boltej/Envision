//-------------------------------------------------------------------
//  FMATRIX.HPP
//
//-- instantiations of float matrix class
//-------------------------------------------------------------------

#pragma once
#include "libs.h"

#include "TMATRIX.HPP"


class LIBSAPI FloatMatrix : public TMatrix< float >
   {
   public:
   typedef float type_data;
      FloatMatrix( UINT _numRows, UINT _numCols)
         : TMatrix< float >( _numRows, _numCols) { }

      FloatMatrix( UINT _numRows, UINT _numCols, float initialValue )
         : TMatrix< float >( _numRows, _numCols, initialValue ) { }

      FloatMatrix( void ) : TMatrix< float >() { }

      FloatMatrix( const type_data *data, UINT _numRows, UINT _numCols)
         : TMatrix< type_data >( data,  _numRows, _numCols ) { }

      //-- multiply a matrix by a vector.  Length of inVector must
      //   be equal to the number of column in the matrix.  
      //   Results are placed in outVector. Length of outVector
      //   must be equal to the number of rows in the matrix
      void Multiply( float *inVector, float *outVector );
      BOOL SolveLU( float *b );  // b must be a vector "rows=cols" long
      void Add(int rows, int cols, float Value);

   };

