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


#include "HistogramArray.h"
#include "Maplayer.h"
#include "tinyxml.h"

#include "PathManager.h"

RandUniform HistogramArray::rn( 0, 1 );
RandUniform HistogramArray::rn1( 0, 1 );

HistogramArray::HistogramArray( MapLayer *pLayer )
: CArray< Histogram*, Histogram* >()
, m_use( HU_UNIFORM )
, m_value( 0 )
, m_pMapLayer( pLayer )
   { }



//-- HistogramArray methods ---------------------------------------------------------

void HistogramArray::Clear()
   { 
   for ( int i=0; i < GetCount(); i++ )
      delete GetAt( i );
   
   CArray< Histogram*, Histogram* >::RemoveAll();
   }


bool HistogramArray::Evaluate( int cell, double &value )
   {
   ASSERT( m_pMapLayer != NULL );
   ASSERT( m_colCategory >= 0 );
   m_value = 0;
   
   // basic idea - find the histogram in the array associated with the
   // current mapLayer value for the category.  once found, sample
   // values out of the histogram according to m_use setting
   // NOTE:  The following code assumes the category specified in the first
   //        histogram is the same as all following histograms in the HistogramArray

   VData categoryValue;
   double dCategoryValue = 0;

   Histogram *pHisto = NULL;

   for ( int i=0; i < GetSize(); i++ )
      {
      Histogram *_pHisto = GetAt( i );

      if ( i == 0 )
         {
         m_pMapLayer->GetData( cell, m_colCategory, categoryValue );

         if ( _pHisto->m_isCategorical == false )
            categoryValue.GetAsDouble( dCategoryValue );
         }

      if ( _pHisto->m_isCategorical )
         {  
         if ( _pHisto->m_value == categoryValue )
            {
            pHisto = _pHisto;
            break;
            }
         }
      else // histogram represents a range of values
         {
         if ( _pHisto->m_minValue <= dCategoryValue && dCategoryValue <= _pHisto->m_maxValue )
            {
            pHisto = _pHisto;
            break;
            }
         }
      }

   if ( pHisto == NULL )      // no matching histogram found?
      {
      m_value = 0;
      value = 0;
      return false;
      }

   // a matching histogram was found - compute a value based on m_use
   switch( m_use )
      {
      case HU_UNIFORM:
         {
         //  get a sample from the bins
         int randValue = (int) rn.RandValue( 0, pHisto->m_observations );
         int curValue = 0;
         int bins = (int) pHisto->m_binArray.GetSize();
         for ( int i=0; i < bins; i++ )
            {
            HISTO_BIN &bin = pHisto->m_binArray.GetAt( i );
            curValue += bin.count;

            if ( randValue < curValue )
               {
               m_value = rn1.RandValue( bin.left, bin.right );
               value = m_value;
               return true;
               }
            }

         value = 0;
         return false;
         }

      case HU_MEAN:
         m_value = pHisto->m_mean;
         value = m_value;
         return true;

      case HU_AREAWTMEAN:
         m_value = pHisto->m_areaWtMean;
         value = m_value;
         return true;

      case HU_STDDEV:
         {
         ASSERT( 0 );
         return false;
         }
      }

   value = 0;
   return false;
   }


