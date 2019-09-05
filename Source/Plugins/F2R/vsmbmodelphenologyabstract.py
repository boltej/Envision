#!/usr/bin/env python 
# -*- coding: utf-8 -*-

""" vsmbmodelphenologyabstract.py

This Abstract Based Class (ABC) file contains all properties and methods to calculate the weather, 
precipitation, show, daily average temperature, PET adjustment, soil surface temperature, 
3 day running average for soil, snow budget, potential daily melt from snow, retention and 
actual loss to snow pack, infiltration and run off, snow depletion, ET,  
precipitation and evaporation to layers, drainage and soil water distribution, 
moisture redistribution or unsaturated flow and finally, the phenology. 
The  phenology must be overridden by a CHU or BMT model to allow proper calculation. 

"""

import sys
import traceback
import time
import datetime
from datetime import timedelta, date
import calendar
import csv
import os
import os.path
import logging
import json
import math
from abc import ABCMeta, abstractmethod  # ABCMeta is compatible between 2.6+ and 3+, don't use 'import ABC', purely 3

import utils
from stationdetails import StationDetails, Station
from snowmelt import SnowMelt
from cropmodel import CropModel

import vsmbglobal

   
class VsmbModelPhenologyAbstract(object): # use ABCMeta for compability with 2.+ and 3+, don't use ABC which is 3+
    __metaclass__ = ABCMeta
    
    """ Calculate VSMB Model for the BMT
    
    This DYNAMIC module calculates the VSMB Wheat Crop. 
    Why dynamic? because it will be loaded from the parent 'vsmbmodel.py'

    Please refer to: The Versatile soil moisture budget (VB) reference manual.pdf

    IMPORTANT !!!
    1) Any dynamic module MUST HAVE this following object name 'VsmbModelArgsObject' in it
    2) Second, dynamic module MUST HAVE a 'thread worker' as a static method 
       to invoke the computation of the model
    3) The 'VsmbModelBmt' MUST HAVE a method name 'process' to be invoked
    
        It is an implementation of a proposal interface which needs:
        - getters
		get_nb_layers
		get_snow_coeff
		get_nb_stage
		get_nb_yearly_stage
		get_z_table1
		get_z_table2
		get_crop_name
		get_crop_id
		get_drying_curve_seed_m
		get_drying_curve_seed_n
		get_drying_curve_h9
		get_drying_curve_r9
		get_root_coeff
		get_bmt_coeff
		get_bmt_celcius
		get_time_spend
        
        - method name that will be fired from the parent thread
                process  

    """    
    
    # constant used for 3 day running average smoothing
    SOIL_TEMP_DAYS = 3  
    FLOAT_SOIL_TEMP_DAYS = 3.0 
    
    # all possible columns supported
    COLUMN_YEAR = 'Year'                                    # Year of the data 
    COLUMN_MON = 'Mon'                                      # Month of the data
    COLUMN_DAY = 'Day'                                      # Day of the data
    COLUMN_MAXTEMP = 'MaxTemp'                              # Daily maximum  temperature 
    COLUMN_MINTEMP = 'MinTemp'                              # Daily minimum  temperature 
    COLUMN_P = 'PCPN'                                       # Daily precipitation
    COLUMN_GDD = 'GDD5'                                     # Temperature  above the base(5 ◦C)
    COLUMN_PET = 'PET'                                      # Daily potential evapotranspiration 
    COLUMN_AET = 'AET'                                      # Daily actual evapotranspiration
    COLUMN_SI = 'SI'                                        # Water stress index 
    COLUMN_TOTAWC = 'TotAWC'                                # Available soil water content
    COLUMN_PRCNAWHC = 'PrcnAWHC'                            # Ratio of total available water content to the Available water holding capacity  
    COLUMN_CROPSTAGE = 'CropStage'                          # Crop stage in one decimal point 
    COLUMN_TOTSOILMOIST = 'TotSoilMoist'                    # Actual soil water total in the whole soil for all layers
    COLUMN_RUNOFF = 'Runoff'                                # Water runoff from unfrozen surface 
    COLUMN_DRAINAGE = 'Drainage'                            # Water drained from the bottom layer
    COLUMN_MOISTLAYERDISTRB = 'MoistLayerDistrib'           # Soil water distributed per layer 
    COLUMN_SEEDDATE = 'SeedDate'                            # Seeding date
    COLUMN_HARVESTDATE = 'HarvestDate'                      # Harvest date
    COLUMN_FROZENRUNOFF = 'FrozenRunoff'                    # Water runoff from frozen surface
    COLUMN_SNOWCOVER = 'SnowCover'                          # Total snow water on the surface in mm
    COLUMN_SNOW = 'Snow'                                    # Daily snowfall
    COLUMN_RAIN = 'Rain'                                    # Daily rainfall
    COLUMN_SNOWMELT = 'SnowMelt'                            # Daily snow melt 
    COLUMN_SOILTEMPAVG3 = 'SoilTempAvg3'                    # Three day running average soil surface temperature 
    COLUMN_GDDACC = 'GDD5_Acc'	    	                    # Accumulative GDD5 since seeding date
    COLUMN_CHUACC = 'CHU_Acc'                       	    # Accumulative CHU since seeding date
    COLUMN_PACC	 = 'PCPN_Acc'           	            # Accumulative precipitation since seeding date
    COLUMN_P_PE	 = 'P-PE'           	                    # Water Demand or Climate Water Balance Index or simply Water Balance Index
    COLUMN_CHU = 'CHU'                            	    # CHU since seeding date
    
    ALL_COLUMNS = [COLUMN_YEAR, COLUMN_MON, COLUMN_DAY, COLUMN_MAXTEMP, \
                   COLUMN_MINTEMP, COLUMN_P, COLUMN_PET, \
                   COLUMN_AET, COLUMN_SI, COLUMN_TOTAWC, COLUMN_PRCNAWHC, \
                   COLUMN_CROPSTAGE, COLUMN_GDD, COLUMN_CHU, \
                   COLUMN_P_PE, COLUMN_RUNOFF, COLUMN_DRAINAGE, \
                   COLUMN_MOISTLAYERDISTRB, COLUMN_SEEDDATE, \
                   COLUMN_HARVESTDATE, COLUMN_FROZENRUNOFF, \
                   COLUMN_SNOWCOVER, COLUMN_TOTSOILMOIST, COLUMN_SNOW, COLUMN_RAIN, \
                   COLUMN_SNOWMELT, COLUMN_SOILTEMPAVG3,  \
                   COLUMN_GDDACC, COLUMN_CHUACC, COLUMN_PACC]
    
    DEFAULT_COLUMNS_GDD =  [COLUMN_YEAR, COLUMN_MON, COLUMN_DAY, COLUMN_MAXTEMP, \
                   COLUMN_MINTEMP, COLUMN_P, COLUMN_GDD, COLUMN_PET, \
                   COLUMN_AET, COLUMN_SI, COLUMN_TOTAWC, COLUMN_PRCNAWHC, \
                   COLUMN_CROPSTAGE]
    
    DEFAULT_COLUMNS_CHU = [COLUMN_YEAR, COLUMN_MON, COLUMN_DAY, COLUMN_MAXTEMP, \
                   COLUMN_MINTEMP, COLUMN_P, COLUMN_CHU, COLUMN_PET, \
                   COLUMN_AET, COLUMN_SI, COLUMN_TOTAWC, COLUMN_PRCNAWHC, \
                   COLUMN_CROPSTAGE]
    
    #  Inputs variables use to compute
    print_columns = None            # No column yet
    start_date = None               # Process Start Date
    end_date = None                 # Process End Date
    method = "PT"                   # PET calculation : PT (default), PM or BR
    phenology = "BMT"               # Model calculation: BMT (default) or CHU
    station = None                  # Class Station
    crop_model = None               # Class Crop Model
    snow_melt = None                # Class SnowMelt
    range_layers = None             # Array of layers
    range_stages = None             # Array of stages
    csv_output_file_station = ""    # CSV output file
    
    is_debug_enabled = False        # Get the logger debug flag just once to save time
    is_info_enabled = False         # Get the logger info flag just once to save time
    time_spend = None               # Total time spent in ms

    def __init__(self, columns, start_date, end_date, method, phenology, crop_model, station, snow_melt, csv_output_file_station):
        """ initialize VSMB Model object containing values 
    
        """    
         
        #  Check this once to speed up comparaison
        if logging.getLogger().getEffectiveLevel() == logging.DEBUG:
            self.is_debug_enabled = True
            
        if logging.getLogger().getEffectiveLevel() == logging.INFO or self.is_debug_enabled == True:
            self.is_info_enabled = True      
        
        # Then validate columns name and keep only valid one
        valid_columns = []
            
        # special trick, 'all' columns name means all columns without specify them
        if columns is not None and len(columns) > 0:
            for col in columns:
                if col.lower() == "all":
                    valid_columns = self.ALL_COLUMNS
                    break   
        
        if len(valid_columns) == 0:
            # The option 'all' was not picked, begin with default columns
            # always begin with the default columns 
            
            # Test crop Corn or Soybean
            if crop_model.get_crop_id() == 3 or crop_model.get_crop_id() == 4:      
                valid_columns= self.DEFAULT_COLUMNS_CHU
            # if not, test for Wheat, Barley, Canola or whatever (crop_model.get_crop_id() == 1 or crop_model.get_crop_id() == 2 or crop_model.get_crop_id() == 5:)
            else: 
                valid_columns= self.DEFAULT_COLUMNS_GDD
 
            # Add missing column provided by the user at the end
            for col in columns:
                found = col in self.ALL_COLUMNS
                
                # Test if exists in our internal list
                if not found:
                    logging.warn('The column name ' + col + ' is not supported in the list of columns, it will be ignored')
                else:
                    # Test if exists in the valid columns
                    found = col in valid_columns
                    
                    if not found:
                        # This is great, a new column, append at the end of the list
                        valid_columns.append(col)

        # Set the desired columns
        self.print_columns = valid_columns
        
        # Set global input parameters
        self.start_date = start_date
        self.end_date = end_date
        self.method = method
        self.phenology = phenology
        self.crop_model = crop_model
        self.station = station  
        self.snow_melt = snow_melt
        
        # Set the file CSV output
        self.csv_output_file_station = csv_output_file_station
        
        # Initialize all ranges for layer and stage
        self.range_layers = range(self.get_nb_layers())
        self.range_stages = range(self.get_nb_stage())
 
        # RULE #1: Make sure that the number of station layers is equal or greater to the number of root coefficient layer
        if ( self.get_nb_layers() > self.get_nb_root_coeff_layers() ):
            warn_msg = "For station ID {}, the number of station layers {} is greater than the number of root coefficient layer {}. Either the calculation of the station layers is wrong OR just add more root coefficient to your JSON configuration.".format(self.station.get_id(),self.get_nb_layers(),self.get_nb_root_coeff_layers())
            logging.warn(warn_msg)
            raise MismatchLayerCountError("layers mismatch", warn_msg)

        # RULE #2: The very first station layer thickness must be equal or lower than the first root coefficient layer thickness
        first_station_thickness = self.get_min_thickness()
        first_root_coeff_thickness = self.get_min_root_coeff_thickness()
        if first_station_thickness > first_root_coeff_thickness:
            warn_msg = "For station ID {}, the first station thickness {} is greater than the root coefficient thickness {} which it cannot be possible.".format(first_station_thickness, first_root_coeff_thickness)
            logging.warn(warn_msg)
            raise ThicknessTooBigError("thickness too big", warn_msg)            
        
        # Following variables set time spent per method
        self.timer_getDayWeatherData = 0.0
        self.timer_determinePrecipitationType = 0.0 
        self.timer_determineSnowEquivalent = 0.0 
        self.timer_determineDayAverageTemp = 0.0 
        self.timer_determineAdjustedPET = 0.0 
        self.timer_determinePhenology = 0.0 
        self.timer_determineSoilSurfaceTemp = 0.0 
        self.timer_determineThreeDaySoilSurfaceAverage = 0.0 
        self.timer_determineSnowBudget = 0.0 
        self.timer_determinePotentialDailyMelt = 0.0 
        self.timer_determineRetentionAndLoss = 0.0 
        self.timer_deteremineInfiltrationAndRunoff = 0.0 
        self.timer_determineSnowDepletion = 0.0 
        self.timer_determineET = 0.0 
        self.timer_applyPrecipitationAndEvaporationToLayers = 0.0 
        self.timer_calculateDrainageAndSoilWaterDistribution = 0.0 
        self.timer_accountForMoistureRedistributionOrUnsaturatedFlow = 0.0 
        self.timer_outputDayVSMBResults = 0.0 
        self.timer_writeRow = 0.0 
        self.timer_getNextDay = 0.0 
        self.timer_spend = 0.0         
        
 
    """
        The following are all the 'getter names' to implement
    """

    def get_nb_layers(self):
        return self.station.get_layer_count()
    
    def get_nb_root_coeff_layers(self):
        return self.crop_model.get_nb_root_coeff_layers()
    
    def get_snow_coeff(self, idx=0):
        return self.crop_model.get_snow_coeff()[idx]
    
    def get_nb_stage(self):
        return self.crop_model.get_nb_stage()

    def get_nb_yearly_stage(self):
        return self.crop_model.get_nb_yearly_stage()
    
    def get_z_table1(self, idx=0):
        return self.crop_model.get_z_table1(idx)
    
    def get_z_table2(self, idx=0):
        return self.crop_model.get_z_table2(idx)
    
    def get_crop_name(self):
        return self.crop_model.get_crop_name()
    
    def get_crop_id(self):
        return self.crop_model.get_crop_id()
    
    def get_drying_curve_seed_m(self):
        return self.crop_model.get_drying_curve_seed_m()
    
    def get_drying_curve_seed_n(self):
        return self.crop_model.get_drying_curve_seed_n()
    
    def get_drying_curve_h9(self):
        return self.crop_model.get_drying_curve_h9()
    
    def get_drying_curve_r9(self):
        return self.crop_model.get_drying_curve_r9()

    def get_root_coeff(self, stage_inx=0, layer_inx=0):
        return self.crop_model.get_root_coeff(stage_inx, layer_inx)
    
    def get_min_root_coeff_thickness(self):
        return self.crop_model.get_min_root_coeff_thickness()
    
    def get_min_thickness(self):
        return self.crop_model.get_min_thickness()
    
    def get_max_thickness(self):
        return self.crop_model.get_max_thickness()    
    
    def get_table_coeff(self):
        return self.crop_model.get_phenology(self.phenology).get_coeff_table()
    
    def get_properties(self, property_name, default_value):
        return self.crop_model.get_phenology(self.phenology).get_properties(property_name, default_value)    
    
    def get_bmt_celcius(self):
        return self.crop_model.get_bmt_celcius()  
    
    def get_time_spend(self):
        return self.time_spend
    
    """
        This method let's Windows/Unix give some time to delay execution to other processes. It prevents locking.  
    """        
    def timing_delay_execution(self):
        # This is an arbritary number, don't give too much high
        ms = vsmbglobal.GET_TIMING_DELAY_EXECUATION_IN_MS()
        time.sleep(ms)      
    
    """
        The following is the method 'process' to implement
    """    
    def process(self, threadErrors = None):
        #  This following variables are used to track time spend per a method * by the number of looping
        #  it represents the total amount spend per function
        
        try:
            self.time_spend = [] 

            nb_layers = self.get_nb_layers()
            nb_stages = self.get_nb_stage()
            
            must_track_speed = vsmbglobal.IS_TRACK_SPEED()
            
            if must_track_speed:
                secProcessStart = utils.current_millisecond()
                logging.warn("Process will be monitored")   
                
            if self.is_info_enabled:    
                logging.info ("Running VsmbProcess for " + str(nb_layers) + " layers and " + str(nb_stages) + " stages")
            
            self.range_of_layers = range(nb_layers)
            self.dSoilMoistContent = [0.0 for x in self.range_of_layers]   #  array with soil moist content for each zone
            self.dKCoef = [0.0 for x in self.range_of_layers]              #  soil moisture coefficient determined for differenc crop stages, used in AET calcualtion
            self.SWX = [0.0 for x in self.range_of_layers]                 #  ? used in ceres model adaptation, soil water in layers(zones) volume fraction?
            self.dFlux = [0.0 for x in self.range_of_layers]               #  downward flux?
            self.dWaterLoss = [0.0 for x in self.range_of_layers]          #  water loss, used to determine AET
            self.dContent2 = [0.0 for x in self.range_of_layers]           #  ?
            self.dDelta = [0.0 for x in self.range_of_layers]              #  ?
            self.WF = [0.0 for x in self.range_of_layers]                  #  weighting factor
            self.SW = [0.0 for x in self.range_of_layers]                  #  Soil water content
            self.iCropStageDate  = [0 for x in range(nb_stages)]       #  array stores dates for crop stage change
            self.iNiceDayCount = 0                                     #  counts number of good days, used to determine seed date
            self.iSeedDate = 0                                         #  seed date, set by either number of good days or if not enough good days set to latest seed date
            self.iHarvestDate = 0                                      #  date for harvest, determined by crop stage or if crop is late set to latest harverst date
            self.iBMTStage = 0                                         #  variable used to determine if crop stage should be incremented (only wheat implemented so far)
            self.iBMTStageDec = 0.0                                    #  to output decimal version of CropStage
            self.iLatestSeed = 0                                       #  1) seed and harvest limit dates. At the moment hard coded. Would be a good idea to determine for each station.
            self.iEarliestSeed = 0                                     #  idem as the comment 1)           
            self.iLatestHarvest = 0                                    #  idem as the comment 1)  
            self.iEarliestHarvest = 0                                  #  idem as the comment 1)  
            self.SI_acc = 0.0                                          #  
            self.GDD_acc = 0.0                                         #  
            self.P_acc = 0.0                                           #  
            self.CHU_acc = 0.0                                         #      
            self.dict_iSeedDates = {}                                  #  Integer key, Integer value
            self.dict_iCropStage1 = {}                                 #  Integer key, Integer value
            self.dict_iCropStage2 = {}                                 #  Integer key, Integer value
            self.dict_iCropStage3 = {}                                 #  Integer key, Integer value
            self.dict_iCropStage4 = {}                                 #  Integer key, Integer value
            self.dict_iCropStage5 = {}                                 #  Integer key, Integer value          
            self.list_hasReachedStage = [False for x in range(6)]      #  
            self.dGrowDD = 0.0                                         #  variable to indicate growth day
            self.dCHU = 0.0                                            #  variable to indicated growth day based on heat units            
            self.dTa = 0.0                                             #  averate temperature
            self.dBMT = 0
            self.dExcessTotal = 0.0
            self.ppe = 0.0                                             #  water demand
            

            self.dPermWilt = [0.0 for x in self.range_of_layers]           #  wilting point
            self.dAWHC = [0.0 for x in self.range_of_layers]               #  zone available water holding capacity
            self.dZoneDepth = [0.0 for x in self.range_of_layers]          #  zone depth
            self.DUL = [0.0 for x in self.range_of_layers]                 #  Drained upper limit
            self.dSat = [0.0 for x in self.range_of_layers]                #  zone saturation
            self.PLL = [0.0 for x in self.range_of_layers]                 #  lower limit of plant extractable soil water 
            self.dVoidCapacity = [0.0 for x in self.range_of_layers]       #  void capacity (total porosity) for each zone
            
            # Pointer to the station object
            ptr_station = self.station
            
            # Always retrieve the first value of all layers capacity 
            # The void capacity array should match the same layer count
            self.dVoidCapacity =  [i[0] for i in ptr_station.get_capacity()]
            
            if len(self.dVoidCapacity) != nb_layers:
                raise MismatchLayerCountError("layers", "The number of layers do not match")
                
            
            if ( ptr_station.is_dynamic_layer() ):
                #  for each layer determine dynamic Permanent Wilt, AWHC, thickness, DUL, Saturation, dPermWilt
                #  Column 0=thickness   1=Porosity  2=FieldCapacity   3=PermamentWilting(mm)
                #        dZoneDepth       dSat         DUL              dPermWilt 
                #  dPermWilt and PLL are same thing , one is in mm and one is in % 
                
                # Convert it once
                float_nb_layers = float(nb_layers)
                for i in self.range_of_layers:
                    dep_in_mm = ptr_station.get_capacity()[i][0]
                    dep = dep_in_mm / float_nb_layers
                    default_depth = int(utils.dotNetRoundUpEmulation(dep))
                    
                    ptr_station.layer_max_cap[i] = default_depth        
                    self.dZoneDepth[i] =  float(default_depth) * 10.0
                    self.dSat[i] =  ptr_station.get_capacity()[i][1] / 100  
                    self.DUL[i] =  ptr_station.get_capacity()[i][2]  / 100  
                    self.dPermWilt[i] = ptr_station.get_capacity()[i][3]
                    self.PLL[i] =  self.dPermWilt[i] / self.dZoneDepth[i]   
                    self.dAWHC[i] = (self.DUL[i] * self.dZoneDepth[i] ) - self.dPermWilt[i]
                    
                    cap = self.dAWHC[i] / float_nb_layers
                    default_cap = int(utils.dotNetRoundUpEmulation(cap))
                    
                    ptr_station.total_cap += default_cap
                    ptr_station.layer_max_depth[i] = default_cap
            else:
                #  for each layer determine static Permanent Wilt, AWHC, ZoneDepth, self.DUL, Saturation, self.PLL
                for i in self.range_of_layers:
                    self.dPermWilt[i] = float(ptr_station.layer_max_cap[i]) * 0.3
                    self.dAWHC[i] = float(ptr_station.layer_max_cap[i])
                    self.dZoneDepth[i] = float(ptr_station.layer_max_depth[i]) * 10.0
                    self.DUL[i] = (self.dAWHC[i] + self.dPermWilt[i]) / self.dZoneDepth[i]
                    self.dSat[i] = self.dVoidCapacity[i] / self.dZoneDepth[i]
                    self.PLL[i] = self.dPermWilt[i] / self.dZoneDepth[i]
                
            # convert field capacity and water content to plant available water and total void spaces to excess pore spaces.
            # !!Note: this code was taken from PDI program without modifications, appears to be different from old Fortran code.!!
            for i in self.range_of_layers:
                #  initial PAW set here?
                self.dSoilMoistContent[i] = float(ptr_station.layer_max_cap[i]) * 0.5
                max_perm_wilt =  self.DUL[i] * self.dZoneDepth[i] * 0.95
                if self.dPermWilt[i] > max_perm_wilt:
                    self.dPermWilt[i] = max_perm_wilt
                if self.dVoidCapacity[i] < self.dSoilMoistContent[i]:
                    self.dVoidCapacity[i] = self.dSoilMoistContent[i]
                #  if original value was AWHC (or a percentage thereof) why are we subtracting wilt?
                self.dSoilMoistContent[i] -= self.dPermWilt[i]
                self.dContent2[i] = self.dSoilMoistContent[i]
                self.dVoidCapacity[i] -= self.dAWHC[i] + self.dPermWilt[i]
                if self.dVoidCapacity[i] < 0.0:
                    self.dVoidCapacity[i] = 0.0
             
            #  total excess (drainable) pore spaces
            #  over layer 0        (dExcessTop)
            #  over all root zones (self.dExcessTotal)
            for i in self.range_of_layers:
                self.dExcessTotal += self.dVoidCapacity[i]
        
            #  default excess to double AWHC to not impede drainage
            if self.dExcessTotal <= 0.0:
                for i in self.range_of_layers:
                    self.dVoidCapacity[i] = self.dAWHC[i] * 2.0
        
            self.dSoilTempHistory = [ 999.9 for x in range(self.SOIL_TEMP_DAYS)] 
            
            self.iCurrentCropStage = 0
            self.T2 = 0.0
            self.dSnowCover = 0.0
            self.dSnowCoef = self.get_snow_coeff(0)
            self.dSoilTempSum = 0.0
            self.dSoilTemp1 = 0.0
            self.dSoilTemp2 = 0.0    
            self.dTotalDepth = 0.0
            self.dDeltaSum = 0.0
            self.dAvailWaterToday = 0
            self.dAvailWaterYester = 0    
                
            if self.is_debug_enabled:
                logging.debug("Initialization before any computation")
                logging.debug("Dumping self.dPermWilt array")
                logging.debug(self.dPermWilt)   
                logging.debug("Dumping self.dAWHC array")
                logging.debug(self.dAWHC)   
                logging.debug("Dumping self.dZoneDepth array")
                logging.debug(self.dZoneDepth)   
                logging.debug("Dumping self.DUL array")
                logging.debug(self.DUL)   
                logging.debug("Dumping self.dSat array")
                logging.debug(self.dSat)   
                logging.debug("Dumping self.PLL array")
                logging.debug(self.PLL)     
                logging.debug("Dumping self.dSoilMoistContent array")
                logging.debug(self.dSoilMoistContent)     
                logging.debug("Dumping self.dVoidCapacity array")
                logging.debug(self.dVoidCapacity)    
                logging.debug("Dumping self.dContent2 array")
                logging.debug(self.dContent2)  
                logging.debug("Dumping self.dExcessTotal")
                logging.debug(self.dExcessTotal)   
                logging.debug("Dumping snow_coeff")
                logging.debug(self.crop_model.get_snow_coeff())      
            
            currentDate = self.start_date 
            
            #  delete it if already exists
            if os.path.isfile(self.csv_output_file_station):
                if self.is_info_enabled:    
                    logging.info ("This file " + self.csv_output_file_station + " had to be deleted because it was already there")  
                os.remove(self.csv_output_file_station) 
                
            python_release = utils.release_version()
            
            #  When this is time to write to a CSV file, python 2 and 3 differ to avoid double carriage return
            if python_release <= 2:
                csvfile = open(self.csv_output_file_station , 'wb') 
            else:
                csvfile = open(self.csv_output_file_station , 'w', newline="") 
            
            with csvfile:
                spamwriter = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_NONE)

                logging.debug("Looping through from {} to {}".format(currentDate,self.end_date))  
                
                # restore original values
                ptr_station.restore()
                
                # Calculate Solar Radiation once for all days only for the method Baier Robertson 
                self.latitude = ptr_station.get_latitude()
                if self.method == "BR": # Baier Robertson
                    self.rstop, self.photp = utils.calculateSolarRadiation(self, self.latitude)
                    if logging.getLogger().getEffectiveLevel() == logging.DEBUG:
                        logging.debug("Dumping solar radiation rstop")
                        logging.debug(self.rstop)   
                        logging.debug("Dumping solar radiation photp")
                        logging.debug(self.photp)                              
                else:
                    self.rstop = None
                    self.photp = None
                
                #  loop through dates
                #  note some variables (such as snow cover) will be accumulated
                list_columns = []
                line_count = 0
                    
                #  Model might need to pre-process something before beginning the date looping
                self.timing_delay_execution()
                
                if self.preProcess():
                    while currentDate <= self.end_date:   

                        line_count += 1
                        
                        if line_count % 250 == 0:
                            # This loop is very intense, so let other processes do their work
                            self.timing_delay_execution()                      
                           
                        if self.is_debug_enabled:
                            logging.debug("Processing with current date {}".format(currentDate))     
                        
                        # For debugging purpose
                        #if currentDate.year == 1985 and currentDate.month == 5 and currentDate.day == 20:
                        #    print("debug here")
                        
                        #  General comment, it is better to check if the logging is in DEBUG mode rather not having this extra line
                        #  Python will evaluate the whole debug arguments before checking if it has really to debug that infomration 
                        #  slowing down the whole process
                        
                        self.currentDaysOfYear = utils.get_days_of_the_year(currentDate) 
        
                        if self.is_debug_enabled:
                            logging.info("\tDays Of Year {}".format(self.currentDaysOfYear))   
                        
                        #  I commented out 'logging.warn' because if I need to dig more, the code will be there
                        
                        #  get weather for the day, calculate it once to save time
                        if self.is_debug_enabled:
                            logging.debug("\tGet getDayWeatherData with current date")   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()
                        self.getDayWeatherData(currentDate)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_getDayWeatherData += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method getDayWeatherData took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")                     
                      
                        #  determine type of precipitation: snow/rain
                        
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determinePrecipitationType with {}".format(self.dTx))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                    
                        self.determinePrecipitationType(self.dTx)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determinePrecipitationType += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determinePrecipitationType took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")          
                            
                        #  determine snow equivalent
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineSnowEquivalent with {}, {}, {}".format(self.dPt,self.dRain,self.dSnowCoef))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                    
                        self.determineSnowEquivalent(self.dPt, self.dRain, self.dSnowCoef)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineSnowEquivalent += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineSnowEquivalent took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")           
                                        
                        #  determine daily average temperature
                        if self.is_debug_enabled:
                            logging.debug("\tCalculate determineDayAverageTemp with {}, {}".format(self.dTx, self.dTn))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                      
                        self.determineDayAverageTemp(self.dTx, self.dTn)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineDayAverageTemp += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineDayAverageTemp took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")        
                
                        # Calculate the proper Phenology
                        if self.is_debug_enabled:
                            logging.debug("\tCalculate calculatePhenology with current date")   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.calculatePhenology(currentDate)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determinePhenology += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method calculatePhenology took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")                          
                        
                        #  determine adjustedPET (adjustment based on snow cover)
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineAdjustedPET with {}, current date".format(self.dSnowWatEq))  
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()   
                        self.determineAdjustedPET(self.dSnowWatEq, currentDate)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineAdjustedPET += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineAdjustedPET took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")          
                                        
                        #  determine soil surface temperature
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineSoilSurfaceTemp with {}, {}, {}".format(self.dTa, self.dTx, self.dTn))     
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determineSoilSurfaceTemp(self.dTa, self.dTx, self.dTn)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineSoilSurfaceTemp += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineSoilSurfaceTemp took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")      
                                        
                        #  determine 3 day running average for soil temperature
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineThreeDaySoilSurfaceAverage with {}".format(self.dSoilTempHistory))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determineThreeDaySoilSurfaceAverage()
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineThreeDaySoilSurfaceAverage += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineThreeDaySoilSurfaceAverage took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")        
                                        
                        #  determine snow budget
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineSnowBudget with {}".format(self.dSoilMoistContent))     
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determineSnowBudget()
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineSnowBudget += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineSnowBudget took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")      
                                        
                        #  determine potential daily melt from snow
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determinePotentialDailyMelt with {}".format(self.dTa))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determinePotentialDailyMelt(self.dTa)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determinePotentialDailyMelt += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determinePotentialDailyMelt took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")      
                                        
                        #  determine retention and actual loss to snow pack
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineRetentionAndLoss with {}, {}, {}".format(self.T2, self.dDailySnowMelt, self.dPackRetainMelt))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determineRetentionAndLoss(self.dDailySnowMelt)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineRetentionAndLoss += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineRetentionAndLoss took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")      
                                        
                        #  determine Infiltration and run off
                        if self.is_debug_enabled:
                            logging.debug("\tDeteremine deteremineInfiltrationAndRunoff with {}, {}, {}".format(self.dMeltDrain, self.dRain, self.dSoilTemp1))     
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.deteremineInfiltrationAndRunoff(self.dMeltDrain, self.dRain, self.dSoilTemp1)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_deteremineInfiltrationAndRunoff += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method deteremineInfiltrationAndRunoff took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")       
                                        
                        #  determine snow depletion
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine determineSnowDepletion with {}, {}".format(self.dAdjustedPET, self.dSnowCover))    
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determineSnowDepletion()
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineSnowDepletion += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineSnowDepletion took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")         
                                        
                        #  determine ET
                        if self.is_debug_enabled:
                            logging.debug("\tDetermine ET")     
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.determineET()
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_determineET += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method determineET took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")       
                                        
                        #  apply precipitation and evaporation to layers
                        if self.is_debug_enabled:
                            logging.debug("\tApply applyPrecipitationAndEvaporationToLayers with {}, {}, {}, {}".format(self.dWaterLoss, self.dDelta, self.dContent2, self.dSoilMoistContent))    
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                          
                        self.applyPrecipitationAndEvaporationToLayers()
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_applyPrecipitationAndEvaporationToLayers += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method applyPrecipitationAndEvaporationToLayers took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")         
                                        
                        #  calculate drainage and soil water distribution
                        if self.is_debug_enabled:
                            logging.debug("\tCalculate calculateDrainageAndSoilWaterDistribution with {}, {}, {}, {}, {}, {}, {}, {}".format(self.dSurfaceWater, self.dRunoff, self.dRunoffFrozen, \
                                                                                                                                        self.dSoilMoistContent, self.dPermWilt, self.dZoneDepth, self.dSat, self.DUL))   
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                      
                        self.calculateDrainageAndSoilWaterDistribution(self.dSurfaceWater, self.dRunoff, self.dRunoffFrozen)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_calculateDrainageAndSoilWaterDistribution += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method calculateDrainageAndSoilWaterDistribution took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")       
                                        
                        #  account for moisture redistribution or unsaturated flow
                        if self.is_debug_enabled:
                            logging.debug("\tCalculate accountForMoistureRedistributionOrUnsaturatedFlow")      
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                      
                        self.accountForMoistureRedistributionOrUnsaturatedFlow()
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_accountForMoistureRedistributionOrUnsaturatedFlow += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method accountForMoistureRedistributionOrUnsaturatedFlow took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")        
                                        
                        #  Wrting output CSV
                        if self.is_debug_enabled:
                            logging.debug("\tGenerate output line with current date")                 
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                      
                        columns = self.outputDayVSMBResults(currentDate) #  calculate stress index = 1- AET/PET
                        list_columns.append(columns)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_outputDayVSMBResults += (secMethodEnd - secMethodStart)
                            ##logging.warn("Method outputDayVSMBResults took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")      
                        
                        #  Add one day
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()                       
                        currentDate = currentDate + timedelta(days=1)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_getNextDay += (secMethodEnd - secMethodStart)
                            ##logging.warn("Get next day took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")                     
        
                    #  Model might need to pre-process before allowing writing columns
                    if self.preWritingColumns():
                            
                        #  Write all lines once
                        if must_track_speed:
                            secMethodStart = utils.current_millisecond()   
                    
                        ## Dynamic Version that goes through columns
                        header_columns = []
                        for col in self.print_columns:
                            if col == self.COLUMN_MOISTLAYERDISTRB:
                                for i in self.range_of_layers:
                                    header_columns.append("Moist-L{0}".format(i+1))
                            else:
                                header_columns.append(col)
                            
                        spamwriter.writerow(header_columns)
                                
                        ## Version 1 - initial columns to print out
                        ##spamwriter.writerow(["Year","Mon","Day","MaxTemp","MinTemp","P","GDD","PET","AET","SI","TotMoist","PrcnAWHC","CropStage"])
                        
                        ## Version 1.1 modified columns to print out
                        ##spamwriter.writerow(["Year","Mon","Day","MaxTemp","MinTemp","Pcpn","GDD5","PET","AET","SI","Runoff","WaterLeach","TotMoist","Moist-L1","Moist-L2","Moist-L3","Moist-L4","PrcnAWHC","CropStage","SeedDate","HarvestDate","Frozen Runoff","Snow Loss","Pack Melt Retained","Snow Cover"])
                        
                        spamwriter.writerows(list_columns)
                        if must_track_speed:
                            secMethodEnd = utils.current_millisecond()
                            self.timer_writeRow += (secMethodEnd - secMethodStart)
                            ##logging.warn("Write row took " + "{0:.4f}".format(secMethodEnd - secMethodStart) + " ms")       
                                
            if must_track_speed:
                # aggragte all timers per method
                secProcessEnd = utils.current_millisecond()
                time_spend = secProcessEnd - secProcessStart
                self.time_spend.append(round(self.timer_getDayWeatherData))
                self.time_spend.append(round(self.timer_determinePrecipitationType))
                self.time_spend.append(round(self.timer_determineSnowEquivalent))
                self.time_spend.append(round(self.timer_determineDayAverageTemp))
                self.time_spend.append(round(self.timer_determinePhenology))
                self.time_spend.append(round(self.timer_determineAdjustedPET))
                self.time_spend.append(round(self.timer_determineSoilSurfaceTemp))
                self.time_spend.append(round(self.timer_determineThreeDaySoilSurfaceAverage))
                self.time_spend.append(round(self.timer_determineSnowBudget))
                self.time_spend.append(round(self.timer_determinePotentialDailyMelt))
                self.time_spend.append(round(self.timer_determineRetentionAndLoss))
                self.time_spend.append(round(self.timer_deteremineInfiltrationAndRunoff))
                self.time_spend.append(round(self.timer_determineSnowDepletion))
                self.time_spend.append(round(self.timer_determineET))
                self.time_spend.append(round(self.timer_applyPrecipitationAndEvaporationToLayers))
                self.time_spend.append(round(self.timer_calculateDrainageAndSoilWaterDistribution))
                self.time_spend.append(round(self.timer_accountForMoistureRedistributionOrUnsaturatedFlow))
                self.time_spend.append(round(self.timer_outputDayVSMBResults))
                self.time_spend.append(round(self.timer_writeRow))
                self.time_spend.append(round(self.timer_getNextDay))
                logging.warn("Process took for {} iterations {} in ms for a grand total of {}".format(line_count, self.time_spend, sum(self.time_spend))) 
        
        except Exception as e:
            msgErr = "Process caugth exception=" + traceback.format_exc()
            logging.error(msgErr)
            traceback.format_exc()
            raise #re-raise
        
    """
       The following is the left over 'non-mandatory' methods/getters used by the 'process' method
    """  
        
    def getDayWeatherData(self, currentDay): 
        """ reads daily max, min temp and precipitation for the station #Inputs: day as date #Outputs: N/A #Special: updates daily variables dTx, dTn and dPt
    
        """   
        if currentDay.month == 1 and currentDay.day == 1: 
            # yearly initialization of seeding date and phenology parameters
            self.iSeedDate = 0
            self.iNiceDayCount = 0
            self.iBMTStage = 0
            self.dBMT = 0
            self.iHarvestDate = utils.MAX_DAYS
            self.iCropStageDate = [0 for x in self.range_stages] 
            
        # reset crop stage after November 1st
        if self.currentDaysOfYear >= 305:
            self.iBMTStageDec = 0.0
                 
        #  build date
        #  set the values of days we will want to compare
        year = currentDay.year
        nmakeup_date = datetime.datetime(year, 4, 15)
        #  Days of the year and make an array in subtracting 1
        #  WARNING: This function is very slow
        self.iEarliestSeed = utils.get_days_of_the_year(nmakeup_date) #   nmakeup_date.timetuple().tm_yday 
        
        nmakeup_date = datetime.datetime(year, 6, 15)
        self.iLatestSeed = utils.get_days_of_the_year(nmakeup_date) #   nmakeup_date.timetuple().tm_yday     
        
        nmakeup_date = datetime.datetime(year, 8, 15)
        self.iEarliestHarvest = utils.get_days_of_the_year(nmakeup_date) #   nmakeup_date.timetuple().tm_yday     
        
        nmakeup_date = datetime.datetime(year, 9, 30)
        self.iLatestHarvest = utils.get_days_of_the_year(nmakeup_date) #   nmakeup_date.timetuple().tm_yday             
        
        # get daily data
        # reads daily max, min temp and precipitation for the station
        ptr_station = self.station
        self.dTx = ptr_station.get_tx(year, self.currentDaysOfYear)
        self.dTn = ptr_station.get_tn(year, self.currentDaysOfYear)
        self.dPt = ptr_station.get_pt(year, self.currentDaysOfYear)
        self.dGrowDD = self.calculateGrowDD(self.dTx, self.dTn) 
        self.dCHU = self.calculateCHU(self.dTx, self.dTn)      
        
        if self.is_debug_enabled:
            logging.debug("\t\t month={}, day={}, iSeedDate={}, iNiceDayCount={}, iBMTStage={}, dBMT={}, iHarvestDate={}, iCropStageDate={}, nmakeup_date={}, iEarliestSeed={}, iLatestSeed={}, iEarliestHarvest={}, iLatestHarvest={}, dTx={}, dTn={}, dPt={} dGrowDD={} dCHU={}".format( \
                currentDay.month, \
                currentDay.day,  \
                self.iSeedDate, \
                self.iNiceDayCount, \
                self.iBMTStage, \
                self.dBMT, \
                self.iHarvestDate, \
                self.iCropStageDate, \
                nmakeup_date, \
                self.iEarliestSeed, \
                self.iLatestSeed, \
                self.iEarliestHarvest, \
                self.iLatestHarvest, \
                self.dTx, \
                self.dTn, \
                self.dPt, \
                self.dGrowDD, \
                self.dCHU))            

    def calculateGrowDD(self, dTmax, dTmin):
        """ determines calculate GDD
        
        """         
        dGrowDD = ((dTmax + dTmin) / 2.0) - 5.0
        if dGrowDD < 0:
            dGrowDD = 0.0
        
        return dGrowDD

            
    def determinePrecipitationType(self, dTx):
        """ determines if precipitation is snow or rain based on daily temperature

        """  
        
        if dTx <= 0:
            self.dRain = 0.0
            self.dSnow = self.dPt
        else:
            self.dRain = self.dPt
            self.dSnow = 0.0
            
        if self.is_debug_enabled:
            logging.debug("\t\t dRain={}, dSnow={}".format( \
                self.dRain, \
                self.dSnow))               
     
    def determineSnowEquivalent(self, dPt, dRain, dSnowCoef):
        """ determine snow water equivalent
    
        """  
        
        self.dSnowWatEq = (dPt - dRain) * dSnowCoef
        
        if self.dSnowWatEq < 0.0:
            self.dSnowWatEq = 0.0
         
        if self.is_debug_enabled:
            logging.debug("\t\t dSnowWatEq={},".format(self.dSnowWatEq))               

    def determineDayAverageTemp(self, dTx, dTn):
        """ calculate daily average temperature
   
        """  
        
        self.dTa = (dTx + dTn) / 2.0
        
        if self.is_debug_enabled:
            logging.debug("\t\t dTa={},".format(self.dTa))              

    @abstractmethod
    def calculatePhenology(self, currentDate): 
        """ To be implemented if a Phenology model must be calculated
    
        """        
        # If not implemented, it will be skipped and values could be odd
        pass

    @abstractmethod
    def preProcess(self): 
        """ To be implemented if a Phenology might need to init other variables or any operations before beginning the date loop
            Returning:
               True means to the caller to keep going the Phenology Model
               False means to the caller to stop the Phenology Model
            
        """        
        # If not implemented, it will always true
        return True    

    @abstractmethod
    def preWritingColumns(self): 
        """ To be implemented if a Phenology might need to pre-process before allowing writing columns after ending the date loop
            Returning:
               True means to the caller to keep going the Phenology Model
               False means to the caller to stop the Phenology Model
            
        """        
        # If not implemented, it will always true
        return True      
      
    def determineAdjustedPET(self, dSnowWatEq, currentDay):
        """ calculate adjustedPET based on snow equivalent
    
        """  
        self.dSnowCover += dSnowWatEq
        
        if self.method == "BR": # Baier Robertson
            self.dComputedPET = self.station.get_pe_baier_robertson(currentDay, self.rstop)
        elif self.method == "PM": # Penman-Monteith
            self.dComputedPET = self.station.get_pe_penman(currentDay)
        else:
            # Default Priestley-Taylor
            self.dComputedPET = self.station.get_pe(currentDay, self.dSnowCover)
        
        self.dAdjustedPET = self.dComputedPET
        
        # For every day, there is P (which is an input from raw data and this the total precipitation for the day). 
        # P-PE thus PE is the potential loss every day and the difference between the two is what we call demand.
        # During very hot days (spring and summer) it is likely to have negative values but if it rains a bunch,
        # then the loss will be low and value will be positive. 
        # Thus, dPt (P) = Daily precipitation   minus   dComputedPET (PE) = Potential loss every day
        self.ppe = self.dPt - self.dComputedPET;
        
        if self.is_debug_enabled:
            logging.debug("\t\t dSnowCover={}, dComputedPET={}, dComputedPET={} p-pe={}".format( \
                self.dSnowCover, \
                self.dComputedPET, \
                self.dAdjustedPET,
                self.ppe))           
        
    def determineSoilSurfaceTemp(self, dTa, dTx, dTn):
        """ find the soil sufrace temperature
    
        """  
        soil_surface_temp = dTa + 0.25 * (dTx - dTn)
        self.dSoilTempHistory[self.SOIL_TEMP_DAYS - 1] = soil_surface_temp
        
        if self.is_debug_enabled:
            logging.debug("\t\t dSoilTempHistory={}".format( \
                self.dSoilTempHistory))            
        
    def determineThreeDaySoilSurfaceAverage(self):
        """ calculate three day running average for soil temperature
    
        """  

        for j in range(self.SOIL_TEMP_DAYS):
            if (self.dSoilTempHistory[j] != 999.9):
                self.dSoilTempSum += self.dSoilTempHistory[j]
            
        self.dSoilTempSnowAdjust = 0
        self.dSoilTempAve = self.dSoilTempSum / self.FLOAT_SOIL_TEMP_DAYS
        self.dSoilTempSum = 0.0
        
        exp = math.exp #  Speed up built-in function pointer
        if self.dSnowCover > 0.0:
            self.dSoilTempSnowAdjust = self.dSnowCover / (self.dSnowCover + exp(2.303 - 0.2197 * self.dSnowCover))
        else:
            self.dSoilTempSnowAdjust = 0.0
            
        self.dSoilTemp2 = self.dSoilTemp1
        self.dSoilTemp1 = self.dSoilTempSnowAdjust * self.dSoilTemp2 + (1 - self.dSoilTempSnowAdjust) * self.dSoilTempAve # move the temp accumulator one day forward
        self.dSoilTempHistory[self.SOIL_TEMP_DAYS - 1] = self.dSoilTemp1
        
        for j in range(self.SOIL_TEMP_DAYS - 1): 
            self.dSoilTempHistory[j] = self.dSoilTempHistory[j + 1]
            
        if self.is_debug_enabled:
            logging.debug("\t\t dSoilTempSnowAdjust={}, dSoilTempAve={}, dSoilTempSum={}, dSoilTemp2={}, dSoilTemp1={}, dSoilTempHistory={}".format( \
                self.dSoilTempSnowAdjust, \
                self.dSoilTempAve, \
                self.dSoilTempSum, \
                self.dSoilTemp2, \
                self.dSoilTemp1, \
                self.dSoilTempHistory))            
            
    def determineSnowBudget(self):
        """ calculate water equivalent of snow pack and holding capacity
    
        """  
        
        ## This peace is the same than ...
        self.dDeficitTotal = self.station.get_total_awc() - sum(self.dSoilMoistContent)
        
        ## ... this following peace
        ##self.dDeficitTotal = self.station.get_total_awc()
        ##for j in self.range_layers:
        ##    self.dDeficitTotal -= self.dSoilMoistContent[j]
            
        self.dPackRetainMelt = self.dSnowCover * 0.15
        self.dMeltDrain = 0.0
        
        if self.is_debug_enabled:
            logging.debug("\t\t dDeficitTotal={}, dPackRetainMelt={}, dMeltDrain={}".format( \
                self.dDeficitTotal, \
                self.dPackRetainMelt, \
                self.dMeltDrain))           
        
    def determinePotentialDailyMelt(self, dTa):
        """ calculate potential daily melt
    
        """  
        
        # correct McKay value loaded
        self.dDailySnowMelt = self.snow_melt.get_mckey_value(self.currentDaysOfYear, dTa) 
        
        if self.dTx < 0:
            self.dDailySnowMelt = 0.0
            self.dPackRetainMelt = 0.0
            
        if self.dDailySnowMelt >= self.dSnowCover:
            self.dDailySnowMelt = self.dSnowCover
            self.dPackRetainMelt = 0.0
            self.dSnowCover = 0.0
        else:
            self.dSnowCover = self.dSnowCover - self.dDailySnowMelt
            
        if self.is_debug_enabled:
            logging.debug("\t\t dDailySnowMelt={}, dPackRetainMelt={}, dSnowCover={}".format( \
                self.dDailySnowMelt, \
                self.dPackRetainMelt, \
                self.dSnowCover))                       
            
    def determineRetentionAndLoss(self, dDailySnowMelt):
        """ calculate retention and loss to snow pack
    
        """  
        
        self.dPackSnowMelt = self.T2 + dDailySnowMelt
        
        if self.dPackSnowMelt < self.dPackRetainMelt:
            self.dPackRetainMelt = self.dPackSnowMelt
            self.dMeltDrain = 0.0
        else:
            self.dMeltDrain = self.dPackSnowMelt - self.dPackRetainMelt
            
        self.T2 = self.dPackRetainMelt
        
        if self.dMeltDrain <= 0.0:
            self.dMeltDrain = 0.0
            
        if self.is_debug_enabled:
            logging.debug("\t\t dPackSnowMelt={}, dPackRetainMelt={}, dMeltDrain={}, T2={}".format( \
                self.dPackSnowMelt, \
                self.dPackRetainMelt, \
                self.dMeltDrain, \
                self.T2))              
            
    def deteremineInfiltrationAndRunoff(self, dMeltDrain, dRain, dSoilTemp1):
        """ rather large function to determine Infiltration and Run off 
    
        """  
        
        self.dMeltInfil = dMeltDrain
        self.dDeficitTotal -= dMeltDrain
        
        if self.dSnowCover <= 0.0:
            self.dSnowCover = 0.0 # modified mcKay above to be examined 
            
        # (13) infiltration rate adjustment due to freezing of soil 
        self.dRunoff = 0.0
        self.dRunoffFrozen = 0.0 
        
        # (14) original runoff equation was replaced by wole using the scs eqution shown below
        self.dSurfaceWater = dRain + dMeltDrain
        
        if self.dSurfaceWater != 0.0: 
            # used the hob-krogman's technique to calculate dRunoff if the soil is frozen    
            # second method for calculating dRunoff in a frozen soil
            if (self.dSoilTemp1 < 0.0) and (dMeltDrain > 0.0):
                self.dRunoffFrozen = self.dSurfaceWater * (self.dSoilMoistContent[0] / self.dAWHC[0])
            else: # s value from scs equation, transfer to mm scale # curve numbers for dry and wet soilsoil
                CNPW = 0.0 # para. of curve number when soil is wet
                CNPD = 0.0 # para. of curve number when soil is dry
                AC2 = 100.0 - StationDetails.CURVE2
                DXX = 0.0 
                
                """ 
                calculation of self.WF for each layer # October 5, 1987 
                change the depmax to 45 cm when calculate wx value 
                assume only the top 45 cm water content affects runoff 
                calculate runoff by williams -scs curve no. technique 
                calculation of runoff according to scs curve number
                """
                
                exp = math.exp #  Speed up to use math object
                self.dTotalDepth = 0.0

                for l in self.range_layers: 
                    self.dTotalDepth += (self.dZoneDepth[l] / 10.0)
                    WX = 1.016 * (1.0 - exp(-4.16 * self.dTotalDepth / 45.0))
                    self.WF[l] = WX - DXX
                    DXX = WX
                    
                dWetCurveNum = StationDetails.CURVE2 * exp(0.006729 * AC2)
                dDryCurveNum = max(0.4 * StationDetails.CURVE2, StationDetails.CURVE2 - 20.0 * AC2 / (AC2 + exp(2.533 - 0.0636 * AC2)))
                if StationDetails.CURVE2 > 96.0:
                    dDryCurveNum = StationDetails.CURVE2 * (0.02 * StationDetails.CURVE2 - 1.0)
                    
                if dDryCurveNum >= 100.0:
                    dDryCurveNum = 100.0
                    
                if dWetCurveNum >= 100.0:
                    dWetCurveNum = 100.0

                for l in self.range_layers: 
                    self.SW[l] = (self.dSoilMoistContent[l] + self.dPermWilt[l]) / self.dZoneDepth[l]
                    CNPW += (self.SW[l] / self.DUL[l]) * self.WF[l]
                    CNPD += ((self.SW[l] - self.PLL[l]) / (self.DUL[l] - self.PLL[l])) * self.WF[l]
                    
                if CNPD >= 1.0:
                    dCurveNum = StationDetails.CURVE2 + (dWetCurveNum - StationDetails.CURVE2) * CNPW
                else:
                    dCurveNum = dDryCurveNum + (StationDetails.CURVE2 - dDryCurveNum) * CNPD
                    
                if dCurveNum == 0.0:
                    dCurveNum = 0.99
                    
                dSMX = 254.0 * (100.0 / dCurveNum - 1.0) 
                
                # reduce the retention factor if soil is frozen formula adapted from epic model, 
                # this method was # found inappropriate as such use the hob-krogman's technique shown below. 
                dPB = self.dSurfaceWater - 0.2 * dSMX
                
                if dPB > 0:
                    self.dRunoff = (dPB ** 2.0) / (self.dSurfaceWater + 0.8 * dSMX)
                    if self.dRunoff < 0.0:
                        self.dRunoff = 0.0
                        
        self.dDailyAET = 0.0
        
        if self.is_debug_enabled:
            logging.debug("\t\t dMeltInfil={}, dDeficitTotal={}, dSnowCover={}, dRunoff={}, dRunoffFrozen={}, dSurfaceWater={}, dTotalDepth={}, WF={}, SW={}, dDailyAET={}".format( \
                self.dMeltInfil, \
                self.dDeficitTotal,  \
                self.dSnowCover, \
                self.dRunoff, \
                self.dRunoffFrozen, \
                self.dSurfaceWater, \
                self.dTotalDepth, \
                self.WF, \
                self.SW, \
                self.dDailyAET))            
        
    def determineSnowDepletion(self):
        """ calculate snow depletion
    
        """  
        
        self.dSnowLoss = 0.0
        
        if self.dSnowCover > 10.0: # allow sublimation to go on at maximum PET only if more than 10 mm of snow
            if self.dSnowCover > self.dAdjustedPET:
                self.dSnowCover -= self.dAdjustedPET
                self.dSnowLoss = self.dAdjustedPET
                self.dAdjustedPET = 0.0
            else:
                self.dAdjustedPET -= self.dSnowCover
                self.dSnowLoss = self.dSnowCover
                self.dSnowCover = 0.0
                
        if self.is_debug_enabled:
            logging.debug("\t\t dSnowLoss={}, dSnowCover={}, dAdjustedPET={}".format( \
                self.dSnowLoss, \
                self.dSnowCover, \
                self.dAdjustedPET))         
                
    def determineET(self):
        """ find the ET for each layer
    
        """  
        
        self.dActEvap = 0.0
        self.dDeltaB = 0.0
        self.dRelMoistCont = 0.0
        self.dDeltaA = 0.0 
        nb_layers = self.get_nb_layers() 
        
        floor = math.floor #  Speed up built-in function pointer

        #  (16) calculates ET twice, once with rain then with no rain # root coefficients are used here
        for l in range(2): 
            for i in self.range_layers: 
                self.dRelMoistCont = self.dSoilMoistContent[i] / self.dAWHC[i]
                
                if l == 1:
                    self.dDeltaA = self.dContent2[i] / self.dAWHC[i]
                    
                if self.dRelMoistCont > 1.0:
                    self.dRelMoistCont = 1.0
                    
                if self.dDeltaA > 1.0:
                    self.dDeltaA = 1.0
                    
                self.dKCoef[i] = self.get_root_coeff(self.iCurrentCropStage, i)
                if (self.iCurrentCropStage >= StationDetails.KADJUSTSTAGE - 1) and (i != 0):
                    j = 1
                    while j <= i:
                        k = j - 1 # adjusting soil moisture coefficient for stress
                        if l == 0:
                            self.dKCoef[i] += self.dKCoef[i] * self.dKCoef[k] * (1.0 - self.dSoilMoistContent[k] / self.dAWHC[k])
                        else:
                            self.dKCoef[i] += self.dKCoef[i] * self.dKCoef[k] * (1.0 - self.dContent2[k] / self.dAWHC[k])
                        j = j + 1
                        
                IT = int(floor(self.dRelMoistCont * 100.0) - 1)
                
                if l == 1:
                    IT = int(floor(self.dDeltaA * 100.0) - 1) # should we be using a floor function here or should we round? #Dim IT As Integer = CInt(dRelMoistCont * 100.0) - 1 #If (l = 1) Then IT = CInt(self.dDeltaA * 100.0) - 1
                
                if IT < 0:
                    self.dWork = 0.0
                elif StationDetails.KNTROL <= nb_layers:
                    if i > StationDetails.KNTROL - 1:
                        self.dWork = self.get_z_table2(IT)
                    else:
                        self.dWork = self.get_z_table1(IT)
                elif self.iCurrentCropStage == 0:
                    self.dWork = self.get_z_table2(IT)
                else:
                    self.dWork = self.get_z_table1(IT) 
                    
                # water loss
                if l == 0:
                    self.dActEvap = self.dRelMoistCont * self.dWork * self.dAdjustedPET * self.dKCoef[i]
                    
                    if self.dActEvap > self.dSoilMoistContent[i]:
                        self.dActEvap = self.dSoilMoistContent[i]
                        
                    self.dWaterLoss[i] = self.dActEvap
                    self.dDailyAET += self.dWaterLoss[i]
                else:
                    self.dDeltaB = self.dDeltaA * self.dWork * self.dComputedPET * self.dKCoef[i]
                    
                    if self.dDeltaB > self.dContent2[i]:
                        self.dDeltaB = self.dContent2[i]
                        
                    self.dDelta[i] = self.dDeltaB
                    self.dDeltaSum += self.dDelta[i]
                    
                if self.is_debug_enabled:
                    logging.debug("\t\t l={} i={} dActEvap={}, dWork={} IT={}".format( \
                        l, \
                        i, \
                        self.dActEvap, \
                        self.dWork, \
                        IT, \
                        self.dAWHC, \
                        self.dContent2))                     
            
        self.dDailyAET += self.dSnowLoss
        
        if self.is_debug_enabled:
            logging.debug("\t\t dActEvap={}, dDeltaB={}, dRelMoistCont={} dDeltaSum={} dDeltaA={} dDailyAET={} dDelta={} dWaterLoss={} dKCoef={} dAWHC={} dContent2={}".format( \
                self.dActEvap, \
                self.dDeltaB, \
                self.dRelMoistCont, \
                self.dDeltaSum, \
                self.dDeltaA, \
                self.dDailyAET, \
                self.dDelta, \
                self.dWaterLoss, \
                self.dKCoef, \
                self.dAWHC, \
                self.dContent2))     

        
    def applyPrecipitationAndEvaporationToLayers(self):
        """ apply precipitation and evaporation to each soil layer
    
        """  

        for i in self.range_layers: 
            self.dSoilMoistContent[i] -= self.dWaterLoss[i]
            self.dContent2[i] -= self.dDelta[i]
            
            if self.dContent2[i] < 0.0:
                self.dContent2[i] = 0.0
                
            if self.dSoilMoistContent[i] < 0.0:
                self.dSoilMoistContent[i] = 0.0
                
        self.dWaterLoss = [0.0 for x in self.range_layers]
        self.dDelta = [0.0 for x in self.range_layers]
            
        if self.is_debug_enabled:
            logging.debug("\t\t dSoilMoistContent={}, dContent2={}, dWaterLoss={} dDelta={}".format( \
                self.dSoilMoistContent, \
                self.dContent2, \
                self.dWaterLoss, \
                self.dDelta))                 
            
    def calculateDrainageAndSoilWaterDistribution(self, dSurfaceWater, dRunoff, dRunoffFrozen):
        """ find drainage and soil water distribution

        """  
        
        self.dFlux[0] = dSurfaceWater - (dRunoff + dRunoffFrozen)
        
        nb_layers = self.get_nb_layers() - 1
        
        for l in self.range_layers: 
            self.HOLDw = self.dSat[l] * self.dZoneDepth[l] - (self.dSoilMoistContent[l] + self.dPermWilt[l])
            if (self.dFlux[l] == 0.0) or (self.dFlux[l] <= self.HOLDw):
                self.dSoilMoistContent[l] += self.dFlux[l]
                self.dDrain = ((self.dSoilMoistContent[l] + self.dPermWilt[l]) - self.DUL[l] * self.dZoneDepth[l]) # self.DUL = constant for (layer,station) # StationDetails.DRS2 = 0.8
                if l == nb_layers:
                    self.dDrain = ((self.dSoilMoistContent[l] + self.dPermWilt[l]) - StationDetails.DRS2 * self.DUL[l] * self.dZoneDepth[l])
                if self.dDrain < 0.0:
                    self.dDrain = 0.0
                self.dSoilMoistContent[l] -= self.dDrain
                self.dFlux[l] = self.dDrain
            elif self.dFlux[l] > self.HOLDw:
                self.dDrain = (self.dSat[l] - self.DUL[l]) * self.dZoneDepth[l]
                self.dSoilMoistContent[l] = self.dSat[l] * self.dZoneDepth[l] - self.dDrain - self.dPermWilt[l]
                self.dFlux[l] -= self.HOLDw + self.dDrain
                
            if l < nb_layers:
                self.dFlux[l + 1] = self.dFlux[l]
            
        if l >= nb_layers:
            l = nb_layers
            
        self.WLEACH = self.dFlux[l]
        
        self.dFlux = [0.0 for x in self.range_layers] 
            
        if self.is_debug_enabled:
            logging.debug("\t\t dFlux={}, HOLDw={}, dSoilMoistContent={} dDrain={} WLEACH={}".format( \
                self.dFlux, \
                self.HOLDw, \
                self.dSoilMoistContent, \
                self.dDrain, \
                self.WLEACH))              
            
    def accountForMoistureRedistributionOrUnsaturatedFlow(self):
        """ This is modification adapted from the ceres model to account for moisture redistribution or unsaturated flow also determines grow days 
    
        """  

        for l in self.range_layers: 
            self.SWX[l] = (self.dSoilMoistContent[l] + self.dPermWilt[l]) / self.dZoneDepth[l]
         
        # if the first layer is less than 50cm deep then begin processing the second layer   
        if self.dZoneDepth[0] <= 50.0: 
            self.iStartLayer = 1
        else:
            self.iStartLayer = 0
            
        #  Speed up built-in function pointer
        exp = math.exp 
        
        # only go to the second last layer as ... # layer L is the layer being processed # layer m is the layer below the layer being processed
        l = self.iStartLayer
        nb_layers = self.get_nb_layers() - 2
        while l <= nb_layers: 
            m = l + 1
            dMoisture1 = self.dSoilMoistContent[l] / self.dZoneDepth[l]
            dMoisture2 = self.dSoilMoistContent[m] / self.dZoneDepth[m]
            dDbar = 0.88 * exp(35.4 * (dMoisture1 + dMoisture2) * 0.5)
            
            if dDbar > 100:
                dDbar = 100
                
            dFLow = 10.0 * dDbar * (dMoisture2 - dMoisture1) / ((self.dZoneDepth[l] + self.dZoneDepth[m]) * 0.5)
            self.WFlow = dFLow * 10.0 / self.dZoneDepth[l]
            
            if dFLow > 0:
                if self.SWX[l] + self.WFlow > self.dSat[l]:
                    dFLow = (self.dSat[l] - self.SWX[l]) * (self.dZoneDepth[l] / 10.0)
                else:
                    if self.SWX[l] + self.WFlow > self.DUL[l]:
                        dFLow = (self.DUL[l] - self.SWX[l]) * (self.dZoneDepth[l] / 10.0)
            else:
                if self.SWX[m] - self.WFlow > self.dSat[m]:
                    dFLow = (self.SWX[m] - self.dSat[m]) * (self.dZoneDepth[m] / 10.0)
                else:
                    if self.SWX[m] - self.WFlow > self.DUL[m]:
                        dFLow = (self.SWX[m] - self.DUL[m]) * (self.dZoneDepth[m] / 10.0)
                        
            self.SWX[l] += dFLow / (self.dZoneDepth[l] / 10.0)
            self.SWX[m] -= dFLow / (self.dZoneDepth[m] / 10.0)
            self.dSoilMoistContent[l] = self.SWX[l] * self.dZoneDepth[l] - self.dPermWilt[l]
            self.dSoilMoistContent[m] = self.SWX[m] * self.dZoneDepth[m] - self.dPermWilt[m]
            l = l + 1 
          
        if self.is_debug_enabled:
            logging.debug("\t\t SWX={}, iStartLayer={}, WFlow={} SWX={} dSoilMoistContent={} dGrowDD={}".format( \
                self.SWX, \
                self.iStartLayer, \
                self.WFlow, \
                self.SWX, \
                self.dSoilMoistContent, \
                self.dGrowDD))            
         

    def calculateCHU(self, dTmax, dTmin):
        """ determines calculate CHU
        
        """         
        CHUday = 0.0
        CHUnight = 0.0
        if dTmax >= 10:
            #Speed trick: Have only one subtract operation to do instead of 3
            substract_dtMax = dTmax - 10.0
            CHUday = 3.33 * substract_dtMax - 0.084 * substract_dtMax * substract_dtMax
        else:
            CHUday = 0.0

        if dTmin >= 4.4:
            CHUnight = 1.8 * (dTmin - 4.4)
        else:
            CHUnight = 0.0

        return 0.5 * (CHUday + CHUnight)     
    
    def outputDayVSMBResults(self, currentDate):
        """ writes daily results of VSMB calculations into the file, updates dAvailWaterToday
            
        Args:
             currentDate (date): Date value
    
        Return: It returns a list of values in string
        
        """  
        
        # calculate stress index = 1- AET/PET
        if self.dAdjustedPET != 0:
            SI = 1 - self.dDailyAET / self.dAdjustedPET
        else:
            if self.dDailyAET == 0:
                SI = 1.0
            else:
                SI = 0.0
        
        #  Number of days in a year/month
        num_days = calendar.monthrange(currentDate.year, currentDate.month)[1]
        
        # Use 'sum' to accumulate a whole array
        dSoilMoistContentSum = sum(self.dSoilMoistContent)
        totPermWilt = sum(self.dPermWilt)
        dAWHCSum = sum(self.dAWHC)
        tolMoistList = [0.0 for x in self.range_layers]
        for l in self.range_layers: 
            tolMoistList[l] = self.dSoilMoistContent[l] + self.dPermWilt[l]
            # See the 'sum' done earlier
            #dSoilMoistContentSum += self.dSoilMoistContent[l]
            #totPermWilt += self.dPermWilt[l]
            #dAWHCSum += self.dAWHC[l]
            
            
                
        totMoist = dSoilMoistContentSum + totPermWilt
        prcnAWHC = dSoilMoistContentSum / dAWHCSum      
        
        # Accumulate some stats
        if self.currentDaysOfYear == 1 or self.currentDaysOfYear == 305:
            # Set the accumulated stress index value to be 0 at the beginning 
            # of every year and on September 1st
            self.GDD_acc = 0.0       
            self.CHU_acc = 0.0      
            self.P_acc = 0.0  
            self.SI_acc = 0.0
        else:
            if self.currentDaysOfYear >= 121 and self.currentDaysOfYear <= 304:
                # Accumulate between May 1st and August 31st
                self.SI_acc += SI               # Accumulate stress index
                self.GDD_acc += self.dGrowDD    # Accumulate heat units
                self.P_acc += self.dPt          # Accumulate precipitation
                self.CHU_acc += self.dCHU       # Accumulate crop heat units                
                
        columns = []
        
        for col in self.print_columns:
            if col == self.COLUMN_YEAR:
                #  Print Year
                columns.append(currentDate.year)
    
            elif col == self.COLUMN_MON:
                #  Print Month
                columns.append(currentDate.month)
        
            elif col == self.COLUMN_DAY:
                #  Print Day
                columns.append(currentDate.day)
        
            elif col == self.COLUMN_MAXTEMP:
                #  Print MaxTemp
                columns.append(utils.format_float_value(self.dTx))
        
            elif col == self.COLUMN_MINTEMP:
                #  Print MinTemp
                columns.append(utils.format_float_value(self.dTn))
        
            elif col == self.COLUMN_P:
                #  Print P
                columns.append(utils.format_float_value(self.dPt))
        
            elif col == self.COLUMN_GDD:
                #  Print GDD
                columns.append(utils.format_float_value(self.dGrowDD))
        
            elif col == self.COLUMN_PET:
                #  Print PET
                columns.append(utils.format_float_value(self.dAdjustedPET))
        
            elif col == self.COLUMN_AET:
                #  Print AET
                columns.append(utils.format_float_value(self.dDailyAET))
        
            elif col == self.COLUMN_SI:
                #  Print SI
                columns.append(utils.format_float_value(SI))
        
            elif col == self.COLUMN_TOTAWC:                 # to me, column is inverted self.COLUMN_TOTSOILMOIST:
                #  Print TotSoilMoistContent
                columns.append(utils.format_float_value(dSoilMoistContentSum))
        
            elif col == self.COLUMN_PRCNAWHC:
                #  Print PrcnAWHC
                columns.append(utils.format_float_value(round(prcnAWHC,7)))   
        
            elif col == self.COLUMN_CROPSTAGE:
                #  Print CropStage
                # either iBMTStageDec?? or iCurrentCropStage
                columns.append(utils.format_float_value(self.iBMTStageDec))
        
            elif col == self.COLUMN_RUNOFF:
                #  Print Runoff
                columns.append(utils.format_float_value(self.dRunoff))
        
            elif col == self.COLUMN_DRAINAGE:
                #  Print WaterLeach
                columns.append(utils.format_float_value(self.WLEACH))
        
            elif col == self.COLUMN_TOTSOILMOIST:           # to me, column is inverted self.COLUMN_TOTAWC:     
                #  Print  Available soil water content
                columns.append(utils.format_float_value(totMoist))        
        
            elif col == self.COLUMN_MOISTLAYERDISTRB:
                for l in self.range_layers: 
                    columns.append(utils.format_float_value(tolMoistList[l]))   
        
            elif col == self.COLUMN_SEEDDATE:     
                #  Print SeedDate
                columns.append(self.iSeedDate)
        
            elif col == self.COLUMN_HARVESTDATE:     
                #  Print HarvestDate 
                columns.append(self.iHarvestDate)
        
            elif col == self.COLUMN_FROZENRUNOFF:    
                #  Print Frozen Runoff 
                columns.append(utils.format_float_value(self.dRunoffFrozen, mustBeRounded = True))
                
            elif col == self.COLUMN_SNOWCOVER:     
                #  Print Snow Cover
                columns.append(utils.format_float_value(self.dSnowCover, mustBeRounded = True))       
    
            elif col == self.COLUMN_SNOW:     
                #  Print Daily snowfall
                columns.append(utils.format_float_value(self.dSnow, mustBeRounded = True))     
            
            elif col == self.COLUMN_RAIN:     
                #  Print Daily rainfall 
                columns.append(utils.format_float_value(self.dRain, mustBeRounded = True))     
            
            elif col == self.COLUMN_SOILTEMPAVG3:     
                #  Print Three day running average soil surface temperature  
                columns.append(utils.format_float_value(self.dSoilTempAve, mustBeRounded = True))                

            elif col == self.COLUMN_GDDACC:     
                #  Print GDD Accumulation 
                columns.append(utils.format_float_value(self.GDD_acc))          
                
            elif col == self.COLUMN_CHUACC:    
                #  Print CHU Accumulation 
                columns.append(utils.format_float_value(self.CHU_acc))     
                
            elif col == self.COLUMN_PACC:     
                #  Print 'P'cpn Accumulation 
                columns.append(utils.format_float_value(self.P_acc))  
                
            elif col == self.COLUMN_P_PE:     
                #  Print water demand      
                columns.append(utils.format_float_value(self.ppe))
                
            elif col == self.COLUMN_CHU:    
                #  Print CHU Accumulation 
                columns.append(utils.format_float_value(self.dCHU))    
                
            elif col == self.COLUMN_SNOWMELT:    
                #  Print CHU Accumulation 
                columns.append(utils.format_float_value(self.dDailySnowMelt))                
                
                ### Was decided by Ashon and Yinsuo that it was not be printed
                ###elif col == self.COLUMN_SNOWLOSS:    
                ###    #  Print Snow Loss
                ###    columns.append("{0:.2f}".format(round(self.dSnowLoss,7)))
                ###         
                ###elif col == self.COLUMN_PACKMELTRETAINED:     
                ###    Print Pack Melt Retained
                ###    columns.append("{0:.2f}".format(round(self.dPackRetainMelt,7)))
                ###            
        
        # check if day is end of the month and do something
        if currentDate.day == num_days:
            for j in self.range_layers:
                self.dContent2[j] = self.dSoilMoistContent[j]
                
        if self.is_debug_enabled:
            logging.debug("\t\t SI={}, num_days={}, dContent2={}".format( \
                SI, \
                num_days, \
                self.dContent2))    
        
        return columns;

    def is_column_checked(self, column_name):
        """ Test is the column name belongs to the one provided
            
        Args:
             column_name (string): Column Name
             
        Return: It returns true if the column name belongs to print columns list or false if not
    
        """  
        
        return column_name in self.print_columns
        
class MismatchLayerCountError(Exception):
    """ Exception raised for an invalid layer count

    Attributes:
        expr -- input expression in which the error occurred
        msg  -- explanation of the error
    """

    def __init__(self, expr, msg):
        self.expr = expr
        self.msg = msg
        
class ThicknessTooBigError(Exception):
    """ Exception raised for an thickness to begin calculation

    Attributes:
        expr -- input expression in which the error occurred
        msg  -- explanation of the error
    """

    def __init__(self, expr, msg):
        self.expr = expr
        self.msg = msg