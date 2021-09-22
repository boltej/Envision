// PSWCP.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "PSWCP.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>
#include <PathManager.h>
#include <EnvConstants.h>
#include <DeltaArray.h>
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//using namespace std;

extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new PSWCP; }

///////////////////////////////////////////////////////
//                 PSWCP
///////////////////////////////////////////////////////

const short LULC_A = 0;
const short LULC_A_AG = 1;
const short LULC_A_FOREST = 2;
const short LULC_A_DEVELOPED = 3;

const short LULC_B = 1;




/*
11 Household, single family units 100 1000
12 Household, 2 - 4 units 100 1000
13 Household, multiunits(5 or more) 100 1000
14 Residential condominiums 100 1000
15 Mobile home parks or courts 100 1000
16 Hotels / motels 1000 1000
17 Institutional lodging 1000 1000
18 All other residential not elsewhere coded 1000 1000
19 Vacation and cabin 1000 1000
21 Food and kindred products 1000 1000
22 Textile mill products 1000 1000
23 Apparel and other finished products made from fabrics, leather, and similar 1000 1000
24 Lumber and wood products(except furniture) 1000 1000
25 Furniture and fixtures 1000 1000
26 Paper and allied products 1000 1000
27 Printing and publishing 1000 1000
28 Chemicals 1000 1000
29 Petroleum refining and related industries 1000 1000
30 Rubber and miscellaneous plastic products 1000 1000
31 Leather and leather products 1000 1000
32 Stone, clayand glass products 1000 1000
33 Primary metal industries 1000 1000
34 Fabricated metal products 1000 1000
35 Professional scientific, and controlling instruments; photographicand optical 1000 1000
39 Miscellaneous manufacturing 1000 1000
41 Railroad / transit transportation 1000 1000
42 Motor vehicle transportation 1000 1000
43 Aircraft transportation 100 1000
44 Marine craft transportation 1000 1000
45 Highway and street right of way 1000 1000
46 Automobile parking 1000 1000
47 Communication 1000 1000
48 Utilities 100 1000
49 Other transportation, communication, and utilities not classified elsewhere 1000 1000
50 Condominiums - other than residential condominiums 1000 1000
51 Wholesale trade 1000 1000
52 Retail trade - building materials, hardware, and farm equipment 1000 1000
53 Retail trade - general merchandise 1000 1000
54 Retail trade - food 1000 1000
55 Retail trade - automotive, marine craft, aircraft, and accessories 1000 1000
56 Retail trade - apparel and accessories 1000 1000
57 Retail trade - furniture, home furnishingsand equipment 1000 1000
58 Retail trade - eating and drinking 1000 1000
59 Other retail trade 1000 1000
61 Finance, insurance, and real estate services 1000 1000
62 Personal services 1000 1000
63 Business services 1000 1000
64 Repair services 1000 1000
65 Professional services 1000 1000
66 Contract construction services 1000 1000

67 Governmental services 100 1000
68 Educational services 100 1000
69 Miscellaneous services 100 1000
71 Cultural activities and nature exhibitions 100 1000
72 Public assembly 100 1000
73 Amusements 100 1000
74 Recreational activities 100 1000
75 Resorts and group camps 100 1000
76 Parks 100 1000
79 Other cultural, entertainmentand recreational 100 100
81 Agriculture(not classified under current use law) 100 100
82 Agriculture related activities 100 100
83 Agriculture classified under current use chapter 84.34 RCW 100 100
84 Fishing activities and related services 100 100
85 Mining activities and related services 100 1000
88 Designated forest land under chapter 84.33 RCW 10 10
89 Other resource production 10 1000
91 Undeveloped land 10 1000
92 Noncommercial forest 10 10
93 Water areas 0 0
94 Open space land classified under chapter 84.34 RCW 10 10
95 Timberland classified under chapter 84.34 RCW 10 10
99 Other undeveloped land 10 1000
*/



TABLECOL colInfo[] = {
      { WQ_DB_TABLE, NULL, -1, "AU_ID"     },
      { WQ_DB_TABLE, NULL, -1, "LG_M1"     },
      { WQ_DB_TABLE, NULL, -1, "LG_M2"     },
      { WQ_DB_TABLE, NULL, -1, "ACRES"  },
      { WQ_DB_TABLE, NULL, -1, "SQ_MILES"  },
      { WQ_DB_TABLE, NULL, -1, "STRM_MI"   },
      //{ WQ_DB_TABLE, NULL, -1, "ASMI"      },  // not using?
      { WQ_DB_TABLE, NULL, -1, "RE"        },
      { WQ_DB_TABLE, NULL, -1, "K"         },
      { WQ_DB_TABLE, NULL, -1, "SLP"       },
      { WQ_DB_TABLE, NULL, -1, "LH"        },
      { WQ_DB_TABLE, NULL, -1, "ERST_MI"   },
      { WQ_DB_TABLE, NULL, -1, "SRP"       },
      { WQ_DB_TABLE, NULL, -1, "SRM"       },
      { WQ_DB_TABLE, NULL, -1, "RDN_MI"    },

      { WQ_M1_TABLE, NULL, -1, "MW"        },
      { WQ_M1_TABLE, NULL, -1, "RDN"       },
      { WQ_M1_TABLE, NULL, -1, "S_MW"      },
      { WQ_M1_TABLE, NULL, -1, "ERST"      },
      { WQ_M1_TABLE, NULL, -1, "SE"        },
      { WQ_M1_TABLE, NULL, -1, "S_SE"      },
      { WQ_M1_TABLE, NULL, -1, "CE"        },
      { WQ_M1_TABLE, NULL, -1, "S_CE"      },
      { WQ_M1_TABLE, NULL, -1, "S_M1"      },
      { WQ_M1_TABLE, NULL, -1, "S_M1_CAL"  },
      { WQ_M1_TABLE, NULL, -1, "P_SR"       },
      { WQ_M1_TABLE, NULL, -1, "PSO"       },
      { WQ_M1_TABLE, NULL, -1, "PSI"       },
      { WQ_M1_TABLE, NULL, -1, "P_SO"      },
      { WQ_M1_TABLE, NULL, -1, "P_SI"      },
      { WQ_M1_TABLE, NULL, -1, "P_M1"      },
      { WQ_M1_TABLE, NULL, -1, "P_M1_CAL"  },
      { WQ_M1_TABLE, NULL, -1, "MSI"       },
      { WQ_M1_TABLE, NULL, -1, "M_SRM"      },
      { WQ_M1_TABLE, NULL, -1, "M_M1"      },
      { WQ_M1_TABLE, NULL, -1, "M_M1_CAL"  },
      { WQ_M1_TABLE, NULL, -1, "NSI"       },
      { WQ_M1_TABLE, NULL, -1, "N_SI"       },
      { WQ_M1_TABLE, NULL, -1, "N_M1"      },
      { WQ_M1_TABLE, NULL, -1, "N_M1_CAL"  },
      { WQ_M1_TABLE, NULL, -1, "PA_SI"     },
      { WQ_M1_TABLE, NULL, -1, "PA_M1"     },
      { WQ_M1_TABLE, NULL, -1, "PA_M1_CAL" },

      { WQ_RP_TABLE, NULL, -1, "SED_RP"    },
      { WQ_RP_TABLE, NULL, -1, "P_RP"      },
      { WQ_RP_TABLE, NULL, -1, "ME_RP"     },
      { WQ_RP_TABLE, NULL, -1, "N_RP"      },
      { WQ_RP_TABLE, NULL, -1, "Pa_RP"     },
      //{ WQ_RP_TABLE, NULL, -1, "S_M1_CAL"  },  duplicates
      //{ WQ_RP_TABLE, NULL, -1, "P_M1_CAL"  },
      //{ WQ_RP_TABLE, NULL, -1, "M_M1_CAL"  },
      //{ WQ_RP_TABLE, NULL, -1, "N_M1_CAL"  },

      { WF_DB1_TABLE, NULL, -1, "AV_PREC"  },
      { WF_DB1_TABLE, NULL, -1, "SRS_PCT"  },
      { WF_DB1_TABLE, NULL, -1, "DPWT_PCT" },
      { WF_DB1_TABLE, NULL, -1, "LK_PCT"   },
      { WF_DB1_TABLE, NULL, -1, "UC_MI"    },
      { WF_DB1_TABLE, NULL, -1, "MC_MI"    },
      { WF_DB1_TABLE, NULL, -1, "PERMH"    },
      { WF_DB1_TABLE, NULL, -1, "PERML"    },
      //{ WF_DB1_TABLE, NULL, -1, "ACRES"    }, duplicate
      { WF_DB1_TABLE, NULL, -1, "UCHP_AREA"},
      { WF_DB1_TABLE, NULL, -1, "SLPWT_AC" },
      { WF_DB1_TABLE, NULL, -1, "SLPWT_PCT"},

      { WF_DB2_TABLE, NULL, -1, "FL_PCT"   },
      //{ WF_DB2_TABLE, NULL, -1, "LK_PCT"   },
      { WF_DB2_TABLE, NULL, -1, "IMP_AC"   },
      { WF_DB2_TABLE, NULL, -1, "IMP_PCT"  },
      { WF_DB2_TABLE, NULL, -1, "W_UR_AC"  },
      { WF_DB2_TABLE, NULL, -1, "W_RU_AC"  },
      { WF_DB2_TABLE, NULL, -1, "UC_ALT_MI"},
      { WF_DB2_TABLE, NULL, -1, "U_AC"     },
      { WF_DB2_TABLE, NULL, -1, "BU_AC"    },
      { WF_DB2_TABLE, NULL, -1, "LI_AC"    },
      { WF_DB2_TABLE, NULL, -1, "RD_MI"    },
      { WF_DB2_TABLE, NULL, -1, "RD_DEN"   },
      { WF_DB2_TABLE, NULL, -1, "WELL_CNT" },
      { WF_DB2_TABLE, NULL, -1, "WELL_DEN" },
      { WF_DB2_TABLE, NULL, -1, "UCHP_U"   },
      { WF_DB2_TABLE, NULL, -1, "UCHP_R"   },
      { WF_DB2_TABLE, NULL, -1, "SLPW_U"   },
      { WF_DB2_TABLE, NULL, -1, "SLPW_R"   },

      { WF_M1_TABLE, NULL, -1, "I_SS"      },
      { WF_M1_TABLE, NULL, -1, "WLS"       },
      { WF_M1_TABLE, NULL, -1, "WT_LK"     },
      { WF_M1_TABLE, NULL, -1, "P"         },
      { WF_M1_TABLE, NULL, -1, "RS"        },
      { WF_M1_TABLE, NULL, -1, "IDE"       },
      { WF_M1_TABLE, NULL, -1, "I_DE"      },
      { WF_M1_TABLE, NULL, -1, "RECHH"     },
      { WF_M1_TABLE, NULL, -1, "RECHL"     },
      { WF_M1_TABLE, NULL, -1, "IR"        },
      { WF_M1_TABLE, NULL, -1, "I_R"       },
      { WF_M1_TABLE, NULL, -1, "SD"        },
      { WF_M1_TABLE, NULL, -1, "SWD"       },
      { WF_M1_TABLE, NULL, -1, "IDI"       },
      { WF_M1_TABLE, NULL, -1, "I_DI"      },

      { WF_M2_TABLE, NULL, -1, "UW"        },
      { WF_M2_TABLE, NULL, -1, "RW"        },
      { WF_M2_TABLE, NULL, -1, "DW"        },
      { WF_M2_TABLE, NULL, -1, "D_WS"      },
      { WF_M2_TABLE, NULL, -1, "UDS"       },
      { WF_M2_TABLE, NULL, -1, "MDS"       },
      { WF_M2_TABLE, NULL, -1, "DST"       },
      { WF_M2_TABLE, NULL, -1, "D_STS"     },
      { WF_M2_TABLE, NULL, -1, "D_RD"      },
      { WF_M2_TABLE, NULL, -1, "D_WEL"     },
      { WF_M2_TABLE, NULL, -1, "UUS"       },
      { WF_M2_TABLE, NULL, -1, "URS"       },
      { WF_M2_TABLE, NULL, -1, "SWU" },
      { WF_M2_TABLE, NULL, -1, "SWR" },
      { WF_M2_TABLE, NULL, -1, "WD" },
      { WF_M2_TABLE, NULL, -1, "D_WD" },

      { HAB_TERR_TABLE, NULL, -1, "New_AU" },
      { HAB_TERR_TABLE, NULL, -1, "Integ_Inde" },
      { HAB_TERR_TABLE, NULL, -1, "norm_PHS"   },
      { HAB_TERR_TABLE, NULL, -1, "norm_oakgr" },
      { HAB_TERR_TABLE, NULL, -1, "ovrall_IND" },

      { HCI_CATCH_TABLE, NULL, -1, "HCI_CAT" },

      { NULL_TABLE,  NULL, -1, NULL }
   };

