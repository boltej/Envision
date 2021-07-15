// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ShoreZone.h"

#include <UNITCONV.H>

/*
<output_group name = "ShoreZone" layer = "ShoreZone" >
<output layer = 'ShoreZone' name = 'Stability Class (m)'        query = "LENGTH &gt; 0"     value = "LENGTH"            type = "sum" stratify_by = "chng_type" / >
<output layer = 'ShoreZone' name = 'Biotic Assemblages (m)'     query = 'LENGTH &gt; 0'     value = 'LENGTH'            type = 'sum' stratify_by = 'hab_calc' / > <!--Predicted intertidal biotic assemblage.                                                         -->
<output layer = 'ShoreZone' name = 'Pct Shoreline Modified'     query = 'LENGTH &gt; 0'     value = 'sm_tot_pct'        type = 'lenWtAvg' / > <!-- 0 - 100 Total percentage of shoreline modified.                                                   -->
<output layer = 'ShoreZone' name = 'Pct Primary Mod'            query = 'LENGTH &gt; 0'     value = 'sm1_pct'           type = 'lenWtAvg' / >    <!--Estimated percent occurrence of the primary shoreline modification type in tenths.              -->
<output layer = 'ShoreZone' name = 'Pct Secondary Mod'          query = 'LENGTH &gt; 0'     value = 'sm2_pct'           type = 'lenWtAvg' / >    <!-- 0 - 100 Estimated percent occurrence of the secondary shoreline modification type in tenths.      -->
<output layer = 'ShoreZone' name = 'Pct Tertiary Mod'           query = 'LENGTH &gt; 0'     value = 'sm3_pct'           type = 'lenWtAvg' / >    <!-- 0 - 100 Estimated percent occurrence of the secondary shoreline modification type in tenths.      -->
<output layer = 'ShoreZone' name = 'Primary Mod Type (ft)'      query = 'LENGTH &gt; 0'     value = 'sm1_ft'            type = 'sum' stratify_by = 'sm1_type' / >    <!--Primary type of shoreline modification occurring within the unit.                               -->
<output layer = 'ShoreZone' name = 'Secondary Mod Type (ft)'    query = 'LENGTH &gt; 0'     value = 'sm2_ft'            type = 'sum' stratify_by = 'sm2_type' / >    <!--Secondary type of shoreline modification occurring within the unit.                             -->
<output layer = 'ShoreZone' name = 'Tertiary Mod Type (ft)'     query = 'LENGTH &gt; 0'     value = 'sm3_ft'            type = 'sum' stratify_by = 'sm3_type' / >    <!--Secondary type of shoreline modification occurring within the unit.                             -->
<output layer = 'ShoreZone' name = 'Primary Mod Type (ft)'      query = 'sm1_ft &gt; 0'     value = 'sm1_ft'            type = 'sum' stratify_by = 'sm1_type' / >    <!--Primary type of shoreline modification occurring within the unit.                               -->
<output layer = 'ShoreZone' name = 'Secondary Mod Type (ft)'    query = 'sm2_ft &gt; 0'     value = 'sm2_ft'            type = 'sum' stratify_by = 'sm2_type' / >    <!--Secondary type of shoreline modification occurring within the unit.                             -->
<output layer = 'ShoreZone' name = 'Tertiary Mod Type (ft)'     query = 'sm3_ft &gt; 0'     value = 'sm3_ft'            type = 'sum' stratify_by = 'sm3_type' / >    <!--Secondary type of shoreline modification occurring within the unit.                             -->
<output layer = 'ShoreZone' name = 'Ramps (#)'                  query = 'ramp &gt; 0'       value = 'ramp'             type = 'sum' / > <!--Number of boat ramps that occur in the unit.                                                    -->
<output layer = 'ShoreZone' name = 'Piers/Docks (#)'            query = 'pierdock &gt; 0'   value = 'pierdock'          type = 'sum' / > <!--Number of piers or wharves that occur within the unit.                                          -->
<output layer = 'ShoreZone' name = 'Small Slips (#)'            query = 'slip_small &gt; 0' value = 'slip_small'        type = 'sum' / > <!--Estimated number of recreational(or small) slips.                                              -->
<output layer = 'ShoreZone' name = 'Large Slips (#)'            query = 'slip_large &gt; 0' value = 'slip_large'        type = 'sum' / > < !--Estimated number of slips for ocean - going vessels(~> 100').                                     -->
<output layer = 'ShoreZone' name = 'Railways (mi)'              query = 'railroad=="Y"'     value = 'LENGTH/5280'       type = 'sum' / > <!--Railbed in contact with shorezone.                                                              -->
<output layer = 'ShoreZone' name = 'Pct Riparian'               query = 'LENGTH &gt; 0'     value = 'ripar_pct'         type = 'lenWtAvg' / > <!--Estimated percent of unit with vegetation overhanging the intertidal zone.                      -->
<output layer = 'ShoreZone' name = 'Riparian (mi)'              query = 'ripar_ft &gt; 0'   value = 'ripar_ft/5280'     type = 'sum' / > <!--Estimated length of riparian vegetation overhanging the intertidal zone.                        -->
<output layer = 'ShoreZone' name = 'Intertidal Area (ft2)'      query = 'itz_width &gt; 0'  value = 'itz_width*LENGTH'  type = 'sum' / > <!--Intertidal zone width.                                                                          -->
<output layer = 'ShoreZone' name = 'Verrucaria (mi)'            query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'ver_unit' / >
<output layer = 'ShoreZone' name = 'Dune Grasses (mi)'          query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'gra_unit' / >
<output layer = 'ShoreZone' name = 'Sedge (mi)'                 query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'sed_unit' / >
<output layer = 'ShoreZone' name = 'Salt-Tolerant Assm (mi)'    query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'tri_unit' / >
<output layer = 'ShoreZone' name = 'Salicornia (mi)'            query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'sal_unit' / >
<output layer = 'ShoreZone' name = 'Spartina (mi)'              query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'spa_unit' / >
<output layer = 'ShoreZone' name = 'Fucus (mi)'                 query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'fuc_unit' / >
<output layer = 'ShoreZone' name = 'Common Barnacle (mi)'       query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'bar_unit' / >
<output layer = 'ShoreZone' name = 'Blue Mussel (mi)'           query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'bmu_unit' / >
<output layer = 'ShoreZone' name = 'Ulva (mi)'                  query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'ulv_unit' / >
<output layer = 'ShoreZone' name = 'Gracilaria (mi)'            query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'gca_unit' / >
<output layer = 'ShoreZone' name = 'Oysters (mi)'               query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'oys_unit' / >
<output layer = 'ShoreZone' name = 'Cal Mussel (mi)'            query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'mus_unit' / >
<output layer = 'ShoreZone' name = 'Red Algae (mi)'             query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'red_unit' / >
<output layer = 'ShoreZone' name = 'Burrowing Shrimp (mi)'      query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'cal_unit' / >
<output layer = 'ShoreZone' name = 'Soft Brown Kelps (mi)'      query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'sbr_unit' / >
<output layer = 'ShoreZone' name = 'Sargassum (mi)'             query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'sar_unit' / >
<output layer = 'ShoreZone' name = 'Chocolate Brown Helps (mi)' query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'chb_unit' / >
<output layer = 'ShoreZone' name = 'Green Surfgrass (mi)'       query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'sur_unit' / >
<output layer = 'ShoreZone' name = 'Densraster (mi)'            query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'den_unit' / >
<output layer = 'ShoreZone' name = 'Eelgrass (mi)'              query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'zos_unit' / >
<output layer = 'ShoreZone' name = 'Bull Kelp (mi)'             query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'ner_unit' / >
<output layer = 'ShoreZone' name = 'Macrocytis Kelp (mi)'       query = 'LENGTH &gt; 0'     value = 'LENGTH/5280'       type = 'sum' stratify_by = 'mac_unit' / >

   < / output_group>
*/



