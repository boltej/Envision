import numpy as np
import pandas as pd
import os
import subprocess
import shutil
import geopandas as gpd
from zipfile import ZipFile

from PublishCommon import *

'''
zip up and publish:
   1) IDU_{yearXXXX}_{scenario}_Run0.shp for given years
   2) DeltaArray_{scenario}_Run0.csv
   3) Model Output CSVs

'''


def ZipAndPost(file, files, scenario):
    # file is the file name used for the zipfile

    #shutil.copy(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_reduced.csv', 
    if scenario == None:
        outZip = f'{explorerStudyAreaPath}/Data/{file}.zip'
    else:
        outZip = f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/{file}.zip'

    print("zipping ", outZip)
    with ZipFile(outZip, 'w') as zip:
        for _file in files:
            if scenario == None:
                inpath = f'{localStudyAreaPath}/{_file}' 
            else:
                inpath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/{_file}' 
    
            print("  adding ", inpath)
            zip.write(inpath)

    print("  all done!")
    #shutil.copy(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_reduced.csv', f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0')



def _Set3DData(idus,data,fields,year,startRow):
    j = startRow
    for _, row in idus.iterrows():
        pt =  row['geometry'].centroid
        data[j][0] = pt.x
        data[j][1] = pt.y
        data[j][2] = year

        k = 0
        for field in fields:
            data[j][3+k] = row[field]
            k += 1
        j += 1
    return j 
      


def _Set3DData2(idus,data,field,time,type):
    for i, _row in idus.iterrows():
        data[i][time] = _row[field].astype(type)
    return      

def _Set3DData3(idus,data,field,fieldInfo,year):
    for idu, _row in idus.iterrows():
        iduVal = _row[field]
        mfiType = fieldInfo.get("mfiType")
        bin = 0

        attrs = fieldInfo.findall("attributes/attr")
        for attr in attrs:
            if mfiType == '1' and iduVal == int(attr.get('value')): # categorical and match
                break
            elif mfiType == '0' and iduVal <= float(attr.get('maxVal')): # continious and match
                break

            bin += 1

        if bin >= len(attrs):
            _bin=np.uint8(255)  # not found
        else:
            _bin = np.uint8(bin)

        #_bin =  np.array(bin, dtype=np.uint8)

        data[year-startYear][idu] = _bin  # _row[field].astype(type)
    return      

'''
attr0Values (int32 or float * nPts * nTimes)
idu0 ... iduN
[1.0 2.1 4.3 ] t0
[1.0 2.1 4.3 ] t0

2D array (nTimes x nPts), each row is a time slice
'''

'''
Data layout (binary):
nPts (int32)
nTimes  (int32)
nAttrs (int32)
ptsXArray (int32*nPts)
ptsYArray (int32*nPts)

attr0Name (32bytes)
attr0Type (int32) (0=int,1=float)
attr0Values (int32 or float * nPts (rows) * nTimes)
...
attrNName (32bytes)
attrNType (int32) (0=int,1=float)
attrNValues (int32 or float * nPts * nTimes)
'''