HAB_SCORE habScoreInfo[] = {
         { LULC_A, LULC_A_AG, LULC_A_DEVELOPED, 0.5f },  // ag -> developed
         { LULC_A, LULC_A_AG, LULC_A_DEVELOPED, 0.5f },  // ag -> developed
         { -1,-1,-1,0.0f }
   };



CMapStringToPtr colMap;    // key = fieldname, value=ptr to COL

CMap<int, int, LULCINFO*, LULCINFO*> lulcMap;   // key = lulc code, value =ptr to LULC_INFO

//typedef std::pair<LONG,LONG> KEY;
std::map<HAB_SCORE,float> habScoreMap;

PSWCP::PSWCP(void)
   : EnvModelProcess()
   , m_pIDULayer(NULL)
   , m_col_IDU_AUW_ID(-1)
   , m_col_IDU_WqS_rp(-1)
   , m_col_IDU_WqP_rp(-1)
   , m_col_IDU_WqMe_rp(-1)
   , m_col_IDU_WqN_rp(-1)
   , m_col_IDU_WqPa_rp(-1)

   , m_col_IDU_Hab_IntIndex(-1)
   , m_col_IDU_Hab_PHS(-1)
   , m_col_IDU_Hab_OakGrove(-1)
   , m_col_IDU_Hab_OverallIndex(-1)
   , m_col_IDU_LULC_A(-1)
   , m_col_IDU_LULC_B(-1)
   , m_col_IDU_CONSERVE(-1)
   , m_col_IDU_AREA(-1)

   , m_pAUWLayer(NULL)
   , m_pAUHLayer(NULL)
   , m_pTerrHabLayer(NULL)

   , m_pWqDbTable(NULL)
   , m_pWqM1Table(NULL)
   , m_pWqRpTable(NULL)
   , m_pWfDb1Table(NULL)
   , m_pWfDb2Table(NULL)
   , m_pWfM1Table(NULL)
   , m_pWfM2Table(NULL)
   , m_pWfRpTable(NULL)
   , m_pHCITable(NULL)
   , m_pHPCTable(NULL)
   , m_AUWIndex_IDU()
   , m_AUHIndex_IDU()
   , m_HCIIndex_IDU()
   , m_numHCICat(-1)
   {
   int i = 0;
   while (colInfo[i].table >= 0)
      {
      void* pCol;
      if (colMap.Lookup(colInfo[i].field, pCol) == TRUE)
         {
         CString msg;
         msg.Format("  PSWCP: Duplicate table field encountered: Table: %i, field: %s", (int)colInfo[i].table, (LPCTSTR)colInfo[i].field);
         Report::LogInfo(msg);
         }
      else
         {
         // add to map
         }
      colMap[colInfo[i].field] = &colInfo[i];
      i++;
      }

   lulcMap[0] = new LULCINFO("Background(0)", 10, 1000);
   lulcMap[1] = new LULCINFO("Unclassified (1)", 10, 1000);
   lulcMap[2] = new LULCINFO("Developed,  High  Intensity (2)", 100, 1000);
   lulcMap[3] = new LULCINFO("Developed, Medium Intensity (3)", 100, 1000);
   lulcMap[4] = new LULCINFO("Developed, Low Intensity (4)", 100, 1000);
   lulcMap[5] = new LULCINFO("Developed,  Open  Space (5)", 100, 1000);
   lulcMap[6] = new LULCINFO("Agricultural Land Cultivated Crops (6)", 100, 100);
   lulcMap[7] = new LULCINFO("Pasture/Hay (7)", 100, 100);
   lulcMap[8] = new LULCINFO("Grassland/Herbaceous (8)", 100, 100);
   lulcMap[9] = new LULCINFO("Deciduous Forest (9)", 10, 10);
   lulcMap[10] = new LULCINFO("Evergreen  Forest (10)", 10, 10);
   lulcMap[11] = new LULCINFO("Mixed Forest (11)", 10, 10);
   lulcMap[12] = new LULCINFO("Scrub/Shrub (12)", 10, 100);
   lulcMap[13] = new LULCINFO("Palustrine  Forested  Wetland (13)", 10, 10);
   lulcMap[14] = new LULCINFO("Palustrine Scrub/Shrub Wetland (14)", 10, 10);
   lulcMap[15] = new LULCINFO("Palustrine Emergent Wetland (Persistent) (15)", 10, 10);
   lulcMap[16] = new LULCINFO("Estuarine Forested Wetland (16)", 10, 10);
   lulcMap[17] = new LULCINFO("Estuarine Scrub/Shrub Wetland (17)", 10, 10);
   lulcMap[18] = new LULCINFO("Estuarine Emergent Wetland (18)", 10, 10);
   lulcMap[19] = new LULCINFO("Unconsolidated Shore (19)", 0, 0);
   lulcMap[20] = new LULCINFO("Barren Land (20)", 0, 0);
   lulcMap[21] = new LULCINFO("Open Water (21)", 0, 0);
   lulcMap[22] = new LULCINFO("Palustrine  Aquatic Bed (22)", 0, 0);
   lulcMap[23] = new LULCINFO("Estuarine  Aquatic  Bed (23)", 0, 0);
   lulcMap[24] = new LULCINFO("Tundra (24)", 0, 0);
   lulcMap[25] = new LULCINFO("Ice/Snow  (25)", 0, 0);
   
   
   //habScoreMap.insert(std::make_pair(KEY(MAKELONG(LULC_A,LULC_A_AG),MAKELONG(LULC_A,LULC_A_DEVELOPED)), -0.3f));

   
   
   }

PSWCP::~PSWCP(void)
   {
   if (m_pWqDbTable != NULL)
      delete m_pWqDbTable;
   if (m_pWqM1Table != NULL)
      delete m_pWqM1Table;
   if (m_pWqRpTable != NULL)
      delete m_pWqRpTable;

   if (m_pWfDb1Table != NULL)
      delete m_pWfDb1Table;
   if (m_pWfDb2Table != NULL)
      delete m_pWfDb2Table;
   if (m_pWfM1Table != NULL)
      delete m_pWfM1Table;
   if (m_pWfM2Table != NULL)
      delete m_pWfM2Table;
   if (m_pWfRpTable != NULL)
      delete m_pWfRpTable;

   if (m_pHCITable != NULL)
      delete m_pHCITable;
   if (m_pHPCTable != NULL)
      delete m_pHPCTable;

   // delete allocated LULCINFO's
   POSITION p = lulcMap.GetStartPosition();
   LULCINFO* pInfo = NULL;
   int lulcCode;

   while (p != NULL) {
      lulcMap.GetNextAssoc(p, lulcCode, pInfo);
      delete pInfo;
      }

   lulcMap.RemoveAll();
   }

// override API Methods
bool PSWCP::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   if (!ok)
      return FALSE;

   m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // load csv tables for various submodels
   LoadTables();

   // water models
 //  InitWaterAssessments(pEnvContext);

   // habitat models
 //  InitHabAssessments(pEnvContext);

   // HCI
   InitHCIAssessment(pEnvContext);


   return true;
   }

bool PSWCP::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   // reset annual output variables

   return true;
   }

bool PSWCP::Run(EnvContext* pEnvContext)
   {
   int currentYear = pEnvContext->currentYear;

   //RunWQAssessment(pEnvContext);
   //
   //// water flow model
   //RunWFAssessment(pEnvContext);
   //
   //// habitat
   //RunHabAssessment(pEnvContext);
   //
   //// HCI
   RunHCIAssessment(pEnvContext);



   return true;
   }



