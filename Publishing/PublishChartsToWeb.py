from __future__ import annotations
from tkinter import FALSE
import sys
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import plotly.graph_objects as go
import plotly.colors as pc
import geopandas as gp
import plotly.express as px
from plotly.subplots import make_subplots
from simpledbf import Dbf5
import os
import json
import random
import seaborn as sns
from pathlib import Path

from sklearn.linear_model import LinearRegression
import matplotlib

from PublishCommon import *


matplotlib.use('agg')

plot_bgColor = 'rgba(245,245,245,1)'

def GenFigure(fig, chartName, scenario):
    os.makedirs(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Charts', exist_ok=True)
    
    if showCharts:
        fig.show()
    else:
        print(f"Writing {explorerStudyAreaPath}/Outputs/{scenario}/Run0/Charts/{chartName}.html" )
        fig.write_html(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Charts/{chartName}.html', include_plotlyjs=False, full_html=False )

def GenSSI(text, name, path, uploadFolder):
        with open(path + name + '.ssi', 'w') as f:
            print(f"Writing {path}{name}.ssi" )
            f.write(text)

def ComputePop(data):
    pop = data['PopDens']*data['AREA']
    return pop


def GenSnipTables(scenarios):
    '''
       Generates the following:
         1) {localStudyAreaPath}/Outputs/{scenario}/Run0/LACountsByFN.csv
         2) {explorerStudyAreaPath}/Outputs/{scenario}/Run0/Tables/LACountsByFN.html
         3) {explorerStudyAreaPath}/Outputs/{scenario}/Run0/Tables/SNIPStats.html
    '''
    for scenario in  scenarios:  
        print( "Loading IDU database...")
        dbf = Dbf5(f"{localStudyAreaPath}/Outputs/{scenario}/Run0/IDU_Year2020_{scenario}_Run0.dbf") 
        df = dbf.to_dataframe()  # convert to pandas dataframe

        print("Subsetting fields")
        df = df[["FN_ID", "Profile", "SN_Engager", "PopDens", "AREA"]]

        d = {}  # key=FN_ID, value=(iduCount,actorCount, pop)
        dEng = {} # dictionary tracking engagers
        for index, row in df.iterrows():
            fnID = int(row["FN_ID"])
            profileID = int(row["Profile"])
            engager = row["SN_Engager"]
            pop = row['PopDens']*row['AREA']

            cLA = 0
            uLA = 0
            cPop = 0
            uPop = 0

            hasEngager = pd.notna(engager)

            if hasEngager:
                eList = engager.split(",")

                if fnID in dEng.keys():
                    engs = dEng[fnID]   # this is a set (unique list)  of engagers in this FN
                    for e in eList:
                        engs.add(e)
                    dEng[fnID] = engs
                else:
                    dEng[fnID] = set(eList)
        
            if profileID > 0 and hasEngager:
                cLA = 1
                cPop = pop
            else:
                uLA = 1
                uPop = pop

            if fnID in d.keys():
                (iduCount, _cLA, _uLA, _cPop, _uPop, totalPop) = d[fnID]
                d[fnID] = (iduCount+1,  _cLA+cLA, _uLA+uLA, _cPop+cPop, _uPop+uPop, totalPop+pop)
            else:
                d[fnID] = (1, cLA, uLA, cPop, uPop, pop)


            #cp = d[fnID][3]
            #up = d[fnID][4]
            # p = d[fnID][5]
            #if cp+up > p:
            #    print("issue!")

        # have fn dict, make dataframe
        fnIDs = []
        iduCounts = []
        cLAs = []
        uLAs = []
        cPops = []
        uPops = []
        totalPops = []
        cEngs = []
        fPopEngs = []
        for fn in d.keys():
            fnIDs.append(fn)
            (iduCount,cLA, uLA, cPop, uPop, totalPop) = d[fn]
            iduCounts.append(iduCount)
            cLAs.append(cLA)
            uLAs.append(uLA)
            cPops.append(int(cPop))
            uPops.append(int(uPop))
            totalPops.append(int(totalPop))
            if fn in dEng:
                cEngs.append(len(dEng[fn]))
            else:
                cEngs.append(0)

            if totalPop == 0:
                fPopEngs.append(0)
            else:
                fPopEngs.append(cPop/totalPop)

        df = pd.DataFrame({'FN_ID': fnIDs, 'IDUs':iduCounts, 'Connected LActors':cLAs, 'Unconnected LActors':uLAs, 
                'Unique Engagers': cEngs, 'Connected Pop':cPops, 'Unconnected Pop': uPops, 'Total Pop': totalPops,
                'Frac Pop Engaged': fPopEngs})
        df = df.sort_values(by='FN_ID')
        df.to_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/LACountsByFN.csv',index=False)
        print(f'Writing {explorerStudyAreaPath}/Outputs/{scenario}/Run0/Tables/LACountsByFN.html')

        os.makedirs(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Tables', exist_ok=True)
        df.to_html(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Tables/LACountsByFN.html', index=False,justify='center')


def GenSnipFigures(scenarios):
    for scenario in scenarios:

        # Fireshed Neighborhood - number of connections to SNIP
        print('Loading FNs.shp')
        fns = gp.read_file(f'{localStudyAreaPath}/FNs_wgs.shp')

        fns.to_file(f'{explorerStudyAreaPath}/Outputs/fns.json', driver='GeoJSON')

        #print( "Reprojecting to EPSG:4326 (WGS84)...")
        #fns.to_crs(epsg=4326, inplace=True)

        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/LACountsByFN.csv')

        fns = fns.merge(data, on='FN_ID', how='right')

        (left,bottom,right,top)  = fns.total_bounds

        fig = px.choropleth_mapbox(fns,
                               title='Numbers of Landscape actors connected to WMN<br><sup>By Fireshed Neighborhood</sup>',
                               geojson=fns.geometry,
                               locations=fns.index,
                               color="Connected LActors",
                               color_continuous_scale=px.colors.sequential.Reds,
                               center={"lat": (top+bottom)/2, "lon": (left+right)/2},
                               mapbox_style="open-street-map",
                               zoom=7.2)
        fig.update_layout({'plot_bgcolor': plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',})
        GenFigure(fig, "ConnectedLAs_map", scenario)

        fig = px.choropleth_mapbox(fns,
                               title='Population connected to WMN<br><sup>By Fireshed Neighborhood</sup>',
                               geojson=fns.geometry,
                               locations=fns.index,
                               color="Connected Pop",
                               color_continuous_scale=px.colors.sequential.Reds,
                               center={"lat": (top+bottom)/2, "lon": (left+right)/2},
                               mapbox_style="open-street-map",
                               zoom=7.2)
        fig.update_layout({'plot_bgcolor': plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',})
        GenFigure(fig, "ConnectedPop_map", scenario)

        fig = px.choropleth_mapbox(fns,
                               title='Numbers of Unique Engagers connected to WMN<br><sup>By Fireshed Neighborhood</sup>',
                               geojson=fns.geometry,
                               locations=fns.index,
                               color="Unique Engagers",
                               color_continuous_scale=px.colors.sequential.Reds,
                               center={"lat": (top+bottom)/2, "lon": (left+right)/2},
                               mapbox_style="open-street-map",
                               zoom=7.2)
        fig.update_layout({'plot_bgcolor': plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',})
        GenFigure(fig, "UniqueEngagers_map", scenario)

        fig = px.choropleth_mapbox(fns,
                               title='Fraction of Population Engaged by WMN<br><sup>By Fireshed Neighborhood</sup>',
                               geojson=fns.geometry,
                               locations=fns.index,
                               color="Frac Pop Engaged",
                               color_continuous_scale=px.colors.sequential.Reds,
                               center={"lat": (top+bottom)/2, "lon": (left+right)/2},
                               mapbox_style="open-street-map",
                               zoom=7.2)
        fig.update_layout({'plot_bgcolor': plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',})
        GenFigure(fig, "FracPopEngaged_map", scenario)

    return


def GenSnipReactivities(scenarios):
    for scenario in scenarios:
        # Network Reactivities
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/WMN_{scenario}_Run0.csv')
    
        fig = go.Figure()
        times = data.iloc[1:,0]
        inputActivation = data.loc[1:,"inputActivation"]
        meanNonLANodeReactivity = data.loc[1:,"meanNonLAReactivity"]
        meanLANodeReactivity = data.loc[1:,"meanLAReactivity"]
        meanNonLAEdgeTransEff = data.loc[1:,"meanNonLAEdgeTransEff"]
        meanLAEdgeTransEff = data.loc[1:,"meanLAEdgeTransEff"]
        '''
        stdNodeReactivity = data.loc[1:,"stdNodeReactivity"]
        stdLAReactivity = data.loc[1:,"stdLAReactivity"]
        stdEdgeTransEff = data.loc[1:,"stdEdgeTransEff"]
        
        stdNodeReactivityUpper = meanNodeReactivity + stdNodeReactivity
        stdNodeReactivityLower =  meanNodeReactivity - stdNodeReactivity
        stdLAReactivityUpper = meanLAReactivity + stdLAReactivity
        stdLAReactivityLower =  meanLAReactivity- stdLAReactivity
        stdEdgeTransEffUpper = meanEdgeTransEff + stdEdgeTransEff
        stdEdgeTransEffLower =  meanEdgeTransEff - stdEdgeTransEff

        # variance areas
        fig.add_trace(go.Scatter(x=times, y=stdNodeReactivityUpper, fill='none', mode="lines"))
        fig.add_trace(go.Scatter(x=times, y=stdNodeReactivityLower, fill='tonexty', mode="lines"))

        fig.add_trace(go.Scatter(x=times, y=stdLAReactivityUpper, fill='none', mode="lines"))
        fig.add_trace(go.Scatter(x=times, y=stdLAReactivityLower, fill='tonexty', mode="lines"))

        fig.add_trace(go.Scatter(x=times, y=stdEdgeTransEffUpper, fill='none', mode="lines"))
        fig.add_trace(go.Scatter(x=times, y=stdEdgeTransEffLower, fill='tonexty', mode="lines"))
        '''
        # means
        fig.add_trace(go.Scatter(x=times, y=inputActivation, mode='lines', name='Landscape Signal'))
        fig.add_trace(go.Scatter(x=times, y=meanNonLANodeReactivity, mode='lines', name='Mean Network Node Reactivity' )) #,
            #error_y=dict(
            #    type='data', # value of error bar given in data coordinates
            #    array=stdNodeReactivity,
            #    visible=True)))
        fig.add_trace(go.Scatter(x=times, y=meanLANodeReactivity, mode='lines', name='Mean Landscape Actor Reactivity')) #,
            #error_y=dict(
            #    type='data', # value of error bar given in data coordinates
            #    array=stdLAReactivity,
            #    visible=True)))
        fig.add_trace(go.Scatter(x=times, y=meanNonLAEdgeTransEff, mode='lines', name='Mean Network Edge TranEff')) #,
            #error_y=dict(
            #    type='data', # value of error bar given in data coordinates
            #    array=stdEdgeTransEff,
            #    visible=True)))

        fig.add_trace(go.Scatter(x=times, y=meanLAEdgeTransEff, mode='lines', name='MeanLandscape Edge TranEff')) #,
            #error_y=dict(
            #    type='data', # value of error bar given in data coordinates
            #    array=stdEdgeTransEff,
            #    visible=True)))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
        'title': f"Wildfire Management Network<br><sub>Transmission/Reaction Results, {scenario}"})

        GenFigure(fig, "WMN_NetMetrics2_chart", scenario)  
    return


def GenSnipReactivities2(scenarios):
    for scenario in scenarios:
        # Network Reactivities
        data = GenSnipStats(scenario)
    
        fig = go.Figure()
        times = data.iloc[1:,0]
        inputActivation = data.loc[1:,"Input Signal"]
        meanNodeReactivity = data.loc[1:,"Mean Network Reactivity"]
        meanLAReactivity = data.loc[1:,"Mean LA Reactivity"]
        meanEdgeTransEff = data.loc[1:,"Mean Network Edge Trust"]
        meanEdgeLATransEff = data.loc[1:,"Mean LA Trust"]
        # means
        fig.add_trace(go.Scatter(x=times, y=inputActivation, mode='lines', name='Landscape Signal'))
        fig.add_trace(go.Scatter(x=times, y=meanNodeReactivity, mode='lines', name='Mean Network Node Reactivity'))
        fig.add_trace(go.Scatter(x=times, y=meanLAReactivity, mode='lines', name='Mean Landscape Actor Reactivity'))
        fig.add_trace(go.Scatter(x=times, y=meanEdgeTransEff, mode='lines', name='Mean Edge TranEff'))
        fig.add_trace(go.Scatter(x=times, y=meanEdgeLATransEff, mode='lines', name='Mean LA Edge TranEff'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': "Wildfire Management Network<br><sub>Transmission/Reaction Results - {scenario}"})

        GenFigure(fig, "NetMetrics_chart", scenario)  
    return



def GenSnipStats(scenario):
    
    meanInputSignal = []
    meanNNReactivity = []
    meanLAReactivity = []
    meanNETrust = []
    meanLATrust = []
    years = []
    for year in range(startYear, endYear):
        path=f'{localStudyAreaPath}/Outputs/{scenario}/Run0/SNIP_WMN_Year{year}_{scenario}_Run0.json'
        with open(path) as f:
            data = json.load(f)
            nodes = data['network']['nodes']
            edges = data['network']['edges']
            meanInput  = 0
            inputCount = 0
            meanNNR = 0
            meanLAR = 0
            meanETr = 0
            meanLAtr = 0
            naCount = 0
            laCount = 0
            naEdgeCount = 0
            laEdgeCount = 0
            nodeDict = {}
            for node in nodes:
                nodeDict[node['name']] = node
                reactivity = node['reactivity']
                if node['type'] in [ 'network', 'assessor', 'engager']:
                    meanNNR += reactivity
                    naCount += 1
                elif node['type'] == 'landscape':
                    meanLAR += reactivity
                    laCount += 1
                elif node['type'] == 'input':
                    meanInput += reactivity
                    inputCount += 1
            for edge in edges:
                trust  = float(edge['trust'])
                toNode = nodeDict[edge['to']]
                if toNode['type'] == 'landscape':
                    meanLAtr += trust
                    laEdgeCount += 1
                elif toNode['type'] in [ 'network', 'assessor', 'engager']:
                    meanETr += trust
                    naEdgeCount += 1
            #print( f'Mean Input={meanInput/inputCount:.2f} from {inputCount} nodes')
            #print( f'Mean Network Node Reactivity={meanNNR/naCount:.2f} from {naCount} nodes, Mean LA Node Reactivity={meanLAR/laCount:.2f} from {laCount} nodes')
            #print( f'Mean Network Edge Trust={meanETr/naEdgeCount:.2f} from {naEdgeCount} edges, Mean LA Edge Trust={meanLAtr/laEdgeCount:.2f} from {laEdgeCount} edges')
            #print()
            meanInputSignal.append(meanInput/inputCount)
            meanNNReactivity.append(meanNNR/naCount)
            meanLAReactivity.append(meanLAR/laCount)
            meanNETrust.append(meanETr/naEdgeCount)
            meanLATrust.append(meanLAtr/laEdgeCount)
            years.append(year)

    zipped = list(zip(years,meanInputSignal, meanNNReactivity, meanLAReactivity, meanNETrust, meanLATrust))

    df = pd.DataFrame(zipped, columns=['Year','Input Signal', 'Mean Network Reactivity', 'Mean LA Reactivity', 'Mean Network Edge Trust', 'Mean LA Trust'])
    return df


def GenSnipPlots(scenarios):
    
    figOutreach = go.Figure()
    figSocialCap = go.Figure()
       
    for scenario in scenarios:

        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/ModelOutput_{scenario}_Run0.csv')
        data1 = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/SocialCapital_{scenario}_Run0.csv')

        data1 = data1.replace(" -nan(ind)", 0)

        figAttitudeOutreach = go.Figure()
        figAttitudeNoOutreach = go.Figure()

        times = data.iloc[2:,0]
        yOutreach=data.loc[2:,'Report.Outreach Area (ha)']

        
        if True:  #"NoM" not in scenario:
            yAttitudeOutreach=data1.loc[1:,'Actor Attitude-Outreach.value'].astype(np.float32)
            yAttitudeNoOutreach=data1.loc[1:,'Actor Attitude-NoOutreach.value'].astype(np.float32)
            yAttOutStdDev = data1.loc[1:,'Actor Attitude-Outreach.stddev'].astype(np.float32)
            yAttNoOutStdDev = data1.loc[1:,'Actor Attitude-NoOutreach.stddev'].astype(np.float32)
   
            ySocialCap=data.loc[2:,'Report.Social Capital']

            figOutreach.add_trace(go.Scatter(x=times, y=yOutreach, mode='lines', name=f"{scenario}", line={'color': scenarioColors[scenario]}))
            figSocialCap.add_trace(go.Scatter(x=times, y=ySocialCap, mode='lines', name=f"{scenario}", line={'color': scenarioColors[scenario]}))

            figAttitudeOutreach.add_trace(go.Scatter(x=times, y=yAttitudeOutreach, mode='lines', name="Actor Attitude-Outreach"))
            figAttitudeOutreach.add_trace(go.Scatter(x=times, y=yAttitudeOutreach+yAttOutStdDev, mode='lines', name='+1 stddev', marker=dict(color="#444"),
                line=dict(width=0), showlegend=False))
            figAttitudeOutreach.add_trace(go.Scatter(x=times, y=yAttitudeOutreach-yAttOutStdDev, mode='lines', name='-1 stddev', marker=dict(color="#444"),
                line=dict(width=0), fillcolor='rgba(68, 68, 68, 0.3)', fill='tonexty', showlegend=False))

            figAttitudeNoOutreach.add_trace(go.Scatter(x=times, y=yAttitudeNoOutreach, mode='lines', name="Actor Attitude-No Outreach"))
            figAttitudeNoOutreach.add_trace(go.Scatter(x=times, y=yAttitudeNoOutreach+yAttNoOutStdDev, mode='lines', name='+1 stddev', marker=dict(color="#444"),
                line=dict(width=0), showlegend=False))
            figAttitudeNoOutreach.add_trace(go.Scatter(x=times, y=yAttitudeNoOutreach-yAttNoOutStdDev, mode='lines', name='-1 stddev', marker=dict(color="#444"),
                line=dict(width=0), fillcolor='rgba(168, 68, 68, 0.3)', fill='tonexty', showlegend=False))

        figAttitudeOutreach.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actor Attitude (Outreach)[-1,1]<br><sub>{scenario} scenario"})
        figAttitudeNoOutreach.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actor Attitude (No Outreach)[-1,1]<br><sub>{scenario} scenario"})

        GenFigure(figAttitudeOutreach, "ActorAttitudeOutreach_chart", scenario) 
        GenFigure(figAttitudeNoOutreach, "ActorAttitudeNoOutreach_chart", scenario) 


    figOutreach.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Outreach (ha)<br><sub>All scenarios"})
 
    figSocialCap.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Social Capital [-1,1]<br><sub>All scenarios"})

    GenFigure(figOutreach, "Outreach_chart", 'All') 
    GenFigure(figSocialCap, "SocialCap_chart", 'All') 


    return



def GenA2rxnSamples(scenarios, sampleSize=1000):
    print('Reading IDUs at ' + path)
    _idus = gp.read_file(path).to_crs('EPSG:3857')


    for scenario in scenarios:
        path = f"{mapServerPath}/maps/{scenario}/{startYear}/IDU_Year{startYear}_{scenario}_reduced.shp"

        print('Reading IDUs at ' + path)
        idus = gp.read_file(path).to_crs('EPSG:3857')

        print(f"Generating {sampleSize} samples")
        #samples = np.random.randint(16, 256, size=3)
        _iduSamples = random.sample(range(0, idus.shape[0]), idus.shape[0])
        _iduSamplesSoFar = 0

        iduSamples = np.zeros(sampleSize)  # contains idu index for each sample
        
        # iterate through IDUs.  if the IDU is in the index, get rgb values from IDU.XML and populate
        for i, row in _idus.iterrows():
            profile = row['PROFILE']   
            if profile > 0:
                iduIndex = int(row['IDU_INDEX'])
                iduSamples[_iduSamplesSoFar] = iduIndex
                _iduSamplesSoFar += 1

            if _iduSamplesSoFar >= sampleSize:
                break


        # for each idu in iduSamples, 

        #, we've identifed the IDUs to sample, run them through the delta array. 



        a2rxns = []  # will contain sampleSize arrays, each a time series


        print( "Loading Delta Array")
        deltaArray = pd.read_csv( f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_Reduced.csv') # NOTE!!!!

        year = startYear-1

        #print("Creating Initial rasters")
        #for field in fields:
        #    GeneratePNGsFromIDUs(idus, iduRaster, iduLookup, field, fieldInfos[field], scenario, 'start')

        # iterate though delta array, applying deltas.  On year boundaries, save the maps
        for i, delta in deltaArray.iterrows():
            dYear = delta['year']

            # new year?  save raster before proceeding
            if dYear > year:
                while (year < dYear): # in case deltaarry is missing years
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
def GenPNGIDUMaps(scenarios, sampleSize=1000):
    for scenario in scenarios:
        path = f"{mapServerPath}/maps/{scenario}/{startYear}/IDU_Year{startYear}_{scenario}_reduced.shp"

        print('Reading IDUs at ' + path)
        idus = gp.read_file(path).to_crs('EPSG:3857')

        print(f"Generating {sampleSize} samples")
        #samples = np.random.randint(16, 256, size=3)
        samples = random.sample(range(0, idus.shape[0]), idus.shape[0])
        samplesSoFar = 0

        print( "Loading Delta Array")
        deltaArray = pd.read_csv( f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_Reduced.csv') # NOTE!!!!

        year = startYear-1

        #print("Creating Initial rasters")
        #for field in fields:
        #    GeneratePNGsFromIDUs(idus, iduRaster, iduLookup, field, fieldInfos[field], scenario, 'start')

        # iterate though delta array, applying deltas.  On year boundaries, save the maps
        for i, delta in deltaArray.iterrows():
            dYear = delta['year']

            # new year?  save raster before proceeding
            if dYear > year:
                while (year < dYear): # in case deltaarry is missing years
                    print( f'Generating PNGS for year {year+1}')
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

def GenWildfireFigures(scenarios):

    figBurnedArea = go.Figure()
    #figBurnedDwellings = go.Figure()
    #figBurnedDwellingsValue = go.Figure()
    #figBurnedDwellingsFraction = go.Figure()

    for scenario in scenarios:
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/FireOccuranceByOwner-All_Fire_(ha)_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        federal = data.loc[1:,"Federal"]
        privNI = data.loc[1:,"State"]
        privI = data.loc[1:,"Private Non-Industrial"]
        tribal = data.loc[1:,"Tribal"]
        home = data.loc[1:,"Homeowner"]
        fig.add_trace(go.Scatter(x=times, y=federal, stackgroup='one', mode='none', name='Federal'))
        fig.add_trace(go.Scatter(x=times, y=privNI,  stackgroup='one', mode='none', name='State Node Reactivity'))
        fig.add_trace(go.Scatter(x=times, y=privI,   stackgroup='one', mode='none', name='Private Non-industrial'))
        fig.add_trace(go.Scatter(x=times, y=tribal,  stackgroup='one', mode='none', name='Tribal'))
        fig.add_trace(go.Scatter(x=times, y=home,    stackgroup='one', mode='none', name='Homeowner'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Burned Area by Ownership (ha)<br><sub>{scenario} scenario"})

        GenFigure(fig, "FireByOwnership_chart", scenario)  


        # burned area
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/FireOccurance_{scenario}_Run0.csv')
        wfArea = data.loc[1:,"Wildfire(ha)"]
        figBurnedArea.add_trace(go.Scatter(x=times, y=wfArea, name=scenario,line={'color':scenarioColors[scenario]}))
        

        # Potential Flame Length
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Potential_Flame_Length_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        for field in ['Potential FL 0 - 1.5 ft (ha)','Potential FL 1.5 - 3 ft (ha)','Potential FL 3 - 4.5 ft (ha)',
                'Potential FL 4.5 - 7.5 ft (ha)', 'Potential FL 7.5 - 10 ft (ha)','Potential FL > 10 ft (ha)']:
            y = data.loc[1:,field]
            fig.add_trace(go.Scatter(x=times, y=y, stackgroup='one', mode='none', name=field))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Flame Length Area by Severity<br><sub>{scenario} scenario"})

        GenFigure(fig, "PotFlameLenBySeverity_chart", scenario)  

        # Fire Risk distribution
        #print( "Loading Fire Risk Samples...")
        #print("Subsetting fields")
        #df = df[["FireRisk", "FN_ID", "Profile", "SN_Engager", "PopDens", "AREA"]]

        #path = f"{mapServerPath}/maps/{scenario}/{startYear}/IDU_Year{startYear}_{scenario}_reduced.shp"
        #print('Reading IDUs at ' + path)
        #idus = gp.read_file(path)

        '''

        print( "Loading Delta Array")
        deltaArray = pd.read_csv( f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DeltaArray_{scenario}_Reduced.csv') # NOTE!!!!

        # iterate though delta array, applying deltas.  On year boundaries, save the maps
        print("extracting deltas")
        fireRisks = []  # will contain all fire risks  
        for i, delta in deltaArray.iterrows():
            iduField = delta['field']
            if iduField == 'FireRisk':
                dNewValue = delta['newValue']
                fireRisks.append(dNewValue)

        figHist = go.Figure()
        figHist.add_trace(go.Histogram(
                    x=fireRisks,
                    histnorm='percent',
                    name='FireRisk', # name used in legend and hover labels
                    #xbins=dict( # bins used for histogram
                    #start=-4.0,
                    #end=3.0,
                    #size=0.5
                    #),
                    nbinsx = 100,
                    marker_color='#EB89B5',
                    opacity=0.75
                    ))

        figHist.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Fire Risk Distribution<br><sub>{scenario} scenario"
            #xaxis_title_text:'Value', # xaxis label
            #yaxis_title_text='Count' # yaxis label
            #bargap=0.2, # gap between bars of adjacent location coordinates
            #bargroupgap=0.1 # gap between bars of the same location coordinates
            })
        
        GenFigure(figHist, "FireRiskDist_chart", scenario)  
        '''

    figBurnedArea.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Area Burned by Wildfire (ha)<br><sub>All scenarios"})

    #figBurnedDwellings.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
    #        'title': f"Number of Dwellings Damaged by Wildfire <br><sub>All scenarios"})
    #figBurnedDwellingsValue.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
    #        'title': f"Cost of Building Damage from Wildfire ($) <br><sub>All scenarios"})
    #figBurnedDwellingsFraction.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
    #        'title': f"Average Fraction of Building Damaged by Wildfire<br><sub>All scenarios"})

    GenFigure(figBurnedArea, "BurnedArea_All_chart", "All")  
    #GenFigure(figBurnedDwellings, "BurnedDwellings_chart", "All")  
    #GenFigure(figBurnedDwellingsValue, "BurnedDwellingsValue_chart", "All")  
    #GenFigure(figBurnedDwellingsFraction, "BurnedDwellingFraction_chart", "All")  



def GenPopPlots(scenarios):

    figFracInUGB = go.Figure()
    figFracNewInUGB = go.Figure()

    for scenario in scenarios:
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Population_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        popInUGB = data.loc[1:,"Population Within UGBs"]
        popOutUGB = data.loc[1:,"Population Outside UGBs"]
        fig.add_trace(go.Scatter(x=times, y=popInUGB, stackgroup='one', mode='none', name='Population Within UGBs'))
        fig.add_trace(go.Scatter(x=times, y=popOutUGB, stackgroup='one', mode='none', name='Population Outside UGBs'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Population Trajectories<br><sub>{scenario} scenario"})

        GenFigure(fig, "Population_chart", scenario)  

        ################ avail capacity
        fig = go.Figure()
        times = data.iloc[1:,0]
        capInUGB = data.loc[1:,"Available Capacity within UGBs (percent)"]
        capOutUGB = data.loc[1:,"Available Capacity Outside UGBs (percent)"]

        capBend = data.loc[1:,"Available Capacity-Bend (percent)"]
        capRedmond = data.loc[1:,"Available Capacity-Redmond (percent)"]
        capSisters = data.loc[1:,"Available Capacity-Sisters (percent)"]
        capLaPine = data.loc[1:,"Available Capacity-LaPine (percent)"]
        
        fig.add_trace(go.Scatter(x=times, y=capBend, mode='lines', name="Bend")) 
        fig.add_trace(go.Scatter(x=times, y=capRedmond, mode='lines', name="Redmond")) 
        fig.add_trace(go.Scatter(x=times, y=capSisters, mode='lines', name="Sisters")) 
        fig.add_trace(go.Scatter(x=times, y=capLaPine, mode='lines', name="LaPine")) 
        #fig.add_trace(go.Scatter(x=times, y=capInUGB, mode='lines', name="Within UGBs")) 
        fig.add_trace(go.Scatter(x=times, y=capOutUGB, mode='lines',name='Rural'))



        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Available Capacity Fraction<br><sub>{scenario} scenario"},
            yaxis_title='Fraction of Capacity Available')

        GenFigure(fig, "PopAvailCapacity_chart", scenario)

        # percent of pop in UGB
        times = data.iloc[1:,0]
        popInUGBs = data.loc[1:,"Population Within UGBs"]
        popTotal = data.loc[1:,"Total Population"]
        y1 = popInUGBs/popTotal
        figFracInUGB.add_trace(go.Scatter(x=times, y=y1, mode='lines', name=f"{scenario}", line={'color':scenarioColors[scenario]})) 

        # percent of new pop in UGB
        times = data.iloc[1:,0]
        popInUGBs = data.loc[1:,"Population Within UGBs"]
        popTotal = data.loc[1:,"Total Population"]

        y1 = (popInUGBs-popInUGBs.values[0])/(popTotal-popTotal.values[0])
        figFracNewInUGB.add_trace(go.Scatter(x=times, y=y1, mode='lines', name=f"{scenario}", line={'color':scenarioColors[scenario]})) 

 
        # available capacities by UGB
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/ModelOutput_{scenario}_Run0.csv')
        fig = go.Figure()
        fig1 = go.Figure()
        times = data.iloc[1:,0]

        for field in ["Rural","LaPine","Sisters","Bend","Redmond"]:
            y=data.loc[1:,f'Population.Population-{field}.Available Capacity']
            y1=data.loc[1:,f'Population.Population-{field}.Actual']
            fig.add_trace(go.Scatter(x=times, y=y, stackgroup='one', mode='none', name=field))
            fig1.add_trace(go.Scatter(x=times, y=y1-y1.values[0], stackgroup='one', mode='none', name=field))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Available Capacity for New Growth by UGB<br><sub>{scenario} scenario"})
        fig1.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"New Population by UGB<br><sub>{scenario} scenario"})

        GenFigure(fig, "AvailCapacityByUGB_chart", scenario) 
        GenFigure(fig1, "NewPopByUGB_chart", scenario) 

        # dwelling untes by UGB
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Dwelling_Units_(#)_{scenario}_Run0.csv')
        fig0 = go.Figure()
        fig1 = go.Figure()
        times = data.iloc[1:,0]

        for field in ["Rural","LaPine","Sisters","Bend","Redmond"]:
            y=data.loc[1:,field]
            y0=data.loc[0,field]
            fig0.add_trace(go.Scatter(x=times, y=y, stackgroup='one', mode='none', name=field))
            fig1.add_trace(go.Scatter(x=times, y=y-y0, stackgroup='one', mode='none', name=field))

        fig0.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Dwelling Units by UGB<br><sub>{scenario} scenario"})
        fig1.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"New Dwelling Units by UGB<br><sub>{scenario} scenario"})

        GenFigure(fig0, "DwellingUnitsByUGB_chart", scenario)  
        GenFigure(fig1, "NewDwellingUnitsByUGB_chart", scenario)  

        # dwelling untes in WUI
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DwellingsInWUI-NumberOfDwellings_{scenario}_Run0.csv')
        fig0 = go.Figure()
        fig1 = go.Figure()
        times = data.iloc[1:,0]

        for field in ['High Density Intermix','Medium Density Intermix','Low Density Intermix','High Density Interface','Medium Density Interface','Low Density Interface']:
            y=data.loc[1:,field]
            y0=y.values[0]
            fig0.add_trace(go.Scatter(x=times, y=y, stackgroup='one', mode='none', name=field))
            fig1.add_trace(go.Scatter(x=times, y=y-y0, stackgroup='one', mode='none', name=field))

        fig0.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Dwelling Units in the WUI<br><sub>{scenario} scenario"})
        fig1.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"New Dwelling Units in the WUI<br><sub>{scenario} scenario"})

        GenFigure(fig0, "DwellingUnitsByWUIType_chart", scenario)  
        GenFigure(fig1, "NewDwellingUnitsByWUIType_chart", scenario)  
    
        # dwellings by zone
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/DwellingsByZone_{scenario}_Run0.csv')
        fig0 = go.Figure()
        fig1 = go.Figure()
        times = data.iloc[1:,0]
        for field in ['Low Dens Res','Low-Med Dens Res','Med Dens Res','High Dens Res','Mixed Use','Rural Res','Wildfire Resilient Res','Urban Reserve','Exclusive Farm Use','Forest Protection','Commercial','Industrial','Urban Open Space','Resort']:
            y=data.loc[1:,field]
            y0=y.values[0]
            fig0.add_trace(go.Scatter(x=times, y=y, stackgroup='one', mode='none', name=field))
            fig1.add_trace(go.Scatter(x=times, y=y-y0, stackgroup='one', mode='none', name=field))

        fig0.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Dwelling Units By Zone<br><sub>{scenario} scenario"})
        fig1.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"New Dwelling Units ByZone<br><sub>{scenario} scenario"})

        GenFigure(fig0, "DwellingUnitsByZone_chart", scenario)  
        GenFigure(fig1, "NewDwellingUnitsByZone_chart", scenario)  

    
        # urban growth boundary areas
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/UrbanDev_{scenario}_Run0.csv')
        fig0 = go.Figure()
        fig1 = go.Figure()
        times = data.iloc[1:,0]
        for field in ['Bend.Area (ac)','Redmond.Area (ac)','Sisters.Area (ac)','LaPine.Area (ac)']:
            y=data.loc[1:,field]
            y0=y.values[0]
            fig0.add_trace(go.Scatter(x=times, y=y, stackgroup='one', mode='none', name=field))
            fig1.add_trace(go.Scatter(x=times, y=y-y0, stackgroup='one', mode='none', name=field))

        fig0.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"UGB Area (ac)<br><sub>{scenario} scenario"})
        fig1.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"New UGB Area (ac)<br><sub>{scenario} scenario"})

        GenFigure(fig0, "UGBArea_chart", scenario)  
        GenFigure(fig1, "NewUGBArea_chart", scenario)  


        # urban growth boundary areas
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Population_{scenario}_Run0.csv')
        fig0 = go.Figure()
        times = data.iloc[1:,0]

        fields = ['UGB Density-Bend (DU/acre)','UGB Density-LaPine (DU/acre)','UGB Density-Sisters (DU/acre)','UGB Density-Redmond (DU/acre)','Rural Density (DU/acre)']

        for field in fields:
            y=data.loc[1:,field]
            y0=y.values[0]
            fig0.add_trace(go.Scatter(x=times, y=y, mode='lines', name=field))

        fig0.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Average Density (DU/ac)<br><sub>{scenario} scenario"})
     
        GenFigure(fig0, "UGBDensity_chart", scenario)  

    figFracInUGB.update_layout(plot_bgcolor=plot_bgColor,paper_bgcolor='rgba(0, 0, 0, 0)',
            title=f"Fraction of Population Within UGB<br><sub>All scenarios",
            yaxis_title='Fraction')

    figFracNewInUGB.update_layout(plot_bgcolor=plot_bgColor,paper_bgcolor='rgba(0, 0, 0, 0)',
            title=f"Fraction of New Population Within UGB<br><sub>All scenarios",
            yaxis_title='Fraction')

    GenFigure(figFracInUGB, "PopFracInUGBs_chart", "All")
    GenFigure(figFracNewInUGB, "NewPopFracInUGBs_chart", "All")

 

def GenFirewiseFigures(scenarios):
    for scenario in scenarios:

        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/FireWise_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        nDUWuiFirewise = data.loc[1:,"NumberDwellingsWuiFirewise"]
        nDUWui = data.loc[1:,"NumberDwellingsWui"]
        nDURuralFirewise = data.loc[1:,"NumberDwellingsRuralFirewise"]
        nDURural = data.loc[1:,"NumberDwellingsRural"]
        nDUUrbanFirewise = data.loc[1:,"NumberDwellingsUrbanFirewise"]
        nDUUrban = data.loc[1:,"NumberDwellingsNoNewMaterials"]
        nDUNewFirewiseMat = data.loc[1:,"NumberDwellingsNewFirewiseMaterials"]

        fig.add_trace(go.Scatter(x=times, y=nDURuralFirewise, stackgroup='one', mode='none', name='Rural'))
        fig.add_trace(go.Scatter(x=times, y=nDUUrbanFirewise, stackgroup='one', mode='none', name='Urban'))
        fig.add_trace(go.Scatter(x=times, y=nDUWuiFirewise, mode='lines',  name='In WUI'))
        fig.add_trace(go.Scatter(x=times, y=nDUNewFirewiseMat, mode='lines',  name='FireWise Materials'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Dwelling Units Engaging with Firewise (#)<br><sub>{scenario} scenario",
            'yaxis_title': "Dwelling Unit Count (#)" })

        GenFigure(fig, "FireWiseNDUs_chart", scenario)  

        fig1 = go.Figure()
        fig1.add_trace(go.Scatter(x=times, y=nDURuralFirewise/nDURural, mode='lines', name='Rural'))
        fig1.add_trace(go.Scatter(x=times, y=nDUUrbanFirewise/nDUUrban, mode='lines', name='Urban'))
        fig1.add_trace(go.Scatter(x=times, y=nDUWuiFirewise/nDUWui, mode='lines', name='Within WUI'))

        totalFW = nDUUrbanFirewise + nDURuralFirewise
        totalDU = nDUUrban + nDURural
        fig1.add_trace(go.Scatter(x=times, y=totalFW/totalDU, mode='lines', name='Total'))

        fig1.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Fraction of Dwelling Units Engaging with Firewise (#)<br><sub>{scenario} scenario",
            'yaxis_title': "Fraction Firewise" })
        GenFigure(fig1, "FireWiseFractions_chart", scenario)  



def GenForestResourcesFigures(scenarios):

    figVolTotal = go.Figure()
    figValTotal = go.Figure()

    for scenario in scenarios:

        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/VolumeAllTrees_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        live = data.loc[1:,"Volume of Live Trees (m3)"]
        dead = data.loc[1:,"Volume of Dead Trees (m3)"]
        fig.add_trace(go.Scatter(x=times, y=live, stackgroup='one', mode='none', name='Live Volume'))
        fig.add_trace(go.Scatter(x=times, y=dead, stackgroup='one', mode='none', name='Dead Volume'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Live and Dead Tree Volume (m3)<br><sub>{scenario} scenario"})

        GenFigure(fig, "VolumeAllTrees_chart", scenario)  

        ################ Carbon
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Carbon_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        live = data.loc[1:,"Live Carbon (Mg)"]
        dead = data.loc[1:,"Dead Carbon (Mg)"]
        fig.add_trace(go.Scatter(x=times, y=live, stackgroup='one', mode='none', name='Live Carbon'))
        fig.add_trace(go.Scatter(x=times, y=dead, stackgroup='one', mode='none', name='Dead Carbon'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Live and Dead Carbon (Mg)<br><sub>{scenario} scenario"})

        GenFigure(fig, "Carbon_chart", scenario)  

        ######## Harvest Volumes
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/WoodProducts_{scenario}_Run0.csv')

        #fig = go.Figure()
        fig = make_subplots(specs=[[{"secondary_y": True}]])
        times = data.iloc[1:,0]
        y1 = data.loc[1:,"Merch Harvest Live(m3)"]
        y2 = data.loc[1:,"Salvage Harvest (m3)"]
        y3 = data.loc[1:,"Saw Timber Total (m3)"]

        fig.add_trace(go.Scatter(x=times, y=y1, stackgroup='one', mode='none', name='Merch Harvest Live'), secondary_y=False)
        fig.add_trace(go.Scatter(x=times, y=y2, stackgroup='one', mode='none', name='Salvage Harvest'), secondary_y=False)
        fig.add_trace(go.Scatter(x=times, y=y3, mode='lines',  name='Saw Timber'), secondary_y=True )

        fig.update_layout(plot_bgcolor=plot_bgColor,paper_bgcolor='rgba(0, 0, 0, 0)',
            title=f"Harvest and Supply Volumes<br><sub>{scenario} scenario")
        fig.update_yaxes(title_text="Harvest Volume (m3)", secondary_y=False) 
        fig.update_yaxes(title_text="Saw Timber (m3)", secondary_y=True, range=[20000000,50000000]) 
        GenFigure(fig, "HarvestVolumes_chart", scenario) 

        # vol/value of saw timeber burned
        figVol = go.Figure()
        figVal = go.Figure()

        sawVolBurnedSRF= data.loc[1:,"Burned Saw Timber-SRF (m3)"]
        sawValueBurnedSRF = data.loc[1:,"Value of Burned Saw Timber-SRF ($)"]/1000000

        figVol.add_trace(go.Scatter(x=times, y=sawVolBurnedSRF, stackgroup='one', mode='none', name='Stand Replacing Fires'))
        figVal.add_trace(go.Scatter(x=times, y=sawValueBurnedSRF,  stackgroup='one', mode='none', name='Stand Replacing Fires'))

        figVol.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Volumne of Timber Lost in Fires (m3)<br><sub>{scenario} scenario",
            'yaxis_title': "Volumne (m3"})
        
        figVal.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Value of Timber Lost in Fires (m3)<br><sub>{scenario} scenario",
            'yaxis_title': "Value ($)"})

        GenFigure(figVol, f"BurnedTimberVolume_chart", scenario)  
        GenFigure(figVal, f"BurnedTimberValue_chart", scenario)  
        
    GenFigure(figVolTotal, "BurnedTimberVolumeAll_chart", "All")
    GenFigure(figValTotal, "BurnedTimberValueAll_chart", "All")


def GenRiskFigures(scenarios):

    figActHousingLoss = go.Figure()
    figPotHousingLoss = go.Figure()
    figActTimberLoss = go.Figure()
    figPotTimberLoss = go.Figure()
    figActHousingLossCount = go.Figure()
    figPotHousingLossCount = go.Figure()
    figActTimberLossVol = go.Figure()
    figPotTimberLossVol = go.Figure()
    figActHousingLossFrac = go.Figure()
    figPotHousingLossFrac = go.Figure()
    figActTimberLossFrac = go.Figure()
    figPotTimberLossFrac = go.Figure()

    figLossAct = go.Figure()
    figLossPot = go.Figure()

    figRiskAct = go.Figure()
    figRiskPot = go.Figure()
    figRiskTot = go.Figure()

    figRiskTotIDU = go.Figure()

    for scenario in scenarios:

        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Fire_Risk_{scenario}_Run0.csv')
        data1 = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/ModelOutput_{scenario}_Run0.csv')

        data.replace("-nan(ind)", np.nan)

        # SHOULD BE ONES
        times = data.iloc[1:,0]
        ahl = data.loc[1:,"Actual Loss-Housing ($)"]
        phl = data.loc[1:,"Potential Loss-Housing ($)"]
        atl = data.loc[1:,"Actual Loss-Timber ($)"]*1000
        ptl = data.loc[1:,"Potential Loss-Timber ($)"]*1000
        
        ahlc = data.loc[1:,"Actual Loss-Houses Impacted (#)"]
        phlc = data.loc[1:,"Potential Loss-Housed Impacted (#)"]
        atlv = data.loc[1:,"Actual Loss-Timber Vol (m3)"]
        ptlv = data.loc[1:,"Potential Loss-Timber Vol (m3)"]

        ahlf = data.loc[1:,"Actual Loss Frac-Housing"]
        phlf = data.loc[1:,"Potential Loss Frac-Housing"]
        atlf = data.loc[1:,"Actual Loss Frac-Timber"]
        ptlf = data.loc[1:,"Potential Loss Frac-Timber"]

        lama = data.loc[1:,"Loss-Actual ($ MovingAvg)"]
        lpma = data.loc[1:,"Loss-Potential ($ MovingAvg)"]
        la = data.loc[1:,"Loss-Actual ($)"]
        lp = data.loc[1:,"Loss-Potential ($)"]

        riskAct = data.loc[1:,"Risk-Actual"]
        riskPot = data.loc[1:,"Risk-Potential"]
        riskTot = data.loc[1:,"Risk-Total"]

        riskTotIDU = data1.loc[1:,"Report.FireRisk (Mean)"]

        #	Actual Loss Frac-Housing	Potential Loss Frac-Housing	Actual Loss Frac-Timber	Potential Loss Frac-Timber	Risk

        figActHousingLoss.add_trace(go.Scatter(x=times, y=ahl, mode='lines', name=scenario)) 
        figPotHousingLoss.add_trace(go.Scatter(x=times, y=phl, mode='lines', name=scenario)) 
        figActTimberLoss.add_trace(go.Scatter(x=times, y=atl, mode='lines', name=scenario)) 
        figPotTimberLoss.add_trace(go.Scatter(x=times, y=ptl, mode='lines', name=scenario)) 
        figActHousingLossCount.add_trace(go.Scatter(x=times, y=ahlc, mode='lines', name=scenario)) 
        figPotHousingLossCount.add_trace(go.Scatter(x=times, y=phlc, mode='lines', name=scenario)) 
        figActTimberLossVol.add_trace(go.Scatter(x=times, y=atlv, mode='lines', name=scenario)) 
        figPotTimberLossVol.add_trace(go.Scatter(x=times, y=ptlv, mode='lines', name=scenario))
        
        figActHousingLossFrac.add_trace(go.Scatter(x=times, y=ahlf.values, mode='lines', name=scenario))

        figPotHousingLossFrac.add_trace(go.Scatter(x=times, y=phlf, mode='lines', name=scenario))
        figActTimberLossFrac.add_trace(go.Scatter(x=times, y=atlf, mode='lines', name=scenario))
        figPotTimberLossFrac.add_trace(go.Scatter(x=times, y=ptlf, mode='lines', name=scenario))

        figLossAct.add_trace(go.Scatter(x=times, y=la, mode='lines', name=scenario))
        figLossAct.add_trace(go.Scatter(x=times, y=lama, mode='lines', name=scenario + "-MovAvg"))
        figLossPot.add_trace(go.Scatter(x=times, y=lp, mode='lines', name=scenario))
        figLossPot.add_trace(go.Scatter(x=times, y=lpma, mode='lines', name=scenario + "-MovAvg"))

        figRiskAct.add_trace(go.Scatter(x=times, y=riskAct, mode='lines', name=scenario)) 
        figRiskPot.add_trace(go.Scatter(x=times, y=riskPot, mode='lines', name=scenario)) 
        figRiskTot.add_trace(go.Scatter(x=times, y=riskTot, mode='lines', name=scenario)) 
        figRiskTotIDU.add_trace(go.Scatter(x=times, y=riskTotIDU, mode='lines', name=scenario)) 
      
    figActHousingLoss.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Housing Loss From Wildfire ($)<br><sub>{scenario} scenario",
            'yaxis_title': "Value ($)"})

    figPotHousingLoss.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Housing Loss From Wildfire ($)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figActTimberLoss.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Timber Loss From Wildfire ($)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figPotTimberLoss.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Timber Loss From Wildfire ($)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figActHousingLossCount.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Housing Count Damaged by Wildfire (m3)<br><sub>All scenarios",
            'yaxis_title': "Count"})

    figPotHousingLossCount.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Housing Count Damaged by Wildfire (m3)<br><sub>All scenarios",
            'yaxis_title': "Count"})

    figActTimberLossVol.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Timber Volume Loss From Wildfire (m3)<br><sub>All scenarios",
            'yaxis_title': "Volume (m3)"})

    figPotTimberLossVol.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Timber Volume Loss From Wildfire (m3)<br><sub>All scenarios",
            'yaxis_title': "Volume (m3)"})
    
    
    figActHousingLossFrac.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Housing Loss Fraction<br><sub>All scenarios",
            'yaxis_title': "Fraction", 'yaxis_tickformat': ".0%"})
    

    figPotHousingLossFrac.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Housing Loss Fraction<br><sub>All scenarios",
            'yaxis_title': "Fraction", 'yaxis_tickformat': ".0%"})

    figActTimberLossFrac.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Timber Loss Fraction<br><sub>All scenarios",
            'yaxis_title': "Fraction", 'yaxis_tickformat': ".0%"})
    
    figPotTimberLossFrac.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Timber Loss Fraction<br><sub>All scenarios",
            'yaxis_title': "Fraction", 'yaxis_tickformat': ".0%"})

    figLossAct.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Wildfire Risk Score (Backward-looking)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figLossPot.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Wildfire Risk Score  (5-year Moving Average) (Forward-looking)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figRiskAct.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Actual Wildfire Risk Score (Backward-looking)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figRiskPot.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Potential Wildfire Risk Score (Forward-looking)<br><sub>All scenarios",
            'yaxis_title': "Value ($)"})

    figRiskTot.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Global Combined Risk Signal (<i>Fire Risk</i>)<br><sub>All scenarios",
            'yaxis_title': "Scaled Score"})

    figRiskTotIDU.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"IDU-Aggregated Combined Risk Signal (<i>FireRisk</i>)<br><sub>All scenarios",
            'yaxis_title': "Scaled Score"})

    GenFigure(figActHousingLoss, "ActHousingLossWF_All_chart", "All")
    GenFigure(figPotHousingLoss, "PotHousingLossWF_All_chart", "All")
    GenFigure(figActTimberLoss, "ActTimberLossWF_All_chart", "All")
    GenFigure(figPotTimberLoss, "PotTimberLossWF_All_chart", "All")
    GenFigure(figActHousingLossCount, "ActHousingLossCountWF_All_chart", "All")
    GenFigure(figPotHousingLossCount, "PotHousingLossCountWF_All_chart", "All")
    GenFigure(figActTimberLossVol, "ActTimberLossVolWF_All_chart", "All")
    GenFigure(figPotTimberLossVol, "PotTimberLossVolWF_All_chart", "All")

    GenFigure(figActHousingLossFrac, "ActHousingLossFrac_ALL_chart", "All")
    GenFigure(figPotHousingLossFrac, "PotHousingLossFrac_ALL_chart", "All")
    GenFigure(figActTimberLossFrac, "ActTimberLossFrac_ALL_chart", "All")
    GenFigure(figPotTimberLossFrac, "PotTimberLossFrac_ALL_chart", "All")
    GenFigure(figLossAct, "ActWildfireRiskMovAvg_ALL_chart", "All")
    GenFigure(figLossPot, "PotWildfireRiskMovAvg_ALL_chart", "All")

    GenFigure(figRiskAct, "ActWildfireRisk_All_chart", "All")
    GenFigure(figRiskPot, "PotWildfireRisk_All_chart", "All")
    GenFigure(figRiskTot, "TotWildfireRisk_All_chart", "All")
    GenFigure(figRiskTotIDU, "TotWildfireRiskIDU_All_chart", "All")