def Gen3DData(scenarios, fields):
    #fields = [f[0] for f in fieldDefs]   # extract list of field names 

    fieldInfos = LoadIduXML(fields)
    years = endYear-startYear+1

    for scenario in scenarios:
        print( "Loading Initial IDUs")
        iduPath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/IDU_Year{startYear}_{scenario}_Run0_reduced.shp'
        idus = gpd.read_file(iduPath)
        iduCount = idus.shape[0]

        # if needed, populate points centroids
        if None in fields:
            print("Populating centroid points")
            ptsXArray = np.zeros(iduCount, dtype=np.float32)
            ptsYArray = np.zeros(iduCount, dtype=np.float32)
            for i, row in idus.iterrows():
                pt =  row['geometry'].centroid
                ptsXArray[i] = float(pt.x)
                ptsYArray[i] = float(pt.y)

            ptsZArray = np.zeros(years, dtype=np.float32)
            for i in range(0, years):
                ptsZArray[i] = np.float32(startYear+i)
                #start = i*iduCount
                #end = (i+1)*iduCount
                #ptsZArray[start:end] = startYear+i

            outPath = f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/PointCloud/'
            os.makedirs(outPath, exist_ok=True)
            outPath = outPath + f'IDU_{scenario}_Run0_Geom.pcg'

            # Open in "wb" mode to write a new file, or "ab" mode to append
            with open(outPath, "wb") as outfile:
                nTimes = endYear-startYear+1
                outfile.write(np.array([np.int32(iduCount),np.int32(nTimes)], dtype=np.int32).tobytes())
                outfile.write(ptsXArray.tobytes())   # np.float32
                outfile.write(ptsYArray.tobytes())
                outfile.write(ptsZArray.tobytes())
                outfile.close()

        #for other fields, iterate delta array
        # load the delta array in anticipation of playing it out
        fields = [f for f in fields if f != None]

        if len(fields) == 0:
            return
        
        print( "Loading Delta Array")
        deltaPath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_reduced.csv'
        deltaArray = pd.read_csv(deltaPath)

        # make a matrix wiyh "years" rows and "iduCount" columns, one for each field
        years = endYear-startYear+1
        datalist = {}
        for field in fields:
            datalist[field] = np.zeros((years, iduCount), dtype=np.uint8)
        year = startYear-1
        
         # iterate though delta array, applying deltas.  On year boundaries, save the maps
        for i, delta in deltaArray.iterrows():
            dYear = delta['year']
            if dYear >= endYear:
                break
            # new year?  save current year before proceeding
            if dYear > year:
                while (year < dYear): # in case deltaarry is missing years
                    year += 1
                    print('Processing year ', year)
                    for field in fields:
                        _Set3DData3(idus, datalist[field], field, fieldInfos[field], year) #, datatype)

            # apply current delta to current map
            iduField = delta['field']
            dIdu = delta['idu']
            dNewValue = delta['newValue']
            iduCol = idus.columns.get_loc(iduField)
            idus.iat[dIdu,iduCol] = dNewValue

        # done with years, write final year
        #print("Creating Final rasters")
        outPath = f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/PointCloud/'
        os.makedirs(outPath, exist_ok=True)
        for field in fields:
            _Set3DData3(idus, datalist[field], field, fieldInfos[field], year)
            #cols = ['x','y','Year'] + fields
            #df = pd.DataFrame(data,columns=cols) '''
                # Open in "wb" mode to write a new file, or "ab" mode to append
            with open(outPath+ f'{field}_{scenario}_Run0.pcd', "wb") as outfile:
                nTimes = endYear-startYear+1
                outfile.write(np.array([iduCount,nTimes], dtype=np.int32).tobytes())
                outfile.write(datalist[field].tobytes())
                outfile.close()


def rgb_to_int(r, g, b):
    return (r << 16) + (g << 8) + b