bool PSWCP::InitWaterAssessments(EnvContext* pEnvContext)
   {
   this->CheckCol(m_pIDULayer, m_col_IDU_AUW_ID, "AUW_ID", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqS_rp, "WqS_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqP_rp, "WqP_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqMe_rp, "WqMe_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqN_rp, "WqN_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqPa_rp, "WqPa_rp", TYPE_STRING, CC_MUST_EXIST);

   // load/create index for getting IDUs for each AU from the idu layer
   CString indexPath;
   if (PathManager::FindPath("PSWCP/WQ_RP.idx", indexPath) < 0)
      {
      // doesn't exist, build (and save) it
      Report::LogInfo("  PSWCP: Building index WQ_RP.idx");
      m_AUWIndex_IDU.BuildIndex(m_pIDULayer->m_pDbTable, m_col_IDU_AUW_ID);

      indexPath = PathManager::GetPath(PM_PROJECT_DIR);
      indexPath += "PSWCP/WQ_RP.idx";
      m_AUWIndex_IDU.WriteIndex(indexPath);
      m_AUWIndex_IDU.WriteIndexText(indexPath + ".txt");
      }
   else
      {
      m_AUWIndex_IDU.ReadIndex(m_pIDULayer->m_pDbTable, indexPath);
      }

   // write WQ_RP values to IDUs by iterating through the WQ-RP table, writing
   // values to the IDU's using the index for 
   CUIntArray recordArray;
   for (int row = 0; row < m_pWqRpTable->GetRowCount(); row++)
      {
      // get the AU_ID for this record
      int auID = 0;
      GetTableValue(WQ_DB_TABLE, "AU_ID", row, auID);

      int count = m_AUWIndex_IDU.GetRecordArray(m_col_IDU_AUW_ID, VData(auID), recordArray);

      CString s_rp, p_rp, me_rp, n_rp, pa_rp;
      GetTableValue(WQ_RP_TABLE, "SED_RP", row, s_rp);
      GetTableValue(WQ_RP_TABLE, "P_RP", row, p_rp);
      GetTableValue(WQ_RP_TABLE, "ME_RP", row, me_rp);
      GetTableValue(WQ_RP_TABLE, "N_RP", row, n_rp);
      GetTableValue(WQ_RP_TABLE, "Pa_RP", row, pa_rp);

      for (int j = 0; j < count; j++)
         {
         int idu = recordArray[j];

         if (idu < m_pIDULayer->GetRecordCount())  // for partial loads
            {
            m_pIDULayer->SetData(idu, m_col_IDU_WqS_rp, s_rp);
            m_pIDULayer->SetData(idu, m_col_IDU_WqP_rp, p_rp);
            m_pIDULayer->SetData(idu, m_col_IDU_WqMe_rp, me_rp);
            m_pIDULayer->SetData(idu, m_col_IDU_WqN_rp, n_rp);
            m_pIDULayer->SetData(idu, m_col_IDU_WqPa_rp, pa_rp);
            }
         }
      }  // end of: for each row in WQ_RP table


   // populate IDU columns
   //this->CheckCol(m_pIDULayer, m_col_WQ_AU, "S_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);


   //// initialize column info
   //pIDULayer->SetColData(m_colIduRepairYrs, VData(-1), true);
   //pIDULayer->SetColData(m_colIduBldgStatus, VData(0), true);

   //// add output variables
   ////this->AddOutputVar("Annual Repair Expenditures", m_annualRepairCosts, "");
   //this->AddOutputVar("Habitable Structures (count)", m_nInhabitableStructures, "");
   //this->AddOutputVar("Habitable Structures (pct)", m_pctInhabitableStructures, "");

   return true;
   }


bool PSWCP::InitHabAssessments(EnvContext* pEnvContext)
   {
   this->CheckCol(m_pIDULayer, m_col_IDU_AUH_ID, "AUH_ID", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_Hab_IntIndex, "HabIntInde", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_Hab_PHS, "HabPHS", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_Hab_OakGrove, "HabOakGrov", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_Hab_OverallIndex, "HabOvrInde", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_LULC_B, "LULC_B", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_LULC_A, "LULC_A", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_CONSERVE, "CONSERVE", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_AREA, "AREA", TYPE_FLOAT, CC_MUST_EXIST);

   // load/create index for getting IDUs for each AU from the idu layer
   CString indexPath;
   if (PathManager::FindPath("PSWCP/HAB_TERR.idx", indexPath) < 0)
      {
      // doesn't exist, build (and save) it
      Report::LogInfo("  PSWCP: Building index HAB_TERR.idx");
      m_AUHIndex_IDU.BuildIndex(m_pIDULayer->m_pDbTable, m_col_IDU_AUH_ID);

      indexPath = PathManager::GetPath(PM_PROJECT_DIR);
      indexPath += "PSWCP/HAB_TERR.idx";
      m_AUHIndex_IDU.WriteIndex(indexPath);
      m_AUHIndex_IDU.WriteIndexText(indexPath + ".txt");
      }
   else
      {
      m_AUHIndex_IDU.ReadIndex(m_pIDULayer->m_pDbTable, indexPath);
      }

   // write HAB values to IDUs by iterating through the hab table, writing
   // values to the IDU's using the index for 
   CUIntArray recordArray;
   int missingCount = 0;
   m_pIDULayer->SetColNull(m_col_IDU_Hab_IntIndex);
   m_pIDULayer->SetColNull(m_col_IDU_Hab_PHS);
   m_pIDULayer->SetColNull(m_col_IDU_Hab_OakGrove);
   m_pIDULayer->SetColNull(m_col_IDU_Hab_OverallIndex);
   
   int setIDUCount = 0;
   int usedAUCount = 0;

   for (int row = 0; row < m_pHabTerrTable->GetRowCount(); row++)
      {
      // get the AU_ID for this record
      int auID = 0;
      GetTableValue(HAB_TERR_TABLE, "New_AU", row, auID);

      if (auID == 0)
         continue;

      // Get the IDU's with this AU_ID value in the IDU [AUH_ID] column
      int count = m_AUHIndex_IDU.GetRecordArray(m_col_IDU_AUH_ID, VData(auID), recordArray);

      if (count < 0)
         missingCount++;
      else
         usedAUCount++;

      float integ_Inde, norm_PHS, norm_oakgr, ovrall_IND;
      GetTableValue(HAB_TERR_TABLE, "Integ_Inde", row, integ_Inde);   // 0-1
      GetTableValue(HAB_TERR_TABLE, "norm_PHS",   row, norm_PHS  );   // 0-100
      GetTableValue(HAB_TERR_TABLE, "norm_oakgr", row, norm_oakgr);   // 0-100
      GetTableValue(HAB_TERR_TABLE, "ovrall_IND", row, ovrall_IND);   // 0-100

      // iterate through associated IDU's writing values as appropriate
      for (int j = 0; j < count; j++)
         {
         int idu = recordArray[j];

         if (idu < m_pIDULayer->GetRecordCount())  // for partial loads
            {
            int lulcA = 0;
            m_pIDULayer->GetData(idu, m_col_IDU_LULC_A, lulcA);

            if (lulcA == 3 || lulcA == 9 || lulcA == 0)  // developed or water
               {
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_IntIndex, -1);       // habitat integrity index
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_PHS, -1);            // priority habitat for sppecies of interest
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_OakGrove, -1);       // oak grove habitat
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_OverallIndex, -1);   // overall (combined) index
               }
            else
               {
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_IntIndex, integ_Inde);       // habitat integrity index
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_PHS, norm_PHS);              // priority habitat for sppecies of interest
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_OakGrove, norm_oakgr);       // oak grove habitat
               m_pIDULayer->SetData(idu, m_col_IDU_Hab_OverallIndex, ovrall_IND);   // overall (combined) index
               setIDUCount++;
               }
            }
         }
      }  // end of: for each row in table

   if (missingCount > 0)
      {
      CString msg;
      msg.Format("   PSWCP: AUH_index found %i AU with no associated IDUs", missingCount);
      Report::LogWarning(msg);
      }

   CString msg;
   msg.Format("   PSWCP: Set %i IDUs from %i AUH", setIDUCount, usedAUCount );
   Report::LogWarning(msg);
   return true;
   }

