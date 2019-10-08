
#include <EnvExtension.h>

#define _EXPORT __declspec( dllexport )


enum EOH_SCENARIO { EOH_STATUS_QUO=0, EOH_COWBOY=1, EOH_GROWTH_REGULATION=2,
   EOH_LANDSCAPE_REGULATION=3, EOH_LANDSCAPE_ORIENTATION=4};

enum EOH_WATERSHED { EOH_OTTAWA_MISS=3, EOH_OTTAWA_SOUTH=4, EOH_RIDEAU=5, EOH_ST_LAWRENCE=6 };


class _EXPORT EOHabitat : public EnvModelProcess
   {
   public:
      EOHabitat()
         : EnvModelProcess()
         , m_scenario(EOH_STATUS_QUO)
         // , m_pctWetlandCovers({0, 0, 0, 0}),
         // , pctImpervious
         , m_colArea(-1)
         , m_colLulcA(-1)
         , m_colLulcB(-1)
         , m_colDStream(-1)
         , m_colWShedID(-1)
         , m_totalArea(0)
         {
         }


   public:
      bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext* pEnvContext, bool);
      bool Run(EnvContext* pEnvContext);

   protected:
      float GetMaxPatchSize(MapLayer* pLayer, int wshedID);
      float ExpandPatch(MapLayer* pLayer, int wshedID, bool visited[], float& sizeSoFar);

      // inputs
      EOH_SCENARIO m_scenario;

      // outputs
      float m_pctWetlandCovers[4];
      float m_pctForestCovers[4];
      float m_pctStreamCovers[4];
      float m_pctImpervious[4];
      float m_coreForestSize[4];

      // columns
      int m_colArea;
      int m_colLulcA;
      int m_colLulcB;
      int m_colDStream;
      int m_colWShedID;

      // other
      float m_totalArea;
      float m_watershedAreas[4];

      CMap<int, int, int, int> m_watershedLookup;  // key = watershed ID, value=array index


   };