import xml.etree.ElementTree as ET
import geopandas as gpd
import json


# 1. PublishMapsToWeb
# 2. PublishChartsToWeb
# 3. PublishDataToWeb
# 4. PublishVideoToWeb


studyArea = ""
localStudyAreaPath = ''
explorerStudyAreaPath = ''
iduXmlPath = ''
mapServerPath = ''
startYear = 0
endYear = 0
scenarios=[]
scenarioColors= {}
datasets = {}

def LoadStudyAreaDef(_studyArea):
    global studyArea, localStudyAreaPath, iduXmlPath, explorerStudyAreaPath
    global mapServerPath, startYear, endYear, scenarios, scenarioColors 

    with open(f'd:/Envision/Publishing/{_studyArea}.json') as f:
        d = json.load(f)
        
        studyArea = d['studyArea']
        localStudyAreaPath = d['localStudyAreaPath']
        iduXmlPath = d['iduXmlPath']
        explorerStudyAreaPath = d['explorerStudyAreaPath']
        mapServerPath = d['mapServerPath']
        startYear = d['startYear']
        endYear = d['endYear']
        scenarios=d['scenarios']
        scenarioColors=d['scenarioColors']

        _datasets = d['datasets']

        for dsName in _datasets:  
            fieldDefs = []
            for fd in _datasets[dsName]['fieldDefs']: 
                field = fd[0]
                type = int
                if fd[1] == 'float':
                    type = float

                cast = False
                if fd[2] == 'True':
                    cast = True

                scalar = None
                if fd[3] != -99:
                    scalar = fd[3]

                fieldDefs.append( [fd[0], type, cast, scalar])

            datasets[dsName] = {"fieldDefs": fieldDefs }


def LoadIduXML(fields):
    return LoadDatasetXML(iduXmlPath + "/idu.xml", fields)
    '''
    # create element tree object
    tree = ET.parse(iduXmlPath + "/idu.xml")
    
    # get root element
    root = tree.getroot()

    fieldInfos = {}

    for field in fields:
        fieldInfo = root.find( f".//field[@col='{field}']")  # 
        fieldInfos[field] = fieldInfo

    return fieldInfos
    '''

def LoadDatasetXML(xmlPath, fields):
    print("Loading dataset XML: ", xmlPath)

    # create element tree object
    tree = ET.parse(xmlPath)
    
    # get root element
    root = tree.getroot()

    fieldInfos = {}

    for field in fields:
        fieldInfo = root.find( f".//field[@col='{field}']")  # 
        fieldInfos[field] = fieldInfo

    return fieldInfos


def SaveIDUCoverage(idus, outPath, fields):

    fieldInfos = LoadIduXML(fields)

    schema = gpd.io.file.infer_schema(idus)

    for field in fields:
        fieldInfo = fieldInfos[field]
        datatype =  fieldInfo.get('datatype')
        
        if datatype in ['Long','long','Int','int','Integer', 'integer']:
            schema['properties'][field] = 'int32:10'
        elif datatype in ['Short','short']:
            schema['properties'][field] = 'int32:4'

    idus.to_file(driver = 'ESRI Shapefile', filename= outPath, schema=schema)



def GenIduIndexGeoJSON():
    idus = gpd.read_file(f'{localStudyAreaPath}/idu.shp', include_fields=['geometry'])
    idus = idus.simplify(10).to_crs(epsg=4326)

    idus.to_file(f'{localStudyAreaPath}/Outputs/GeoJson/IDU_INDEX_4326.geojson', driver="GeoJSON")  