bool PSWCP::InitHCIAssessment(EnvContext* pEnvContext)
   {
      this->CheckCol(m_pIDULayer, m_col_IDU_HCI_Dist, "HCI_Dist", TYPE_FLOAT, CC_MUST_EXIST);
      this->CheckCol(m_pIDULayer, m_col_IDU_HCI_Shed, "HCI_Cat", TYPE_INT, CC_MUST_EXIST);
      this->CheckCol(m_pIDULayer, m_col_IDU_HCI_Value, "HCI", TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(m_pIDULayer, m_col_IDU_LULC_B, "LULC_B", TYPE_INT, CC_MUST_EXIST);
      // load databases
      LoadTable(HCI_VALUE_TABLE);
      LoadTable(HCI_CATCH_TABLE);


      /*
      // update colInfos
      int i = 0;
      while (colInfo[i].table != NULL_TABLE)
      {
         switch (colInfo[i].table)
         {
         case HCI_CATCH_TABLE:  colInfo[i].pTable = m_pHCITable;   break;
         }

         colInfo[i].col = colInfo[i].pTable->GetCol(colInfo[i].field);
         i++;
      }
      */
      // load/create index for getting IDUs for each HCI Watershed from the idu layer
      CString indexPath;
      if (PathManager::FindPath("PSWCP/HCI_RP.idx", indexPath) < 0)
         {
         // doesn't exist, build (and save) it
         Report::LogInfo("  PSWCP: Building index HCI_RP.idx");
         m_numHCICat = m_HCIIndex_IDU.BuildIndex(m_pIDULayer->m_pDbTable, m_col_IDU_HCI_Shed);

         indexPath = PathManager::GetPath(PM_PROJECT_DIR);
         indexPath += "PSWCP/HCI_RP.idx";
         m_HCIIndex_IDU.WriteIndex(indexPath);
         }
      else
         {
         m_numHCICat = m_HCIIndex_IDU.ReadIndex(m_pIDULayer->m_pDbTable, indexPath);
         }



   // add output variables
   //this->AddOutputVar("Annual Repair Expenditures", m_annualRepairCosts, "");


      return true;
      }


bool PSWCP::RunWFAssessment(EnvContext* pEnvContext)
   {
   // Model 1:  Importance Value
   // (Precip + Timing of Delivery) + (Surface Storage) + subsurface flow + (recharge + discharge);

   SolveWfM1WatDel();
   SolveWfM1SurfStorage();
   SolveWfM1RechargeDischarge();

   // Model 2: degradation
   SolveWfM2WatDel();
   SolveWfM2SurfStorage();
   SolveWfM2Recharge();    // NEEDS WORK, DOCS UNCLEAR
   SolveWfM2Discharge();
   SolveWfM2EvapTrans();

   return true;
   }


bool PSWCP::RunWQAssessment(EnvContext* pEnvContext)
   {
   SolveWqM1Sed();
   SolveWqM1Phos();
   SolveWqM1Metals();
   SolveWqM1Nit();
   SolveWqM1Path();

   // SolveWqM2();
   return true;
   }


bool PSWCP::RunHabAssessment(EnvContext* pEnvContext)
   {
   SolveHabTerr(pEnvContext);

   // Set REL_Impact = 1000 for all records, then recalculate REL_Impact for following LandUse codes as follows :
   // Commercialand industrial land uses; 249 = Dept of Corrections, 45 = Highways
    /*
   if ( LandUse in(16, 17, 18, 19, . . ., 41, 42, 44, 45, 46, 47, 49, 50, 51, . . ., 66, 249)
      Then Rel_Impact = 1000
#----------------------------------------------------------------------------------------------------------------------------------
      # residential and recreational land uses
      # not in Woodland / Prairie Mosaic Zone
      If((LandUse <= 15) OR(LandUse in(43, 48, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 85)))
      AND(veg_zone > 1) Then
      Rel_Impact = -900 * [PR_High_Tr] + 1000 # relative impact score between 100 and 1000
      # in Woodland / Prairie Mosaic Zone
      if ((LandUse <= 15) OR(LandUse in(43, 48, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 85)))
         AND(veg_zone = 1) Then
         Rel_Impact = -900 * (0.33 * [PR_Low_Gra] + [PR_TreShrb]) + 1000 # [PR_Low_Gra] + [PR_TreShrb] < 1
#----------------------------------------------------------------------------------------------------------------------------------
         # undeveloped land use and not presently assigned codes(just to be safe)
         # not in Woodland / Prairie Mosaic Zone
         If(LandUse in(86, 89, 91, 96, 97, 98, 99)) AND(veg_zone > 1) Then
         Rel_Impact = -990 * [PR_TreShrb] + 1000 # relative impact score between 10 and 1000
         # in Woodland / Prairie Mosaic Zone
         If(LandUse in(86, 89, 91, 96, 97, 98, 99)) AND(veg_zone = 1) Then
         REl_Impact = -990 * (0.67 * [PR_Low_Gra] + [PR_TreShrb]) + 1000 # score between 10 and 1000
#----------------------------------------------------------------------------------------------------------------------------------
         If LandUse in(81, 82, 83, 84) Then REL_Impact = 100 # agriculture
         # private timberland, commercial forest, WDFW(221, 222), and open space on Dept of Defense(341)
         If LandUse in(87, 88, 92, 94, 95, 221, 222, 341) Then REL_Impact = 10
         If LandUse in(101, 211, 351) Then REL_Impact = 8 #municipal watersheds, DNR, BLM
         If LandUse in(311, 335) Then REL_Impact = 4 #USFWS, National Forest - recreation or undesignated
         If LandUse in(231, 322) Then REL_Impact = 15 #WA State Parks, National Park - historic park
         If LandUse = 321 Then REL_Impact = 1 #National Parks
         If LandUse in(93, 331, 332) Then REL_Impact = 0 #water, federal wilderness, federal roadless areas  */
   
   return 0;
   }


bool PSWCP::RunHCIAssessment(EnvContext* pEnvContext)
   {
   SolveHCI();
   return true;
   }



//-------------------------------------------------
//-------- sediment export potential --------------
//-------------------------------------------------
int PSWCP::SolveWqM1Sed()
   {
   // normalizing arrays 
   float maxSE[LG_COUNT] = { 0,0,0,0,0 };
   float maxCE[LG_COUNT] = { 0,0,0,0,0 };
   float maxMW[LG_COUNT] = { 0,0,0,0,0 };
   float maxSM1[LG_COUNT] = { 0,0,0,0,0 };
   float maxISS[LG_COUNT] = { 0,0,0,0,0 };

   // pass one - compute normalizing array
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);

      if (lgIndex == LG_NULL)
         continue;

      // collect data for calculations
      float re = 0;     // rainfall erosivity
      float k = 0;
      float slp = 0;    // slope
      float lh = 0;
      float erstMi = 0; // total stream miles
      float streamMiles = 0;
      float areaSqMi = 0;
      GetTableValue(WQ_DB_TABLE, "SQ_MILES", row, areaSqMi);
      GetTableValue(WQ_DB_TABLE, "STRM_MI", row, streamMiles);
      GetTableValue(WQ_DB_TABLE, "RE", row, re);
      GetTableValue(WQ_DB_TABLE, "K", row, k);
      GetTableValue(WQ_DB_TABLE, "SLP", row, slp);
      GetTableValue(WQ_DB_TABLE, "LH", row, lh);
      GetTableValue(WQ_DB_TABLE, "ERST_MI", row, erstMi);

      float asd = streamMiles / areaSqMi;
      float erst = erstMi / streamMiles;

      // sources
      float se = re * k * slp;      // surface erosion
      float mw = lh * asd;          // mass wasting
      float ce = slp * erst;      // channel erosion

      // update values in WQ_M1 table
      SetTableValue(WQ_M1_TABLE, "SE", row, se);
      SetTableValue(WQ_M1_TABLE, "MW", row, mw);
      SetTableValue(WQ_M1_TABLE, "ERST", row, erst);
      SetTableValue(WQ_M1_TABLE, "CE", row, ce);

      // update values in WQ_RP table
      //SetTableValue(WQ_RP_TABLE, "S_M1_CAL", row, sed_m1);

      // check for normalizing max NEED TO ORG BY LANDSCAPE GROUP
      if (ce > maxCE[lgIndex])
         maxCE[lgIndex] = ce;

      if (mw > maxMW[lgIndex])
         mw = maxMW[lgIndex];

      if (se > maxSE[lgIndex])
         maxSE[lgIndex] = se;
      }

   // pass 2
   // normalize and write sed-related  variables
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);

      if (lgIndex == LG_NULL)
         continue;

      float mw = 0, se = 0, ce = 0;
      GetTableValue(WQ_M1_TABLE, "MW", row, mw);
      GetTableValue(WQ_M1_TABLE, "SE", row, se);
      GetTableValue(WQ_M1_TABLE, "CE", row, ce);

      // update normalized values in WQ_M1 table
      float s_mw = mw / maxMW[lgIndex];
      float s_ce = ce / maxCE[lgIndex];
      float s_se = se / maxSE[lgIndex];
      SetTableValue(WQ_M1_TABLE, "S_MW", row, s_mw);
      SetTableValue(WQ_M1_TABLE, "S_SE", row, s_se);
      SetTableValue(WQ_M1_TABLE, "S_CE", row, s_ce);

      // update normalized values in WQ_RP table

      // solve source model
      // sinks
      float i_ss = 0;   // water flow process surface storage subcomponent
      GetTableValue(WF_M1_TABLE, "I_SS", row, i_ss);

      float sed_m1 = (se + mw + ce) - i_ss;        // model 1 (export) score for sediments, unnormalized
      SetTableValue(WQ_M1_TABLE, "S_M1", row, sed_m1);

      if (sed_m1 > maxSM1[lgIndex])
         maxSM1[lgIndex] = sed_m1;
      }

   // pass 3
   // normalize and write s_m1
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);

      if (lgIndex == LG_NULL)
         continue;

      float sed_m1 = 0;
      GetTableValue(WQ_M1_TABLE, "S_M1", row, sed_m1);

      float sed_m1_cal = sed_m1 / maxSM1[lgIndex];
      SetTableValue(WQ_M1_TABLE, "S_M1_CAL", row, sed_m1_cal);
      //SetTableValue(WQ_RP_TABLE, "S_M1_CAL", row, sed_m1_cal);   // duplicate name!
      }

   return 1;
   }


//---------------------------------------------------
//-------- phosphorus export potential --------------
//---------------------------------------------------
int PSWCP::SolveWqM1Phos()
   {
   float maxPE[LG_COUNT] = { 0,0,0,0,0 };
   float maxPSO[LG_COUNT] = { 0,0,0,0,0 };
   float maxPSI[LG_COUNT] = { 0,0,0,0,0 };
   float maxSRP[LG_COUNT] = { 0,0,0,0,0 };
   float maxPM1[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1
   // find max PE values for normalizing
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float pe = 1;     // ????? can't find field "PC" in the datasets(Phosphorus content)
      if (pe > maxPE[lgIndex])
         maxPE[lgIndex] = pe;

      float srp = 0;
      GetTableValue(WQ_DB_TABLE, "SRP", row, srp);
      if (srp > maxSRP[lgIndex])
         maxSRP[lgIndex] = srp;
      }

   // pass 2
   // compute model for each AU
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float pe = 0;     // phosphorus enrichment
      //GetTableValue(WQ_M1_TABLE, "PE", row, pe);   // ???????? NEED TO CALCULATE THIS!!
      float i_pe = pe / maxPE[lgIndex];

      // sources
      float s_se = 0, s_mw = 0, s_ce = 0;
      GetTableValue(WQ_M1_TABLE, "S_MW", row, s_mw);     // mass wasting
      GetTableValue(WQ_M1_TABLE, "S_SE", row, s_se);     // surface erosion
      GetTableValue(WQ_M1_TABLE, "S_CE", row, s_ce);     // channel resion
      float pso = (s_se + s_mw + s_ce + pe);  // un-normalized

      // sinks
      float srp = 0;    // soil retention of phosphorus
      GetTableValue(WQ_DB_TABLE, "SRP", row, srp);
      float p_sr = srp / maxSRP[lgIndex];    // normalized version

      float i_ss = 0;  // surface storage (from water flow process)
      GetTableValue(WF_M1_TABLE, "I_SS", row, i_ss);

      // sink model
      float psi = p_sr + i_ss;  // un-nnormalized 

      // update tables with intermediate values
      SetTableValue(WQ_M1_TABLE, "P_SR", row, p_sr);
      SetTableValue(WQ_M1_TABLE, "PSO", row, pso);   // p source (un-normalized)
      SetTableValue(WQ_M1_TABLE, "PSI", row, psi);
      }

   // pass 3
   // compute final model for each AU
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float pso = 0, psi = 0;
      GetTableValue(WQ_M1_TABLE, "PSO", row, pso);   // p source (un-normalized)
      GetTableValue(WQ_M1_TABLE, "PSI", row, psi);

      float p_so = pso / maxPSO[lgIndex];    // normalized source
      float p_si = psi / maxPSI[lgIndex];    // normalized sink

      float p_m1 = p_so - p_si;
      if (p_m1 > maxPM1[lgIndex])
         maxPM1[lgIndex] = p_m1;

      // update tables with intermediate values
      SetTableValue(WQ_M1_TABLE, "P_SO", row, p_so);   // p source (unnormalized)
      SetTableValue(WQ_M1_TABLE, "P_SI", row, p_si);
      SetTableValue(WQ_M1_TABLE, "P_M1", row, p_m1);
      }

   // pass 4
   // normalize and write p_m1
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);

      if (lgIndex == LG_NULL)
         continue;

      float p_m1 = 0;
      GetTableValue(WQ_M1_TABLE, "P_M1", row, p_m1);

      float p_m1_cal = p_m1 / maxPM1[lgIndex];
      SetTableValue(WQ_M1_TABLE, "P_M1_CAL", row, p_m1_cal);
      //SetTableValue(WQ_RP_TABLE, "P_M1_CAL", row, p_m1_cal);  // duplicate
      }

   return 1;
   }