def Gen3DDeltaData(scenarios, fields):
    #fields = [f[0] for f in fieldDefs]   # extract list of field names 

    fieldInfos = LoadIduXML(fields)
    years = endYear-startYear+1

    for scenario in scenarios:
        # get IDUs to compute centroids
        print( "Loading Initial IDUs")
        iduPath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/IDU_Year{startYear}_{scenario}_Run0_reduced.shp'
        idus = gpd.read_file(iduPath)
        iduCount = idus.shape[0]    

        #for other fields, iterate delta array
        # load the delta array in anticipation of playing it out
        fields = [f for f in fields if f != None]

        if len(fields) == 0:
            return
        
        print( "Loading Delta Array")
        deltaPath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_reduced.csv'
        deltaArray = pd.read_csv(deltaPath)

        # make a matrix with "years" rows and "iduCount" columns, one for each field
        years = endYear-startYear
        binLists = {}   # datalist: key=field, value=matrix(years,idus)
        rgbLists = {}   # datalist: key=field, value=matrix(years,idus)
        for field in fields:
            binLists[field] = np.zeros((years, iduCount), dtype=np.int8)  # contains 1-based bin values
            rgbLists[field] = np.zeros((years, iduCount), dtype=np.uint32)  # contains rgb values
        
         # iterate though delta array, applying deltas.  On year boundaries, save the maps
        deltaCount = 0
        for i, delta in deltaArray.iterrows():
            dField = delta['field']

            if i % 1000000 == 0:
                print(f"  Delta {i} of {len(deltaArray.index)}")
                
            if dField in fields:
                dOldValue = delta['oldValue']
                dNewValue = delta['newValue']
                if dOldValue != dNewValue:
                    dYear = delta['year']
                    dIdu = delta['idu']
                    
                    # get the class of the new value
                    fieldInfo = fieldInfos[dField]
                    mfiType = fieldInfo.get("mfiType")
                    bin = 0

                    attrs = fieldInfo.findall("attributes/attr")
                    bin = 0
                    rgb = rgb_to_int(250,250,250)
                    for attr in attrs:
                        if mfiType == '1' and dNewValue == int(attr.get('value')): # categorical and match
                            color = attr.get('color')  # (158,238,158)
                            color = color.replace('(','').replace(')','')
                            r,g,b = color.split(",")
                            rgb = rgb_to_int(int(r), int(g), int(b))
                            break
                        elif mfiType == '0' and dNewValue <= float(attr.get('maxVal')): # continious and match
                            color = attr.get('color')  # (158,238,158)
                            color = color.replace('(','').replace(')','')
                            r,g,b = color.split(",")
                            rgb = rgb_to_int(int(r), int(g), int(b))
                            break
                        bin += 1

                    if bin >= len(attrs):
                        _bin=np.uint8(-1)  # not found
                    else:
                        _bin = np.uint8(bin)
                        deltaCount += 1

                    binLists[dField][dYear-startYear][dIdu] = _bin  # _row[field].astype(type)
                    rgbLists[dField][dYear-startYear][dIdu] = rgb  # _row[field].astype(type)

        print(f"Before Identification: Count={deltaArray.shape[0]}, after identification: Count={deltaCount} deltas")
         
        # done iterating delta array - we've identified delta points (idus/times) where change occured.
        # next, generate a binary dataset containing class value for each idu for each field
        for field in fields:
            binList = binLists[field]
            rgbList = rgbLists[field]

            #deltaCount = np.count_nonzero(binList)
            print(f'{field} has {deltaCount} deltas')

            minx = 99999999
            maxx = -99999999
            miny = 99999999
            maxy = -99999999
            minz = startYear
            maxz = endYear
            
            binArray = np.zeros(deltaCount, dtype=np.uint8)
            rgbArray = np.zeros(deltaCount, dtype=np.uint32) + rgb_to_int(250,250,250)
            ptsXArray = np.zeros(deltaCount, dtype=np.float32)
            ptsYArray = np.zeros(deltaCount, dtype=np.float32)
            ptsZArray = np.zeros(deltaCount, dtype=np.float32)
            i = 0
            print("Identifying delta points")
            for year in range(years):
                for idu in range(iduCount):
                    bin = binList[year][idu]
                    if bin > 0:
                        binArray[i] = bin  # 1-based bin index
                        rgbArray[i] = rgbLists[field][year][idu]  # rgb uint
                        
                        pt = idus.loc[idu,'geometry'].centroid                        
                        ptsXArray[i] = float(pt.x)
                        ptsYArray[i] = float(pt.y)
                        ptsZArray[i] = float(startYear+year)  # actual year, not year of run
                        
                        if float(pt.x) < minx:
                            minx = pt.x
                        if float(pt.x) > maxx:
                            maxx = pt.x
                        if float(pt.y) < miny:
                            miny = pt.y
                        if float(pt.y) > maxy:
                            maxy = pt.y
                        i += 1

            outPath = f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/PointCloud/'
            os.makedirs(outPath, exist_ok=True)
            #file = outPath + f'IDU_Delta_{scenario}_Run0_{field}.pcd'
            #with open(file, "wb") as outfile:
            #    outfile.write(np.array([np.int32(i)], dtype=np.int32).tobytes())
            #    outfile.write(data.tobytes())   # np.float32
            #    outfile.close()

            file = outPath + f'IDU_Delta_{scenario}_Run0_{field}.pc'
            with open(file, "wb") as outfile:
                outfile.write(np.array([np.int32(i)], dtype=np.int32).tobytes())
                outfile.write(np.array([minx,maxx,miny,maxy,minz,maxz], dtype=np.float32).tobytes())
                outfile.write(ptsXArray.tobytes())   # np.float32
                outfile.write(ptsYArray.tobytes())
                outfile.write(ptsZArray.tobytes())
                outfile.write(binArray.tobytes())   # np.uint8
                outfile.close()


            '''file = outPath + f'IDU_Delta_{scenario}_Run0_{field}.pcd'
            with open(file, "wt") as outfile:
                outfile.write(f'VERSION 0.7\n')
                outfile.write(f'FIELDS x y z rgb\n')
                outfile.write(f'SIZE 4 4 4 4\n')
                outfile.write(f'TYPE F F F U\n')  # NOTE unsigned int
                outfile.write(f'COUNT 1 1 1 1\n')
                outfile.write(f'WIDTH {len(ptsXArray)}\n')
                outfile.write(f'HEIGHT 1\n')
                outfile.write(f'VIEWPOINT 0 0 0 1 0 0 0\n')
                outfile.write(f'POINTS {len(ptsXArray)}\n')
                outfile.write(f'DATA ascii\n')

                for i in range(0,len(ptsXArray)):
                    outfile.write(f'{ptsXArray[i]:.6f} {ptsYArray[i]:.6f} {ptsZArray[i]:.6f} {rgbArray[i]}\n')
                outfile.close()
            '''

    return



