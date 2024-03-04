
import pandas as pd
import os
import subprocess
import shutil
import geopandas as gpd
import xml.etree.ElementTree as ET
import json
from xml.dom import minidom
import time
import datatable as dt

from sqlalchemy import create_engine

from simpledbf import Dbf5

import EnvPublishCommon as pub


'''
Processing map pipeline for publishing sequance of delta maps to the web

Inputs: 1) Initial IDU coverage
        2) delta array
        3) rasterized IDU_INDEX

Steps:
1) Run envision, saving delta array 
2) use DeltaExtractor to extract deltas of interest, one deltaarray for each attribute of interest
      e.g. deltaExtractor deltaArray_{scenario}_Run{run}.csv DeltaArray_{scenario}_LULC_A.csv LULC_A    <- repeat for each attribute
3) run the GenerateDeltaMaps python script to generate rasters for each year for each attribute, e.g. RMap_LULC_A_{scenario}_Run0.png
4) write resulting rasters to the explorer web site
     
       

 inputfile.csv outputfile.csv FIELD1,FIELD2



'''

# not used
def DeltaSummary( deltaFile ):
    
    dt_df = dt.fread(deltaFile)
    df = dt_df.to_pandas()
    #df = pd.read_csv(deltaFile)

    # set up dictionary
    deltaColInfo = {}

    rows,_ = df.size()

    for i, row in df.iterrows():
        if i % 10000 == 0:
            print('Processing record', i, 'of', rows)

        col = row['field']  # this is an IDU col

        if col in deltaColInfo:
            deltaColInfo[col] =deltaColInfo[col]+1
        else:
            deltaColInfo[col] = 0
        #year,idu,col,field,oldValue,newValue,type
        #2019,27,123,PLAN_SCORE,-1,-99,2006

    for col in deltaColInfo:
        print( col, deltaColInfo[col])