//extern "C" EnvExtension* Factory(EnvContext*)
//   { 
//   return (EnvExtension*) new ExampleModel; 
//   }

bool ShoreZone::Init(EnvContext* pContext, LPCTSTR initStr)
   {
   m_dataObj.SetName("ShoreZone");
   for (int i=0; i < SZCOLS; i++)
      m_dataObj.SetLabel(i, szCols[i].label.c_str());

   this->AddOutputVar("ShoreZone Metrics", &m_dataObj, ""); 

   MapLayer* pIDULayer = (MapLayer*)pContext->pMapLayer;
   MapLayer* pSZLayer = pIDULayer->GetMapPtr()->GetLayer("ShoreZone");

   this->CheckCol(pSZLayer, m_colChngType,   "chng_type",  TYPE_STRING, CC_MUST_EXIST);
   //this->CheckCol(pSZLayer, m_colHabCalc,    "hab_calc",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSmTotPct,   "sm_tot_pct", TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm1Pct,     "sm1_pct",    TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm2Pct,     "sm2_pct",    TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm3Pct,     "sm3_pct",    TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm1Ft,      "sm1_ft",     TYPE_FLOAT,  CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm2Ft,      "sm2_ft",     TYPE_FLOAT,  CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm3Ft,      "sm3_ft",     TYPE_FLOAT,  CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm1Type,    "sm1_type",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm2Type,    "sm2_type",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSm3Type,    "sm3_type",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colRamp,       "ramp",       TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colPierDock,   "pierdock",   TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSlipSm,     "slip_small", TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSlipLg,     "slip_large", TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colRail,       "railroad",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colRipPct,     "ripar_pct",  TYPE_FLOAT,  CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colRipFt,      "ripar_ft",   TYPE_FLOAT,  CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colItzWidth,   "itz_width",  TYPE_INT,    CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colVerUnit,    "ver_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colGraUnit,    "gra_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSedUnit,    "sed_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colTriUnit,    "tri_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSalUnit,    "sal_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSpaUnit,    "spa_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colFucUnit,    "fuc_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colBarUnit,    "bar_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colBmuUnit,    "bmu_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colUlvUnit,    "ulv_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colGcaUnit,    "gca_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colOysUnit,    "oys_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colMusUnit,    "mus_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colRedUnit,    "red_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colCalUnit,    "cal_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSbrUnit,    "sbr_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSarUnit,    "sar_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colChbUnit,    "chb_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colSurUnit,    "sur_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colDenUnit,    "den_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colZosUnit,    "zos_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colNerUnit,    "ner_unit",   TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(pSZLayer, m_colMacUnit,    "mac_unit",   TYPE_STRING, CC_MUST_EXIST );

   return true;
   }


