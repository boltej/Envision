// AlgLib.cpp : Defines the entry point for the console application.
//

#include <EnvLibs.h>
#pragma hdrstop

//#include <stdlib.h>
//#include <stdio.h>
//#include <math.h>

#include "AlgLib.h"

//using namespace alglib;

//int RunSpline3dTrilinear( void );
//int RunSpline3dVector( void );


//void ALRadialBasisFunction3D::Save( CString &str ) { std::string _str; rbfserialize( m_model, _str ); str = _str.c_str(); }
//void ALRadialBasisFunction3D::Load( LPCTSTR str ) { std::string _str( str ); rbfunserialize( _str, m_model ); }

int ALRadialBasisFunction3D::CreateFromCSV( LPCTSTR inputFile )
   {
   FDataObj inputData (U_UNDEFINED);

   if ( inputData.ReadAscii( inputFile ) < 0 )
      return -1;

   int rows = inputData.GetRowCount();
   int cols = inputData.GetColCount();

   int inputDim  = 3;
   int outputDim = cols - 3; 
   int dims = inputDim + outputDim;  // 3 input dimensions, 1 model output
   int pts = rows;

   real_2d_array dataPts;
   dataPts.setlength( pts, dims );

   // for each point in the input file, add it to the dataPts array
  
   for ( int i=0; i < rows; i++ )
      for ( int j=0; j < cols; j++ )
         {
         dataPts( i, j ) = inputData( i, j ); /// CHECK ORDER!@!@@
         }

   // set up the functions
   Create( outputDim );
   SetPoints( dataPts, pts );
   Build();

   return outputDim;
   }


void ALRadialBasisFunction3D::SetPoints( double *xyzf, int size )
   {
   int rows = size / (m_inputDim+m_outputDim );

   real_2d_array pts;
   pts.setcontent( rows, (m_inputDim+m_outputDim), xyzf );

   alglib::rbfsetpoints( m_model, pts, size );
   }


void ALRadialBasisFunction3D::SetPoints( FDataObj &inputData )
   {
   m_outputDim = inputData.GetColCount() - 3;
   ASSERT( m_outputDim > 0 );

   int pts  = inputData.GetRowCount();
   int cols = m_inputDim+m_outputDim;

   real_2d_array dataPts;
   dataPts.setlength( pts, cols );

   // for each point in the input file, add it to the dataPts array
   for ( int row=0; row < pts; row++ )
      for ( int col=0; col < cols; col++ )
         {
         dataPts( row, col ) = inputData.GetAsDouble( col, row ); 
         }
      
   alglib::rbfsetpoints( m_model, dataPts, pts );
   }



int ALRadialBasisFunction3D::GetAt( double x, double y, double z, double *f )
   { 
   real_1d_array input;
   input.setlength( 3 );
   input[0] = x;
   input[1] = y;
   input[2] = z;
   
   real_1d_array output;

   int outputCount = this->GetOutputCount();
   output.setlength( outputCount );
     
   alglib::rbfcalc( m_model, input, output );

   for ( int i=0; i < outputCount; i++ )
      f[i] = output[i];

   return outputCount;
   }