//---------------------------------------------------
//-------- metals export potential --------------
//---------------------------------------------------
int PSWCP::SolveWqM1Metals()
   {
   float maxMSI[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1
   // find max PE values for normalizing
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float m_srm = 0;     // soil retention - metals
      GetTableValue(WQ_M1_TABLE, "M_SRM", row, m_srm);

      float i_ss = 0;  // surface storage (from water flow process)
      GetTableValue(WF_M1_TABLE, "I_SS", row, i_ss);

      // update tables with intermediate values
      float msi = m_srm - i_ss;
      SetTableValue(WQ_M1_TABLE, "MSI", row, msi);

      if (msi > maxMSI[lgIndex])
         maxMSI[lgIndex] = msi;
      }

   // pass 2
   // compute final model for each AU
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float msi = 0;
      GetTableValue(WQ_M1_TABLE, "MSI", row, msi);   // msi (un-normalized)

      float m_m1 = msi / maxMSI[lgIndex];    // normalized sink

      // update tables with intermediate values
      SetTableValue(WQ_M1_TABLE, "M_M1", row, m_m1);
      SetTableValue(WQ_M1_TABLE, "M_M1_CAL", row, m_m1);   // NOTE: cal same a non-cal
      //SetTableValue(WQ_RP_TABLE, "M_M1_CAL", row, m_m1);
      }

   return 1;
   }


//---------------------------------------------------
//-------- nitrogen export export potential --------------
//---------------------------------------------------
int PSWCP::SolveWqM1Nit()
   {
   float maxRDN[LG_COUNT] = { 0,0,0,0,0 };
   float maxNSI[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1
   // find max values for normalizing
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float rdn_mi = 0;    // riparian area denitrification potential
      GetTableValue(WQ_DB_TABLE, "RDN_MI", row, rdn_mi);

      float sqMi = 0;
      GetTableValue(WQ_DB_TABLE, "SQ_MILES", row, sqMi);

      float rdn = rdn_mi / sqMi;
      SetTableValue(WQ_M1_TABLE, "RDN", row, rdn);

      if (rdn > maxRDN[lgIndex])
         maxRDN[lgIndex] = rdn;
      }

   // pass 2
   // update NSI values
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float rdn = 0;    // riparian denitrogen potential
      GetTableValue(WQ_M1_TABLE, "RDN", row, rdn);   // msi (un-normalized)

      float wls = 0;    // wetland/lake storage
      GetTableValue(WF_M1_TABLE, "WLS", row, wls);

      float n_rdn = rdn / maxRDN[lgIndex];    // normalized sink

      float nsi = wls + n_rdn;     // 
      // update tables with intermediate values
      SetTableValue(WQ_M1_TABLE, "NSI", row, nsi);

      if (nsi > maxNSI[lgIndex])
         maxNSI[lgIndex] = nsi;
      }

   // pass 3
   // compute final model for each AU
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float nsi = 0;
      GetTableValue(WQ_M1_TABLE, "NSI", row, nsi);
      float n_si = nsi / maxNSI[lgIndex];

      SetTableValue(WQ_M1_TABLE, "N_SI", row, n_si);
      SetTableValue(WQ_M1_TABLE, "N_M1", row, 1 - n_si);
      SetTableValue(WQ_M1_TABLE, "N_M1_CAL", row, 1 - n_si);   // NOTE: cal same a non-cal
      //SetTableValue(WQ_RP_TABLE, "N_M1_CAL", row, 1 - n_si);
      }

   return 1;
   }


//---------------------------------------------------
//-------- pathogen export export potential ---------
//---------------------------------------------------
int PSWCP::SolveWqM1Path()
   {
   float maxDPWT[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1
   // find max values for normalizing
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float dpwt = 0;    // riparian area denitrification potential
      GetTableValue(WF_DB1_TABLE, "DPWT_PCT", row, dpwt);

      if (dpwt > maxDPWT[lgIndex])
         maxDPWT[lgIndex] = dpwt;
      }

   // pass 2
   // update values
   for (int row = 0; row < m_pWqDbTable->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float dpwt = 0;    // riparian denitrogen potential
      GetTableValue(WF_DB1_TABLE, "DPWT_PCT", row, dpwt);   // msi (un-normalized)

      // update tables with intermediate values
      float pa_si = dpwt / maxDPWT[lgIndex];
      SetTableValue(WQ_M1_TABLE, "PA_SI", row, pa_si);


      SetTableValue(WQ_M1_TABLE, "N_SI", row, pa_si);
      SetTableValue(WQ_M1_TABLE, "PA_M1", row, 1 - pa_si);
      SetTableValue(WQ_M1_TABLE, "PA_M1_CAL", row, 1 - pa_si);   // NOTE: cal same a non-cal
      //SetTableValue(WQ_RP_TABLE, "PA_M1_CAL", row, 1 - pa_si);
      }

   return 1;
   }


//---------------------------------------------------
//-------- water delivery importance ----------------
//---------------------------------------------------
int PSWCP::SolveWfM1WatDel()
   {
   // normalizing arrays 
   float maxP[LG_COUNT] = { 0,0,0,0,0 };
   float maxSRS[LG_COUNT] = { 0,0,0,0,0 };
   float maxIDE[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float av_prec = 0;      // precip  [0-1] avg annual precip/unit area
      GetTableValue(WF_DB1_TABLE, "AV_PREC", row, av_prec);

      if (av_prec < maxP[lgIndex])
         maxP[lgIndex] = av_prec;

      float srs_pct = 0;      //snow/rain-on-snow timing effect
      GetTableValue(WF_DB1_TABLE, "SRS_PCT", row, srs_pct);

      if (srs_pct < maxSRS[lgIndex])
         maxSRS[lgIndex] = srs_pct;
      }

   // pass 2 - intermediate calcs
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float av_prec = 0;      // precip  [0-1] avg annual precip/unit area
      GetTableValue(WF_DB1_TABLE, "AV_PREC", row, av_prec);
      float p = av_prec / maxP[lgIndex];

      float srs_pct = 0;      // snow/rain-on-snow timing effect
      GetTableValue(WF_DB1_TABLE, "SRS_PCT", row, srs_pct);

      float rs = srs_pct / maxSRS[lgIndex];

      float ide = p + rs;

      if (ide > maxIDE[lgIndex])
         maxIDE[lgIndex] = ide;

      SetTableValue(WF_M1_TABLE, "P", row, p);
      SetTableValue(WF_M1_TABLE, "RS", row, rs);
      SetTableValue(WF_M1_TABLE, "IDE", row, ide);
      }

   // pass 3 - final calcs
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float ide = 0;
      GetTableValue(WF_M1_TABLE, "IDE", row, ide);

      float i_de = ide / maxIDE[lgIndex];
      SetTableValue(WF_M1_TABLE, "I_DE", row, i_de);
      }

   return 1;
   }


//---------------------------------------------------
//-------- Surface Storage importance ---------------
//---------------------------------------------------
int PSWCP::SolveWfM1SurfStorage()
   {
   // normalizing arrays 
   float maxWLS[LG_COUNT] = { 0,0,0,0,0 };
   float maxSTS[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float dpwt_pct = 0;  // depressional wetlands storage
      GetTableValue(WF_DB1_TABLE, "DPWT_PCT", row, dpwt_pct);

      float lk_pct = 0;  // storage in lakes
      GetTableValue(WF_DB1_TABLE, "LK_PCT", row, lk_pct);

      float wt_lk = dpwt_pct + lk_pct;
      SetTableValue(WF_M1_TABLE, "WT_LK", row, wt_lk);

      if (wt_lk < maxWLS[lgIndex])
         maxWLS[lgIndex] = wt_lk;

      float uc_mi = 0, mc_mi = 0, sq_miles = 0;   // unconfined/moderately confined floodplain storage
      GetTableValue(WF_DB1_TABLE, "UC_MI", row, uc_mi);
      GetTableValue(WF_DB1_TABLE, "MC_MI", row, mc_mi);
      GetTableValue(WQ_DB_TABLE, "SQ_MILES", row, sq_miles);

      float unss = 3 * uc_mi / sq_miles;
      float mcss = 2 * mc_mi / sq_miles;
      float un_mc = unss + mcss;
      SetTableValue(WF_M1_TABLE, "UNSS", row, unss);
      SetTableValue(WF_M1_TABLE, "MCSS", row, mcss);
      SetTableValue(WF_M1_TABLE, "UN_MC", row, un_mc);

      if (un_mc > maxSTS[lgIndex])
         maxSTS[lgIndex] = un_mc;
      }

   // pass 2 - final calcs
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float wt_lk = 0;
      GetTableValue(WF_M1_TABLE, "WT_LK", row, wt_lk);

      float wls = wt_lk / maxWLS[lgIndex];
      SetTableValue(WF_M1_TABLE, "WLS", row, wls);

      float un_mc = 0;
      GetTableValue(WF_M1_TABLE, "UN_MC", row, un_mc);

      float sts = un_mc / maxSTS[lgIndex];
      SetTableValue(WF_M1_TABLE, "STS", row, sts);
      }

   return 1;
   }


