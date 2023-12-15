import geopandas as gpd
import cv2
import os
import io
import json
import xml.dom.minidom as minidom
from pyproj import Transformer

import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib.lines import Line2D

from owslib.wms import WebMapService
import contextily as cx


# Import everything needed to edit video clips
from moviepy.editor import *


from PublishCommon import *


def GenPNGsFromIDUs(scenarios, fieldDefs, startYear, endYear):
    
    fields = [f[0] for f in fieldDefs]

    mapInfo = None
    bounds = None
    minx = miny = maxx = maxy = bounds
    with open(f'{explorerStudyAreaPath}/Outputs/IDUInfo.json','r') as mapInfoFile:
        mapInfo = json.load(mapInfoFile)
        mapInfoFile.close()
        bounds = mapInfo['boundingBox']
        minx = bounds['left']
        maxx = bounds['right']
        miny = bounds['bottom']
        maxy = bounds['top']

        crs = mapInfo['crs']
        transformer = Transformer.from_crs(crs, "EPSG:3857",always_xy=True)
        minx, miny = transformer.transform(minx,miny)
        maxx, maxy = transformer.transform(maxx,maxy)
        aspect = (maxy-miny)/(maxx-minx)    
    
    if bounds == None:
        print("Unable to find bounds")
        return
    
    width=1024
    padding=10000

    ugbs = gpd.read_file(f'{explorerStudyAreaPath}/Outputs/Urban_Growth_Boundary.json').to_crs(3857)
    fns =  gpd.read_file(f'{explorerStudyAreaPath}/Outputs/fns.json').to_crs(3857)
       
    for scenario in scenarios:
        os.makedirs(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Images/', exist_ok=True)

        for field in fields:
            for year in range(startYear, endYear):
                print(f"  Getting map for {field} for {year}")
                wms = WebMapService(f'http://envweb.bee.oregonstate.edu/mapserver/mapserv?map=D:/MapServer/apps/deschutes/maps/{scenario}/{year}.map', version='1.1.1')
                response = wms.getmap(layers=[field], srs='EPSG:3857', bbox=(minx,miny,maxx,maxy), size=(width, int(width*aspect)), format='image/png', transparent=True)
                # get image data from response
                image = io.BytesIO(response.read())
                data = plt.imread(image)

                fig = plt.figure(figsize=(12, 9))
                #ax = fig.add_axes((0,0,8,6))
                ax = fig.gca()
                xmin = minx-padding
                xmax = maxx+padding
                ymin = miny-padding
                ymax = maxy+padding

                ax.set_xlim(xmin, xmax)
                ax.set_ylim(ymin,ymax)
                ax.set_axis_off()
                cx.add_basemap(ax)
                ax.imshow(data,origin="upper", extent=(minx, maxx, miny, maxy), alpha=0.65)
                label = GenLegend(ax,field)
                ugbs.boundary.plot(ax=ax,edgecolor='#222222', linewidth=1)
                fns.boundary.plot(ax=ax,edgecolor="#4040ff", linewidth=0.5)
                
                ax.text( xmin+0.98*(xmax-xmin), ymin+0.76*(ymax-ymin), f'{label}', color='black', fontsize=18, ha='right')
                #plt.show()
                plt.savefig(f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Images/{field}_{scenario}_{year}.png', bbox_inches='tight', pad_inches=0) #, transparent=True)
                plt.close(fig)
                



def GenVideoFromPNGs(scenarios, fieldDefs, startYear, endYear):
    fields = [f[0] for f in fieldDefs]

    for scenario in scenarios:
        for field in fields:
            image_folder = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Images/'
            video_folder = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Videos/'  #Fields/'
            video_name = video_folder + f'{field}_{scenario}.avi'

            os.makedirs(video_folder, exist_ok=True)

            print( f'processing field {field}')
            images = [img for img in os.listdir(image_folder) if img.endswith(".png") and img.startswith(field)]
            images.sort()

            frame = cv2.imread(os.path.join(image_folder, images[0]))
            height, width, layers = frame.shape
            fps=1
            codec = 0
            video = cv2.VideoWriter(video_name, codec, fps, (width,height))

            for image in images:
                video.write(cv2.imread(os.path.join(image_folder, image)))

            cv2.destroyAllWindows()
            video.release()

    return


def GenLegend(ax,field):
    # parse an xml file by name
    file = minidom.parse(f'{localStudyAreaPath}/idu.xml')

    #use getElementsByTagName() to get tag
    fields = file.getElementsByTagName('field')
    for f in fields:
        col = f.attributes['col'].nodeValue
        if col != field:
            continue
        # found field, extract relevant info
        #mfiType = f.attributes['mfiType'].nodeValue    #0=quantity bins, 1=category bins
        #dataType = f.attributes['datatype'].nodeValue
        legend = []
        for c in f.childNodes:
            if c.nodeName == 'attributes':
                for a in c.childNodes:
                    if a.nodeName == "attr":
                        color = a.attributes['color'].nodeValue    # (Rd,Gr,Bl)  -> hex
                        if color[0] == '(':
                            color = color[1:-1]

                        colors = color.split(",")
                        colors = [int(c) for c in colors]
                        hexcolor =f'#{colors[0]:02x}{colors[1]:02x}{colors[2]:02x}'
                        label = a.attributes['label'].nodeValue
                        patch = Rectangle((0,0), 12, 24, facecolor=hexcolor, fill=True, edgecolor='black', label=label)                            
                        legend.append(patch)
                        ##attrs.append((a.attributes['value'].nodeValue, a.attributes['color'].nodeValue,a.attributes['label'].nodeValue))

        legend.append(Line2D([0],[0], color='#222222', lw=1, label="UGB 2020"))
        legend.append(Line2D([0],[0], color='#4040ff', lw=1, label="FNs"))

        ax.legend(handles=legend,loc='center right')  # loc=
        return f.attributes['label'].nodeValue

    return None

def GenCombinedVideo(scenarios, fields, name, xf,yf, fontsize):  # 0.80,0.02, 120
    isField = False
    if len(fields) < 2:
        isField = True

    for scenario in scenarios:
        os.makedirs(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Videos/Fields',exist_ok=True)
        os.makedirs(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Videos/Themes',exist_ok=True)

        clips = []
        for row in range(len(fields)):
            rowClips = []
            for col in range(len(fields[row])):
                field = fields[row][col]
                path = f'{localStudyAreaPath}/Outputs/{scenario}/Run0/Videos/{field}_{scenario}.avi'
                clip = VideoFileClip(path)
                rowClips.append(clip)

            clips.append(rowClips)

        # stacking clips
        final = clips_array(clips)

        # Generate a text clip. You can customize the font, color, etc.

        txt_clips = []

        for year in range(startYear,endYear):
            txt_clip = TextClip(f"{year}",fontsize=fontsize,color='black')
            txt_clip = txt_clip.set_duration(1).set_start(year-startYear,change_end=True)
            txt_clips.append(txt_clip)


        final_txt = concatenate_videoclips(txt_clips).set_position((xf,yf), relative=True)

        # Overlay the text clip on the first video clip
        final = CompositeVideoClip([final,final_txt])

        if isField == True:
            final.write_videofile(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Videos/Fields/{name}_{scenario}.mp4', fps=1)
        else:
            final.write_videofile(f'{explorerStudyAreaPath}/Outputs/{scenario}/Run0/Videos/Themes/{name}_{scenario}.mp4', fps=1)
        return

def GenVideo(scenarios, fieldDefs, startYear, endYear ):
    fields = [f[0] for f in fieldDefs] 
    
    # generate bitmaps
    #GenPNGsFromIDUs(scenarios, fieldDefs,startYear,endYear )

    # generate videos
    GenVideoFromPNGs(scenarios, fieldDefs, startYear, endYear)

    # combined into a single video
    GenCombinedVideo(scenarios, fieldClipsSnip, "SNIP", 0.83, 0.015, 120 )
    GenCombinedVideo(scenarios, fieldClipsFire, "Fire",0.83,0.015, 120)
    GenCombinedVideo(scenarios, fieldClipsUrban, "Urban", 0.83,0.015, 120)

    for field in fields:
        print("processing ", field)
        GenCombinedVideo(scenarios, [[field]], field, 0.82,0.015, 60)
    return



fieldClipsSnip = [
    ['A2rxn','SocialCap'],
    ['OUTREACH','DISTURB']]

fieldClipsFire = [    
    ['FireRisk','SocialCap'],
    ['FlameLen','DISTURB']]

fieldClipsUrban = [    
    ['PopDens','N_DU','ZONE'],
    ['NewPopDens','New_DU','FireRisk']]




def main():
    # Get the value of the "--file" option
    global scenarios
    print("Available Scenarios:", scenarios)
    if len(sys.argv) > 1:
        index = sys.argv.index("-s")
        scenarios = [sys.argv[index + 1]]
        print(f"######## Processing scenario {scenarios[0]}")
       
    else:
        print(f"######## Running all scenarios {scenarios}")

    GenPNGsFromIDUs(scenarios, fieldDefs, startYear, endYear )
    GenVideo(scenarios, fieldDefs, startYear, endYear)

 
if __name__ == "__main__":
    main()







