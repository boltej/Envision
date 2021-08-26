!  SWAT_DLL.f90 
!
!  FUNCTIONS/SUBROUTINES exported from SWAT_DLL.dll:
!  SWAT_DLL - subroutine 
!
 
subroutine SWAT_DLL

  ! Expose subroutine Test_DLL to users of this DLL
  !
  !DEC$ ATTRIBUTES DLLEXPORT::SWAT_DLL

  use parm
  ! Variables
  LOGICAL(4) status
 ! Body of Test_DLL
!      implicit none
      
      prog = "SWAT Sep 7    VER 2018/Rev 670"
!      write (*,1000)
! 1000 format(1x,"               SWAT2018               ",/,             
!     &          "               Rev. 670               ",/,             
!     &          "      Soil & Water Assessment Tool    ",/,             
!     &          "               PC Version             ",/,             
!     &          " Program reading from file.cio . . . executing",/)

!! process input

      status = CHANGEDIRQQ('C:\Envision\StudyAreas\CalFEWS\Swat\Updated_files_SWAT_Tular')
      call getallo
      call allocate_parms
      call readfile
      call readbsn
      call readwwq
      if (fcstyr > 0 .and. fcstday > 0) call readfcst
      call readplant             !! read in the landuse/landcover database
      call readtill              !! read in the tillage database
      call readpest              !! read in the pesticide database
      call readfert              !! read in the fertilizer/nutrient database
      call readurban             !! read in the urban land types database
      call readseptwq            !! read in the septic types database
      call readlup
      call readfig
      call readatmodep
      call readinpt
      call std1
      call std2
      call openwth
      call headout

      !! convert integer to string for output.mgt file
      subnum = ""
      hruno = ""
      do i = 1, mhru
        write (subnum(i),fmt=' (i5.5)') hru_sub(i)
        write (hruno(i),fmt=' (i4.4)') hru_seq(i)  
      end do

      if (isproj == 2) then 
        hi_targ = 0.0
      end if

!! save initial values
      if (isproj == 1) then
        scenario = 2
        call storeinitial
      else if (fcstcycles > 1) then
        scenario =  fcstcycles
        call storeinitial
      else
        scenario = 1
      endif
        if (iclb /= 4) then
      do iscen = 1, scenario
    
        !! simulate watershed processes
        call simulate

        !! perform summary calculations
        call finalbal
        call writeaa
        call pestw

        !!reinitialize for new scenario
        if (scenario > iscen) call rewind_init
      end do
         end if
      do i = 101, 109 
        close (i)
      end do
      close(124)
!      write (*,1001)
 !1001 format (/," Execution successfully completed ")

end subroutine SWAT_DLL