//---------------------------------------------------
//-------- Recharge/discharge importance ------------
//---------------------------------------------------
int PSWCP::SolveWfM1RechargeDischarge()
   {
   // normalizing arrays 
   float maxIR[LG_COUNT] = { 0,0,0,0,0 };
   float maxUCHP[LG_COUNT] = { 0,0,0,0,0 };
   float maxSLPWT[LG_COUNT] = { 0,0,0,0,0 };
   float maxIDI[LG_COUNT] = { 0,0,0,0,0 };

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float av_prec = 0, permh = 0, perml = 0;  // 
      GetTableValue(WF_DB1_TABLE, "AV_PREC", row, av_prec);
      GetTableValue(WF_DB1_TABLE, "PERMH", row, permh);
      GetTableValue(WF_DB1_TABLE, "PERML", row, perml);

      // NOTE: THIS PRODUCES RESULTS INCONSISTENT WITH ORIGINAL CALCS!!!!!!
      float rechH = ((av_prec * 0.838f) - 9.77f) * permh;
      float rechL = ((av_prec * 0.497f) - 5.03f) * perml;

      // SO JUST READ FROM M1 FILE FOR NOW
      GetTableValue(WF_M1_TABLE, "RECHH", row, rechH);
      GetTableValue(WF_M1_TABLE, "RECHL", row, rechL);

      float acres = 0;
      GetTableValue(WQ_DB_TABLE, "ACRES", row, acres);

      float ir = (rechH + rechL) / acres;    // importance 
      SetTableValue(WF_M1_TABLE, "IR", row, ir);

      if (ir < maxIR[lgIndex])
         maxIR[lgIndex] = ir;

      float uchp_area = 0;
      GetTableValue(WF_DB1_TABLE, "UCHP_AREA", row, uchp_area);

      if (uchp_area > maxUCHP[lgIndex])
         maxUCHP[lgIndex] = uchp_area;

      // slope wetland discharge
      float slpwt_ac = 0;
      GetTableValue(WF_DB1_TABLE, "SLPWT_AC", row, slpwt_ac);

      float slpwt_pct = slpwt_ac / acres;  // NOTE: calls for *100 in docs, but this isn't in results so left out here
      SetTableValue(WF_DB1_TABLE, "SLPWT_PCT", row, slpwt_pct);

      if (slpwt_pct > maxSLPWT[lgIndex])
         maxSLPWT[lgIndex] = slpwt_pct;
      }

   // pass 2 - intermdiate calcs
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float ir = 0;
      GetTableValue(WF_M1_TABLE, "IR", row, ir);

      float i_r = ir / maxIR[lgIndex];
      SetTableValue(WF_M1_TABLE, "I_R", row, i_r);

      float uchp_area = 0;
      GetTableValue(WF_DB1_TABLE, "UCHP_AREA", row, uchp_area);

      float sd = uchp_area / maxUCHP[lgIndex];
      SetTableValue(WF_M1_TABLE, "SD", row, sd);

      float slpwt_pct = 0;
      GetTableValue(WF_DB1_TABLE, "SLPWT_PCT", row, slpwt_pct);

      float swd = slpwt_pct / maxSLPWT[lgIndex];
      SetTableValue(WF_M1_TABLE, "SWD", row, swd);

      float idi = sd + swd;
      SetTableValue(WF_M1_TABLE, "IDI", row, idi);

      if (idi > maxIDI[lgIndex])
         maxIDI[lgIndex] = idi;
      }

   // pass 3
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float idi = 0;
      GetTableValue(WF_M1_TABLE, "IDI", row, idi);

      float i_di = idi / maxIDI[lgIndex];
      SetTableValue(WF_M1_TABLE, "I_DI", row, i_di);
      }
   //??????  GROUNDWATER?  V2 ADDITION?


   return 1;
   }



//---------------------------------------------------
//-------- water delivery degredation  --------------
//---------------------------------------------------
int PSWCP::SolveWfM2WatDel()
   {
   // normalizing arrays 
   float maxFL = 0;   // why X, U for LG_M2?????
   float maxIMP = 0;

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float fl_pct = 0;      // forest loss percent
      GetTableValue(WF_DB2_TABLE, "FL_PCT", row, fl_pct);

      if (fl_pct > maxFL)
         maxFL = fl_pct;

      float imp_ac = 0, acres = 0;      // percent impervious
      GetTableValue(WF_DB2_TABLE, "IMP_AC", row, imp_ac);
      GetTableValue(WQ_DB_TABLE, "ACRES", row, acres);
      float imp_pct = imp_ac / acres;
      SetTableValue(WF_DB2_TABLE, "IMP_PCT", row, imp_pct);

      if (imp_pct > maxIMP)
         maxIMP = imp_pct;
      }

   // pass 2 - get normalizing factors
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float imp_pct = 0;
      SetTableValue(WF_DB2_TABLE, "IMP_PCT", row, imp_pct);

      float imp = imp_pct / maxIMP;
      SetTableValue(WF_M1_TABLE, "IMP", row, imp);
      }

   return 1;
   }


//---------------------------------------------------
//-------- Surface Storage degradation --------------
//---------------------------------------------------
int PSWCP::SolveWfM2SurfStorage()
   {
   float maxDW = 0;
   float maxSTS = 0;

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      // storage in wetlands
      float w_ur_ac = 0, w_ru_ac = 0, acres = 0;
      GetTableValue(WF_DB2_TABLE, "W_UR_AC", row, w_ur_ac);
      GetTableValue(WF_DB2_TABLE, "W_RU_AC", row, w_ru_ac);
      GetTableValue(WQ_DB_TABLE, "ACRES", row, acres);

      float uw = 3 * w_ur_ac / acres;     // loss of storage wetlands in urban areas
      float rw = 2 * w_ru_ac / acres;     // loss of storage wetlands in rural areas
      SetTableValue(WF_M2_TABLE, "UW", row, uw);
      SetTableValue(WF_M2_TABLE, "RW", row, uw);

      if (uw + rw > maxDW)
         maxDW = uw + rw;

      // storage in floodplains
      float uc_alt_mi = 0, mc_alt_mi = 0, sq_miles = 0;
      GetTableValue(WF_DB2_TABLE, "UC_ALT_MI", row, uc_alt_mi);
      GetTableValue(WF_DB2_TABLE, "MC_ALT_MI", row, mc_alt_mi);
      GetTableValue(WF_DB2_TABLE, "MC_ALT_MI", row, sq_miles);

      float uds = 3 * uc_alt_mi / sq_miles;   //???? NOTE: SLIGHT DEVIATION FROM SPREADSHEET VALUES
      float mds = 2 * mc_alt_mi / sq_miles;
      SetTableValue(WF_M2_TABLE, "UDS", row, uds);
      SetTableValue(WF_M2_TABLE, "MDS", row, mds);

      if (uds + mds > maxSTS)
         maxSTS = uds + mds;
      }

   // pass 2 - final calcs
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float uw = 0, rw = 0;     // loss of storage wetlands
      GetTableValue(WF_M2_TABLE, "UW", row, uw);
      GetTableValue(WF_M2_TABLE, "RW", row, uw);

      float dw = uw + rw;
      float d_ws = dw / maxDW;
      SetTableValue(WF_M1_TABLE, "DW", row, dw);
      SetTableValue(WF_M1_TABLE, "D_WS", row, d_ws);

      float uds = 0, mds = 0;     // loss of floodplain
      GetTableValue(WF_M2_TABLE, "UDS", row, uds);
      GetTableValue(WF_M2_TABLE, "MDS", row, mds);

      float dst = uds + mds;
      float d_sts = dst / maxSTS;
      SetTableValue(WF_M1_TABLE, "DST", row, dst);
      SetTableValue(WF_M1_TABLE, "D_STS", row, d_sts);
      }

   return 1;
   }


//---------------------------------------------------
//-------- Recharge degradation -----------
//---------------------------------------------------
int PSWCP::SolveWfM2Recharge()
   {

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      // degradation to recharge
      float u_ac = 0, bu_ac = 0, li_ac = 0;
      GetTableValue(WF_DB2_TABLE, "U_AC", row, u_ac);
      GetTableValue(WF_DB2_TABLE, "BU_AC", row, bu_ac);
      GetTableValue(WF_DB2_TABLE, "LI_AC", row, li_ac);

      float rechCoeff = 0.9f * u_ac + 0.7f * bu_ac + 0.35f * li_ac;    // ???????? Wrong, not clear in docs
      }

   return 1;
   }



//---------------------------------------------------
//-------- Discharge degradation --------------------
//---------------------------------------------------
int PSWCP::SolveWfM2Discharge()
   {
   float maxRDDEN = 0;
   float maxWELL = 0;
   float maxUS = 0;
   float maxSW = 0;

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      // degradation associated with roads 
      float rd_mi = 0, sq_miles = 0;
      GetTableValue(WF_DB2_TABLE, "RD_MI", row, rd_mi);
      GetTableValue(WQ_DB_TABLE, "SQ_MILES", row, sq_miles);

      float rd_den = rd_mi / sq_miles;
      GetTableValue(WF_DB2_TABLE, "RD_DEN", row, rd_den);

      if (rd_den < maxRDDEN)
         maxRDDEN = rd_den;

      // degradation due to groundwater extraction
      float well_cnt = 0;
      GetTableValue(WF_DB2_TABLE, "WELL_CNT", row, well_cnt);

      float well_den = well_cnt / sq_miles;
      SetTableValue(WF_DB2_TABLE, "WELL_DEN", row, well_den);

      // degradation due t discharge in floodplains with altered permeability
      float uchp_u = 0, uchp_r = 0;
      GetTableValue(WF_DB2_TABLE, "UCHP_U", row, uchp_u);
      GetTableValue(WF_DB2_TABLE, "UCHP_R", row, uchp_r);

      float uus = 3 * uchp_u / sq_miles;
      float urs = 2 * uchp_r / sq_miles;
      SetTableValue(WF_M2_TABLE, "UUS", row, uus);
      SetTableValue(WF_M2_TABLE, "URS", row, urs);

      if (uus + urs > maxUS)
         maxUS = uus + urs;

      // degradation due to dischage in slope wetlands
      float slpw_u = 0, slpw_r = 0;
      GetTableValue(WF_DB2_TABLE, "SLPW_U", row, slpw_u);
      GetTableValue(WF_DB2_TABLE, "SLPW_R", row, slpw_r);

      float swu = 3 * slpw_u / sq_miles;
      float swr = 2 * slpw_r / sq_miles;
      SetTableValue(WF_M2_TABLE, "SWU", row, swu);
      SetTableValue(WF_M2_TABLE, "SWR", row, swr);

      if (swu + swr > maxSW)
         maxSW = swu + swr;
      }

   // pass 2 - intermdiate calcs
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      float rd_den = 0;
      GetTableValue(WF_DB2_TABLE, "RD_DEN", row, rd_den);

      float d_rd = rd_den / maxRDDEN;
      SetTableValue(WF_M2_TABLE, "D_RD", row, d_rd);

      float well_den = 0;
      GetTableValue(WF_DB2_TABLE, "WELL_DEN", row, well_den);

      float d_wel = well_den / maxWELL;
      SetTableValue(WF_M2_TABLE, "D_WEL", row, d_wel);

      float uus = 0, urs = 0;
      GetTableValue(WF_M2_TABLE, "UUS", row, uus);
      GetTableValue(WF_M2_TABLE, "URS", row, urs);

      float d_std = (uus + urs) / maxUS;
      SetTableValue(WF_M2_TABLE, "STD", row, (uus + urs));
      SetTableValue(WF_M2_TABLE, "D_STD", row, d_std);

      float swu = 0, swr = 0;
      GetTableValue(WF_M2_TABLE, "SWU", row, swu);
      GetTableValue(WF_M2_TABLE, "SWR", row, swr);

      float d_wd = (swu + swr) / maxSW;
      SetTableValue(WF_M2_TABLE, "WD", row, (swu + swr));
      SetTableValue(WF_M2_TABLE, "D_WD", row, d_wd);
      }


   return 1;
   }

