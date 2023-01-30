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
/* LCPGenerator.cpp - maintaining the lcp file and the
** values of layers within. See .h for more info
**
** 2011.03.23 - tjs - error handling completed
*/

#include "stdafx.h"
#pragma hdrstop

#include "LCPGenerator.h"
#include <MapLayer.h>
#include "FlamMapAP.h"
#include <Report.h>
#include <PathManager.h>

#include <EnvModel.h>


extern FlamMapAP *gpFlamMapAP;

LCPGenerator::LCPGenerator() :
   m_nGridNumLayers(0),
   m_sourceCanopyCoverUnits(0),
   m_sourceHeightUnits(0),
   m_sourceBaseHeightUnits(0),
   m_sourceBulkDensityUnits(0),
   m_pLCPHeader(NULL),
   m_pLCPLayers(NULL),
   m_pPolyGridLkUp(NULL),
   m_pVegClassLCPLookup(NULL)
{
   
}

LCPGenerator::LCPGenerator( LPCTSTR LCPFName, 
      LPCTSTR VegClassLCPLookupFName, 
      PolyGridLookups *pgLookup)  :
   m_nGridNumLayers(0),
   m_sourceCanopyCoverUnits(0),
   m_sourceHeightUnits(0),
   m_sourceBaseHeightUnits(0),
   m_sourceBulkDensityUnits(0),
   m_pLCPHeader(NULL),
   m_pLCPLayers(NULL),
   m_pPolyGridLkUp(NULL),
   m_pVegClassLCPLookup(NULL)
   {
      InitLCPValues(LCPFName);
      InitVegClassLCPLookup(VegClassLCPLookupFName);
      InitPolyGridLookup(pgLookup);
   }



void LCPGenerator::SetLayerPt(LCP_LAYER layer, const int x, const int y, const short value)
   {
   int ndx = (x * m_pLCPHeader->numEast * m_nGridNumLayers) + (y * m_nGridNumLayers) + layer;
   m_pLCPLayers[ndx] = value;
   } // void LCPGenerator::SetLayerPt(ENV_LCP_LAYER layer, const int x, const int y, const short value);

   
