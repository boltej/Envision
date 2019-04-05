/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#include "stdafx.h"
#include "Analytics.h"

#include <QueryEngine.h>
#include <Maplayer.h>
#include <DeltaArray.h>


////////////////////////////////////////////////////

FeatureClass *FeatureClass::CreateFeatureClass( FeatureType type )
   {
   switch( type )
      {
      case FT_EVENT:
         {
         EventFeatureClass *pFeature = new EventFeatureClass;
         return (FeatureClass*) pFeature;
         }
      }

   return NULL;
   }


//////////////////////////////////////////////////////

Analytics::Analytics()
   {
   }


Analytics::~Analytics()
   {
   }

bool Analytics::SpatioTemporalCorrelation( MapLayer *pLayer, DeltaArray *deltaArray)
   {
   // basic idea:
   // 
   // Starting with a map (at time zero) and a delta array,
   // identify any EventFeatures on the landscape that satisfy it's
   // associated spatiotemporal query.  These are stored.  Once all of the
   // EventFeatures are identified for this time step, a correlation matrix
   // is created for the timestep that contains the frequency of co-occurance
   // of all EventFeatures (e.g. a half-square matrix with sides=number of features.

   // roll the delatarray forward one timestep and repeat, accumulating counts
   // for each time step.



   // cube concept:
   //   time X iduOffset X field X color class representing delta types 










   /*
   

machine leearning problems:

1) find correlations between different "events" in a temporal map,
based on "distance" between the events (close=more correlated, far= not correlated)

An "event" is some landscape phenomena of interest, say a "fire" or a Zoning Change".

Example:  is there correlation between value of property damage from fire
  and addition of houses?

  <!-- define input data sources -->
  <input map='somemap.shp' deltas='somedelta.csv' /> 

  <!-- First, define a set of factors that go into the analysis

  An event is a location in space-time that meets a set of
       spatial and temporal conditions.
	   Spatial Conditions are one of:
			1) a spatial query
			2) ??

	   Temporal conditions are one of:
			1) a specific interval of time, defined by a start time and ending time
			2) a moving window defining the period over which the spatial conditions
			   must be met, defined by a start offset and an end offset (from the current time) 
			 
		A Spatiotemporal query is a query that is satisifed if both the spatial conditions are
		 satisified, and the temporal conditions are satisfied.
	
  The "distance" between two events is defined as occuring 
     in both space and time, requiring normalization factors to
	 be defined for weighing the importance of time relative to space
	


  An 'event' is defined by:
          1) a query that defines where is space it is defined
		  2) temporal condition information
		  3) an expression that returns the "value" of the factor for an IDU
		  4) the type of the data - continuous or discrete
		  5) whether absolute values or changes of values over some step should be used
    

  <event name='property damage' 
		  query="delta( PROPDAMAGE, 3 ) > 0"      <--- these two lines define a spatiotemporal query
          period_start='-1' period_end='2050' window_start_offset='0' window_end_offset='4'
		  value="PROPDAMAGE" type='continuous'    <--- these are optional
		  />
		  -->

  <!-- add more as needed -->
  
  <!-- model -->
  <model type="correlation"  />

  this model type will generate a correlation matrix between 
  each of the event types defined in the <events> section.
  The correlation value in the matrix is the spatiotemporal distance-weighted 
  frequency at which the two events were observed simultaneously.
  */

   return true;
   }