//---------------------------------------------------
//-------- evaptrans degradation --------------------
//---------------------------------------------------
int PSWCP::SolveWfM2EvapTrans()
   {
   float maxIMP = 0;

   // pass 1 - get normalizing factors
   for (int row = 0; row < m_pWfDb2Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      // degradation associated with roads 
      float imp_pct = 0;
      GetTableValue(WF_DB2_TABLE, "IMP_PCT", row, imp_pct);

      if (imp_pct > maxIMP)
         maxIMP = imp_pct;
      }

   // pass 2 - intermdiate calcs
   for (int row = 0; row < m_pWfDb1Table->GetRowCount(); row++)
      {
      LSGROUP lgIndex = GetLSGroupIndex(row);
      if (lgIndex == LG_NULL)
         continue;

      // degradation associated with roads 
      float imp_pct = 0;
      GetTableValue(WF_DB2_TABLE, "IMP_PCT", row, imp_pct);

      float d_l = imp_pct / maxIMP;
      SetTableValue(WF_M2_TABLE, "D_L", row, d_l);
      }


   return 1;
   }

//---------------------------------------------------
//-------- evaptrans degradation --------------------
//---------------------------------------------------
int PSWCP::SolveHCI()
   {
// iterate IDUs using the HCI watershed index
   ///calculate HPC (HCI_Dist * HPC coefficent) and sum for subwatersheds, along with calculating HCVworst
        //write sums back to IDUs

   m_pIDULayer->m_readOnly = false;
   float value=0.0f;
   int cat = 0;
   VData lulc=-1;
   int HPC_col = 0.0f;

   CUIntArray recordArray;
   for (int row = 0; row < m_pHCITable->GetRowCount(); row++)
   {
      // get the cat_ID for this record
      int hciCat = 0;
      //GetTableValue(HCI_CATCH_TABLE, "HCI_CAT", row, hciCat);
      hciCat = m_pHCITable->GetAsInt(1,row);
      float HCV=0;
      float HCVworst=0;
      //Get the HCI value for the watershed
      int count = m_HCIIndex_IDU.GetRecordArray(m_col_IDU_HCI_Shed, VData(hciCat), recordArray);
      for (int j = 0; j < count; j++)
         {
         int idu = recordArray[j];

         if (idu < m_pIDULayer->GetRecordCount())  // for partial loads
            {
            m_pIDULayer->GetData(idu, m_col_IDU_HCI_Dist, value);
            m_pIDULayer->GetData(idu, m_col_IDU_HCI_Shed, cat);
            m_pIDULayer->GetData(idu, m_col_IDU_LULC_B, lulc);
           
            int rowIndex = m_pHPCTable->Find(0, lulc,0);//get the row
            float HPC = m_pHPCTable->GetAsFloat(1, rowIndex);
            float HPCg = HPC * value;
            HCVworst+=35.0f*value;
            HCV+=HPCg;
            }
         }
      
      //Add the calculated HCI to the IDUs
      for (int j = 0; j < count; j++)
         {
         int idu = recordArray[j];
         if (idu < m_pIDULayer->GetRecordCount())  // for partial loads
            {
            m_pIDULayer->SetData(idu, m_col_IDU_HCI_Value, HCV/HCVworst);
            }     
         }  // end of: for each row in HCI watershed index
      }  
   m_pIDULayer->m_readOnly = true;

   return 1;
}


int PSWCP::SolveHabTerr(EnvContext* pEnvContext)
   {
   // Propagate IDU Changes - Habitat
   // basic idea:
   // 1) scan the delta array looking for changes to relevent columns.  These include any column with 
   //    info affecting one of the three terrestrial habitat indicators (ecosystem integrity, PHS, oak grove)
   // 2) when found, update the appropriate score in the habitat table for the AUH associated with the IDU,
   //    proportional to the area of the IDU compared to the AUH.
   //
   //  Deltas of interest include: LULC_A - ag to developed
   //                                       forest to developed
   //                                       * to Open/Conservation

   DeltaArray* deltaArray = pEnvContext->pDeltaArray;

   if (deltaArray != NULL)
      {
      INT_PTR size = deltaArray->GetSize();
      if (size > 0)
         {
         for (INT_PTR i = pEnvContext->firstUnseenDelta; i < size; i++)
            {
            DELTA& delta = ::EnvGetDelta(deltaArray, i);
            if (delta.col == m_col_IDU_LULC_A)
               {
               int newLulc = delta.newValue.GetInt();
               int oldLulc = delta.oldValue.GetInt();
               
               if (oldLulc == 3 || oldLulc == 9 || oldLulc == 0) // developed or water? skip
                  {
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_IntIndex, -1, SET_DATA);       // habitat integrity index
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_PHS, -1, SET_DATA);            // priority habitat for sppecies of interest
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_OakGrove, -1, SET_DATA);       // oak grove habitat
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_OverallIndex, -1, SET_DATA);
                  continue;
                  }
               
               bool update = false;
               float scalar = 0;

               //---------------------------------------------------//
               //-------------- APPLY SCORING RULES-----------------//
               //---------------------------------------------------//

               // AG to DEVELOPED
               if (newLulc == 3 && oldLulc != 1)
                  {
                  update = true;
                  scalar = 0.5;
                  }

               // FOREST to DEVELOPED
               else if (newLulc == 3 && oldLulc != 2)
                  {
                  scalar = 0.1f;    // assume 90 percent degregations
                  update = true;
                  }

               // ANYTHING to CONSERVATION
               else if (newLulc == 5 && oldLulc != 5)
                  {
                  scalar = 1.10f;
                  update = true;
                  }

               if (update)
                  {
                  float integ_Inde, norm_PHS, norm_oakgr;
                  m_pIDULayer->GetData(delta.cell, m_col_IDU_Hab_IntIndex, integ_Inde);  // habitat integrity index (0-1)
                  m_pIDULayer->GetData(delta.cell, m_col_IDU_Hab_PHS, norm_PHS);         // priority habitat for sppecies of interest (0-100)
                  m_pIDULayer->GetData(delta.cell, m_col_IDU_Hab_OakGrove, norm_oakgr);  // oak grove habitat (0-100)

                  integ_Inde *= scalar;
                  norm_PHS *= scalar;
                  norm_oakgr *= scalar;
                  if (integ_Inde > 1)
                     integ_Inde = 1;
                  if (norm_PHS > 100)
                     norm_PHS = 100;
                  if (norm_oakgr > 100)
                     norm_oakgr = 100;

                  float overallIndex = 0;   // max of the three subcomponents (0-100)
                  if (integ_Inde*100 > overallIndex)
                     overallIndex = integ_Inde*100;
                  if (norm_PHS > overallIndex)
                     overallIndex = norm_PHS;
                  if (norm_oakgr > overallIndex)
                     overallIndex = norm_oakgr;

                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_IntIndex, integ_Inde, ADD_DELTA);       // habitat integrity index
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_PHS, norm_PHS, ADD_DELTA);              // priority habitat for sppecies of interest
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_OakGrove, norm_oakgr, ADD_DELTA);       // oak grove habitat
                  UpdateIDU(pEnvContext, delta.cell, m_col_IDU_Hab_OverallIndex, overallIndex, ADD_DELTA);
                  }
               }
            }
         }
      }

   // apply deltas generated above, since the IDU's need to be updated before the remaining code is run
   ::EnvApplyDeltaArray(pEnvContext->pEnvModel);

   // IDU's have been updated to reflect changes in LULC_A, update the database AU's 
   CUIntArray recordArray;

   for (int row = 0; row < m_pHabTerrTable->GetRowCount(); row++)
      {
      // get the AU_ID for this record
      int auID = 0;
      GetTableValue(HAB_TERR_TABLE, "New_AU", row, auID);

      int count = m_AUHIndex_IDU.GetRecordArray(m_col_IDU_AUH_ID, VData(auID), recordArray);
      if (count > 0)
         {
         // area-weighted averages
         float agg_integ_Inde = 0, agg_norm_PHS = 0, agg_norm_oakgr = 0, ag_ovrall_IND = 0;
         float totalArea = 0;

         for (int j = 0; j < count; j++)
            {
            int idu = recordArray[j];

            if (idu < m_pIDULayer->GetRecordCount())  // for partial loads
               {
               float integ_Inde, norm_PHS, norm_oakgr, ovrall_IND, area;

               m_pIDULayer->GetData(idu, m_col_IDU_Hab_IntIndex, integ_Inde);       // habitat integrity index
               m_pIDULayer->GetData(idu, m_col_IDU_Hab_PHS, norm_PHS);              // priority habitat for sppecies of interest
               m_pIDULayer->GetData(idu, m_col_IDU_Hab_OakGrove, norm_oakgr);       // oak grove habitat
               m_pIDULayer->GetData(idu, m_col_IDU_Hab_OverallIndex, ovrall_IND);   // overall (combined) index
               m_pIDULayer->GetData(idu, m_col_IDU_AREA, area);   //idu area

               agg_integ_Inde += integ_Inde * area;
               agg_norm_PHS += norm_PHS * area;
               agg_norm_oakgr += norm_oakgr * area;
               totalArea += area;
               }
            }

         agg_integ_Inde /= totalArea;
         agg_norm_PHS /= totalArea;
         agg_norm_oakgr /= totalArea;

         float overallIndex = 0;   // max of the three subcomponents
         if (agg_integ_Inde > overallIndex)
            overallIndex = agg_integ_Inde;
         if (agg_norm_PHS > overallIndex)
            overallIndex = agg_norm_PHS;
         if (agg_norm_oakgr > overallIndex)
            overallIndex = agg_integ_Inde;

         SetTableValue(HAB_TERR_TABLE, "Integ_Inde", row, agg_integ_Inde);
         SetTableValue(HAB_TERR_TABLE, "norm_PHS", row, agg_norm_PHS);
         SetTableValue(HAB_TERR_TABLE, "norm_oakgr", row, agg_norm_oakgr);
         SetTableValue(HAB_TERR_TABLE, "ovrall_IND", row, agg_norm_oakgr);
         }
      }
   return true;
   }