BOOL LCPGenerator::InitLCPValues(LPCTSTR _fName)
   {
   CString msg;
   size_t  nBytesRead;

   m_sourceCanopyCoverUnits = 1;
   m_sourceHeightUnits = 2;
   m_sourceBaseHeightUnits = 2;
   m_sourceBulkDensityUnits = 1;

   m_nGridNumLayers = 8;

   m_pLCPHeader = new LCPHeader;
 
   // Read the starting LCP file into memory
   CString fName;
   PathManager::FindPath( _fName, fName );

   FILE *pStartingLCPFile = NULL;

   if(!(pStartingLCPFile = fopen(fName,"rb"))) 
      {
      msg.Format(_T("Error: Unable to open Initial FlamMap LCP file **%s**!\n"), fName );
      Report::ErrorMsg(msg);
      return FALSE;
      }

   nBytesRead = fread( 
      (void *)m_pLCPHeader, 
      (size_t)sizeof(char), 
      (size_t)sizeof(LCPHeader), 
      pStartingLCPFile);

   if(feof(pStartingLCPFile) || nBytesRead != (size_t)sizeof(LCPHeader)) 
      {
      msg.Format(_T("Error: Unable to read header from Initial FlamMap LCP file %s!\n"), fName );
      Report::ErrorMsg(msg);
      fclose(pStartingLCPFile);
      return FALSE;
      }

   // Setting the units for the LCP values
   //g_RunParams.IsSetIntParam((_T("VegClassHeightUnits")));
   //g_RunParams.IsSetIntParam((_T("VegClassBulkDensUnits")));
   //g_RunParams.IsSetIntParam((_T("VegClassCanopyCoverUnits")));

   // Canopy base height and canopy height
   m_pLCPHeader->BUnits = m_pLCPHeader->HUnits = 3;
   m_sourceBaseHeightUnits = m_sourceHeightUnits = gpFlamMapAP->m_vegClassHeightUnits;

   if(m_sourceBaseHeightUnits != 1 &&
      m_sourceBaseHeightUnits != 2 &&
      m_sourceBaseHeightUnits != 3 &&
      m_sourceBaseHeightUnits != 4
      )
   {
      Report::ErrorMsg(_T("Illegal height units in FlamMapAP init file!"));
      return false;
   }

   m_pLCPHeader->PUnits = 3;
   m_sourceBulkDensityUnits = gpFlamMapAP->m_vegClassBulkDensUnits;

   if(m_pLCPHeader->PUnits != 1 &&
      m_pLCPHeader->PUnits != 2 &&
      m_pLCPHeader->PUnits != 3 &&
      m_pLCPHeader->PUnits != 4
      )
   {
      Report::ErrorMsg(_T("Illegal Bulk Density units in FlamMapAP init file!"));
      return false;
   }

   m_pLCPHeader->CUnits = 1;
   m_sourceCanopyCoverUnits = gpFlamMapAP->m_vegClassCanopyCoverUnits;

   if(m_pLCPHeader->CUnits != 1 &&
      m_pLCPHeader->CUnits != 2
      )
   {
      Report::ErrorMsg(_T("Illegal Canopy Cover units in FlamMapAP init file!"));
      return false;
   }

   int lcpLayersSz = m_nGridNumLayers * (int) m_pLCPHeader->numNorth * (int) m_pLCPHeader->numEast;
   
   m_pLCPLayers = new short[lcpLayersSz];

   
   //CString msg3;
   //msg3.Format( "size ist %i", lcpLayersSz );
   //Report::InfoMsg( msg3 );

   nBytesRead = fread(
      (void *)m_pLCPLayers, 
      sizeof(short), 
      lcpLayersSz, 
      pStartingLCPFile);

   if(nBytesRead != (size_t)lcpLayersSz) 
      {
      msg.Format(_T("Error: Unable to read LCP from FlamMap LCP file %s!\n"), fName );
      Report::ErrorMsg(msg);
      fclose(pStartingLCPFile);
      return FALSE;
      }
   
   fclose(pStartingLCPFile);

   return TRUE;

   } // LCPGenerator::InitLCPValues(LPCTSTR fName); 


BOOL LCPGenerator::InitVegClassLCPLookup(LPCTSTR VegClassLCPLookupFName) 
   {
   bool rtrnVal = true;

   m_pVegClassLCPLookup = new VegClassLCPLookup(VegClassLCPLookupFName, m_sourceCanopyCoverUnits, m_sourceHeightUnits, m_sourceBaseHeightUnits, m_sourceBulkDensityUnits);
   if( ! gpFlamMapAP->m_runStatus )
      rtrnVal = false;

   return rtrnVal;
   } // BOOL LCPGenerator::InitVegClassLCPLookup(LPCTSTR VegClassLCPLookupFName)


void LCPGenerator::InitPolyGridLookup(PolyGridLookups *pgLookup) 
   {
   ASSERT(pgLookup != NULL);
   m_pPolyGridLkUp = pgLookup;
   } // void LCPGenerator::InitPolyGridLookup(PolyGridLookups *pgLookup)


LCPGenerator::~LCPGenerator()
   {
   if(m_pLCPHeader != NULL)
      delete m_pLCPHeader;

   if(m_pLCPLayers != NULL)
      delete [] m_pLCPLayers;

   if(m_pVegClassLCPLookup != NULL)
      delete m_pVegClassLCPLookup;
   } // ~LandscapeGenerator::LandscapeGenerator()