/*

int _tmain(int argc, _TCHAR* argv[])
   {
   RunSpline3dTrilinear();
   RunSpline3dVector();
   
   if ( argc < 2 )
      return -1;

   ALRadialBasisFunction3D rbf;
   rbf.CreateFromCSV( argv[ 1 ] );

   // sample at some points
   double value = rbf.ValueAt( 0, 0, 0 );

   return 0;
   }





// spline3d_trilinear example
int RunSpline3dTrilinear( void )
   {
    //
    // We use trilinear spline to interpolate f(x,y,z)=x+xy+z sampled 
    // at (x,y,z) from [0.0, 1.0] X [0.0, 1.0] X [0.0, 1.0].
    //
    // We store x, y and z-values at local arrays with same names.
    // Function values are stored in the array F as follows:
    //     f[0]     (x,y,z) = (0,0,0)
    //     f[1]     (x,y,z) = (1,0,0)
    //     f[2]     (x,y,z) = (0,1,0)
    //     f[3]     (x,y,z) = (1,1,0)
    //     f[4]     (x,y,z) = (0,0,1)
    //     f[5]     (x,y,z) = (1,0,1)
    //     f[6]     (x,y,z) = (0,1,1)
    //     f[7]     (x,y,z) = (1,1,1)
    //
    real_1d_array x = "[0.0, 1.0]";
    real_1d_array y = "[0.0, 1.0]";
    real_1d_array z = "[0.0, 1.0]";
    real_1d_array f = "[0,1,0,2,1,2,1,3]";
    double vx = 0.50;
    double vy = 0.50;
    double vz = 0.50;
    double v;
    spline3dinterpolant s;

    // build spline
    spline3dbuildtrilinearv(x, 2, y, 2, z, 2, f, 1, s);

    // calculate S(0.5,0.5,0.5)
    v = spline3dcalc(s, vx, vy, vz);
    printf("%.4f\n", double(v)); // EXPECTED: 1.2500
    return 0;
}


//spline3d_vector example
int RunSpline3dVector( void )
{
    //
    // We use trilinear vector-valued spline to interpolate {f0,f1}={x+xy+z,x+xy+yz+z}
    // sampled at (x,y,z) from [0.0, 1.0] X [0.0, 1.0] X [0.0, 1.0].
    //
    // We store x, y and z-values at local arrays with same names.
    // Function values are stored in the array F as follows:
    //     f[0]     f0, (x,y,z) = (0,0,0)
    //     f[1]     f1, (x,y,z) = (0,0,0)
    //     f[2]     f0, (x,y,z) = (1,0,0)
    //     f[3]     f1, (x,y,z) = (1,0,0)
    //     f[4]     f0, (x,y,z) = (0,1,0)
    //     f[5]     f1, (x,y,z) = (0,1,0)
    //     f[6]     f0, (x,y,z) = (1,1,0)
    //     f[7]     f1, (x,y,z) = (1,1,0)
    //     f[8]     f0, (x,y,z) = (0,0,1)
    //     f[9]     f1, (x,y,z) = (0,0,1)
    //     f[10]    f0, (x,y,z) = (1,0,1)
    //     f[11]    f1, (x,y,z) = (1,0,1)
    //     f[12]    f0, (x,y,z) = (0,1,1)
    //     f[13]    f1, (x,y,z) = (0,1,1)
    //     f[14]    f0, (x,y,z) = (1,1,1)
    //     f[15]    f1, (x,y,z) = (1,1,1)
    //
    real_1d_array x = "[0.0, 1.0]";
    real_1d_array y = "[0.0, 1.0]";
    real_1d_array z = "[0.0, 1.0]";
    real_1d_array f = "[0,0, 1,1, 0,0, 2,2, 1,1, 2,2, 1,2, 3,4]";
    double vx = 0.50;
    double vy = 0.50;
    double vz = 0.50;
    spline3dinterpolant s;

    // build spline
    spline3dbuildtrilinearv(x, 2, y, 2, z, 2, f, 2, s);

    // calculate S(0.5,0.5,0.5) - we have vector of values instead of single value
    real_1d_array v;
    spline3dcalcv(s, vx, vy, vz, v);
    printf("%s\n", v.tostring(4).c_str()); // EXPECTED: [1.2500,1.5000]
    return 0;
}



int main(int argc, char **argv)
{
    //
    // This example shows how to solve least squares problems with RBF-ML algorithm.
    // Below we assume that you already know basic concepts shown in the RBF_D_QNN and
    // RBF_D_ML_SIMPLE examples.
    //
    rbfmodel model;
    rbfreport rep;
    double v;

    //
    // We have 2-dimensional space and very simple fitting problem - all points
    // except for two are well separated and located at straight line. Two
    // "exceptional" points are very close, with distance between them as small
    // as 0.01. RBF-QNN algorithm will have many difficulties with such distribution
    // of points:
    //     X        Y
    //     -2       1
    //     -1       0
    //     -0.005   1
    //     +0.005   2
    //     +1      -1
    //     +2       1
    // How will RBF-ML handle such problem?
    //
    rbfcreate(2, 1, model);
    real_2d_array xy0 = "[[-2,0,1],[-1,0,0],[-0.005,0,1],[+0.005,0,2],[+1,0,-1],[+2,0,1]]";
    rbfsetpoints(model, xy0);

    // First, we try to use R=5.0 with single layer (NLayers=1) and moderate amount
    // of regularization. Well, we already expected that results will be bad:
    //     Model(x=-2,y=0)=0.8407    (instead of 1.0)
    //     Model(x=0.005,y=0)=0.6584 (instead of 2.0)
    // We need more layers to show better results.
    rbfsetalgomultilayer(model, 5.0, 1, 1.0e-3);
    rbfbuildmodel(model, rep);
    v = rbfcalc2(model, -2.0, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 0.8407981659
    v = rbfcalc2(model, 0.005, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 0.6584267649

    // With 4 layers we got better result at x=-2 (point which is well separated
    // from its neighbors). Model is now many times closer to the original data
    //     Model(x=-2,y=0)=0.9992    (instead of 1.0)
    //     Model(x=0.005,y=0)=1.5534 (instead of 2.0)
    // We may see that at x=0.005 result is a bit closer to 2.0, but does not
    // reproduce function value precisely because of close neighbor located at
    // at x=-0.005. Let's add two layers...
    rbfsetalgomultilayer(model, 5.0, 4, 1.0e-3);
    rbfbuildmodel(model, rep);
    v = rbfcalc2(model, -2.0, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 0.9992673278
    v = rbfcalc2(model, 0.005, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 1.5534666012

    // With 6 layers we got almost perfect fit:
    //     Model(x=-2,y=0)=1.000    (perfect fit)
    //     Model(x=0.005,y=0)=1.996 (instead of 2.0)
    // Of course, we can reduce error at x=0.005 down to zero by adding more
    // layers. But do we really need it?
    rbfsetalgomultilayer(model, 5.0, 6, 1.0e-3);
    rbfbuildmodel(model, rep);
    v = rbfcalc2(model, -2.0, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 1.0000000000
    v = rbfcalc2(model, 0.005, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 1.9965775952

    // Do we really need zero error? We have f(+0.005)=2 and f(-0.005)=1.
    // Two points are very close, and in real life situations it often means
    // that difference in function values can be explained by noise in the
    // data. Thus, true value of the underlying function should be close to
    // 1.5 (halfway between 1.0 and 2.0).
    //
    // How can we get such result with RBF-ML? Well, we can:
    // a) reduce number of layers (make model less flexible)
    // b) increase regularization coefficient (another way of reducing flexibility)
    //
    // Having NLayers=5 and LambdaV=0.1 gives us good least squares fit to the data:
    //     Model(x=-2,y=0)=1.000
    //     Model(x=-0.005,y=0)=1.504
    //     Model(x=+0.005,y=0)=1.496
    rbfsetalgomultilayer(model, 5.0, 5, 1.0e-1);
    rbfbuildmodel(model, rep);
    v = rbfcalc2(model, -2.0, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 1.0000001620
    v = rbfcalc2(model, -0.005, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 1.5042954378
    v = rbfcalc2(model, 0.005, 0.0);
    printf("%.2f\n", double(v)); // EXPECTED: 1.4957042013
    return 0;
   }

   */
