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
#include "WET_Hydro.h"
#include <unitconv.h>
#include <math.h>
#include <omp.h>


void WET_Hydro::GetCanopyStorage(VEGETATION_INFO *pVegetation, MET_INFO *pMet, int story)
   {
   COleDateTime t( m_time );
   int month = t.GetMonth();
   float LAI = 5.0f;float precip=0.0f;float maxCanopyStorage=0.0f;
   float storage = pVegetation->vegetationLayerStateArray[story]->storage;
   if (story==0)
      {
      precip = pMet->precip;
      LAI = pVegetation->pParameters->pOverstoryParam->LAI[month];
      maxCanopyStorage = 10E-4f*LAI*pVegetation->pParameters->f; // in Meters
      }
   else
      {
      LAI = pVegetation->pParameters->pUnderstoryParam->LAI[month];
      precip = pVegetation->vegetationLayerStateArray[0]->throughfall;
      maxCanopyStorage = 10E-4f*LAI; // in Meters
      }   
   pVegetation->vegetationLayerStateArray[story]->storage=storage+((precip-pVegetation->vegetationLayerStateArray[story]->evaporation)*m_timeStep); //storage in m of water
   if (pVegetation->vegetationLayerStateArray[story]->storage > maxCanopyStorage)
      {
      pVegetation->vegetationLayerStateArray[story]->throughfall = (pVegetation->vegetationLayerStateArray[story]->storage-maxCanopyStorage)/m_timeStep ; //as a rate m/d
      pVegetation->vegetationLayerStateArray[story]->storage = maxCanopyStorage  ;
      }
   else
      pVegetation->vegetationLayerStateArray[story]->throughfall = 0.0f;
   }


float WET_Hydro::GetIncomingSolar(float temp)
   {
   float pi = 3.141592f;
   COleDateTime t( m_time );
   float julianDay = t.GetDayOfYear();
   m_latitude = 33.0/57.2957795;//convertToRadians
   float solarDeclination=0.4903f*sin(2.0f*pi/365.0f*julianDay-1.405f);
   //float solarDeclination = 23.45f*3.1415f/180.0f*sin(2.0f*3.1415f*((284+julianDay)/365.25));
   float sd_degrees = solarDeclination*57.2957795 ;
   float sunsetHourAngle = acos(-tan(m_latitude)*tan(solarDeclination));
   float sHA_degress = sunsetHourAngle*57.2957795 ;
   float N = 24.0f/pi*sunsetHourAngle;
   float dr = 1+0.033*cos(2*pi/365*julianDay);
   float So_mm_d=15.392*dr*(sunsetHourAngle*sin(m_latitude)*sin(solarDeclination) + cos(m_latitude)*cos(solarDeclination)*sin(sunsetHourAngle)); 
   float lamda = 2.501f - (0.00236f * temp); // Latent heat of evaporation (MJ/kg) 

   //return So_mm_d*lamda/11.574;//convert to MJ/m2/d
   return So_mm_d*2.45f;  
   }

float WET_Hydro::LongwaveRadiationFromHandbookHydrology(UPSLOPE_INFO *pUpslope)
   {
   // Estimate the long wave radiation
   // the exchange of long wave radiation between vegetation / soil and atmosphere/clouds is represented
   // by the ln equation below.
   float temp = pUpslope->pMet->temp;
   float so = GetIncomingSolar(temp); // MJ/day, extraterrestrial radiation
   float as = 0.25f; // fracton of extraterrestrial radiation S0 on overcast days (set n=0)
   float bs = 0.50f; //fraction of extraterrestrial radiation S0 on clear days
   float n = 12.0f;  // number of bright sunshine hours per day
   float N = 12.0f;  // number of hours of daylight
   float st =  (as + bs * (n/N)) * so; // Total incoming Short Wave Radiation
   st = pUpslope->pMet->solarRad; // MJ/m2/day
   float ac = 1.0f;  //correlation coefficient ranges from 0.34 to 0.44 (1.0 for humid areas)
   float bc = 0.0f; //correlation coefficient ranges from -0.14 to -0.25 (0 for humid areas)
   float sto = (as + bs) * so; //solar radiation for clear skies (st with n/N = 1)
   float f = (ac * (st / sto)) + bc;  // cloudiness factor
   if (f < 0.0f)
      f=0.0f;
   //float epsilon = (float)pow((-0.02f + 0.261f), (-0.00-0777f * temp * temp));  //net emmissivity between atmosphere and ground
   //float epsilon = -0.02f+0.261f*exp(-0.0000777f*temp*temp);
   float ae=0.34f;
   float be=-0.14f;
   float epsilon=ae+be*sqrt(pUpslope->pMet->vaporPressure);  //Shuttleworth, 1993 4.2.8
   float ph = 4.903E-9f; //  Stefan - Boltzmann Constant MJ/m2 K-4 day
   float Rl = -f * epsilon * ph * pow((temp + 273.2f),4) ; //exchange of long wave radiation between ground and air
   return Rl;//MJ/m2/d
   }