void LCPGenerator::PrepLandscape(EnvContext *pEnvContext) 
   {
   // Choices for getting values:
   //
   // polygon plurality - take the value
   // from the polygon which comprises largest portion
   // of the cell
   //
   // value plurality - take the value which accounts for
   // the largest portion of the cell
   //
   // weighted mean, max value, min value - pretty self
   // explanatory.
   //
   // Initially, using polygon plurality for all fields

   CString msg;
   
   int i, j;
   int nMaxPolyCnt = 0,
       nPolyCnt = 0,
      *pnPolyNdxs = NULL,
      *pnPolyVDDTStates = NULL,
       nPluralPolyNdx,
       vegClassCol,
       vegClass,
       variantCol,
       variant,
       voidVegClassCnt = 0,
      useFModelPlus = 0,
      fuelModelPlusCol = -1,
	  regionCol = -1,
	  region = 0,
	  pvtCol = -1,
	  pvt = 0;

   float *pfPolyPortions = NULL;

   int ttlPairs = 0,
       voidCvrType = 0,
       voidStrctStg = 0,
       voidBoth = 0;
   __int64 nCellsFModel = 0;
   vegClassCol = gpFlamMapAP->m_vegClassCol;
   variantCol  = gpFlamMapAP->m_variantCol;
   fuelModelPlusCol = gpFlamMapAP->m_fModel_plusCol;
   regionCol = gpFlamMapAP->m_regionCol;
   pvtCol = gpFlamMapAP->m_pvtCol;
   REAL xMin, xMax, yMin, yMax;
   pEnvContext->pMapLayer->GetExtents(xMin, xMax, yMin, yMax);

   int startRow = int( (GetNorth() - yMax) / GetCellDim() );
   int startCol = int( (xMin - GetWest()) / GetCellDim() );

   for (i = 0; i < m_pPolyGridLkUp->GetNumGridRows(); i++)
   {
	   for (j = 0; j < m_pPolyGridLkUp->GetNumGridCols(); j++)
	   {
		   // Using polygon plurality
		   nPluralPolyNdx = m_pPolyGridLkUp->GetPolyPluralityForGridPt(i, j);

		   if (nPluralPolyNdx > 0)
		   {
			   pEnvContext->pMapLayer->GetData(nPluralPolyNdx, vegClassCol, vegClass);
			   pEnvContext->pMapLayer->GetData(nPluralPolyNdx, variantCol, variant);

			   if (fuelModelPlusCol >= 0)
			   {
				   pEnvContext->pMapLayer->GetData(nPluralPolyNdx, fuelModelPlusCol, useFModelPlus);
			   }
			   else
				   useFModelPlus = 0;
			   if (regionCol >= 0)
			   {
				   pEnvContext->pMapLayer->GetData(nPluralPolyNdx, regionCol, region);
			   }
			   else
				   region = 0;
			   if (pvtCol >= 0)
			   {
				   pEnvContext->pMapLayer->GetData(nPluralPolyNdx, pvtCol, pvt);
			   }
			   else
				   region = 0;
			   //FmodelPlus metrics
			   if (useFModelPlus)
				   nCellsFModel++;

			   // Now get the values for the LCP layers and update them for
			   // the FlamMap run.

			   if (vegClass < 0)
				   voidVegClassCnt++;

			   /*SetFuelModel  (i+startRow, j+startCol, m_pVegClassLCPLookup->GetFuelModel(vegClass, variant, useFModelPlus));
			   SetCanopyCover(i+startRow, j+startCol, m_pVegClassLCPLookup->GetCanopyCover(vegClass, variant));
			   SetStandHeight(i+startRow, j+startCol, m_pVegClassLCPLookup->GetStandHeight(vegClass, variant));
			   SetBaseHeight (i+startRow, j+startCol, m_pVegClassLCPLookup->GetBaseHeight(vegClass, variant));
			   SetBulkDensity(i+startRow, j+startCol, m_pVegClassLCPLookup->GetBulkDensity(vegClass, variant));*/
			   SetFuelModel(i + startRow, j + startCol, m_pVegClassLCPLookup->GetFuelModel(vegClass, region, pvt, variant, useFModelPlus));
			   SetCanopyCover(i + startRow, j + startCol, m_pVegClassLCPLookup->GetCanopyCover(vegClass, region, pvt, variant));
			   SetStandHeight(i + startRow, j + startCol, m_pVegClassLCPLookup->GetStandHeight(vegClass, region, pvt, variant));
			   SetBaseHeight(i + startRow, j + startCol, m_pVegClassLCPLookup->GetBaseHeight(vegClass, region, pvt, variant));
			   SetBulkDensity(i + startRow, j + startCol, m_pVegClassLCPLookup->GetBulkDensity(vegClass, region, pvt, variant));
		   }
		   else
		   {
			   SetFuelModel(i + startRow, j + startCol, 99);// m_pVegClassLCPLookup->GetFuelModel(vegClass, region, pvt, variant, useFModelPlus));
			   SetCanopyCover(i + startRow, j + startCol, -9999);// m_pVegClassLCPLookup->GetCanopyCover(vegClass, region, pvt, variant));
			   SetStandHeight(i + startRow, j + startCol, -9999);// m_pVegClassLCPLookup->GetStandHeight(vegClass, region, pvt, variant));
			   SetBaseHeight(i + startRow, j + startCol, -9999);// m_pVegClassLCPLookup->GetBaseHeight(vegClass, region, pvt, variant));
			   SetBulkDensity(i + startRow, j + startCol, -9999);//m_pVegClassLCPLookup->GetBulkDensity(vegClass, region, pvt, variant));
		   }
	   } // for(j=0;j<m_pPolyGridLkUp->numGridCols;i++)
   } // for(i=0;i<m_pPolyGridLkUp->numGridRows;i++)

   double propFModel = ((double)nCellsFModel) / (double)(m_pPolyGridLkUp->GetNumGridRows() * m_pPolyGridLkUp->GetNumGridCols());
   msg.Format(_T("Proportion cells using FModelplus: %lf"), propFModel);
   Report::Log(msg);

		// Build filename and write to file

   LPCTSTR scenarioName = NULL;

   if ( pEnvContext->pEnvModel->m_pScenario != NULL )
      scenarioName = pEnvContext->pEnvModel->m_pScenario->m_name;
   else
      scenarioName = _T("Default");

   TCHAR shortName[200];
   shortName[0] = 0;
   int len = 0;
   for( len = 0; len < strlen(scenarioName) && len < 200; len++)
      {
      if(!isalnum(scenarioName[len]))
            break;
      }

   strncpy(shortName, scenarioName, len);
   shortName[len] = 0;

   CString LCPFName;
   LCPFName.Format(_T("%s_%s_%03d_%04d.lcp"), (LPCTSTR) gpFlamMapAP->m_scenarioLCPFNameRoot, shortName, pEnvContext->run,  pEnvContext->currentYear );

   WriteFile(LCPFName, pEnvContext);

   if ( pnPolyNdxs )
      delete [] pnPolyNdxs;
   if ( pnPolyVDDTStates )
      delete [] pnPolyVDDTStates;
   if ( pfPolyPortions )
      delete [] pfPolyPortions;
   } // void LCPGenerator::PrepLandscape(EnvContext *pEnvContext) 


