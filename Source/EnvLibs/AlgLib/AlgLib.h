#pragma once
#ifndef ALGLIB_H
#define ALGLIB_H
#endif

#include "interpolation.h"
//#include "ap.h"
//#include "alglibinternal.h"
//#include "alglibmisc.h"

#include "../FDATAOBJ.H"

using namespace alglib;

/*-------------------------------------------------------
Usage:

// from file
   ALRadialBasisFunction3D rbf;

   // optionally set type of model 
   // (radius should be about avg distance between neighboring points)
   rbf.SetQNN( 1.0, 5.0 );    // OR
   rbf.SetML( 1.0, 5, 0.005 );   // radius, layers, lambdaV 

   // load the file (number of outputs inferred from file columns)
   rbf.CreateFromCSV( csvFile );

   // query for values
   double value = rbf.GetAt( 1, 1, 0 );


// from data
   ALRadialBasisFunction3D rbf;
   rbf.Create( 1 );  // number of outputs

   // optionally set type of model 
   // (radius should be about avg distance between neighboring points)
   rbf.SetML( 1.0, 5, 0.005 );   // radius, layers, lambdaV 

   FDataObj inputData;
   inputData.ReadAscii( "somefile.csv" );
   
   rbf.SetPoints( inputData );  // rows are input points, col0=x, col1=y, col2=z, col3=f0, col4=f1 etc...

   rbf.Build();
   
   double value = rbf.GetAt( 1, 1, 0 );   

----------------------------------------------------------*/

enum ALGOLTYPE { AT_QNN, AT_ML };


class LIBSAPI ALRadialBasisFunction3D
   {
   public:
      ALRadialBasisFunction3D( ALGOLTYPE type=AT_QNN ) : m_model(), m_algoType( type ), m_inputDim( 3 ), m_outputDim( 1 ) { }
      
      int  CreateFromCSV( LPCTSTR inputFile );   // creates, setpoints, and builds

      void Create( int ny )  { rbfcreate( 3, ny, m_model ); }
      
      /*!    Q       -   Q parameter, Q>0, recommended value - 1.0
             Z       -   Z parameter, Z>0, recommended value - 5.0  */
      void SetQNN( double q=1.0, double z=5.0 ) { m_algoType = AT_QNN; alglib::rbfsetalgoqnn( m_model, q, z ); }
      
      /*!  rbase   -   RBase parameter, RBase>0
           layers  -   NLayers parameter, NLayers>0, recommended value  to  start
                       with - about 5.
           lambdaV -   regularization value, can be useful when  solving  problem
                       in the least squares sense.  Optimal  lambda  is  problem-
                       dependent and require trial and error. In our  experience,
                       good lambda can be as large as 0.1, and you can use  0.001
                       as initial guess.
                       Default  value  - 0.01, which is used when LambdaV is  not
                       given.  You  can  specify  zero  value,  but  it  is   not
                       recommended to do so.
      
      See http://www.alglib.net/translator/man/manual.cpp.html#unit_rbf for usage tips  */

      void SetML( double rbase=1.0, int layers=5, double lambdaV=0.01 ) { m_algoType = AT_ML; alglib::rbfsetalgomultilayer( m_model, rbase, layers, lambdaV ); }

      void SetPoints( alglib::real_2d_array xyzf, int size ) { alglib::rbfsetpoints( m_model, xyzf, size ); }
      void SetPoints( double *xyzf, int size ); // size should be pts * (inputDim + outputDim)
      void SetPoints( FDataObj &data );         // rows=#pts, cols=inputDim+outputDim

      void Build( void ) { alglib::rbfreport rep; /*alglib::rbfsetalgoqnn( m_model );*/ alglib::rbfbuildmodel( m_model, rep); }

      // serialize/unserialize
      void Save( CString &str ) { std::string _str; rbfserialize( m_model, _str ); str = _str.c_str(); }
      void Save( std::string &str ) { rbfserialize( m_model, str ); }
      void Load( LPCTSTR str ) { std::string _str( str ); rbfunserialize( _str, m_model ); m_outputDim = GetOutputCount(); }

      // get values out
      double GetAt( double x, double y, double z ) { return alglib::rbfcalc3( m_model, x, y, z ); }

      int GetAt( double x, double y, double z, double *f );  // f must be pre-allocated (double[m_outputDim])
      
      // settings
      int GetOutputCount( void ) { ae_int_t nx; ae_int_t ny; real_2d_array xwr; ae_int_t nc; real_2d_array v; 
                                    alglib::rbfunpack( m_model, nx, ny, xwr, nc, v );  return (int) ny; }


   protected:
      alglib::rbfmodel m_model;
      ALGOLTYPE m_algoType;

      int m_inputDim;      // input dimension (always 3)
      int m_outputDim;     // output dimension

   };


//class ALTrilinearSpline3D
//   {
//   public:
//
//
//   protected:
//      spline3dinterpolant m_spline;
//
//   };