bool ShoreZone::InitRun(EnvContext* pContext, bool useInitialSeed)
   {
   m_dataObj.ClearRows();
   return true;
   }


bool ShoreZone::Run(EnvContext* pContext)
   {
   // compute metrics
   MapLayer* pIDULayer = (MapLayer*)pContext->pMapLayer;
   MapLayer* pSZLayer = pIDULayer->GetMapPtr()->GetLayer("ShoreZone");
   if (pSZLayer == NULL)
      return false;

   float values[SZCOLS];
   memset(values, 0, SZCOLS * sizeof(float));

   int   colLength = pSZLayer->GetFieldCol("LENGTH");
   ASSERT(colLength >= 0);
   float totalLength = 0;
   values[0] = pContext->currentYear;

   // iterate over ACTIVE records, evaluating expressions as we go
   for (MapLayer::Iterator i = pSZLayer->Begin(); i != pSZLayer->End(); i++)
      {
      float length = 0;    // "LENGTH" forline layers
      pSZLayer->GetData(i, colLength, length);
      totalLength += length;

      // stability class
      TCHAR chngType = 0;
      pSZLayer->GetData(i, m_colChngType, chngType);
      switch (chngType) {
         case 'A' : values[1] += length;   break;
         case 'E' : values[2] += length;   break;
         case 'S' : values[3] += length;   break;
         }

      // Pct Shoreline Modified
      int smTotPct = 0;
      pSZLayer->GetData(i, m_colSmTotPct, smTotPct);
      values[4] += smTotPct * length;

      int sm1Pct = 0;
      pSZLayer->GetData(i, m_colSm1Pct, sm1Pct);
      values[5] += sm1Pct * length;

      int sm2Pct = 0;
      pSZLayer->GetData(i, m_colSm2Pct, sm1Pct);
      values[6] += sm1Pct * length;

      int sm3Pct = 0;
      pSZLayer->GetData(i, m_colSm3Pct, sm1Pct);
      values[7] += sm1Pct * length;

      // shoreline modification types
      CString sm1Type;
      float sm1Ft = 0;
      pSZLayer->GetData(i, m_colSm1Type, sm1Type);
      pSZLayer->GetData(i, m_colSm1Ft, sm1Ft);
      if (sm1Type.GetLength() > 0) {
         switch (sm1Type[0]) {
            case 'B':   values[8] += sm1Ft;  break;
            case 'C':   values[9] += sm1Ft;  break;
            case 'W':   values[10] += sm1Ft;  break;
            case 'L':   values[11] += sm1Ft;  break;
            case 'S':   values[12] += sm1Ft;  break;
            case 'R':   values[13] += sm1Ft;  break;
            }
         }

      CString sm2Type;
      float sm2Ft = 0;
      pSZLayer->GetData(i, m_colSm2Type, sm2Type);
      pSZLayer->GetData(i, m_colSm2Ft, sm2Ft);
      if (sm1Type.GetLength() > 0) {
         switch (sm2Type[0]) {
            case 'B':   values[8] += sm2Ft;  break;
            case 'C':   values[9] += sm2Ft;  break;
            case 'W':   values[10] += sm2Ft;  break;
            case 'L':   values[11] += sm2Ft;  break;
            case 'S':   values[12] += sm2Ft;  break;
            case 'R':   values[13] += sm2Ft;  break;
            }
         }

      CString sm3Type;
      float sm3Ft = 0;
      pSZLayer->GetData(i, m_colSm3Type, sm3Type);
      pSZLayer->GetData(i, m_colSm3Ft, sm3Ft);
      if (sm1Type.GetLength() > 0) {
         switch (sm3Type[0]) {
            case 'B':   values[8] += sm3Ft;  break;
            case 'C':   values[9] += sm3Ft;  break;
            case 'W':   values[10] += sm3Ft;  break;
            case 'L':   values[11] += sm3Ft;  break;
            case 'S':   values[12] += sm3Ft;  break;
            case 'R':   values[13] += sm3Ft;  break;
            }
         }

      // ramps, piers, slips, railways
      int ramps = 0, pierDocks = 0, slipsSm = 0, slipsLg = 0;
      pSZLayer->GetData(i, m_colRamp, ramps);
      values[14] += ramps;
      pSZLayer->GetData(i, m_colPierDock, pierDocks);
      values[15] += pierDocks;
      pSZLayer->GetData(i, m_colSlipSm, slipsSm);
      values[16] += slipsSm;
      pSZLayer->GetData(i, m_colSlipLg, slipsLg);
      values[17] += slipsLg;

      char railway = 0;
      pSZLayer->GetData(i, m_colRail, railway);
      if ( railway == 'Y')
         values[18] += length;

      // riparian, intertidal
      int ripPct = 0;
      pSZLayer->GetData(i, m_colRipPct, ripPct);
      values[19] += ripPct * length;

      float ripFt = 0;
      pSZLayer->GetData(i, m_colRipFt, ripFt);
      values[20] += ripFt;

      float itzWidth = 0;
      pSZLayer->GetData(i, m_colItzWidth, itzWidth);
      values[21] += itzWidth * length / FT2_PER_MI2;

      // bio units
      const float cf = 0.90f;
      const float pf = 0.25f;
      TCHAR cls;
      pSZLayer->GetData(i, m_colVerUnit, cls);
      switch (cls) {
         case 'M':   values[22] += cf * length;  break;
         case 'N':   values[22] += cf * length;  break;
         case 'W':   values[22] += cf * length;  break;
         }

      pSZLayer->GetData(i, m_colGraUnit, cls);
      switch (cls) {
         case 'C':   values[23] += cf * length;  break;
         case 'P':   values[23] += pf * length;  break;
         }

      pSZLayer->GetData(i, m_colSedUnit, cls);
      switch (cls) {
         case 'C':   values[24] += cf * length;  break;
         case 'P':   values[24] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colTriUnit, cls);
      switch (cls) {
         case 'C':   values[25] += cf * length;  break;
         case 'P':   values[25] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colSalUnit, cls);
      switch (cls) {
         case 'C':   values[26] += cf * length;  break;
         case 'P':   values[26] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colSpaUnit, cls);
      switch (cls) {
         case 'C':   values[27] += cf * length;  break;
         case 'P':   values[27] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colFucUnit, cls);
      switch (cls) {
         case 'C':   values[28] += cf * length;  break;
         case 'P':   values[28] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colBarUnit, cls);
      switch (cls) {
         case 'C':   values[29] += cf * length;  break;
         case 'P':   values[29] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colBmuUnit, cls);
      switch (cls) {
         case 'C':   values[30] += cf * length;  break;
         case 'P':   values[30] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colUlvUnit, cls);
      switch (cls) {
         case 'C':   values[31] += cf * length;  break;
         case 'P':   values[31] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colGcaUnit, cls);
      switch (cls) {
         case 'C':   values[32] += cf * length;  break;
         case 'P':   values[32] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colOysUnit, cls);
      switch (cls) {
         case 'C':   values[33] += cf * length;  break;
         case 'P':   values[33] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colMusUnit, cls);
      switch (cls) {
         case 'C':   values[34] += cf * length;  break;
         case 'P':   values[34] += pf * length;  break;
         }

      pSZLayer->GetData(i, m_colRedUnit, cls);
      switch (cls) {
         case 'C':   values[35] += cf * length;  break;
         case 'P':   values[35] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colCalUnit, cls);
      switch (cls) {
         case 'C':   values[36] += cf * length;  break;
         case 'P':   values[36] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colSbrUnit, cls);
      switch (cls) {
         case 'C':   values[37] += cf * length;  break;
         case 'P':   values[37] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colSarUnit, cls);
      switch (cls) {
         case 'C':   values[38] += cf * length;  break;
         case 'P':   values[38] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colChbUnit, cls);
      switch (cls) {
         case 'C':   values[39] += cf * length;  break;
         case 'P':   values[39] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colSurUnit, cls);
      switch (cls) {
         case 'C':   values[40] += cf * length;  break;
         case 'P':   values[40] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colDenUnit, cls);
      switch (cls) {
         case 'C':   values[41] += cf * length;  break;
         case 'P':   values[41] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colZosUnit, cls);
      switch (cls) {
         case 'C':   values[42] += cf * length;  break;
         case 'P':   values[42] += pf * length;  break;
         }
      pSZLayer->GetData(i, m_colNerUnit, cls);
      switch (cls) {
         case 'C':   values[43] += cf * length;  break;
         case 'P':   values[43] += pf * length;  break;
         }

      pSZLayer->GetData(i, m_colMacUnit, cls);
      switch (cls) {
         case 'C':   values[44] += cf * length;  break;
         case 'P':   values[44] += pf * length;  break;
         }
      }  // end of: for each IDU


   // normalize as needed
   values[1] /= FT_PER_MILE;
   values[2] /= FT_PER_MILE;
   values[3] /= FT_PER_MILE;

   values[4] /= totalLength;
   values[5] /= totalLength;
   values[6] /= totalLength;
   values[7] /= totalLength;

   values[8] /= FT_PER_MILE;
   values[9] /= FT_PER_MILE;
   values[10] /= FT_PER_MILE;
   values[11] /= FT_PER_MILE;
   values[12] /= FT_PER_MILE;
   values[13] /= FT_PER_MILE;

   values[18] /= FT_PER_MILE;
   values[19] /= totalLength;
   values[20] /= FT_PER_MILE;

   for (int i = 22; i < 45; i++)
      values[i] /= FT_PER_MILE;

   this->m_dataObj.AppendRow(values, SZCOLS);

   return true;
   }