def Gen3DDeltaData2(scenarios, fields):
    #fields = [f[0] for f in fieldDefs]   # extract list of field names 

    fieldInfos = LoadIduXML(fields)
    years = endYear-startYear+1

    for scenario in scenarios:
        # get IDUs to compute centroids
        print( "Loading Initial IDUs")
        iduPath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/IDU_Year{startYear}_{scenario}_Run0_reduced.shp'
        idus = gpd.read_file(iduPath)
        iduCount = idus.shape[0]    

        minx, miny, maxx, maxy = idus.total_bounds
        minz = startYear
        maxz = endYear

        #for other fields, iterate delta array
        # load the delta array in anticipation of playing it out
        fields = [f for f in fields if f != None]

        if len(fields) == 0:
            return
        
        print( "Loading Delta Array")
        deltaPath = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_reduced.csv'
        deltaArray = pd.read_csv(deltaPath)

        # make a matrix with "years" rows and "iduCount" columns, one for each field
        years = endYear-startYear
        xLists = {}   # datalist: key=field, value=matrix(years,idus)
        yLists = {}   # datalist: key=field, value=matrix(years,idus)
        zLists = {}   # datalist: key=field, value=matrix(years,idus)
        binLists = {}   # datalist: key=field, value=matrix(years,idus)
        rgbLists = {}   # datalist: key=field, value=matrix(years,idus)
        for field in fields:
            xLists[field] = []
            yLists[field] = []
            zLists[field] = []
            binLists[field] = [] # np.zeros((years, iduCount), dtype=np.int8)  # contains 1-based bin values
            rgbLists[field] = [] # np.zeros((years, iduCount), dtype=np.uint32)  # contains rgb values
        
         # iterate though delta array, applying deltas.  On year boundaries, save the maps
        deltaCount = 0
        for i, delta in deltaArray.iterrows():
            dField = delta['field']

            if i % 1000000 == 0:
                print(f"  Delta {i} of {len(deltaArray.index)}")
                
            if dField in fields:
                dOldValue = delta['oldValue']
                dNewValue = delta['newValue']
                if dOldValue != dNewValue:
                    dYear = delta['year']
                    dIdu = delta['idu']
                    
                    # get the class of the new value
                    fieldInfo = fieldInfos[dField]
                    mfiType = fieldInfo.get("mfiType")
                    bin = 0

                    attrs = fieldInfo.findall("attributes/attr")
                    bin = 0
                    rgb = rgb_to_int(250,250,250)
                    for attr in attrs:
                        if mfiType == '1' and dNewValue == int(attr.get('value')): # categorical and match
                            color = attr.get('color')  # (158,238,158)
                            color = color.replace('(','').replace(')','')
                            r,g,b = color.split(",")
                            rgb = rgb_to_int(int(r), int(g), int(b))
                            break
                        elif mfiType == '0' and dNewValue <= float(attr.get('maxVal')): # continious and match
                            color = attr.get('color')  # (158,238,158)
                            color = color.replace('(','').replace(')','')
                            r,g,b = color.split(",")
                            rgb = rgb_to_int(int(r), int(g), int(b))
                            break
                        bin += 1

                    if bin >= len(attrs):
                        _bin=np.uint8(-1)  # not found
                    else:
                        _bin = np.uint8(bin)
                        deltaCount += 1

                    pt = idus.loc[dIdu,'geometry'].centroid                        
                    xLists[dField].append(float(pt.x))
                    yLists[dField].append(float(pt.y))
                    zLists[dField].append(float(dYear))  # actual year, not year of run
                    binLists[dField].append(_bin)
                    rgbLists[dField].append(rgb)  # _row[field].astype(type)

        #print(f"Before Identification: Count={deltaArray.shape[0]}, after identification: Count={deltaCount} deltas")
         
        # done iterating delta array - we've identified delta points (idus/times) where change occured.
        # next, generate a binary dataset containing class value for each idu for each field
        for field in fields:

            outPath = f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/PointClouds/'
            os.makedirs(outPath, exist_ok=True)

            xs = np.array(xLists[field], dtype=np.float32)
            ys = np.array(yLists[field], dtype=np.float32)
            zs = np.array(zLists[field], dtype=np.float32)
            bins = np.array(binLists[field], dtype=np.uint8)
            rgbs = np.array(rgbLists[field],dtype=np.int32)  # _row[field].astype(type)

            file = outPath + f'IDU_Delta_{scenario}_Run0_{field}.pc'
            print(f'Writing {len(xs)} Deltas to  {file}' )
            with open(file, "wb") as outfile:
                outfile.write(np.array([np.int32(len(xs))], dtype=np.int32).tobytes())
                outfile.write(np.array([minx,maxx,miny,maxy,minz,maxz], dtype=np.float32).tobytes())
                outfile.write(xs.tobytes())   # np.float32
                outfile.write(ys.tobytes())
                outfile.write(zs.tobytes())
                outfile.write(bins.tobytes())   # np.uint8
                outfile.write(rgbs.tobytes())   # np.uint8
                outfile.close()


            '''file = outPath + f'IDU_Delta_{scenario}_Run0_{field}.pcd'
            with open(file, "wt") as outfile:
                outfile.write(f'VERSION 0.7\n')
                outfile.write(f'FIELDS x y z rgb\n')
                outfile.write(f'SIZE 4 4 4 4\n')
                outfile.write(f'TYPE F F F U\n')  # NOTE unsigned int
                outfile.write(f'COUNT 1 1 1 1\n')
                outfile.write(f'WIDTH {len(ptsXArray)}\n')
                outfile.write(f'HEIGHT 1\n')
                outfile.write(f'VIEWPOINT 0 0 0 1 0 0 0\n')
                outfile.write(f'POINTS {len(ptsXArray)}\n')
                outfile.write(f'DATA ascii\n')

                for i in range(0,len(ptsXArray)):
                    outfile.write(f'{ptsXArray[i]:.6f} {ptsYArray[i]:.6f} {ptsZArray[i]:.6f} {rgbArray[i]}\n')
                outfile.close()
            '''

    return