float WET_Hydro::IncomingLongwavePomeroy(MET_INFO *pMet)
   {
   // Estimate the long wave radiation
   float so = GetIncomingSolar(pMet->temp); // MJ/m2/day, extraterrestrial radiation
   float as = 0.25f; // fracton of extraterrestrial radiation S0 on overcast days (set n=0)
   float bs = 0.50f; //fraction of extraterrestrial radiation S0 on clear days
   float n = 12.0f;  // number of bright sunshine hours per day
   float N = 12.0f;  // number of hours of daylight
   float st =  (as + bs * (n/N)) * so; // Total incoming Short Wave Radiation
   st = pMet->solarRad; // MJ/m2/day
   float ac = 1.0f;  //correlation coefficient ranges from 0.34 to 0.44 (1.0 for humid areas)
   float bc = 0.0f; //correlation coefficient ranges from -0.14 to -0.25 (0 for humid areas)
   float sto = (as + bs) * so; //solar radiation for clear skies (st with n/N = 1)
   float f = (ac * (st / sto)) + bc;  // cloudiness factor
   if (f < 0.0f)
      f=0.0f;
   float ph = 5.67E-8f; //  Stefan - Boltzmann Constant W/m2/d
   float temp = pMet->temp+273.15f;
   float epsilon = 1.24*pow((pMet->vaporPressure*10.0f/temp),1.0f/7.0f);//from sicart and pomeroy 2006 taken from brutsaert 1975, with vp converted to mbars (10 mbar per 1 kPa)
   float Rl = ph * pow(temp,4)*f*epsilon ; //exchange of long wave radiation between ground and air in w/m2
   return Rl/11.574f; //convert w/m2 to MJ/m2/d
   }