void LCPGenerator::WriteFile(LPCTSTR fName, EnvContext *pEnvContext) 
   {
   FILE *pLCPFile = fopen( fName, "wb" );
   
   if ( pLCPFile == NULL)
      {
      CString msg( "FlamMap: Error Opening file ");
      msg += fName;
      Report::ErrorMsg( msg );
      }

   LCPHeader tHeader;
   memcpy(&tHeader, m_pLCPHeader, sizeof(LCPHeader));
   tHeader.numNorth = m_pPolyGridLkUp->GetNumGridRows();
   tHeader.numEast = m_pPolyGridLkUp->GetNumGridCols();

   REAL xMin, xMax, yMin, yMax;
   pEnvContext->pMapLayer->GetExtents(xMin, xMax, yMin, yMax);
   
   tHeader.EastUtm = tHeader.hiEast  = xMax;//this->GetEast();
   tHeader.WestUtm = tHeader.loEast = xMin;//this->GetWest();
   tHeader.NorthUtm  =  tHeader.hiNorth= yMax;//this->GetNorth();
   tHeader.SouthUtm = tHeader.loNorth  = yMin;//this->GetSouth();
   
   size_t bytesWritten = fwrite(
      (void *)&tHeader, 
      (size_t)sizeof(char), 
      (size_t)sizeof(LCPHeader), 
      pLCPFile);

   ASSERT(bytesWritten == (size_t)sizeof(LCPHeader));
   int startRow = int ( (GetNorth() - yMax) / GetCellDim() );
   int startCol = int ( (xMin - GetWest()) / GetCellDim() );

   for(int r = 0; r < tHeader.numNorth; r++)
      {
      int offset = m_nGridNumLayers * (r + startRow) * m_pLCPHeader->numEast + m_nGridNumLayers * startCol;
      bytesWritten = fwrite(
         (void *)&m_pLCPLayers[offset], 
         sizeof(short), 
         m_nGridNumLayers * tHeader.numEast,
         pLCPFile);
      }
  
   fclose(pLCPFile);

   gpFlamMapAP->m_lcpFName = fName;

   } // void LCPGenerator::WriteFile(LPCTSTR fName)