int HistogramArray::LoadXml( LPCTSTR _filename )
   {
   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "HistogramArray: Input file '%s' not found - this process will be disabled", _filename );
      Report::ErrorMsg( msg );
      return -1;
      }

   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {
      CString msg( doc.ErrorDesc() );
      msg += ": ";
      msg += filename;
      Report::ErrorMsg( msg );
      return -1;
      }
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();

   bool loadSuccess = true;
   
   TiXmlElement *pXmlHistogramsElement = pXmlRoot->ToElement();
   m_name     = pXmlHistogramsElement->Attribute( "name" );
   m_expr     = pXmlHistogramsElement->Attribute( "value" );
   m_category = pXmlHistogramsElement->Attribute( "category" );
   
   if ( m_category.IsEmpty() || m_name.IsEmpty() )
      {
      CString msg( "Misformed <histograms> element [" );
      msg += m_name;
      msg += "]: A required attribute is missing.";
      Report::ErrorMsg( msg );
      return false;
      }

   m_colCategory = m_pMapLayer->GetFieldCol( m_category );
   if ( m_colCategory < 0 )
      {
      CString msg( "A field referenced by a histogram collection is missing: " );
      msg += m_category;
      msg += " and should be added to the IDU coverage.";
      Report::WarningMsg( msg );
      return false;
      }

   // have basics, get Histograms for this collection
   TiXmlNode *pXmlHistoNode = NULL;
   while( pXmlHistoNode = pXmlRoot->IterateChildren( pXmlHistoNode ) )
      {
      if ( pXmlHistoNode->Type() != TiXmlNode::ELEMENT )
         continue;
         
      ASSERT( lstrcmp( pXmlHistoNode->Value(), "histogram" ) == 0 );
      TiXmlElement *pXmlHistoElement = pXmlHistoNode->ToElement();

      LPCTSTR category = pXmlHistoElement->Attribute( "category" );
      LPCTSTR minVal   = pXmlHistoElement->Attribute( "minValue" );
      LPCTSTR maxVal   = pXmlHistoElement->Attribute( "maxValue" );
      LPCTSTR value    = pXmlHistoElement->Attribute( "value" );
      LPCTSTR mean     = pXmlHistoElement->Attribute( "mean" );
      LPCTSTR areaWtMean = pXmlHistoElement->Attribute( "areaWtMean" );
      LPCTSTR stdDev   = pXmlHistoElement->Attribute( "stdDev" );

      int bins, obs;
      pXmlHistoElement->Attribute( "bins", &bins );
      pXmlHistoElement->Attribute( "obs", &obs );

      if ( category == NULL || ( value == NULL && minVal == NULL && maxVal == NULL ) || mean == NULL || areaWtMean == NULL || stdDev == NULL ) 
         {
         Report::ErrorMsg( _T("Misformed <histogram> element: A required attribute is missing."));
         loadSuccess = false;
         continue;
         }

      Histogram *pHisto = new Histogram;

      pHisto->m_category = category;
      pHisto->m_colCategory = m_pMapLayer->GetFieldCol( category );
      if ( pHisto->m_colCategory < 0 )
         {
         CString msg( _T("Error - column [") );
         msg += category;
         msg += _T("] is missing from the IDU layer - referenced in histogram ");
         msg += m_name;
         Report::ErrorMsg( msg );
         delete pHisto;
         continue;
         }

      if ( minVal && maxVal )
         {
         pHisto->m_minValue = (float ) atof( minVal );
         pHisto->m_maxValue = (float ) atof( maxVal );
         pHisto->m_isCategorical = false;
         }
      else if ( value )
         {
         pHisto->m_value = (float) atof( value );
         pHisto->m_isCategorical = true;
         }
      else
         {
         Report::ErrorMsg( _T("Misformed <histogram> element: A required attribute is missing."));
         loadSuccess = false;
         delete pHisto;
         continue;
         }

      pHisto->m_mean       = (float) atof( mean );
      pHisto->m_areaWtMean = (float) atof( areaWtMean );
      pHisto->m_stdDev     = (float) atof( stdDev );

      pHisto->m_observations = obs;
      Add( pHisto );

      // parse bins
      TiXmlNode *pXmlBinNode = NULL;
      while( pXmlBinNode = pXmlHistoNode->IterateChildren( pXmlBinNode ) )
         {
         if ( pXmlBinNode->Type() != TiXmlNode::ELEMENT )
            continue;
            
         ASSERT( lstrcmp( pXmlBinNode->Value(), "bin" ) == 0 );
         TiXmlElement *pXmlBinElement = pXmlBinNode->ToElement();

         LPCTSTR left  = pXmlBinElement->Attribute( "left" );
         LPCTSTR right = pXmlBinElement->Attribute( "right" );
         LPCTSTR count = pXmlBinElement->Attribute( "count" );

         if ( left == NULL || right == NULL || count == NULL )
            {
            Report::ErrorMsg( _T("Misformed <bin> element: A required attribute is missing."));
            loadSuccess = false;
            continue;
            }
         
         float _left  = (float) atof( left );
         float _right = (float) atof( right );
         UINT  _count = (UINT) atoi( count );

         HISTO_BIN bin( _left, _right, _count );
         pHisto->m_binArray.Add( bin );
         }  // end of: ( parse bins )

      }  // end of: ( parse histograms )

   return (int) GetSize();
   }
