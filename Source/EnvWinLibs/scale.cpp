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
#include "EnvWinLibs.h"

#pragma hdrstop

#include <scale.h>
#include <math.h>

#define PENALTY   (0.02)
#define round(x) (x<0?ceil((x)-0.5):floor((x)+0.5))

void bestscale(double umin, double umax, double *low, double *high, double *ticks)
   {
   if ((low == NULL) || (high == NULL) || (ticks == NULL))
      {
      return;
      }

   double ulow[9];
   double uhigh[9];
   double uticks[9];
   
   goodscales(umin,umax,ulow,uhigh,uticks);

   double udelta = umax - umin;
   double ufit[9];
   double fit[9];
   int k = 0;

   int i;
   for (i=0; i<=8; i++) 
      {
      ufit[i] = ((uhigh[i]-ulow[i])-udelta)/(uhigh[i]-ulow[i]);
      fit[i] = 2*ufit[i] + PENALTY * pow( ((uticks[i]-6.0)>1.0)?(uticks[i]-6.0):1.0 ,2.0);
      if (i > 0) 
         {
         if (fit[i] < fit[k]) 
            {
            k = i;
            }
         }
      }

   *low = ulow[k];
   *high = uhigh[k];
   *ticks = uticks[k];
   }

void goodscales(double xmin, double xmax, double low[], double high[], double ticks[])
   {
   double bestDelta[] = {0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0};

   int i;
   if (xmin == xmax)
      {
      for (i=0; i<=8; i++) 
         {
         low[i] = xmin;
         high[i] = xmax+1;
         ticks[i] = 1;
         }
      return;
      }

   double xdelta = xmax-xmin;
   double delta[9];

   for (i=0; i<=8; i++) 
      {
      delta[i] = pow(10, round(log10(xdelta)-1)) * bestDelta[i];
      high[i] = delta[i] * ceil(xmax/delta[i]);
      low[i] = delta[i] * floor(xmin/delta[i]);
      ticks[i] = round((high[i]-low[i])/delta[i]) + 1;
      }
   }


