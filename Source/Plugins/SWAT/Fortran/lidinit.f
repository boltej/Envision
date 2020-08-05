      subroutine lidinit
    
!!    ~ ~ ~ PURPOSE ~ ~ ~
!!    This subroutine sets default values for LID parameters

!!    ~ ~ ~ INCOMING VARIABLES ~ ~ ~ ~ ~ ~ ~
!!    name             |units         |definition
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    i                |none          |subbasin number
!!    msub             |none          |the number of subbasins
!!    mudb             |none          |the number of urban land uses
!!    ~ ~ ~ OUTGOING VARIABLES ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    name             |units         |definition
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    gr_farea         |none          |fractional area of a green roof to the HRU
!!    gr_sol_k         |mm/hr         |saturated hydraulic conductivity of 
!!                                    |the amended soil layer
!!    gr_sol_por       |none          |total porosity of the amended soil layer
!!                                    |expressed as a fraction of the total volume
!!    gr_sol_fc        |mm/mm H2O     |amount of water held in the amended soil
!!                                    |profile at the field capacity
!!    gr_sol_wp        |mm/mm H2O     |amount of water held in the amended soil
!!                                    |profile at the wilting point
!!    gr_sol_sw(:)     |mm/mm H2O     |amount of water stored in the amended soil profile
!!    gr_sol_sw_last   |mm/mm H2O     |soil water content of the amended soil layer at the last time step in a day
!!    vgcl             |none          |van Genuchten equation's coefficient, l
!!    vgcm             |none          |van Genuchten equation's coefficient, m
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    ~ ~ ~ LOCAL DEFINITIONS ~ ~ ~
!!    name             |units         |definition
!!    ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
!!    kk               |none          |end of file flag


      use parm
      implicit none

      integer :: kk
!      real*8 :: 
    
!!    Common Variables
!!    van Genuchten equation's coefficients
      lid_vgcl = 0.50
      lid_vgcm = 0.50

      lid_qsurf = 0.
      lid_qsurf_total = 0.
      lid_farea_sum = 0.
      lid_cuminf_last = 0.
      lid_sw_last = 0.30
      interval_last = 0.
      lid_f_last = 0.
      lid_cumr_last = 0.
      lid_str_last = 0.
      lid_cumqperc_last = 0.
      lid_cumirr_last = 0.
      lid_sw_add = 0.
     
!!    Green Roof
      do kk = 1, mudb
        if (gr_onoff(i,kk)==1) then
          if (gr_imo(i,kk)<=0)         gr_imo(i,kk) = 1
          if (gr_iyr(i,kk)<=1000)      gr_iyr(i,kk) = iyr
          if (gr_farea(i,kk)<=0)       gr_farea(i,kk) = 0.1
          if (gr_solop(i,kk)==0) then
      ! taking the chracteristics of the native HRU soil
!          else
            if (gr_fc(i,kk)<=0)         gr_fc(i,kk) = 0.40
            if (gr_wp(i,kk)<=0)         gr_wp(i,kk) = 0.15
            if (gr_ksat(i,kk)<=0)       gr_ksat(i,kk) = 0.10
            if (gr_por(i,kk)<=0)        gr_por(i,kk) = 0.50
            if (gr_hydeff(i,kk)<=0)     gr_hydeff(i,kk) = 0.70
            if (gr_soldpt(i,kk)<=0)     gr_soldpt(i,kk) = 0.50
!            if (gr_soldpt(i,kk)<=0)     gr_soldpt(i,kk) = 0.25
          end if
		  if (gr_etcoef(i,kk)<=0)       gr_etcoef(i,kk) = 0.6
        end if
      end do
      
