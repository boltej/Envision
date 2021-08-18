import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import geopandas as gpd


shapeName = "d:/Envision/StudyAreas/OrCoast/Tillamook/idu.shp"
fieldInfoName = "d:/Envision/StudyAreas/OrCoast/idu_full.shp"
outputName = "d:/Envision/StudyAreas/OrCoast/Tillamook/idu.png"
fieldName = "AvgIncome"


colors = 9
cmap = 'Blues'
figsize = (8, 5)

map_df = gpd.read_file(shapeName)
map_df.plot(column=fieldName) #, cmap=cmap, figsize=figsize) #,scheme='equal_interval', k=colors, legend=True)

#plt.show()
plt.savefig(outputName) #, bbox_inches='tight', pad_inches=0)