void WET_Hydro::GetPenmanMonteithET(UPSLOPE_INFO *pUpslope, MET_INFO *pMet, VEGETATION_INFO *pVegetation, int story)
   {
   //DATA/////////////OR ESTIMATES OF IT//////////////////
   float temp = pMet->temp;
   float dewPoint = pMet->dewPoint;
   float pressure = pMet->vaporPressure;
   float rSdown = pMet->solarRad;
   float rLdown = IncomingLongwavePomeroy(pMet);//downward sky longwave radiation    
  // float Rl = EstimateRl(pMet);
   float RH = 100.0f-5.0f*(temp-dewPoint);  //Relative Humidity
   float es =  0.6108f*exp(17.27f*temp/(237.3f+temp));  //saturated vapor pressure in kPa  //shuttleworth 4.2.2
   float ea = (RH/100.0f) * es; // vapor pressure estimated from RH
   float vpd = es - pMet->vaporPressure;  //  vapor pressure deficit (Kpa)
   if (vpd < 0.0f)
      vpd=0.0f;
   //DATA/////////////OR ESTIMATES OF IT//////////////////

   COleDateTime t( m_time );
   int month = t.GetMonth()-1;

   float F = pVegetation->pParameters->f;//fraction of area under canopy
   float Rp=pMet->solarRad*0.5f ;//visible radiation
   float Rpc=0.0f;float rsMin=0.0f;float rsMax=0.0f;float LAI=2.0f;float height=1.0f;
   float ko=0.05f ;
   float k1=0.02f ;
   float tau_overstory =  exp(-ko*pVegetation->pParameters->pOverstoryParam->LAI[month]);//canopy attenuation coefficient for LAI of 3. Similar to as suggested in Wigmosta, 1994
   float tau_understory = exp(-k1*pVegetation->pParameters->pUnderstoryParam->LAI[month]);
   float albedo_overstory = pVegetation->pParameters->pOverstoryParam->albedo[month];// reflectance coefficient (albedo)
   float albedo_understory = pVegetation->pParameters->pUnderstoryParam->albedo[month];

   if (story==0)
      {
      Rpc=pVegetation->pParameters->pOverstoryParam->rpc;// light level where rs = 2rsmin - taken from example vegetation file from DHSVM website..
      rsMin=pVegetation->pParameters->pOverstoryParam->minResistance;  //species dependent minimum resistance 
      rsMax = pVegetation->pParameters->pOverstoryParam->maxResistance; //maximum cuticular resistance 
      LAI = pVegetation->pParameters->pOverstoryParam->LAI[month];
      height = pVegetation->pParameters->pOverstoryParam->height;
    
      }
   else
      {
      Rpc=pVegetation->pParameters->pUnderstoryParam->rpc;// light level where rs = 2rsmin - taken from example vegetation file from DHSVM website..
      rsMin=pVegetation->pParameters->pUnderstoryParam->minResistance;  //species dependent minimum resistance 
      rsMax = pVegetation->pParameters->pUnderstoryParam->maxResistance; //maximum cuticular resistance 
      LAI = pVegetation->pParameters->pUnderstoryParam->LAI[month];
      height = pVegetation->pParameters->pUnderstoryParam->height;
      }
   float sigma = 4.903E-9f; //  Stefan - Boltzmann Constant MJ/m2 K-4 day
   /* Local heat of vaporization, Eq. 4.2.1, Shuttleworth (1993) */ 
   float lamda = 2.5010f - (0.002361f * temp); // Latent heat of evaporation (MJ/kg) 
   
//Aerodynamic Resistance
   float ra=1.0f;//aerodynamic resistance

//Canopy Resistance
  
   float DHSVM_HUGE = 1E20f;
   
   float c=1.0f;//ratio of total to projected LAI

   float f1=0.0f;

   f1=1.0f/(0.176f + 0.77f *temp - 0.0018f*temp*temp); //taken from DHSVM v3.0
   if (f1 <= 0)
      f1 = DHSVM_HUGE;
   float vpdThresh = 4.0f;//VPD causing stomatal closure, in Kpa
   float f2=0.0f;
   if (vpd >= vpdThresh)
      f2=DHSVM_HUGE;
   else
      f2 = 1/(1-vpd/vpdThresh);

   float f3 = 1/((rsMin/rsMax + Rp/Rpc)/(1+Rp/Rpc));
   float f4 = 1.0f;
   if (pUpslope->pUnsat->unsaturatedDepth < 0.3f )  //If the unsaturated depth is shallow, don't limit transpiration (even if the unsaturated zone is dry)
      f4 = 1.0f;
   else
      {

      if (pUpslope->pUnsat->swcUnsat < pUpslope->pHydroParam->wiltPt->value)
         f4 = DHSVM_HUGE;
      else if (pUpslope->pUnsat->swcUnsat < pUpslope->pHydroParam->fieldCapac->value)
         f4 = 1/(pUpslope->pUnsat->swcUnsat-pUpslope->pHydroParam->wiltPt->value)/(pUpslope->pHydroParam->fieldCapac->value-pUpslope->pHydroParam->wiltPt->value);
      else
         f4 = 1.0f;
      }
   float rs = 0;
   if (f1 == DHSVM_HUGE || f2 == DHSVM_HUGE || f3 == DHSVM_HUGE  || f4 == DHSVM_HUGE )
      rs = DHSVM_HUGE;
   else
      rs=rsMin*f1*f2*f3*f4; //species dependent limiting factors (air temp, vpd, PAR, and water content)
   if (rs > rsMax)
      rs=rsMax;
  // if (rs < rsMin)
   //   rs=rsMin;

   float rc=rs/(c*LAI);//canopy resistance

///Aerodynamic Resistance

   float d=2.0f/3.0f*height;//0 plane discplacement height
   float zom = 0.123f*height; // roughness length governing momentum transfer
   float zoh = 0.1f*zom;//roughness length governing transfer of head and vapor
   float zm = 2.0f+height;//height of wind measurement
   float zh = 2.0f+height;//height of humidity measurement
   ra = 1.0f;
   if (pUpslope->pMet->windSpeed > 0 && zm>d)
      ra = log((zm - d)/zom)*log((zh-d)/zom)/0.41f*0.41f*pUpslope->pMet->windSpeed;
   else
      ra = 1.0f;


   //log((2. + Z0_Lower) / Z0_Lower) * log((Zref - d_Lower) / Z0_Lower) / K2;
///Radiation absorbed by Vegetation - should be different for over and understories


   float RsAbsorbed = 0.0f;
   switch ( story )
		{
		case 0:  //overstory
         RsAbsorbed = rSdown*((1.0f-albedo_overstory)-tau_overstory*(1.0f-albedo_understory))*F; //shortwave absorbed by overstory;
		break;
		case 1:  //understory 
         RsAbsorbed = rSdown*((1.0f-albedo_understory)-tau_understory*(1.0f-pUpslope->pHydroParam->albedo->value))*(tau_overstory*F+(1.0f-F));
		break;
		}

//shortwave Radiation absorbed by Soil
   float Rsg = rSdown*tau_understory*(1.0f-0.3)*((1.0f-F)+tau_overstory*F);

//Net overstory Longwave
   float value = pow((temp+273.0f),4);
   float Lu = sigma*value; //air temp should be equal except when snow is on the ground
   float Lo = sigma*value; // //Lu and Lo are longwave out of this canopy layer
   float Lg = Lu; //except when snow is on the ground!!
   float longIn = 0.0f; float longOut=0.0f;
   float RlAbsorbed=0.0f;
   switch ( story )
		{
		case 0:  //overstory
         RlAbsorbed = ((rLdown+Lu)-(2.0f*Lo))*F; //longwave exchange of overstory
        // RlAbsorbed = (Rl + Lu)*F;
        // RlAbsorbed = Rl*F;//a simpfication because we estimated NET Longwave beween Vegetation/Soil or Atmosphere/clouds from Shuttleworth 4.2.7
         //longIn = (Rl+longOutUnder)*F;
        // longOut=longOutOver;
		break;
		case 1:  //understory 
          RlAbsorbed = ((Lo+Lg-2.0f*Lu)*F)+((rLdown+Lg-2.0f*Lu)*(1.0f-F)); //net understory longwave
        // RlAbsorbed = Rl * (1-F) + Lo * F; 
         //RlAbsorbed = Rl*(1-F);
        // longIn = Rl*(1-F)+longOutOver*F;
         //longOut=longOutUnder;
		break;
		}

///total radiation absorbed is sum of long and shortwave
   float Rn = RsAbsorbed + RlAbsorbed;  //this is total net radiation for this particular canopy element
   
   //Evaporation from dry/wet surfaces 
   float atmosphericPressure =  101.325f; ////kPa
   float rho = 3.486f*atmosphericPressure/(275.0f + temp); // density of air  C (kg/m3) Shuttleworth 4.2.4.  
   float cp = 1.0131f; // specific heat of moist air ( kJ/kg degree C)
   //float gamma = 0.0016286f*101.35f/lamda; //psychrometric constant - Standard atmospheric pressure of 1 Atm or 101.35kPa units of kPa/C
   float gamma = cp*atmosphericPressure/(0.622*lamda)/1000.0f;
   float delta = (4098.0f*es)/((237.3f+temp)*(237.3f+temp));  //gradient of saturated vapor pressure in kPa C-1 Shuttleworth 4.2.3

   float Ep = 0.0f;
   
   if (lamda*(delta+gamma) > 0.0f)
      Ep = (delta*Rn + rho*cp*vpd/ra)/(lamda*(delta+gamma));//mm/d
  
   if (Ep < 0.0f)
      Ep = 0.0f;

   //Evap and transpiration calculated independently.  First intercepted water is evaporated at the potential rate
   float evap= min(Ep,pVegetation->vegetationLayerStateArray[story]->storage*1000.0f/m_timeStep);// mm/d
   // transpiration from dry vegetation
   float transp = 0.0f;
   
   if (delta + gamma*(1.0f+rc/ra) > 0.0f)
      transp = (Ep-evap)*(delta + gamma)/(delta + gamma*(1.0f+rc/ra));//mm/d

   float evapTest=1/lamda*((delta*Rn + rho*cp*vpd/ra)/(delta+ gamma*(1.0f+rc/ra)));

   pVegetation->vegetationLayerStateArray[story]->incomingShortwave = rSdown;
   pVegetation->vegetationLayerStateArray[story]->incomingLongwave = rLdown;
   pVegetation->vegetationLayerStateArray[story]->netLongwave = RlAbsorbed;
   pVegetation->vegetationLayerStateArray[story]->netShortwave = RsAbsorbed;
   pVegetation->vegetationLayerStateArray[story]->evaporation = evap/1000.0f;      //convert back to m/d
   pVegetation->vegetationLayerStateArray[story]->transpiration = transp/1000.0f;  //convert back to m/d
   pVegetation->vegetationLayerStateArray[story]->pET = Ep/1000.0f;

   if (story==1)//HACK - soil evap should be calculated outside of this loop....
      {
      //desorption - rate at which soil can supply water to surface for evaporation
      float RnetSoil = Rsg ;//if there is snow, the assmption that Tsoil = Tair is pretty bad...
      
      float m=pUpslope->pHydroParam->poreSizeIndex->value;
      float S1=sqrt(8.0f*pUpslope->pHydroParam->phi->value*pUpslope->pHydroParam->kSat->value*pUpslope->pHydroParam->bubbling->value/3.0f*(1.0f+3.0f*m)*(1.0f*4.0f*m));
      float s2=pow((pUpslope->pUnsat->swcUnsat/pUpslope->pHydroParam->phi->value),(1.0f/2.0f*m)+2.0f);
      float Se=S1*s2;
      float Fe=Se*sqrt(m_timeStep)*1000.0f;//convert to mm/d
      // soil evaporation
      float Es = (delta*RnetSoil + rho*cp*vpd/ra)/(lamda*(delta+gamma));
      if (Es < 0.0f)
         Es=0.0f;
      float soilEvap = min(Es,Fe);
      pVegetation->pParameters->soilEvap = soilEvap/1000.0f;  //convert back to m/d
      }
   }
 