!!    Rain Garden
      do kk = 1, mudb
        if (rg_onoff(i,kk)==1) then
          if (rg_imo(i,kk)<=0)          rg_imo(i,kk) = 1
          if (rg_iyr(i,kk)<=1000)       rg_iyr(i,kk) = iyr
          if (rg_farea(i,kk) <=0)       rg_farea(i,kk) = 0.1
          if (rg_solop(i,kk)==0) then
      ! taking the chracteristics of the native HRU soil
          else
            if (rg_fc(i,kk)<=0)         rg_fc(i,kk) = 0.40
            if (rg_wp(i,kk)<=0)         rg_wp(i,kk) = 0.15
            if (rg_ksat(i,kk)<=0)       rg_ksat(i,kk) = 7.5
            if (rg_por(i,kk)<=0)        rg_por(i,kk) = 0.50
            if (rg_hydeff(i,kk)<=0)     rg_hydeff(i,kk) = 0.10
            if (rg_soldpt(i,kk) <=0)    rg_soldpt(i,kk) = 0.25
          end if
		  if (rg_etcoef(i,kk)<=0)       rg_etcoef(i,kk) = 0.6
          if (rg_dimop(i,kk)==0) then
      ! setting the defailt orifice
!          else
            if (rg_vol(i,kk)<=0)        rg_vol(i,kk) = rg_farea(i,kk)
     & * 0.1 ! assuming the depth of 0.1 m
            if (rg_sarea(i,kk)<=0)      rg_sarea(i,kk) = 0.1
            if (rg_sth(i,kk)<=0)        rg_sth(i,kk) = 0.1
            if (rg_sdia(i,kk)<=0)       rg_sdia(i,kk) = 0.1
            if (rg_bdia(i,kk)<=0)       rg_bdia(i,kk) = 0.1
            if (rg_sts(i,kk)<=0)        rg_sts(i,kk) = 0.1
          end if
          if (rg_orifice(i,kk)==1) then
            if (rg_oheight(i,kk)<=0)    rg_oheight(i,kk) = 0.05
            if (rg_odia(i,kk)<=0)       rg_odia(i,kk) = 0.05
          end if
        end If
      end do
      
!!    CiStern
      do kk = 1, mudb
        if (cs_onoff(i,kk)==1) then
          if (cs_imo(i,kk)<=0)          cs_imo(i,kk) = 1
          if (cs_iyr(i,kk)<=1000)       cs_iyr(i,kk) = iyr
          if (cs_farea(i,kk) <=0)       cs_farea(i,kk) = 0.1
          if (cs_vol(i,kk) <=0) then
              if (cs_rdepth(i,kk) <=0) then
                  cs_vol(i,kk) = 5
              end if
          end if
        end if
      end do

!!    Poropus paVement
      do kk = 1, mudb
        if (pv_onoff(i,kk)==1) then
          if (pv_imo(i,kk)<=0)          pv_imo(i,kk) = 1
          if (pv_iyr(i,kk)<=1000)       pv_iyr(i,kk) = iyr
          if (pv_grvdep(i,kk)<=0)       pv_grvdep(i,kk) = 130
          if (pv_grvpor(i,kk)<=0)       pv_grvpor(i,kk) = 0.35
          if (pv_farea(i,kk) <=0)       pv_farea(i,kk) = 0.1
          if (pv_drcoef(i,kk) <=0)      pv_drcoef(i,kk) = 0.6
          if (pv_solop(i,kk)==0) then
      ! taking the chracteristics of the native HRU soil
          else
            if (pv_fc(i,kk)<=0)         pv_fc(i,kk) = 0.40
            if (pv_wp(i,kk)<=0)         pv_wp(i,kk) = 0.15
            if (pv_ksat(i,kk)<=0)       pv_ksat(i,kk) = 7.5
            if (pv_por(i,kk)<=0)        pv_por(i,kk) = 0.50
            if (pv_hydeff(i,kk)<=0)     pv_hydeff(i,kk) = 0.10
            if (pv_soldpt(i,kk) <=0)    pv_soldpt(i,kk) = 0.25
          end if
        end if
      end do
        
!!    Combine indices for the lids
      do kk = 1, mudb
        if (gr_onoff(i,kk)==1 .or. rg_onoff(i,kk)==1 .or.
     & cs_onoff(i,kk)==1 .or. pv_onoff(i,kk)==1) then
          lid_onoff(i,kk)=1
       end if
      end do
            
      end subroutine