##################### Create Reduced Delta Array ################################
def GenerateReducedDeltaArrays(fieldDefs, scenarios, runs ):
    for run in runs:
        print("---------------------------------------")
        print(" Generating Reduced DeltaArray for Run", run)
        print("---------------------------------------")
        fields = [f[0] for f in fieldDefs]   # extract list of field names
        _fields = ",".join(fields)
        for scenario in scenarios:
            # extract deltas for this field
            print(f"Subsetting delta array using fields ", _fields)
        
            cmd = f"d:/Envision/Source/x64/Release/deltaExtractor {pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/deltaArray_{scenario}_Run{run}.csv {pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/DeltaArray_{scenario}_reduced.csv {_fields}"
            print( "  ", cmd)
            subprocess.call(cmd)
            print()
            os.makedirs(f'{pub.explorerStudyAreaPath}/Outputs/Scenarios/{scenario}/Run{run}', exist_ok = True)
            shutil.copy(f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/DeltaArray_{scenario}_reduced.csv', f'{pub.explorerStudyAreaPath}/Outputs/Scenarios/{scenario}/Run{run}')

    return

def GenerateInitReducedIDUs(fieldDefs, scenarios, runs):
    for run in runs:
        print("------------------------------------------")
        print("Generating Reduced Inital IDUs for Run ", run)
        print("------------------------------------------")
        for scenario in scenarios:
            fields = [f[0] for f in fieldDefs]   # extract list of field names
            fields.append('IDU_INDEX')
            print( "Loading IDU shape file...")
            #idus = gpd.read_file(f'{pub.localStudyAreaPath}/idu.shp', include_fields=fields)
            idus = gpd.read_file(f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/IDU_Year{pub.startYear}_{scenario}_Run{run}.shp', include_fields=fields)
            idus.to_crs(epsg=4326, inplace=True)

            outPath = f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/IDU_Year{pub.startYear}_{scenario}_Run{run}_reduced.shp'
            print('     Saving to ' + outPath)
            idus.to_file(outPath, driver ='ESRI Shapefile')

    return


##################### Generate IDUInfo.json for web ################################

def GenerateMapInfoJSon(datasetName, scenarios,mapType,layerType,dynamic,defaultField):
    print("------------------------------------")
    print(f"Generating {datasetName}Info JSON from {datasetName}.XML ")
    print("------------------------------------")
    fieldDefs = pub.datasets[datasetName]['fieldDefs']
    fields = [f[0] for f in fieldDefs]

    print(f"Loading {datasetName} shape file...")
    _fields = fields.copy()
    _fields.append(f'{datasetName}_INDEX')

    # for IDUs, only need to look at a single run
    if datasetName == 'IDU':
        ds = gpd.read_file(f'{pub.localStudyAreaPath}/Outputs/{scenarios[0]}/Run0/{datasetName}_Year{pub.startYear}_{scenarios[0]}_Run0_reduced.shp',
                              include_fields=_fields)
    else:
        ds = gpd.read_file(f'{pub.localStudyAreaPath}/{datasetName}.shp', include_fields=_fields)

    bounds = ds.total_bounds;  # returns (minx, miny, maxx, maxy)
    crs = ds.crs.to_string()

    bounds4326 = ds.to_crs(crs=4326).total_bounds

    print(f"Loading Field definitions ({datasetName}.xml)...")
    #fieldInfos = pub.LoadIduXML(fields)
    fieldInfos = pub. LoadDatasetXML(f"{pub.iduXmlPath}/{datasetName}.xml", fields)

    GenerateWebMapInfo(datasetName, scenarios, bounds, bounds4326, crs, mapType, layerType,dynamic,defaultField)
    return

def GenerateWebMapInfo(datasetName,scenarios,boundingBox,boundingBox4326,crs, mapType, layerType, dynamic, defaultField):
    fieldDefs = pub.datasets[datasetName]['fieldDefs']

    fields = [f[0] for f in fieldDefs]   # extract list of field names
    (left,bottom,right,top) = boundingBox

    fieldInfos = pub.LoadDatasetXML(f"{pub.iduXmlPath}/{datasetName}.xml", fields)

    mapInfo = {}
    mapInfo['studyArea'] = pub.studyArea
    mapInfo['scenarios'] = ','.join(scenarios)
    mapInfo['boundingBox'] = {"left":left, "bottom":bottom, "right":right, "top":top} 
    (left,bottom,right,top) = boundingBox4326
    mapInfo['boundingBox4326'] = {"left":left, "bottom":bottom, "right":right, "top":top} 
    mapInfo['crs'] = crs
    #mapInfo['image'] = {'width':rasterWidth, 'height':rasterHeight}
    mapInfo['startYear'] = pub.startYear
    mapInfo['endYear'] = pub.endYear
    mapInfo['mapType'] = mapType
    mapInfo['sourceURL'] = "http://envweb.bee.oregonstate.edu/MapServer/mapserv.exe?map=D:/MapServer/apps/{studyArea}/maps/{scenario}/{datasetName}/{year}_{run}.map"
    mapInfo['yearType'] = 'SingleYear'
    mapInfo['layerType'] = layerType
    mapInfo['interactive'] = True
    mapInfo['default_field'] = defaultField
    mapInfo['default_opacity'] = 0.6
    mapInfo['default_color'] = "blue"
    mapInfo['default_field'] = defaultField
    mapInfo['weight'] = 2
    mapInfo['dynamic'] = dynamic

    jFields = {}
    for field in fields:
        fieldInfo = fieldInfos[field]

        if fieldInfo == None:
            print(f"GenerateWebMapInfo(): couldn't find field {field} in {datasetName} Coverage")
        
        else:
            jField = {}
            jField['col'] = fieldInfo.get("col")
            jField['label'] = fieldInfo.get('label')
            jField['datatype'] = fieldInfo.get('datatype')
            jField['mfiType'] = fieldInfo.get('mfiType')

            attrs = fieldInfo.findall('attributes/attr')
            jAttrs = []
            for attr in attrs:
                jAttr = {}
                if attr.get('value') != None:
                    jAttr['value'] = attr.get('value')
                else:
                  jAttr['minVal'] = attr.get('minVal')
                  jAttr['maxVal'] = attr.get('maxVal')

                jAttr['color'] = attr.get('color').lstrip('(').rstrip(')')
                jAttr['label'] = attr.get('label')

                jAttrs.append(jAttr.copy())
            
            jFields[field] = jField
            jField['attributes'] = jAttrs

    mapInfo['fields'] = jFields
    js = json.dumps(mapInfo, indent=2)

    with open(f"{pub.explorerStudyAreaPath}/Outputs/DatasetInfo/{datasetName}Info.json", "w") as outfile:
        outfile.write(js)
    return


def GetIduRGB(fieldInfo,value):
    mfiType = int(fieldInfo.get('mfiType'))   # 0=range, 1=specific values
    attributes = fieldInfo.find('attributes')
    attrs = attributes.findall('attr')

    for attr in attrs:
        if mfiType == 0:  # range?
            if value >= float(attr.get('minVal')) and value < float(attr.get('maxVal')):
                color = attr.get('color').lstrip('(').rstrip(')')
                (r,g,b) = map(int, color.split(','))
                return (r,g,b)
        else:
            if value == int(attr.get('value')):
                color = attr.get('color').lstrip('(').rstrip(')')
                (r,g,b) = map(int, color.split(','))
                return (r,g,b)

    return (250,250,250)  # attribute not defined in fieldInfo
    
'''
def GeneratePNGsFromIDUs(idus, iduRaster, iduLookup, field, fieldInfo, scenario, year):

    # iterate though IDUs, populating associated pixels
    height = iduRaster.shape[0]
    width = iduRaster.shape[1]
    column = idus.columns.get_loc(field)

    red = np.zeros(shape=(height,width), dtype=np.uint8)
    grn = np.zeros(shape=(height,width), dtype=np.uint8)
    blu = np.zeros(shape=(height,width), dtype=np.uint8)

    nNoIndexEntry = 0

    # iterate through IDUs.  if the IDU is in the index, get rgb values from IDU.XML and populate
    for i, row in idus.iterrows():
        #if i % 100000 == 0:
        #    print('   Processing IDU record', i, 'of', idus.shape[0])

        iduIndex = int(row['IDU_INDEX'])
        if iduIndex in iduLookup:
            iduCells = iduLookup[iduIndex]

            # get RGB values for this field/value combo
            value = idus.iloc[i,column]
            (r,g,b) = GetIduRGB(fieldInfo,value)

            #print(f"IDU {i}, value={value}, R: {r}, G: {g}, B:{b} ")
            #print(iduCells)
            for index in iduCells:
                row  = index // width
                col = index % width
                red[row,col] = r
                grn[row,col] = g
                blu[row,col] = b
                #print(f"    index: {index}, cell: {row},{col},")
        else:
            nNoIndexEntry += 1

        # done with this IDU, move to next

    # done with all IDUs.  Load raster data and write output (a PNG with 3 bands (RGB))
    file = f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run0/RasterMaps/RMap_{scenario}_{field}_{year}_Run0.png'
    print( f"writing raster file {file}")
    try:
        os.remove(file)
    except:
        pass

    fieldRaster = rasterio.open(file, 'w', driver='PNG', height=height, width=width, count=3, dtype=np.uint8,
                crs=iduRaster.crs, transform=iduRaster.transform)
    fieldRaster.write(red,1)
    fieldRaster.write(grn,2)
    fieldRaster.write(blu,3)
    fieldRaster.close()
    print( f"  copying raster file {file} to server")
    shutil.copy(file, f'{pub.explorerStudyAreaPath}/Outputs/Scenarios/{scenario}/Run0/RasterMaps')

    return
    '''

'''
def GenerateIDUIndexRaster(idus):
    # for now, just retunr the prebuilt raster
    #print( "Loading Index raster ")
    iduRaster = rasterio.open(pub.localStudyAreaPath + '/Outputs/IDUIndexRaster/idu_raster90_wgs.tif')
    return iduRaster

def BuildIDULookupDict(iduRaster):
    band = iduRaster.read(1)
    
    (rows,cols) = band.shape
    iduLookup = {}
    i = 0
    
    #iterate though grid.  For each grid cell, get the corresponding IDU index.
    # th diction uses IDU index as key, value is an array of corresponding grid cells.
    for row in range(rows):
        if row % 100 == 0:
            print(f"  processing row {row} of {rows}")

        for col in range(cols):
            iduIndex = int(band[row][col])
            if iduIndex >= 0:
                rasterIndex = row*cols + col
                if rasterIndex != i:
                    print('index error')
                if iduIndex in iduLookup:    # if we've seen this one before, append to existing array
                    iduLookup[iduIndex].append(i)
                else:    # if first time, make an array for this iduIndex
                    iduLookup[iduIndex] = [i]  #np.array([i], dtype=np.int32)

            i += 1

    js = json.dumps(iduLookup, indent=2)
    with open(f"{pub.explorerStudyAreaPath}/Outputs/iduLookup.json", "w") as outfile:
        outfile.write(js)

    return iduLookup
'''


'''
def BuildIDULookupDict2(iduRaster):
    band = iduRaster.read(1)
    
    (rows,cols) = band.shape
    iduLookup = {}
    i = 0
    
    #iterate though grid.  For each grid cell, get the corresponding IDU index.
    # th diction uses IDU index as key, value is an array of corresponding grid cells.
    for row in range(rows):
        if row % 100 == 0:
            print(f"  processing row {row} of {rows}")

        for col in range(cols):
            iduIndex = int(band[row][col])
            if iduIndex >= 0:
                rasterIndex = row*cols + col
                if rasterIndex != i:
                    print('index error')
                if iduIndex in iduLookup:    # if we've seen this one before, append to existing array
                    values = iduLookup[iduIndex]
                    iduLookup[iduIndex] = np.append(values, i)
                else:    # if first time, make an array for this iduIndex
                    iduLookup[iduIndex] = np.array([i], dtype=np.int32)

            i += 1

    js = json.dumps(iduLookup, indent=2)
    with open(mapOutputPath + "iduLookup.json", "w") as outfile:
        outfile.write(js)

    return iduLookup
'''

'''
# main driver for creating rasters (PNGs)
def GenerateAllDeltaRasterMaps(scenarios, fieldDefs, cellSize):
    fields = [f[0] for f in fieldDefs]

    print( "Loading IDU shape file...")
    _fields = fields.copy()
    _fields.append('IDU_INDEX')
    idus = gpd.read_file(f'{pub.localStudyAreaPath}/Outputs/IDU_reduced.shp', include_fields=_fields)

    #print( "Reprojecting to EPSG:4326 (WGS84)...")
    #idus.to_crs(epsg=4326, inplace=True)

    print( "Loading Field definitions (IDU.xml)...")
    fieldInfos = LoadIduXML(fields)

    # generate index raster
    iduRaster = GenerateIDUIndexRaster(idus)

    bounds = idus.total_bounds;  # returns (minx, miny, maxx, maxy)
    (left,bottom,right,top) = bounds
    #width = int(right-left)/cellSize
    #height = int(top-bottom)/cellSize
    width = iduRaster.width
    height = iduRaster.height
    GenerateWebMapInfo(scenarios,fieldInfos, bounds, width, height)

    for scenario in scenarios:
        # change cwd to scenario ouputs folder
        os.chdir(pub.localStudyAreaPath + f'/Outputs/{scenario}/Run0') #

        #CreateFieldDeltaArray(scenario, fieldList)

        print( "Loading Delta Array")
        deltaArray = pd.read_csv( f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_Reduced.csv') # NOTE!!!!

        print( "Building IDU Lookup dictionary")
        iduLookup = BuildIDULookupDict(iduRaster)
        print(f"  {idus.shape[0]-len(iduLookup)} of {idus.shape[0]} IDUs have no corresponding raster cells")

        year = pub.startYear-1

        #print("Creating Initial rasters")
        #for field in fields:
        #    GeneratePNGsFromIDUs(idus, iduRaster, iduLookup, field, fieldInfos[field], scenario, 'start')

        # iterate though delta array, applying deltas.  On year boundaries, save the maps
        for i, delta in deltaArray.iterrows():
            dYear = delta['year']

            # new year?  save raster before proceeding
            if dYear > year:
                while (year < dYear): # in case deltaarry is missing years
                    print( f'Generating field rasters for year {year+1}')
                    for field in fields:
                        GeneratePNGsFromIDUs(idus, iduRaster, iduLookup, field, fieldInfos[field], scenario, year+1)
                    year += 1
            # apply current delta to current map
            #dCol = delta['col']
            iduField = delta['field']
            dIdu = delta['idu']
            dNewValue = delta['newValue']
            #dType = delta['type']
            
            iduCol = idus.columns.get_loc(iduField)
            idus.iat[dIdu,iduCol] = dNewValue

        # done with years, write final year
        print("Creating Final rasters")
        for field in fields:
            GeneratePNGsFromIDUs(idus, iduRaster, iduLookup, field, fieldInfos[field], scenario, year+1)

        iduRaster.close()

    return
'''

############ GEOJSON ##################

def GenerateAllDeltaGeoJsonMaps( scenarios, fieldDefs, run) :
    fields  = [f[0] for f in fieldDefs]

    print( "Generating GeoJSON maps - Loading reduced IDU shape file...")
    idus = gpd.read_file(f'{pub.localStudyAreaPath}/Outputs/IDU_reduced.shp')

    #print( "Reprojecting to EPSG:4326 (WGS84)...")
    #idus.to_crs(epsg=4326, inplace=True)

    for scenario in scenarios:
        # change cwd to scenario ouputs folder
        os.chdir(pub.localStudyAreaPath + f'/Outputs/{scenario}/Run{run}') #

        #CreateFieldDeltaArray(scenario, fieldList)

        #print( "Loading Field definitions (IDU.xml)...")
        #fieldDefs = LoadIduXML(fieldList)

        print( "Loading Delta Array")
    
        deltaArray = dt.fread( pub.localStudyAreaPath + f'/Outputs/{scenario}/Run{run}/DeltaArray_{scenario}_Reduced.csv') # NOTE, using reduced list
        deltaArray = deltaArray.to_pandas()

        year = pub.startYear-1

        # iterate though delta array, applying deltas.  On year boundaries, save the maps
        #for i, delta in deltaArray.iterrows():
        for delta in deltaArray.itertuples():
            #dYear = delta['year']
            #dYear = getattr(delta, 'year')
            dYear = delta.year

            # new year?  save map before proceeding
            if dYear > year:
                while (year < dYear): # in case deltaarry is missing years
                    #print( f'Generating GeoJson map for year {year+1}')
                    for fieldDef in fieldDefs:
                        GenGeoJsonMap(idus, fieldDef, scenario, year+1, filter=None, dissolve=True)
                    year += 1

            # apply current delta to current map
            #iduField = delta['field']
            #dIdu = delta['idu']
            #dNewValue = delta['newValue']
            #dType = delta['type']

            #iduField = getattr(delta, 'field')
            #dIdu = getattr(delta, 'idu')
            #dNewValue = getattr(delta, 'newValue')
            iduField = delta.field
            dIdu =delta.idu
            dNewValue = delta.newValue

            iduCol = idus.columns.get_loc(iduField)
            idus.iat[dIdu,iduCol] = dNewValue

        # done with years, write final year
        print("Creating Final GeoJson map")
        for fieldDef in fieldDefs:
            GenGeoJsonMap(idus, fieldDef, scenario, year+1, filter=None, dissolve=True)

    return

def GenGeoJsonMap(idus, fieldDef, scenario, run, year, filter=None, dissolve=False):
    field = fieldDef[0]
    type = fieldDef[1]
    recast = fieldDef[2]

    print( f'Generating GeoJson map [{field}] for {year}'  )
    
    if filter != None:
        startCount =  len(idus.index)
        _idus = idus.query(filter).copy()  #WRONG!~!~
        endCount =  len(_idus.index)
        print(f'     Filtered {startCount} idus to {endCount}; query={filter}')

    # filter unnneeded attributes
    fields = ['geometry', field]
    #_fields.extend(fields)
    _idus = idus[fields].copy()
    if recast:
        _idus[field] = _idus[field].astype(type)  # to_numpyt(dtype=type)
    print('     Selected IDU Count =', len(idus.index))
                
    if dissolve:
        startCount =  len(_idus.index)
        _idus = _idus.dissolve(by=field,as_index=False) #, sort=False)  version 0.9
        _idus = _idus.explode()
        print(f'     Dissolved {startCount} idus on {field} to {len(_idus.index)} features...')

    #print('     Simplifying...')
    #_idus['geometry'] = _idus.simplify(0.1, preserve_topology=True)

    #print('     Reprojecting...')
    #_idus.to_crs(epsg=4326, inplace=True)

    outPath = f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/GeoJson/Map_{field}_{scenario}_{year}.geojson'
    print('     Saving to ' + outPath)
    _idus.to_file(outPath, driver='GeoJSON')

    print(f'     Done processing {field} to {outPath}')
                
'''
def GenLegend(field):
    # parse an xml file by name
    file = minidom.parse(f'{pub.localStudyAreaPath}/idu.xml')

    #use getElementsByTagName() to get tag
    fields = file.getElementsByTagName('field')
    for f in fields:
        col = f.attributes['col'].nodeValue
        if col != field:
            continue
        # found field, extract relevant info
        label = f.attributes['label'].nodeValue
        mfiType = f.attributes['mfiType'].nodeValue    #0=quantity bins, 1=category bins
        dataType = f.attributes['datatype'].nodeValue
        attrs = []
        for c in f.childNodes:
            if c.nodeName == 'attributes':
                for a in c.childNodes:
                    if a.nodeName == "attr":
                        if mfiType == '1':     # categroy bins
                            attrs.append((a.attributes['value'].nodeValue, a.attributes['color'].nodeValue,a.attributes['label'].nodeValue))

        print(f"  Found {len(attrs)} attributes for field {field}")  
        outPath = f"{pub.localStudyAreaPath}/Outputs/Legends/{field}.json"
        print(f"  Writing to output file {outPath}")  
        outFile = open(outPath, "w")
        json.dump({'field': field, 'mfiType': int(mfiType), 'dataType': dataType, 'attrs': attrs},outFile)
        outFile.close()
'''

def GenerateVectorTiles(field, type, scenario, run, year, oflag):    
    _type = f'{type}'[8:-2] 

    if oflag == 0:
        print(f"  Starting process for {field}, {scenario}, {year}")
    if oflag==1:
        print(f"   Setting current working directory to d:{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles")
    os.chdir(f'd:{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles')

    if oflag==1:
        print (f"   Checking output folder {pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles/{field}/{year}")
    os.makedirs(f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles/{field}/{year}', mode = 0o777, exist_ok = True)

    cmd = f"wsl.exe ~/tippecanoe/tippecanoe -Q -pC --attribute-type={field}:{_type} -z14 -Z0 -f --layer={field} --drop-densest-as-needed --extend-zooms-if-still-dropping --name={field} --output-to-directory=/mnt/d/Envision/StudyAreas/{studyArea}/Outputs/{scenario}/Run{run}/VectorTiles/{field}/{year} /mnt/d{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/GeoJson/Map_{field}_{scenario}_{year}.geojson"
    if oflag==1:
        print( "  ", cmd)
    subprocess.call(cmd)
    if oflag==1:
        print()

    src = f"d:/Envision/StudyAreas/{pub.studyArea}/Outputs/{scenario}/Run{run}/VectorTiles/{field}/{year}"
    dest = f"//Eng-EnvWeb/Explorer/Topic/{pub.studyArea}/Outputs/Scenarios/{scenario}/Run{run}/VectorTiles/{field}/{year}"

    if oflag==1:
        print("   Removing ", dest)
    shutil.rmtree(dest, ignore_errors=True)
    if oflag==1:
        print("   Moving ", src)
    shutil.copytree(src, dest)  #, dirs_exist_ok=True)
    #shutil.move(src, dest)  #, dirs_exist_ok=True)
    if oflag==1:
        print()
    if oflag == 0:
        print(f"  Finished process for {field}, {scenario}, {year}")

#from multiprocessing import Pool
 
def GenerateAllVectorTiles(scenarios, runs, fieldDefs):
    for run in runs:
        for year in range(pub.startYear, pub.endYear+1):
            for scenario in scenarios:
                #args = []
                #for field in fieldDefs:
                #    args.append((field[0], field[1], scenario, year,0))
                #pool = Pool(len(args))
                #pool.map(GenerateVectorTiles, args)
                for field in fieldDefs:
                    GenerateVectorTiles(field[0],field[1], scenario, run, year, 1)
    return

# generate a set of vector tiles representing the IDU polygons
def GenIduIndexVectorTiles(scenario, run, genGeoJSON):
    if genGeoJSON:
        print( "Loading IDU shape file...")
        idus = gpd.read_file(f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/IDU_Year{pub.startYear}_{scenario}_Run{run}_reduced.shp',include_fields=['IDU_INDEX']) 
    
        print( 'Generating GeoJson for IDU_reduced.shp'  )
        print('  Simplifying...')
        idus['geometry'] = idus.simplify(10, preserve_topology=True)

        print('  Reprojecting to 4326...')
        idus.to_crs(epsg=4326, inplace=True)

        outPath = f'{pub.localStudyAreaPath}/Outputs/GeoJson/IDU_INDEX.geojson'
        print('     Saving to ' + outPath)
        idus.to_file(outPath, driver='GeoJSON')

    #outPath = f'{pub.localStudyAreaPath}/Outputs/IDUIndexShape/IDU_INDEX.shp'
    #print('     Saving to ' + outPath)
    #idus.to_file(outPath, driver ='ESRI Shapefile')

   
    #--attribute-type one of string, float, int, bool
    #--include=name   attribute name (all included by default)
    #--layer=name     specifies name of output dataset
    #-z18 -Z13  (-z16 -Z0)      most, least zoomed in level
    cmd = (
         "wsl.exe ~/tippecanoe/tippecanoe -Q -pC "
         "--include=IDU_INDEX "
         "--attribute-type=IDU_INDEX:int "
         "-z18 -Z1 -f " 
         "--layer=IDU_INDEX "
         "--drop-densest-as-needed --extend-zooms-if-still-dropping "
         "--name=IDU_INDEX "
        f"--output-to-directory=/mnt/d/Envision/StudyAreas/OrCoast/{pub.studyArea}/Outputs/VectorTiles/IDU_INDEX "
        f"/mnt/d/Envision/StudyAreas/OrCoast/{pub.studyArea}/Outputs/GeoJson/IDU_INDEX.geojson"
        )

    print( "  ", cmd)
    subprocess.call(cmd)
    #print()

    src = f"{pub.localStudyAreaPath}/Outputs/VectorTiles/IDU_INDEX"
    dest = f"{pub.explorerStudyAreaPath}/Outputs/VectorTiles/IDU_INDEX"

    print("   Removing ", dest)
    shutil.rmtree(dest, ignore_errors=True)
    print("   Moving ", src)
    shutil.copytree(src, dest)  #, dirs_exist_ok=True)
    #shutil.move(src, dest)  #, dirs_exist_ok=True)
    print()


# generate a set of vector tiles representing the IDU polygons
def GenIduVectorTiles(scenarios, runs, parallel=False,doGeoJson=True,doTippe=True):
    for run in runs:
        for scenario in scenarios:
            #oGeoJson = True
            if doGeoJson:
                print( "Loading reduced IDU shape file...")
                idus = gpd.read_file(f'{pub.localStudyAreaPath}/Outputs/IDU_reduced.shp') 

                print("Loading Delta Array...")
                deltaArray = dt.fread( pub.localStudyAreaPath + f'/Outputs/{scenario}/Run{run}/DeltaArray_{scenario}_Reduced.csv') # NOTE, using reduced list
                deltaArray = deltaArray.to_pandas()

                print(idus.crs)
                #print('  Simplifying...')
                #idus['geometry'] = idus.simplify(10, preserve_topology=True)
                #print('  Reprojecting to 4326...')
                #idus.to_crs(epsg=4326, inplace=True)

                year = pub.startYear-1

                # iterate though delta array, applying deltas.  On year boundaries, save the maps
                for i, delta in deltaArray.iterrows():
                    dYear = delta['year']

                # new year?  save as geojson before proceeding
                if dYear > year:
                    while (year < dYear): # in case deltaarry is missing years
                        print( f'Processing IDU for year {year+1}')
                        #GenerateIduVectorTileFromIDUs(idus, scenario, year+1)
                        _GenerateIduGeoJson(idus, scenario, year+1)   #VectorTileFromIDUs(idus, scenario, year+1)
                        year += 1

                # apply current delta to current map
                #dCol = delta['col']
                iduField = delta['field']
                dIdu = delta['idu']
                dNewValue = delta['newValue']
                #dType = delta['type']

                iduCol = idus.columns.get_loc(iduField)
                idus.iat[dIdu,iduCol] = dNewValue

                # done with years, write final year
                print(f"Creating Final year {year} GeoJSON")
                #GenerateIduVectorTileFromIDUs(idus, scenario, year+1)
                _GenerateIduGeoJson(idus, scenario, year+1)

            # GeoJson is generated, now run tippecanoe to convert to vector tiles
            #doTippe = True
            if doTippe:
                pids = []
                for year in range(pub.startYear, pub.endYear+1): 
                    # generate an IDU vector tile for teh current year (based on previously generated geojson)
                    pid = _GenerateIduVectorTileFromGeoJson(scenario, year, parallel)
                    if parallel:
                        pids.append(pid)

                for pid in pids:
                    pid.wait()

                # all vector tiles created, copy to server
                for year in range(pub.startYear, pub.endYear+1): 
                    src = f"{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles/IDU_reduced/{year}/"
                    dest = f"{pub.explorerStudyAreaPath}/Outputs/Scenarios/{scenario}/Run{run}/VectorTiles/IDU_reduced/{year}/"
                    print("   Removing ", dest)
                    shutil.rmtree(dest, ignore_errors=True)
                    print("   Copying ", src)
                    #os.makedirs(dest,exist_ok=True)
                    shutil.copytree(src, dest)  #, dirs_exist_ok=True)
                    #shutil.move(src, dest)  #, dirs_exist_ok=True)
                    print()
    return

'''
def GenerateIduVectorTileFromIDUs(idus, scenario, year):
    print( f"Generating IDU Geojson for year {year}")
    outPath = f'{pub.localStudyAreaPath}/Outputs/GeoJson/IDU_reduced_{year}_{scenario}.geojson'
    print('     Saving to ' + outPath)
    idus.to_file(outPath, driver='GeoJSON')

    #outPath = f'{pub.localStudyAreaPath}/Outputs/IDUIndexShape/IDU_INDEX.shp'
    #print('     Saving to ' + outPath)
    #idus.to_file(outPath, driver ='ESRI Shapefile')
    src = f"{pub.localStudyAreaPath}/Outputs/{scenario}/Run0/VectorTiles/IDU_reduced/{year}/"
    dest = f"{pub.explorerStudyAreaPath}/Outputs/{scenario}/Run0/VectorTiles/IDU_reduced/{year}/"
    print("   Making output directory ", src)
    os.makedirs(src,exist_ok=True)
    
    #--attribute-type one of string, float, int, bool
    #--include=name   attribute name (all included by default)
    #--layer=name     specifies name of output dataset
    #-z18 -Z13  (-z16 -Z0)      most, least zoomed in level
    cmd = (
        f"wsl.exe ~/tippecanoe/tippecanoe -Q -pC "
         "--attribute-type=IDU_INDEX:int "
         "-z17 -Z13 -f " 
        f"--layer=IDU_reduced_{year}_{scenario} "
         "--drop-densest-as-needed --extend-zooms-if-still-dropping "
        f"--name=IDU_reduced_{year}_{scenario} "
        f"--output-to-directory=/mnt/d/Envision/StudyAreas/{studyArea}/Outputs/{scenario}/Run0/VectorTiles/IDU_reduced/{year} "
        f"/mnt/d{pub.localStudyAreaPath}/Outputs/GeoJson/IDU_reduced_{year}_{scenario}.geojson"
        )
    print( "  ", cmd)
    subprocess.call(cmd)
    #subprocess.Popen()
    print("   Removing ", dest)
    shutil.rmtree(dest, ignore_errors=True)
    print("   Copying ", src)
    #os.makedirs(dest,exist_ok=True)
    shutil.copytree(src, dest)  #, dirs_exist_ok=True)
    #shutil.move(src, dest)  #, dirs_exist_ok=True)
    print()
'''


def _GenerateIduGeoJson(idus, scenario, year):
    print( f"Generating IDU Geojson for year {year}")
    outPath = f'{pub.localStudyAreaPath}/Outputs/GeoJson/IDU_reduced_{year}_{scenario}.geojson'
    print('     Saving to ' + outPath)
    idus.to_file(outPath, driver='GeoJSON')


def _GenerateIduVectorTileFromGeoJson(scenario, run, year,parallel):
    outDir = f"{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles/IDU_reduced/{year}/"
    print("   Making output directory ", outDir)
    os.makedirs(outDir,exist_ok=True)
    
    #--attribute-type one of string, float, int, bool
    #--include=name   attribute name (all included by default)
    #--layer=name     specifies name of output dataset
    #-z18 -Z13  (-z16 -Z0)      most, least zoomed in level
    cmd = (
        f"wsl.exe ~/tippecanoe/tippecanoe -Q -pC "
         "--attribute-type=IDU_INDEX:int "
         "-z16 -Z13 -f " 
        f"--layer=IDU_reduced_{year}_{scenario} "
         "--drop-densest-as-needed --extend-zooms-if-still-dropping "
        f"--name=IDU_reduced_{year}_{scenario} "
        f"--output-to-directory=/mnt/d/Envision/StudyAreas/{pub.studyArea}/Outputs/{scenario}/Run{run}/VectorTiles/IDU_reduced/{year} "
        f"/mnt/d{pub.localStudyAreaPath}/Outputs/GeoJson/IDU_reduced_{year}_{scenario}.geojson"
        )
    print( "  ", cmd)
    if parallel:
        pid = subprocess.Popen(cmd)
        return pid
    else:
        subprocess.call(cmd)
        return 1

def _GenerateIduIndexVectorTileFromGeoJson(scenario,run, year,parallel):
    outDir = f"{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/VectorTiles/IDU_reduced/{year}/"
    print("   Making output directory ", outDir)
    os.makedirs(outDir,exist_ok=True)
    
    #--attribute-type one of string, float, int, bool
    #--include=name   attribute name (all included by default)
    #--layer=name     specifies name of output dataset
    #-z18 -Z13  (-z16 -Z0)      most, least zoomed in level
    cmd = (
        f"wsl.exe ~/tippecanoe/tippecanoe -Q -pC "
         "--attribute-type=IDU_INDEX:int "
         "-z16 -Z13 -f " 
        f"--layer=IDU_reduced_{year}_{scenario} "
         "--drop-densest-as-needed --extend-zooms-if-still-dropping "
        f"--name=IDU_reduced_{year}_{scenario} "
        f"--output-to-directory=/mnt/d/Envision/StudyAreas/{pub.studyArea}/Outputs/{scenario}/Run{run}/VectorTiles/IDU_reduced/{year} "
        f"/mnt/d{pub.localStudyAreaPath}/Outputs/GeoJson/IDU_reduced_{year}_{scenario}.geojson"
        )
    print( "  ", cmd)
    if parallel:
        pid = subprocess.Popen(cmd)
        return pid
    else:
        subprocess.call(cmd)
        return 1


#import spatialite
import sqlite3
from contextlib import closing

'''
def GenIduIndexDB():
    path = f'{pub.localStudyAreaPath}/Outputs/OHW/Run0/IDU_Year2020_OHW_Run0_reduced.shp'

    with closing(spatialite.connect(path)) as connection:
        #connection.enable_load_extension(True)
        #connection.execute('SELECT load_extension("libspatialite-2.dll")')

        print(connection.execute('SELECT spatialite_version()').fetchone()[0])


    print("Opening idu.shp")
    idus = gpd.read_file(path)
    print("Reducing fields")
    idus = idus[['geometry','IDU_INDEX']]
    with closing(sqlite3.connect(path)) as connection:
        connection.enable_load_extension(True)
        connection.execute('SELECT load_extension("libspatialite-2.dll")')

        with closing(connection.cursor()) as cursor:
            c = connection.cursor()
            idus.iloc[:,:-1].to_sql(name=f'IDU_INDEX', con=connection, if_exists='replace', index=True, index_label="INDEX")
            connection.commit()
    print("Done!")
'''

def UpdateIDUDBTable(scenario, fieldDefs):

    iduDBPath2 = f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs/{scenario}/IDU.sqlite"
    '''
    ALTER TABLE IDUs
    ADD COLUMN column_definition;
    '''
    with closing(sqlite3.connect(iduDBPath2)) as connection:
        with closing(connection.cursor()) as cursor:
            c = connection.cursor()

            for fieldDef in fieldDefs:
                try:
                    type = "INTEGER"
                    if fieldDef[1] == float:
                        type = "REAL"
                    c.execute(f'ALTER TABLE IDUs ADD COLUMN {fieldDef[0]} {type} default null')
                    connection.commit()
                    print(f"Added NEW field {fieldDef} to IDU.sqlite")
                except sqlite3.Error as error:
                    print(f"Field {fieldDef} already present in IDU.sqlite", error)

            for field in [("Year", "INTEGER"), ("Scenario","TEXT"), ("Run","INTEGER")]:
                try:
                    c.execute(f'ALTER TABLE IDUs ADD COLUMN {field[0]} {field[1]} default null')
                    connection.commit()
                    print(f"Added NEW field {field[0]} to IDU.sqlite")
                except sqlite3.Error as error:
                    print(f"Field {field[0]} already present in IDU.sqlite", error)


'''
Create, for each scenario, a set of "reduced" IDUs that are:
1) Copied to the map server
2) copied to the scenario's SQLite database
'''
def GenAnnualIDUReduced(scenarios, runs, doSQL):
    for run in runs:
        print("---------------------------------------")
        print(" Running GenAnnualIDUReduced() for Run", run)
        print("---------------------------------------")
        #engine = create_engine('sqlite://', echo=False)

        for scenario in scenarios:

            path = f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/IDU_Year{pub.startYear}_{scenario}_Run{run}_reduced.shp'
            print( "Loading ", path)
            idus = gpd.read_file(path)
            #idus = Dbf5(f"{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/IDU_Year{pub.startYear}_{scenario}_Run{run}_reduced.dbf") 
            #idus = idus.to_dataframe()  # convert to pandas dataframe

            print( "Loading Delta Array")
            deltaArray = dt.fread(f'{pub.localStudyAreaPath}/Outputs/{scenario}/Run{run}/DeltaArray_{scenario}_Reduced.csv') # NOTE, using reduced list
            deltaArray = deltaArray.to_pandas()

            iduDBPath = f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs/{scenario}/IDU.sqlite"
            os.makedirs(f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs/{scenario}", exist_ok=True)

            year = pub.startYear-1

            # iterate though delta array, applying deltas.  On year boundaries, save the maps
            for delta in deltaArray.itertuples():
                dYear = delta.year

                # new year?  save raster before proceeding
                if dYear > year:
                    while (year < dYear): # in case deltaarry is missing years
                        year += 1
                        print( f'Generating Reduced IDUs for year {year}')
                        # make sure folder exsts
                        _mapServerPath = f"{pub.mapServerPath}/maps/{scenario}/IDU/{year}"
                        os.makedirs(_mapServerPath, exist_ok=True)
                        idus.to_file(f"{_mapServerPath}/IDU_{scenario}_{year}_{run}_reduced.shp")
                        if doSQL:
                            print( f'Generating IDU table for year {year}')
                            WriteDB(iduDBPath, idus, scenario, run, year)


                iduField = delta.field
                dIdu = delta.idu
                dNewValue = delta.newValue

                iduCol = idus.columns.get_loc(iduField)
                idus.iat[dIdu,iduCol] = dNewValue

            # done with years, write final year
            print("Creating Final year IDUs")
            _mapServerPath = f"{pub.mapServerPath}/maps/{scenario}/IDU/{year+1}"
            os.makedirs(_mapServerPath, exist_ok=True)
            idus.to_file(f"{_mapServerPath}/IDU_{scenario}_{year+1}_{run}_reduced.shp")

            if doSQL:
                print( f'Saving IDU table for year {year+1} to {iduDBPath}')
                WriteDB(iduDBPath, idus, scenario, run, year+1)
                print( f'Saving DeltaArray table to {iduDBPath}')
                WriteDB(iduDBPath, deltaArray, scenario, run, year+1, deltaArrayOnly=True)
                print( f'Saving IDU geometry to {iduDBPath}')
                WriteDB(iduDBPath, idus, scenario, run, year+1, geomOnly=True)

    return


def WriteDB(dbPath, db, scenario, run, year, geomOnly=False, deltaArrayOnly=False):
    success = False
    while success == False:
        try:
            #with engine.begin() as connection:
            #    if geomOnly==False:
            #        idus.to_sql(name=f'IDU_{scenario}_{year+1}_0', con=connection, if_exists='replace', index=True, index_label="INDEX")
            #        #deltaArray.to_sql(name=f'Deltas_{scenario}_0', con=connection, if_exists='replace', index=False)
            #    else:
            #        geom = idus['geometry']
            #        geom.to_sql(name=f'IDU_geometry', con=connection, if_exists='replace', index=False)
            #    connection.commit()

            with closing(sqlite3.connect(dbPath)) as connection:
                with closing(connection.cursor()) as cursor:
                    c = connection.cursor()

                    if deltaArrayOnly == True:
                        db.to_sql(name=f'Deltas_{scenario}_{run}', con=connection, if_exists='replace', index=False)
                    #elif geomOnly == True:
                        #geom = db['geometry'].astype('str')
                        #geom.to_sql(name=f'IDU_geometry', con=connection, if_exists='replace', index=True, index_label='ROWINDEX')
                    else:
                        db.drop(columns='geometry').to_sql(name=f'IDU_{scenario}_{year}_{run}', con=connection, if_exists='replace', index=True, index_label="ROWINDEX")

                    connection.commit()
            success=True

        except Exception as ex:
            print(str(ex))
            time.sleep(3)


# not part of core functions
def WriteDBIdus(scenarios, runs):
    for run in runs:
        print("---------------------------------------")
        print(" Running WriteDBIdus() for Run", run)
        print("---------------------------------------")
        #engine = create_engine('sqlite://', echo=False)

        for scenario in scenarios:
            iduDBPath = f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs/{scenario}/IDU.sqlite"
            os.makedirs(f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs/{scenario}", exist_ok=True)

            for year in range(pub.startYear, pub.endYear+1):
                path = f'{pub.mapServerPath}/maps/{scenario}/IDU/{year}/IDU_{scenario}_{year}_{run}_reduced.dbf'
                
                if os.path.exists(path):
                    print( "Loading ", path)

                    idus = Dbf5(path)
                    idus = idus.to_dataframe()  # convert to pandas dataframe

                    with closing(sqlite3.connect(iduDBPath)) as connection:
                        with closing(connection.cursor()) as cursor:
                            c = connection.cursor()
                            idus.to_sql(name=f'IDU_{scenario}_{year}_{run}', con=connection, if_exists='replace', index=True, index_label="ROWINDEX")
                else:
                    print("Files not found:", path)
    return





def GenMapFiles(datasetName, scenarios, runs, geomType, postFix=""):
    for run in runs:
        print("---------------------------------------")
        print(" Running GenMapFiles() ----------------")
        print("---------------------------------------")
        fieldDefs = pub.datasets[datasetName]["fieldDefs"]
    
        fields = [f[0] for f in fieldDefs]   # extract list of field names
    
        print(f"Loading Field definitions ({datasetName}.xml)...")
    
    
        # get a boundiong box for the datasets
    
        bbox = None
        with open(f"{pub.explorerStudyAreaPath}/Outputs/DatasetInfo/{datasetName}Info.json", "r") as infile:
            mapInfo = json.load(infile)
            bbox = mapInfo['boundingBox4326']  # = {"left":left, "bottom":bottom, "right":right, "top":top} 
    
        if datasetName == 'IDU':
            fieldInfos = pub.LoadIduXML(fields)
        else:
            fieldInfos = pub.LoadDatasetXML(f"{pub.iduXmlPath}/{datasetName}.xml", fields) # assumes {ds}.xml in same folder as idu.xml
    
        for scenario in scenarios:
            print(f"  Processing {scenario}, Run {run}")
            for year in range(pub.startYear,pub.endYear+1):
                _mapServerPath = f"{pub.mapServerPath}/maps/{scenario}/{datasetName}"
                os.makedirs(_mapServerPath, exist_ok=True)
                mapFile =  f"{_mapServerPath}/{year}_{run}.map"
                f = open(mapFile,"wt")
    
                f.write( f'''
MAP
	NAME "{pub.studyArea}_{datasetName}_{scenario}_{year}_{run}"
	STATUS ON
	SIZE 800 600
	#SYMBOLSET "../etc/symbols.txt"
	EXTENT {bbox["left"]}  {bbox["bottom"]} {bbox["right"]} {bbox["top"]}
	UNITS DD
	SHAPEPATH "{year}"
	IMAGECOLOR 255 255 255   # background color of image
	#FONTSET "../etc/fonts.txt"
	PROJECTION
		"init=epsg:4326"
END  

WEB
  IMAGEPATH "D:/MapServer/tmp/ms_tmp/" 
  IMAGEURL "/ms_tmp/"
  METADATA
    "wms_title"   			"{pub.studyArea}_{datasetName}_{scenario}_{year}_{run}"
    "wms_abstract"      	"Envision map server, powered by MS4W (https:/ms4w.com/)."      
    "wms_onlineresource" 	"http://envweb.bee.oregonstate.edu/MapServer//mapserv.exe?map=D:/MapServer/apps/{pub.studyArea}/maps/{scenario}/{datasetName}/{year}_{run}.map"
    "wms_service_onlineresource" "https://gatewaygeomatics.com/"    
    "wms_contactperson"		"John Bolte" 
    "wms_contactorganization" "Oregon State University"
    "wms_contactposition"	"Professor"
    "wms_contactelectronicmailaddress" "boltej@oregonstate.edu"
    "wms_srs"   			"EPSG:4326 EPSG:3857 EPSG:4269"
    "wms_getfeatureinfo_formatlist" "text/plain,text/html,application/json,application/vnd.ogc.gml,gml"
    "wms_enable_request" 	"*"
  END
END

OUTPUTFORMAT
  NAME "png"
  DRIVER AGG/PNG
  MIMETYPE "image/png"
  IMAGEMODE RGB
  EXTENSION "png"
  FORMATOPTION "GAMMA=0.75"
END

OUTPUTFORMAT
  NAME "application/json"
  DRIVER "OGR/GEOJSON"
  MIMETYPE "application/json"
  FORMATOPTION "FORM=SIMPLE"
  FORMATOPTION "STORAGE=memory"
END
''' )

                symbolPt = ''
                if geomType == 'POINT':
                    symbolPt = 'SYMBOL "point"'
                    f.write("\n# Point symbol definition ---------------------------------------------\n")
                    f.write(f'''
SYMBOL
  NAME "point"
  TYPE ELLIPSE
  POINTS
    16 16
  END
  FILLED TRUE
END
''')
                '''

# Start of layer definitions
LAYER
    NAME "FloodDepth"
    TYPE RASTER
    STATUS DEFAULT
    DATA bluemarble.gif
END
'''

    
                f.write("\n# Start of LAYER DEFINITIONS ---------------------------------------------\n")

                ext = ""
                if geomType == "RASTER":
                    ext = ".tif"
                for field in fields:
                    fieldID = field
                    if geomType == "RASTER":
                        fieldID = 'pixel'
                    f.write( f'''
  LAYER
    NAME    {field}
    DATA    "{datasetName}_{scenario}_{year}_{run}{postFix}{ext}"
    STATUS  ON
    TYPE    {geomType}
    METADATA
      "wms_title"  "{datasetName}_{field}_{scenario}_{year}_{run}{postFix}"
    END
  
    CLASSITEM  "{fieldID}"
    ''')
            
                    fieldInfo = fieldInfos[field]
                    #datatype = fieldInfo.get('datatype')
                    mfiType = fieldInfo.get('mfiType')

                    attrs = fieldInfo.findall('attributes/attr')
                    for attr in attrs:
                        color = attr.get('color').lstrip('(').rstrip(')').replace(","," ")
                        label = attr.get('label')

                        if mfiType == '1':  # categorical
                            value = attr.get('value')
                            f.write(f'''
      CLASS
        NAME "{label}"
        EXPRESSION "{value}"
        STYLE
          {symbolPt}
          COLOR    {color}
        END
      END''')
                        else:  # continuous
                          minVal = attr.get('minVal')
                          maxVal = attr.get('maxVal')
                          f.write(f'''
      CLASS
        NAME "{label}"
        EXPRESSION (([{fieldID}] >= {minVal}) AND ([{fieldID}] < {maxVal}))
        STYLE
          {symbolPt}
          COLOR      {color}
        END
      END''')
                  
                    f.write('\nEND\n')
    
                f.write('END')
                f.close()
    return


def GenerateGeometry(dataset):
    
    os.makedirs(f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs", exist_ok=True)
    outPath = f"{pub.explorerStudyAreaPath}/Outputs/SQLiteDBs/{dataset}.sqlite"
    inPath = f"{pub.localStudyAreaPath}/{dataset}.shp"

    # load dataset shape file
    print( "Loading ", inPath)
    gdf = gpd.read_file(inPath)


    iduIndexPath = f"{pub.explorerStudyAreaPath}/Outputs/ShapeFiles/IDU_INDEX"
    print( "Writing IDU_INDEX.shp to ShapeFiles folder")
    os.makedirs(iduIndexPath, exist_ok=True)
    iduIndex = gdf[['IDU_INDEX','geometry']].copy()
    print(iduIndex.shape[0])
    iduIndex.to_crs(epsg=4326, inplace=True)
    print(iduIndex.shape[0])
    iduIndex.to_file(filename=iduIndexPath, driver = 'ESRI Shapefile') 

    print( "Writing to database ", outPath)

    with closing(sqlite3.connect(outPath)) as connection:
        with closing(connection.cursor()) as cursor:
            c = connection.cursor()
            geom = []

            gdf["centroid"] = gdf["geometry"].centroid.astype('str')

            for i, row in gdf.iterrows():
                geom.append(row['geometry'].wkt)
                if len(row['geometry'].wkt) > 100000:
                    print(i, len(row['geometry'].wkt))

            gdf['geom'] = gdf['geometry'].astype('str')
            #gdf['geom'] = geom
            df = pd.DataFrame(gdf[["IDU_INDEX", "centroid", "geom"]])
            df.to_sql(name=f'IDU_geometry', con=connection, if_exists='replace', index=True, index_label='ROWINDEX')