def DumpPCHeader(path):
    size = int(np.fromfile(path, dtype=np.int32, count=1, offset=0))
    print('Count: ', size)
    bounds = np.fromfile(path, dtype=np.float32, count=6, offset=4)
    print('Bounds: ', bounds)
    ptsX = np.fromfile(path, dtype=np.float32, count=6, offset=28)
    print('Xs: ', ptsX)
    ptsY = np.fromfile(path, dtype=np.float32, count=6, offset=28+(size*4))
    print('Ys: ', ptsY)
    ptsZ = np.fromfile(path, dtype=np.float32, count=6, offset=28+2*(size*4))
    print('Zs: ', ptsZ)
    bins = np.fromfile(path, dtype=np.uint8, count=6, offset=28+3*(size*4))
    print('Bins: ', bins)
    rgbs = np.fromfile(path, dtype=np.int32, count=6, offset=28+(3*(size*4))+size)
    print('RGBs: ', rgbs)



############ M A I N ###############
'''
Basic idea - 
1) Specify a study area, start/end years,scenario names, and field definitions in the 
   variable block below
'''
fields = [f[0] for f in fieldDefs]   # extract list of field names
Gen3DDeltaData2(scenarios, fields)#           + fields)



outPath = f'{explorerStudyAreaPath}/Outputs/OHW/Run0/PointClouds/'
file = outPath + f'IDU_Delta_OHW_Run0_DISTURB.pc'
DumpPCHeader(file)