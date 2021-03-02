      subroutine FlowInitYear
      !DEC$ ATTRIBUTES DLLEXPORT::FlowInitYear
!!    ~ ~ ~ PURPOSE ~ ~ ~
!!    this subroutine contains the loops governing the modeling of processes
!!    in the watershed 

!!    ~ ~ ~ INCOMING VARIABLES ~ ~ ~
!!    name        |units         |definition
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    biomix(:)   |none          |biological mixing efficiency.
!!                               |Mixing of soil due to activity of earthworms
!!                               |and other soil biota. Mixing is performed at
!!                               |the end of every calendar year.
!!    hi_targ(:,:,:)|(kg/ha)/(kg/ha)|harvest index target of cover defined at
!!                               |planting
!!    icr(:)      |none          |sequence number of crop grown within the
!!                               |current year
!!    idaf        |julian date   |beginning day of simulation
!!    idal        |julian date   |ending day of simulation
!!    idplt(:)    |none          |land cover code from crop.dat
!!    igro(:)     |none          |land cover status code:
!!                               |0 no land cover currently growing
!!                               |1 land cover growing
!!    iyr         |none          |beginning year of simulation
!!    mcr         |none          |max number of crops grown per year
!!    nbyr        |none          |number of years in simulation
!!    ncrops(:,:,:)|
!!    nhru        |none          |number of HRUs in watershed
!!    nro(:)      |none          |sequence number of year in rotation
!!    nrot(:)     |none          |number of years of rotation
!!    nyskip      |none          |number of years to not print output
!!    phu_plt(:)  |heat units    |total number of heat units to bring plant
!!                               |to maturity
!!    sub_lat(:)  |degrees       |latitude of HRU/subbasin
!!    tnyld(:)    |kg N/kg yield |modifier for autofertilization target
!!                               |nitrogen content for plant
!!    tnylda(:)   |kg N/kg yield |estimated/target nitrogen content of
!!                               |yield used in autofertilization
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

!!    ~ ~ ~ OUTGOING VARIABLES ~ ~ ~
!!    name        |units         |definition
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    curyr       |none          |current year in simulation (sequence)
!!    hi_targ(:,:,:)|(kg/ha)/(kg/ha)|harvest index target of cover defined at
!!                               |planting
!!    hvstiadj(:) |(kg/ha)/(kg/ha)|optimal harvest index for current time during
!!                               |growing season
!!    i           |julian date   |current day in simulation--loop counter
!!    icr(:)      |none          |sequence number of crop grown within the
!!                               |current year
!!    id1         |julian date   |first day of simulation in current year
!!    iida        |julian date   |day being simulated (current julian day)
!!    idplt(:)    |none          |land cover code from crop.dat
!!    iyr         |year          |current year of simulation (eg 1980)
!!    laimxfr(:)  |
!!    leapyr      |none          |leap year flag:
!!                               |0  leap year
!!                               |1  regular year
!!    ncrops(:,:,:)|
!!    ncut(:)     |none          |sequence number of harvest operation within
!!                               |a year
!!    ndmo(:)     |days          |cumulative number of days accrued in the
!!                               |month since the simulation began where the
!!                               |array location number is the number of the
!!                               |month
!!    nro(:)      |none          |sequence number of year in rotation
!!    ntil(:)     |none          |sequence number of tillage operation within
!!                               |current year
!!    phu_plt(:)  |heat units    |total number of heat units to bring plant
!!                               |to maturity
!!    phuacc(:)   |none          |fraction of plant heat units accumulated
!!    tnylda(:)   |kg N/kg yield |estimated/target nitrogen content of
!!                               |yield used in autofertilization
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

!!    ~ ~ ~ LOCAL DEFINITIONS ~ ~ ~
!!    name        |units         |definition
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    ic          |none          |counter
!!    idlst       |julian date   |last day of simulation in current year
!!    iix         |none          |sequence number of current year in rotation
!!    iiz         |none          |sequence number of current crop grown
!!                               |within the current year
!!    j           |none          |counter
!!    xx          |none          |current year in simulation sequence
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

!!    ~ ~ ~ SUBROUTINES/FUNCTIONS CALLED ~ ~ ~
!!    Intrinsic: Mod, real*8
!!    SWAT: sim_inityr, std3, xmon, sim_initday, clicon, command
!!    SWAT: writed, writem, tillmix

!!    ~ ~ ~ ~ ~ ~ END SPECIFICATIONS ~ ~ ~ ~ ~ ~
      use parm

      integer :: idlst, j, iix, iiz, ic, mon, ii
      real*8 :: xx
      integer :: eof
      
      eof = 0
      
  !    do curyr = 1, nbyr
  !      write (*,1234) curyr
        
        !! initialize annual variables
        call sim_inityr
       
        !! write header for watershed annual table in .std file
        call std3


        !!determine beginning and ending dates of simulation in ckjj
        if (curyr == 1 .and. idaf > 0) then
          id1 = idaf
        else
          id1 = 1
        end if

        !! set ending day of simulation for year
        idlst = 0
        if (curyr == nbyr .and. idal > 0) then
          idlst = idal
        else
          idlst = 366 - leapyr
        end if
        
        !! set current julian date to begin annual simulation
        iida = 0
        iida = id1

        call xmon
       if (ifirstatmo == 1) then
         ifirstatmo = 0
         if (iatmodep == 1) then 
           iyr_at = iyr_atmo1
           mo_at = mo_atmo1
            do
              mo_atmo = mo_atmo + 1
              if (iyr_at == iyr .and. mo_at == i_mo) exit
              mo_at = mo_at + 1
              if (mo_at > 12) then
                mo_at = 1
                iyr_at = iyr_at + 1
              endif
              if (mo_atmo > 1000) exit
            end do  
         endif
       endif
  !  end do





      
      eof = 0
        !!determine beginning and ending dates of simulation in ckjj
        if (curyr == 1 .and. idaf > 0) then
          id1 = idaf
        else
          id1 = 1
        end if

        !! set ending day of simulation for year
        idlst = 0
        if (curyr == nbyr .and. idal > 0) then
          idlst = idal
        else
          idlst = 366 - leapyr
        end if
        
                !! set current julian date to begin annual simulation
        iida = 0
        iida = id1

        call xmon
       if (ifirstatmo == 1) then
         ifirstatmo = 0
         if (iatmodep == 1) then 
           iyr_at = iyr_atmo1
           mo_at = mo_atmo1
            do
              mo_atmo = mo_atmo + 1
              if (iyr_at == iyr .and. mo_at == i_mo) exit
              mo_at = mo_at + 1
              if (mo_at > 12) then
                mo_at = 1
                iyr_at = iyr_at + 1
              endif
              if (mo_atmo > 1000) exit
            end do  
         endif
       endif

       

    curyr=curyr+1 !  added because we removed the loop through years
  
      end