void LCPGenerator::WriteReadableHeaderToFile(LPCTSTR fName)
   {
   FILE *outFile = fopen(fName, "w");
   if ( outFile == NULL)
      {
      CString msg( "FlamMap: Error Opening file ");
      msg += fName;
      Report::ErrorMsg( msg );
      }

   fprintf(outFile, "crownFuels: %ld\n", m_pLCPHeader->crownFuels);
   fprintf(outFile, "groundFuels: %ld\n", m_pLCPHeader->groundFuels);
   fprintf(outFile, "latitude: %ld\n", m_pLCPHeader->latitude);
   fprintf(outFile, "loEast: %g\n", m_pLCPHeader->loEast);
   fprintf(outFile, "hiEast: %g\n", m_pLCPHeader->hiEast);
   fprintf(outFile, "loNorth: %g\n", m_pLCPHeader->loNorth);
   fprintf(outFile, "hiNorth: %g\n", m_pLCPHeader->hiNorth);
   fprintf(outFile, "loElev: %ld\n", m_pLCPHeader->loElev);
   fprintf(outFile, "hiElev: %ld\n", m_pLCPHeader->hiElev);
   fprintf(outFile, "numElev: %ld\n", m_pLCPHeader->numElev);
   fprintf(outFile, "elevationValues[100]: skipping\n");
   fprintf(outFile, "loSlope: %ld\n", m_pLCPHeader->loSlope);
   fprintf(outFile, "hiSlope: %ld\n", m_pLCPHeader->hiSlope);
   fprintf(outFile, "numSlope: %ld\n", m_pLCPHeader->numSlope);
   fprintf(outFile, "slopeValues[100]: skipping\n");
   fprintf(outFile, "loAspect: %ld\n", m_pLCPHeader->loAspect);
   fprintf(outFile, "hiAspect: %ld\n", m_pLCPHeader->hiAspect);
   fprintf(outFile, "numAspects: %ld\n", m_pLCPHeader->numAspects);
   fprintf(outFile, "aspectValues[100]: skipping\n");
   fprintf(outFile, "loFuel: %ld\n", m_pLCPHeader->loFuel);
   fprintf(outFile, "hiFuel: %ld\n", m_pLCPHeader->hiFuel);
   fprintf(outFile, "numFuel: %ld\n", m_pLCPHeader->numFuel);
   fprintf(outFile, "fuelValues[100]: skipping\n");
   fprintf(outFile, "loCover: %ld\n", m_pLCPHeader->loCover);
   fprintf(outFile, "hiCover: %ld\n", m_pLCPHeader->hiCover);
   fprintf(outFile, "numCover: %ld\n", m_pLCPHeader->numCover);
   fprintf(outFile, "coverValues[100]: skipping\n");
   fprintf(outFile, "loHeight: %ld\n", m_pLCPHeader->loHeight);
   fprintf(outFile, "hiHeight: %ld\n", m_pLCPHeader->hiHeight);
   fprintf(outFile, "numHeight: %ld\n", m_pLCPHeader->numHeight);
   fprintf(outFile, "heightValues[100]: skipping\n");
   fprintf(outFile, "loBase: %ld\n", m_pLCPHeader->loBase);
   fprintf(outFile, "hiBase: %ld\n", m_pLCPHeader->hiBase);
   fprintf(outFile, "numBase: %ld\n", m_pLCPHeader->numBase);
   fprintf(outFile, "baseValues[100]: skipping\n");
   fprintf(outFile, "loDensity: %ld\n", m_pLCPHeader->loDensity);
   fprintf(outFile, "hidensity: %ld\n", m_pLCPHeader->hidensity);
   fprintf(outFile, "numDensity: %ld\n", m_pLCPHeader->numDensity);
   fprintf(outFile, "densityValues[100]: skipping\n");
   fprintf(outFile, "loDuff: %ld\n", m_pLCPHeader->loDuff);
   fprintf(outFile, "hiDuff: %ld\n", m_pLCPHeader->hiDuff);
   fprintf(outFile, "numDuff: %ld\n", m_pLCPHeader->numDuff);
   fprintf(outFile, "duffValues[100]: skipping\n");
   fprintf(outFile, "loWoody: %ld\n", m_pLCPHeader->loWoody);
   fprintf(outFile, "hiWoody: %ld\n", m_pLCPHeader->hiWoody);
   fprintf(outFile, "numWoodies: %ld\n", m_pLCPHeader->numWoodies);
   fprintf(outFile, "woodyValues[100]: skipping\n");
   fprintf(outFile, "numEast: %ld\n", m_pLCPHeader->numEast);
   fprintf(outFile, "numNorth: %ld\n", m_pLCPHeader->numNorth);
   fprintf(outFile, "EastUtm: %g\n", m_pLCPHeader->EastUtm);
   fprintf(outFile, "WestUtm: %g\n", m_pLCPHeader->WestUtm);
   fprintf(outFile, "NorthUtm: %g\n", m_pLCPHeader->NorthUtm);
   fprintf(outFile, "SouthUtm: %g\n", m_pLCPHeader->SouthUtm);
   fprintf(outFile, "GridUnits: %ld\n", m_pLCPHeader->GridUnits);
   fprintf(outFile, "XResol: %g\n", m_pLCPHeader->XResol);
   fprintf(outFile, "YResol: %g\n", m_pLCPHeader->YResol);
   fprintf(outFile, "EUnits: %hd\n", m_pLCPHeader->EUnits);
   fprintf(outFile, "SUnits: %hd\n", m_pLCPHeader->SUnits);
   fprintf(outFile, "AUnits: %hd\n", m_pLCPHeader->AUnits);
   fprintf(outFile, "FOptions: %hd\n", m_pLCPHeader->FOptions);
   fprintf(outFile, "CUnits: %hd\n", m_pLCPHeader->CUnits);
   fprintf(outFile, "HUnits: %hd\n", m_pLCPHeader->HUnits);
   fprintf(outFile, "BUnits: %hd\n", m_pLCPHeader->BUnits);
   fprintf(outFile, "PUnits: %hd\n", m_pLCPHeader->PUnits);
   fprintf(outFile, "DUnits: %hd\n", m_pLCPHeader->DUnits);
   fprintf(outFile, "WOptions: %hd\n", m_pLCPHeader->WOptions);
   fprintf(outFile, "ElevFile[256]: %s\n", m_pLCPHeader->ElevFile);
   fprintf(outFile, "SlopeFile[256]: %s\n", m_pLCPHeader->SlopeFile);
   fprintf(outFile, "AspectFile[256]: %s\n", m_pLCPHeader->AspectFile);
   fprintf(outFile, "FuelFile[256]: %s\n", m_pLCPHeader->FuelFile);
   fprintf(outFile, "CoverFile[256]: %s\n", m_pLCPHeader->CoverFile);
   fprintf(outFile, "HeightFile[256]: %s\n", m_pLCPHeader->HeightFile);
   fprintf(outFile, "BaseFile[256]: %s\n", m_pLCPHeader->BaseFile);
   fprintf(outFile, "DensityFile[256]: %s\n", m_pLCPHeader->DensityFile);
   fprintf(outFile, "DuffFile[256]: %s\n", m_pLCPHeader->DuffFile);
   fprintf(outFile, "WoodyFile[256]: %s\n", m_pLCPHeader->WoodyFile);
   fprintf(outFile, "Description[512]: %s\n", m_pLCPHeader->Description);

   fclose(outFile);

   } // void LCPGenerator::WriteReadableHeaderToFile(LPCTSTR fName)