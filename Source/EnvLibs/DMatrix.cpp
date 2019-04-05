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

#include "Report.h"
#include "DMatrix.h"

#include <math.h>


BOOL LUDecomp( double **a, int n, int *index, double *d );
void LUBkSub( double **a, int n, int *index, double b[] );


void DoubleMatrix::Multiply( double *inVector, double *outVector )
   {
   int rows = GetRows();
   int cols = GetCols();

   for ( int i=0; i < rows; i++ )
      {
      outVector[ i ] = 0;

      for ( int j=0; j < cols; j++ )
         outVector[ i ] += Get( i, j ) * inVector[ j ];
      }
   }



void DoubleMatrix::Add(int row, int col, double value)
   {
   int rows = GetRows();
   int cols = GetCols();
   ASSERT( row < rows );
   ASSERT( col < cols );

   Set( row, col, value + Get( row, col ) );
   }



BOOL DoubleMatrix::SolveLU( double *b )
   {
   //-- note:  this routine trashes the original matrix --//
   //   b has the original RHS values of A x = b, and is 
   //   returned containing the solved x vector
   int rows = GetRows();
   int cols = GetCols();

   if ( rows != cols )
      {
      Report::ErrorMsg( "Solver requires that the coefficient matrix be square!" );
      return FALSE;
      }

   double **a = GetBase();
   double d;
   int *index = new int[ rows ];

   BOOL ok = LUDecomp( a, rows, index, &d );

   if ( ok == FALSE )
      {
      Report::ErrorMsg( "Error in Solver: Coefficient matrix was singular!" );
      return FALSE;
      }

   //-- successful decomp, now run back substitution --//
   LUBkSub( a, rows, index, b );

   delete [] index;
   return TRUE;
   }


const double TINY = 0.0;

BOOL LUDecomp( double **a, int n, int *index, double *d )
   {
   //-- n is the size of the (square) matrix --//
   //   d is returned as an output variable
   int iMax;
   double big, dum, sum, temp;
   double *vv;     // stores implicit scaling of each row

   vv = new double[ n ];
   *d = 1.0;

   for ( int i=0; i < n; i++ )
      {
      big = 0.0;

      for ( int j=0; j < n; j++ )
         if ( ( temp= (double) fabs( a[i][j] ) ) > big )
            big = temp;

      if ( big == 0.0 )
         return 0;     // matrix was singular

      vv[ i ] = (double) 1.0/big;   // save the scaling
      }  // end of: for( i < n )

   for ( int j=0; j < n; j++ )   // loop over colums of Crout's method
      {
      for ( int i=0; i < j; i++ )
         {
         sum = a[ i ][ j ];
         for ( int k=0; k < i; k++ )
            sum -= a[ i ][ k ] * a[ k ][ j ];
         a[ i ][ j ]= sum;
         }

      big = 0.0;

      for ( int i=j; i < n; i++ )
         {
         sum = a[ i ][ j ];
         for ( int k=0; k < j; k++ )
            sum -= a[ i ][ k ] * a[ k ][ j ];
         a[ i ][ j ] = sum;

         if ( (dum=vv[ i ]*(double) fabs( sum ) ) >= big )
            {
            big = dum;
            iMax = i;
            }
         }  // end of:  for ( i < n )

      if ( j != iMax )
         {
         for ( int k=0; k < n; k++ )
            {
            dum = a[ iMax ][ k ];
            a[ iMax ][ k ] = a[ j ][ k ];
            a[ j ][ k ] = dum;
            }

         *d = -(*d);
         vv[ iMax ] = vv[ j ];
         }

      index[ j ] = iMax;
      if ( a[ j ][ j ] == 0.0 )
         a[ j ][ j ] = TINY;

      if ( j != n )
         {
         dum = (double) 1.0/( a[ j ][ j ] );
         for ( int i=j+1; i < n; i++ )
            a[ i ][ j ] *= dum;
         }
      }

   delete [] vv;
   return 1;
   }


void LUBkSub( double **a, int n, int *index, double b[] )
   {
   int ii=-1, ip, i, j;
   double sum;
   for ( i=0; i < n; i++ )
      {
      ip = index[ i ];
      sum = b[ ip ];
      b[ ip ] = b[ i ];
      if ( ii != -1 )
         for ( j=ii; j <= i-1; j++ )
            sum -= a[ i ][ j ] * b[ j ];
      else if ( sum )
         ii = i;
      b[ i ] = sum;
      }

   for ( i=n-1; i >= 0; i-- )
      {
      sum = b[ i ];
      for ( j=i+1; j < n; j++ )
         sum -= a[ i ][ j ] * b[ j ];
      b[ i ] = sum/a[ i ][ i ];
      }
   }

