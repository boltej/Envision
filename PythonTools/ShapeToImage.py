import matplotlib
#matplotlib.use( 'TkAgg' )
#matplotlib.use('module://ipykernel.pylab.backend_inline')
import matplotlib.pyplot as plt
#import numpy as np
import geopandas as gpd
#import sys
#import contextily as ctx
from mpl_toolkits.axes_grid1 import make_axes_locatable


#params = {'legend.fontsize': 4,
#          'legend.handlelength': 2}
#plt.rcParams.update(params)
"""
Command line args

shapefile, fieldInfoFile, outfile, field, filterField, filterop, filterValue )

"""

def get_cmap(n, name='hsv'):
  '''Returns a function that maps each index in 0, 1, ..., n-1 to a distinct 
  RGB color; the keyword argument name must be a standard mpl colormap name.'''
  return plt.cm.get_cmap(name, n)

def ScaleRect(rect, factor):
  width = rect[2]-rect[0]
  height = rect[3]-rect[1]
  xcenter = (rect[2]+rect[0])/2
  ycenter = (rect[3]+rect[1])/2
  width *= factor  # positive
  height *= factor  # positive
  rect[0] = xcenter-width/2
  rect[2] = rect[0]+width
  rect[1] = ycenter-height/2
  rect[3] = rect[1]+height
  return rect

#if len(sys.argv)!=3:
#  print("Syntax: {0} <Shapefile> <Output>".format(sys.argv[0]))
#  sys.exit(-1)

#shapename = sys.argv[1]
#outputname= sys.argv[2]

basePath = "d:/Envision/StudyAreas/OrCoast/"

def CreateMap(
    shapeName = "Tillamook/idu.shp",
    overlayName = None,
    overlayField = None,
    overlayPoints=True,
    overlayCmap="Reds",
    #fieldInfoName = "idu_full.shp",
    outputName = None,
    fieldName = "AvgIncome",
    ugbName = None,  # "Tillamook",
    title = "Buildings Damaged (2030) / Income Class",
    yLabel = "",
    legendLabel = "",
    colors = 5,
    cmap = 'Greens',
    pltOutput=3
):

  idus = gpd.read_file(basePath + shapeName)[[fieldName, 'geometry']]
  print("IDU Count: {}".format(len(idus.index)))

  if ugbName is not None:
    ugb = gpd.read_file( basePath + "Data/ugbs/ugb.shp")
    ugb = ugb[(ugb["InstName"] == ugbName)]

  # set up figure
  figsize = (12, 8)
  fig, ax = plt.subplots(1, 1, figsize=figsize)
  divider = make_axes_locatable(ax)
  cax = divider.append_axes("right", size="5%", pad=0.1)

  idus.plot(column=fieldName, ax=ax, cax=cax,cmap=cmap, k=colors, legend=True, legend_kwds={'label': legendLabel})
  ax.legend(title="Test")
  #ax.set_ylabel(yLabel)

  #ax = idus.plot(column=fieldName, cmap=cmap, figsize=figsize,  k=colors, legend=False) #, scheme='equal_interval') # categorical=false, alpha=0.5
  if ugbName is not None:
    print( "Reading UGB Clipping poly for ", ugbName)
    ugb.plot(ax=ax, facecolor='none', edgecolor='red')
    clipEnvelope = ugb.total_bounds
    print( "clipEnvelope=",clipEnvelope )
    clipEnvelope = ScaleRect( clipEnvelope, 1.2 )
    ax.set_xlim([clipEnvelope[0],clipEnvelope[2]])
    ax.set_ylim([clipEnvelope[1], clipEnvelope[3]])

  if overlayName is not None:
    print( "Reading Overlay ", overlayName)
    # copy GeoDataFrame
    # change geometry
    overlay = gpd.read_file(basePath + overlayName)[[overlayField, 'geometry']]
    overlay = overlay[(overlay[overlayField] > 0)]
    overlay['geometry'] = overlay['geometry'].centroid
    overlay.plot(column=overlayField, ax=ax, cmap=overlayCmap, alpha=0.7, categorical=True, legend=True)


  #ctx.add_basemap(ax)   #, zoom=12)
  ax.set_axis_off()
  ax.set_title(title)


  if pltOutput == 2 or pltOutput == 3:
    print("Saving ",basePath+outputName)
    plt.savefig(basePath + outputName, bbox_inches='tight', pad_inches=0)

  if pltOutput == 1 or pltOutput == 3:
    plt.show()

#######################################
# Bu-lding Damage by Income Class - 2030
def BldgDamageByIncome2030():
  CreateMap( shapeName="Tillamook/idu.shp",
           outputName="Tillamook/Outputs/Baseline-2030/Run0/BldgDamageByIncomeBaseline2030.png",
           fieldName="AvgIncome",
           ugbName="Tillamook",
           overlayName="Tillamook/Outputs/Baseline-2030/Run0/Map_Year2030_Baseline-2030_Run0.shp",
           overlayField="BLDGDAMAGE",
           overlayCmap="Reds",
           overlayPoints=True,
           title="Buildings Damaged (2030) / Income Class - Tillamook",
           yLabel="Income",
           legendLabel="Household Income",
           colors=5,
           cmap='Greens' )






#BldgDamageByIncome()
def BldgDamageByIncome():
  CreateMap( shapeName="Tillamook/idu.shp",
           outputName="Tillamook/Outputs/Baseline-2030/Run0/BldgDamageByIncomeBaseline2030.png",
           fieldName="AvgIncome",
           ugbName="Tillamook",
           overlayName="Tillamook/Outputs/Baseline-2030/Run0/Map_Year2030_Baseline-2030_Run0.shp",
           overlayField="BLDGDAMAGE",
           overlayCmap="Reds",
           overlayPoints=True,
           title="Buildings Damaged (2030) / Income Class - Tillamook",
           yLabel="Income",
           legendLabel="Household Income",
           colors=5,
           cmap='Greens' )






def ClipShape(shp, clip_obj):
  # Create a single polygon object for clipping
  poly = clip_obj.geometry.unary_union
  spatial_index = shp.sindex

  # Create a box for the initial intersection
  bbox = poly.bounds
  # Get a list of id's for each road line that overlaps the bounding box and subset the data to just those lines
  sidx = list(spatial_index.intersection(bbox))
  shp_sub = shp.iloc[sidx]

  # Clip the data - with these data
  clipped = shp_sub.copy()
  clipped['geometry'] = shp_sub.intersection(poly)

  # Return the clipped layer with no null geometry values
  final_clipped = clipped[clipped.geometry.notnull()]