#import os
#import sys
#import subprocess
import shutil
import spatialite
import geopandas as gpd


from contextlib import closing
import PublishCommon


def CopyFireDataToServer(scenarios):
    for scenario in [scenarios[0]]:
        print( "Loading Fire Perimeters ", scenario)
        shutil.copyfile(f'{localStudyAreaPath}/idu.prj', f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Flammap/Perimeter_Run000.prj')
        fires = gpd.read_file(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Flammap/Perimeter_Run000.shp')

        fires = fires.to_crs(epsg=4326)
        geom = fires[['fireID','geometry']].to_json()

        fires.drop("geometry", axis=1, inplace=True)
        fires['Geometry'] = geom

        firesDBPath = f"{explorerStudyAreaPath}/Outputs/Fires/Fires.sqlite"
        with closing(spatialite.connect(firesDBPath)) as connection:
            with closing(connection.cursor()) as cursor:
                c = connection.cursor()
                fires.to_sql(name=f'Fires_{scenario}', con=connection, if_exists='replace', index=True, index_label="IDUINDEX")
                connection.commit()


CopyFireDataToServer(["FTW-FF"])