def GenTreatmentFigures(scenarios):
    
    for scenario in scenarios:
        ###### Treatment costs
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Treatment_Costs_{scenario}_Run0.csv')

        fig = go.Figure()
        times = data.iloc[1:,0]
        pfPol = data.loc[1:,"Prescribed Fire (Policy  $)"]/1000000
        pfPriv = data.loc[1:,"Prescribed Fire (Private  $)"]/1000000
        pfFed = data.loc[1:,"Prescribed Fire (Fed  $)"]/1000000
        tfbFed = data.loc[1:,"Thinning From Below (Fed  $)"]/1000000
        sftFed = data.loc[1:,"Surface Fuel Treatment (Mowing and Grinding) (Fed  $)"]/1000000

        #mt = data.loc[1:,"Mechanical Treatment ($)"]/1000000
        #mtB = data.loc[1:,"Mechanical Treatment (Bio) ($)"]/1000000
        fig.add_trace(go.Scatter(x=times, y=pfPol, stackgroup='one', mode='none', name='Prescribed Fire (Policy)'))
        fig.add_trace(go.Scatter(x=times, y=pfPriv,  stackgroup='one', mode='none', name='Prescribed Fire (Private)'))
        fig.add_trace(go.Scatter(x=times, y=pfFed, stackgroup='one', mode='none', name='Prescribed Fire (Fed)'))
        fig.add_trace(go.Scatter(x=times, y=tfbFed,  stackgroup='one', mode='none', name='Thinning From Below (Fed)'))
        fig.add_trace(go.Scatter(x=times, y=sftFed, stackgroup='one', mode='none', name='Surface Fule Treatment (Fed)'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Treatment Costs (M$)<br><sub>{scenario} scenario",
            'yaxis_title': "M$"})

        GenFigure(fig, f"TreatmentCosts_chart", scenario)  


        #data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/AllocationSet-High_Risk_High_Priority_{scenario}_Run0.csv')
        data = pd.read_csv(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/ManagementDisturb_{scenario}_Run0.csv')



        fig = go.Figure()
        times = data.iloc[1:,0]
        pfPol = data.loc[1:,"Prescribed Fire (Policy  ha)"]
        pfPriv = data.loc[1:,"Prescribed Fire (Private  ha)"]
        pfFed = data.loc[1:,"Prescribed Fire (Fed  ha)"]
        thin = data.loc[1:,"Thinning (ha)"]
        smtree = data.loc[1:,"Small Tree Thinning (ha)"]
        clrcut = data.loc[1:,"Clear-cut (ha)"]
        mowgrn  = data.loc[1:,"Mowing and Grinding (ha)"]
        parharv = data.loc[1:,"Partial Harvest (ha)"]
        salharv = data.loc[1:,"Salvage Logging (ha)"]


        #pfu = data.loc[1:,"Prescribed Fire/Underburning (HRP) 29-Realized Area"]
        #tfb = data.loc[1:,"Thinning From Below - 55-Realized Area"]
        #sft = data.loc[1:,"Surface Fuel Treatment (Mechanical) - 51 {Mowing and Grinding}-Realized Area"]
        #shl = data.loc[1:,"Salvage Harvest - 52 {Salvage Logging}-Realized Area"]
        #ilh = data.loc[1:,"Industrial Lands Harvest - 57 {Partial Harvest Heavy}-Realized Area"]

        fig.add_trace(go.Scatter(x=times, y=pfPol, stackgroup='one', mode='none', name='Prescribed Fire (Policy)'))
        fig.add_trace(go.Scatter(x=times, y=pfPriv, stackgroup='one', mode='none', name='Prescribed Fire (Private)'))
        fig.add_trace(go.Scatter(x=times, y=pfFed, stackgroup='one', mode='none', name='Prescribed Fire (Fed)'))
        fig.add_trace(go.Scatter(x=times, y=thin, stackgroup='one', mode='none', name='Thinning'))
        fig.add_trace(go.Scatter(x=times, y=smtree, stackgroup='one', mode='none', name='Small Tree Thinning'))
        fig.add_trace(go.Scatter(x=times, y=clrcut, stackgroup='one', mode='none', name='Clear Cut'))
        fig.add_trace(go.Scatter(x=times, y=mowgrn, stackgroup='one', mode='none', name='Mow/Grind'))
        fig.add_trace(go.Scatter(x=times, y=parharv, stackgroup='one', mode='none', name='Partial Harvest'))
        fig.add_trace(go.Scatter(x=times, y=salharv, stackgroup='one', mode='none', name='Salvage Harvest'))

        fig.update_layout({'plot_bgcolor':plot_bgColor,'paper_bgcolor': 'rgba(0, 0, 0, 0)',
            'title': f"Treatment Areas (ha)<br><sub>{scenario} scenario"})

        GenFigure(fig, f"TreatmentAreas_chart", scenario)  

############ M A I N ###############



showCharts = False

outcomesSNIP =  True
outcomesFire =  True
outcomesFireWise =  True
outcomesForest = True 
outcomesPop =  True
outcomesRisk = True

def main():
    # Get the value of the "--file" option
    global scenarios
    print("Available Scenarios:", scenarios)
    if len(sys.argv) > 1:
        index = sys.argv.index("-s")
        scenarios = [sys.argv[index + 1]]
        
        print("#############################################")
        print(f"######## Processing scenario {scenarios[0]}")
        print("#############################################")
       
    else:
        print("#############################################")
        print(f"######## Running all scenarios {scenarios}")
        print("#############################################")

    if outcomesFireWise:
        print(f"######## Generating FireWise Plots")
        GenFirewiseFigures(scenarios)

    if outcomesSNIP:
        ####TEMPGenSnipTables(scenarios)
        print(f"######## Generating SNIP Plots")
        GenSnipPlots(scenarios)
        print(f"######## Generating SNIP Figures")
        GenSnipFigures(scenarios)
        print(f"######## Generating SNIP Reactivities")
        GenSnipReactivities(scenarios)
        print(f"######## Generating SNIP Reactivities2")
        GenSnipReactivities2(scenarios)

    if outcomesFire:
       print(f"######## Generating Wildfire Plots")
       GenWildfireFigures(scenarios)

    if outcomesForest:
        print(f"######## Generating Forest Resources Plots")
        GenForestResourcesFigures(scenarios)
        print(f"######## Generating Treatment Plots")
        GenTreatmentFigures(scenarios)

    if outcomesPop:
        print(f"######## Generating POP Plots")
        GenPopPlots(scenarios)

    if outcomesRisk:
        print(f"######## Generating Risk Plots")
        GenRiskFigures(scenarios)



if __name__ == "__main__":
    main()