/*****************************************************************************
  Function name: CalcAerodynamic()

  Purpose      : Calculate the aerodynamic resistance for each vegetation 
                 layer, and the wind 2m above the layer boundary.  In case of 
                 an overstory, also calculate the wind in the overstory.
                 The values are normalized based on a reference height wind 
                 speed, Uref, of 1 m/s.  To get wind speeds and aerodynamic 
                 resistances for other values of Uref, you need to multiply 
                 the here calculated wind speeds by Uref and divide the 
                 here calculated aerodynamic resistances by Uref
                 
  Required     :
    int NVegLayers - Number of vegetation layers
    char OverStory - flag for presence of overstory.  Only used if NVegLayers 
                     is equal to 1
    float Zref     - Reference height for windspeed
    float n        - Attenuation coefficient for wind in the overstory
    float *Height  - Height of the vegetation layers (top layer first)
    float Trunk    - Multiplier for Height[0] that indictaes the top of the 
                     trunk space
    float *U       - Vector of length 2, with wind for vegetation layers
                     If OverStory == TRUE the first value is the wind in
                     the overstory canopy, and the second value the wind 
                     2m above the lower boundary.  Otherwise the first 
                     value is the wind 2m above the lower boundary and 
                     the second value is not used.
    float *U2mSnow - Wind velocity 2m above the snow surface
    float *Ra      - Vector of length 2, with aerodynamic resistance values.  
                     If OverStory == TRUE the first value is the aerodynamic 
                     resistance for the the overstory canopy, and the second 
                     value the aerodynamic resistance for the lower boundary.  
                     Otherwise the first value is the aerodynamic resistance
                     for the lower boundary and the second value is not used.
    float *RaSnow  - Aerodynamic resistance for the snow surface.

  Returns      : void

  Modifies     :
    float *U
    float *U2mSnow
    float *Ra     
    float *RaSnow
   
  Comments     :
*****************************************************************************/
   /*
void WET_Hydro::CalcAerodynamic(int NVegLayers, unsigned char OverStory,
		     float n, float *Height, float Trunk, float *U,
		     float *U2mSnow, float *Ra, float *RaSnow)
   {
   float d_Lower;
   float d_Upper;
   float K2;
   float Uh;
   float Ut;
   float Uw;
   float Z0_Lower;
   float Z0_Upper;
   float Zt;
   float Zw;
 
   K2 = VON_KARMAN * VON_KARMAN;

   // No OverStory, thus maximum one soil layer 
  
   if (OverStory == FALSE)
      {
      if (NVegLayers == 0)
         {
         Z0_Lower = Z0_GROUND;
         d_Lower = 0;
         }
      else 
         {
         Z0_Lower = Z0_MULTIPLIER * Height[0];
         d_Lower = D0_MULTIPLIER * Height[0];
         }

       // No snow 
      U[0] = log((2. + Z0_Lower) / Z0_Lower) / log((Zref - d_Lower) / Z0_Lower);
      Ra[0] = log((2. + Z0_Lower) / Z0_Lower) * log((Zref - d_Lower) / Z0_Lower) / K2;

       // Snow 
      *U2mSnow = log((2. + Z0_SNOW) / Z0_SNOW) / log(Zref / Z0_SNOW);
      *RaSnow = log((2. + Z0_SNOW) / Z0_SNOW) * log(Zref / Z0_SNOW) / K2;
      }

  // Overstory present, one or two vegetation layers possible 
  else
     {
     Z0_Upper = Z0_MULTIPLIER * Height[0];
     d_Upper = D0_MULTIPLIER * Height[0];

     if (NVegLayers == 1) 
        {
        Z0_Lower = Z0_GROUND;
        d_Lower = 0;
        }
     else 
        {
        Z0_Lower = Z0_MULTIPLIER * Height[1];
        d_Lower = D0_MULTIPLIER * Height[1];
        }

    Zw = 1.5 * Height[0] - 0.5 * d_Upper;
    Zt = Trunk * Height[0];
    if (Zt < (Z0_Lower + d_Lower))
      ReportError("Trunk space height below \"center\" of lower boundary", 48);

    // Resistance for overstory 
    Ra[0] = log((Zref - d_Upper) / Z0_Upper) / K2 *
      (Height[0] / (n * (Zw - d_Upper)) *
       (exp(n * (1 - (d_Upper + Z0_Upper) / Height[0])) - 1) + (Zw -
								Height[0]) /
       (Zw - d_Upper) + log((Zref - d_Upper) / (Zw - d_Upper)));

    // Wind at different levels in the profile 
    Uw = log((Zw - d_Upper) / Z0_Upper) / log((Zref - d_Upper) / Z0_Upper);
    Uh =
      Uw - (1 -
	    (Height[0] - d_Upper) / (Zw - d_Upper)) / log((Zref -
							   d_Upper) / Z0_Upper);
    U[0] = Uh * exp(n * ((Z0_Upper + d_Upper) / Height[0] - 1.));
    Ut = Uh * exp(n * (Zt / Height[0] - 1.));

    // resistance at the lower boundary 

    // No snow 
    // case 1: the wind profile to a height of 2m above the lower boundary is 
    //   entirely logarithmic 
    if (Zt > (2. + Z0_Lower + d_Lower)) {
      U[1] =
	Ut * log((2. + Z0_Lower) / Z0_Lower) / log((Zt - d_Lower) / Z0_Lower);
      Ra[1] =
	log((2. + Z0_Lower) / Z0_Lower) * log((Zt -
					       d_Lower) / Z0_Lower) / (K2 * Ut);
    }

    // case 2: the wind profile to a height of 2m above the lower boundary 
    //   is part logarithmic and part exponential, but the top of the overstory 
    //   is more than 2 m above the lower boundary 
    else if (Height[0] > (2. + Z0_Lower + d_Lower)) {
      U[1] = Uh * exp(n * ((2. + Z0_Lower + d_Lower) / Height[0] - 1.));
      Ra[1] =
	log((Zt - d_Lower) / Z0_Lower) * log((Zt -
					      d_Lower) / Z0_Lower) / (K2 *
								      Ut) +
	Height[0] * log((Zref - d_Upper) / Z0_Upper) / (n * K2 *
							(Zw -
							 d_Upper)) * (exp(n *
									  (1 -
									   Zt
									   /
									   Height
									   [0]))
								      -
								      exp(n *
									  (1 -
									   (Z0_Lower
									    +
									    d_Lower
									    +
									    2.)
									   /
									   Height
									   [0])));
    }

    // case 3: the top of the overstory is less than 2 m above the lower 
     //  boundary.  The wind profile above the lower boundary is part logarithmic
    //   and part exponential, but only extends to the top of the overstory 
    else {
      U[1] = Uh;
      Ra[1] =
	log((Zt - d_Lower) / Z0_Lower) * log((Zt -
					      d_Lower) / Z0_Lower) / (K2 *
								      Ut) +
	Height[0] * log((Zref - d_Upper) / Z0_Upper) / (n * K2 *
							(Zw -
							 d_Upper)) * (exp(n *
									  (1 -
									   Zt
									   /
									   Height
									   [0]))
								      - 1);
      fprintf(stderr,
	      "WARNING:  Top of overstory is less than 2 meters above the lower boundary\n");
    }

    // Snow 
    // case 1: the wind profile to a height of 2m above the lower boundary is 
     //  entirely logarithmic 
    if (Zt > (2. + Z0_SNOW)) {
      *U2mSnow = Ut * log((2. + Z0_SNOW) / Z0_SNOW) / log(Zt / Z0_SNOW);
      *RaSnow = log((2. + Z0_SNOW) / Z0_SNOW) * log(Zt / Z0_SNOW) / (K2 * Ut);
    }

    // case 2: the wind profile to a height of 2m above the lower boundary 
    //   is part logarithmic and part exponential, but the top of the overstory 
    //   is more than 2 m above the lower boundary 
    else if (Height[0] > (2. + Z0_SNOW)) {
      *U2mSnow = Uh * exp(n * ((2. + Z0_SNOW) / Height[0] - 1.));
      *RaSnow = log(Zt / Z0_SNOW) * log(Zt / Z0_SNOW) /
	(K2 * Ut) +
	Height[0] * log((Zref - d_Upper) / Z0_Upper) / (n * K2 *
							(Zw -
							 d_Upper)) * (exp(n *
									  (1 -
									   Zt
									   /
									   Height
									   [0]))
								      -
								      exp(n *
									  (1 -
									   (Z0_SNOW
									    +
									    2.)
									   /
									   Height
									   [0])));
    }

    // case 3: the top of the overstory is less than 2 m above the lower boundary.
    //   The wind profile above the lower boundary is part logarithmic and part 
    //   exponential, but only extends to the top of the overstory 
    else {
      *U2mSnow = Uh;
      *RaSnow = log(Zt / Z0_SNOW) * log(Zt / Z0_SNOW) /
	(K2 * Ut) +
	Height[0] * log((Zref - d_Upper) / Z0_Upper) / (n * K2 *
							(Zw -
							 d_Upper)) * (exp(n *
									  (1 -
									   Zt
									   /
									   Height
									   [0]))
								      - 1);
      fprintf(stderr,
	      "WARNING:  Top of overstory is less than 2 meters above the lower boundary\n");
    }
  }
}

*/

   float WET_Hydro::EstimateRl(MET_INFO *pMet)
   {
   float temp = pMet->temp;
   //float lamda = 2.501f - (0.00236f * temp); // Latent heat of evaporation (MJ/kg) 
   // Estimate the NET Incoming Long wave radiation
   // The total radiation is essentially an atmospheric property, and is reduced by cloud cover 
   // and by reflection of radiation by surface roughness (albedo)
   float pi = 3.141592f;
   float LC = -0.443f;

   COleDateTime t( m_time );
   int hour = t.GetHour();
   int doy = t.GetDayOfYear();


   float declin = -23.4f / 57.3f * cos(360.0f / 57.3f * (doy+10.0f) / 365.0f);
   float Latit = 33.23f / 57.3f;

   float p = 279.575f + 0.986f * doy * pi / 180.0f;
   float EqoT = ((-104.7f * sin(p) + 596.2f * sin(2.0f * p) + 4.3f * sin(3.0f * p) - 12.7f * sin(4.0f * p)) - 429.3f * cos(p) - 2.0f * cos(2.0f * p) + 19.3f * cos(3.0f * p)) / 3600;
   float t0 = 12 - LC - EqoT;

   float pphi = (sin(Latit) * sin(declin) + cos(Latit) * cos(declin) * cos(0.262f * (hour - 0.5f - t0)));

   float phi = 0.0f;
   if (-pphi*pphi+1 > 0)
      phi = atan(pphi / sqrt(-pphi * pphi + 1));
   if (phi < 0) 
      phi = 0;

   float MaxSolar = 1020 * sin(phi); //convert watt/m2 to MJ/m2/d
   float TTrans = 0.0f;
   if (MaxSolar > 0)
      TTrans = pMet->solarRad*11.574f / MaxSolar;

   float Clouds = 2.33 - 3.33 * TTrans;
   if (Clouds <= 0)
      Clouds = 0;
   if (Clouds > 1) 
      Clouds = 1;

   float Emiss = (1 - 0.84 * Clouds) * (0.72 + 0.005 * temp) + 0.84 * Clouds;
   float Rl = (Emiss - 0.97) * 0.0000000567f * pow((temp + 273.15), 4);
   return Rl/11.574;///MJ/m2/d
   }

