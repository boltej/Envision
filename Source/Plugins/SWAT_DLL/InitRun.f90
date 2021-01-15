subroutine init(agents_in_door, m_iGrid, agents_in_time, filenam, sz)

  ! Expose subroutine Test_DLL to users of this DLL
  !
  !DEC$ ATTRIBUTES DLLEXPORT::Init

  use parm
  ! Variables

  integer, parameter :: DP = kind(0d0)
  !real(kind=DP), intent(in)
  !real(kind=DP), intent(in)
  real(kind=DP), dimension(*) :: agents_in_time
  integer, dimension(*) :: agents_in_door
  integer :: sz
  character(len=sz)    :: filenam
  LOGICAL(4) status

    do i=1,5
     agents_in_door(i)=2*agents_in_door(i)-1
     agents_in_time(i)=sin(3.1416*0.2*i)
   end do  
  

  
 ! Body of Test_DLL
!      implicit none
      print*,"Hello"
      prog = "SWAT Sep 7    VER 2018/Rev 670"
      write (*,1000)
      1000 format(1x,"               SWAT2018               ")  
 

 !1000 format(1x,"               SWAT2018               ",/,             
 !    &          "               Rev. 670               ",/,             
 !    &          "      Soil & Water Assessment Tool    ",/,             
 !    &          "               PC Version             ",/,             
 !    &          " Program reading from file.cio . . . executing",/)

!! process input

      status = CHANGEDIRQQ(filenam)
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
      iscen=1
      

!      write (*,1001)
 !1001 format (/," Execution successfully completed ")

end subroutine init