bool PSWCP::LoadTables()
   {
   // load databases
   m_pWqDbTable    = LoadTable(WQ_DB_TABLE, "PSWCP/WQ_DB.csv");
   m_pWqM1Table    = LoadTable(WQ_M1_TABLE, "PSWCP/WQ_M1.csv");
   m_pWqRpTable    = LoadTable(WQ_RP_TABLE, "PSWCP/WQ_RP.csv");
   m_pWfDb1Table   = LoadTable(WF_DB1_TABLE, "PSWCP/WF_DB1.csv");
   m_pWfDb2Table   = LoadTable(WF_DB2_TABLE, "PSWCP/WF_DB2.csv");
   m_pWfM1Table    = LoadTable(WF_M1_TABLE, "PSWCP/WF_M1.csv");
   m_pWfM2Table    = LoadTable(WF_M2_TABLE, "PSWCP/WF_M2.csv");
   m_pWfRpTable    = LoadTable(WF_RP_TABLE, "PSWCP/WF_RP.csv");
   m_pHabTerrTable = LoadTable(HAB_TERR_TABLE, "PSWCP/AU_Terrestrial_Indicies_Aug2012.csv");

   // update colInfos
   int i = 0;
   while (colInfo[i].table != NULL_TABLE)
      {
      switch (colInfo[i].table)
         {
         case  WQ_DB_TABLE:   m_pWqDbTable = pTable; filename = "PSWCP/WQ_DB.csv"; break;
         case  WQ_M1_TABLE:   m_pWqM1Table = pTable; filename = "PSWCP/WQ_M1.csv"; break;
         case  WQ_RP_TABLE:   m_pWqRpTable = pTable; filename = "PSWCP/WQ_RP.csv"; break;
         case  WF_DB1_TABLE:  m_pWfDb1Table = pTable; filename = "PSWCP/WF_DB1.csv"; break;
         case  WF_DB2_TABLE:  m_pWfDb2Table = pTable; filename = "PSWCP/WF_DB2.csv"; break;
         case  WF_M1_TABLE:   m_pWfM1Table = pTable; filename = "PSWCP/WF_M1.csv"; break;
         case  WF_M2_TABLE:   m_pWfM2Table = pTable; filename = "PSWCP/WF_M2.csv"; break;
         case  WF_RP_TABLE:   m_pWfRpTable = pTable; filename = "PSWCP/WF_RP.csv"; break;
         case  HAB_TERR_TABLE: m_pHabTerrTable = pTable; filename = "PSWCP/AU_Terrestrial_Indicies_Aug2012.csv"; break;
         case HCI_VALUE_TABLE: m_pHPCTable = pTable; filename="PSWCP/HCI_Values.csv"; break;
         case HCI_CATCH_TABLE: m_pHCITable = pTable; filename = "PSWCP/HCI_Shed.csv"; break;
         }

      int col = colInfo[i].pTable->GetCol(colInfo[i].field);
      if (col < 0)
         {
         CString msg;
         msg.Format("   PSWCP: Table field [%s] was not found in the table", colInfo[i].field);
         Report::ErrorMsg(msg);
         }
      else
         colInfo[i].col = col;

      i++;
      }

   return true;
   }

VDataObj *PSWCP::LoadTable(TABLE t, LPCTSTR filename)
   {
   VDataObj* pTable = new VDataObj(UNIT_MEASURE::U_UNDEFINED);

   CString path;
   if (PathManager::FindPath(filename, path) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format("PSWCP: Input file %s not found", filename);
      Report::ErrorMsg(msg);
      return NULL;
      }

   int rows = pTable->ReadAscii(path, ',');
   CString msg;
   msg.Format("  PSWCP: Loaded %i records from %s", rows, (LPCTSTR)path);
   Report::LogInfo(msg);

   return pTable;
   }

bool PSWCP::GetTableValue(TABLE table, LPCTSTR field, int row, int& value)
   {
   TABLECOL* pCol = (TABLECOL*)colMap[field];

   if (pCol == NULL || pCol->table != table)
      {
      CString msg;
      msg.Format("PSWCP: Bad table request! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   bool ok = pCol->pTable->Get(pCol->col, row, value);
   if (!ok)
      {
      CString msg;
      msg.Format("PSWCP: Unable to get table value! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   return true;
   }

bool PSWCP::GetTableValue(TABLE table, LPCTSTR field, int row, float& value)
   {
   TABLECOL* pCol = (TABLECOL*)colMap[field];

   if (pCol == NULL || pCol->table != table)
      {
      CString msg;
      msg.Format("PSWCP: Bad table request! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   bool ok = pCol->pTable->Get(pCol->col, row, value);
   if (!ok)
      {
      CString msg;
      msg.Format("PSWCP: Unable to get table value! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   return true;
   }

bool PSWCP::GetTableValue(TABLE table, LPCTSTR field, int row, double& value)
   {
   TABLECOL* pCol = (TABLECOL*)colMap[field];

   if (pCol == NULL || pCol->table != table)
      {
      CString msg;
      msg.Format("PSWCP: Bad table request! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   bool ok = pCol->pTable->Get(pCol->col, row, value);
   if (!ok)
      {
      CString msg;
      msg.Format("PSWCP: Unable to get table value! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   return true;
   }


bool PSWCP::GetTableValue(TABLE table, LPCTSTR field, int row, CString& value)
   {
   TABLECOL* pCol = (TABLECOL*)colMap[field];

   if (pCol == NULL || pCol->table != table)
      {
      CString msg;
      msg.Format("PSWCP: Bad table request! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   bool ok = pCol->pTable->Get(pCol->col, row, value);
   if (!ok)
      {
      CString msg;
      msg.Format("PSWCP: Unable to get table value! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   return true;
   }

bool PSWCP::SetTableValue(TABLE table, LPCTSTR field, int row, VData& value)
   {
   TABLECOL* pCol = (TABLECOL*)colMap[field];

   if (pCol == NULL || pCol->table != table)
      {
      CString msg;
      msg.Format("PSWCP: Bad table request when setting value! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   bool ok = pCol->pTable->Set(pCol->col, row, value);
   if (!ok)
      {
      CString msg;
      msg.Format("PSWCP: Unable to set table value! Table %i, field %s", (int)table, (LPCTSTR)field);
      Report::ErrorMsg(msg);
      return false;
      }

   return true;
   }

LSGROUP PSWCP::GetLSGroupIndex(int row)
   {
   LSGROUP lgIndex = LG_NULL;
   CString lg;
   GetTableValue(WQ_DB_TABLE, "LG_M1", row, lg);
   if (lg.GetLength() > 0)
      {
      if (lg[0] == 'D')  // delta
         lgIndex = LG_DELTA;
      else if (lg[0] == 'C')
         lgIndex = LG_COASTAL;
      else if (lg[0] == 'M')
         lgIndex = LG_MOUNTAINOUS;
      else if (lg[0] == 'L')
         lgIndex = (lg.GetLength() == 1) ? LG_LOWLAND : LG_LAKE;
      }
   return lgIndex;
   }

bool PSWCP::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
   {
   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // <acute_hazards>
   CString codePath;

   //XML_ATTR attrs[] = {
   //   // attr            type           address            isReq checkCol
   //   { "python_path",  TYPE_CSTRING,   &m_pythonPath,     true,  0 },
   //   { NULL,           TYPE_NULL,     NULL,               false, 0 } };
   //ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);
   //
   //TiXmlElement* pXmlEvent = pXmlRoot->FirstChildElement("event");
   //if (pXmlEvent == NULL)
   //   {
   //   CString msg("Acute Hazards: no <event>'s defined");
   //   msg += filename;
   //   Report::ErrorMsg(msg);
   //   return false;
   //   }
   //
   //while (pXmlEvent != NULL)
   //   {
   //   AHEvent* pEvent = new AHEvent;
   //
   //   CString pyModulePath;
   //
   //   XML_ATTR attrs[] = {
   //      // attr                  type           address            isReq checkCol
   //      { "year",                TYPE_INT,      &pEvent->m_year,           true,  0 },
   //      { "name",                TYPE_CSTRING,  &pEvent->m_name,           true,  0 },
   //      { "use",                 TYPE_INT,      &pEvent->m_use,            true,  0 },
   //      { "py_function",         TYPE_CSTRING,  &pEvent->m_pyFunction,     true,  0 },
   //      { "py_module",           TYPE_CSTRING,  &pyModulePath,             true,  0 },
   //      { "env_output",          TYPE_CSTRING,  &pEvent->m_envOutputPath,  true,  0 },
   //      { "earthquake_input",    TYPE_CSTRING,  &pEvent->m_earthquakeInputPath,   true,  0 },
   //      { "tsunami_input",       TYPE_CSTRING,  &pEvent->m_tsunamiInputPath,   true,  0 },
   //      { "earthquake_scenario", TYPE_CSTRING,  &pEvent->m_earthquakeScenario, true,  0 },
   //      { "tsunami_scenario",    TYPE_CSTRING,  &pEvent->m_tsunamiScenario, true,  0 },
   //      { NULL,                  TYPE_NULL,     NULL,        false, 0 } };
   //
   //   ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);
   //
   //   if (!ok)
   //      {
   //      delete pEvent;
   //      CString msg;
   //      msg = "Acute Hazards Model:  Error reading <event> tag in input file ";
   //      msg += filename;
   //      Report::ErrorMsg(msg);
   //      return false;
   //      }
   //   else
   //      {
   //      nsPath::CPath path(pyModulePath);
   //      pEvent->m_pyModuleName = path.GetTitle();
   //      pEvent->m_pyModulePath = path.GetPath();
   //      pEvent->m_pAHModel = this;
   //
   //      m_events.Add(pEvent);
   //      }
   //
   //   pXmlEvent = pXmlEvent->NextSiblingElement("event");
   //   }

   return true;
   }