float WET_Hydro::EstimateRs()
   {
   float so = 118.104f; // MJ/day, extraterrestrial radiation
   float as = 0.25f; // fracton of extraterrestrial radiation S0 on overcast days (set n=0)
   float bs = 0.50f; //fraction of extraterrestrial radiation S0 on clear days
   float n = 12.0f;  // number of bright sunshine hours per day
   float N = 12.0f;  // number of hours of daylight
   float Rs =  (as + bs * (n/N)) * so; // Total incoming Short Wave Radiation MJ/m2/d
   return Rs; //MJ/m2/d 
   }

/*****************************************************************************
  Function name: RadiationBalance()

  Purpose      : Calculate the radiation balance for the individual canopy 
                 layers

  Required     :
    int HeatFluxOption - TRUE if surface temperature is being calculated
    float Rs           - Incoming shortwave radiation (W/m2)
    float Ld           - Incoming longwave radiation (W/m2)
    float Tair         - Ambient air temperature (C)
    float Tcanopy      - Canopy temperature from the previous timestep (C)
    float Tsoil        - soil surface temperature from the previous timestep
                         (C)
    int NAct           - Number of vegetation layers above the snow
    VEGTABLE VType     - Information about number of veg layers
    SNOWPIX LocalSnow  - Information about snow conditions at current pixel
    PIXRAD *LocalRad   - Components of radiation balance for current pixel
    
  Returns      : void
  
  Reference:
    Wigmosta, M. S., L. W. Vail, and D. P. Lettenmaier, A distributed 
    hydrology-vegetation model for complex terrain, Water Resour. Res.,
    30(6), 1665-1679, 1994.

  Reference:
    Nijssen and Lettenmaier, A simplified approach for predicting shortwave
    radiation transfer through boreal forest canopies, JGR, 1999.

*****************************************************************************/
void WET_Hydro::DHSVM_RadiationBalance()
   {

   }