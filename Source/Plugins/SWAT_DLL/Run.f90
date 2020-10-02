subroutine Run

  ! Expose subroutine Test_DLL to users of this DLL
  !
  !DEC$ ATTRIBUTES DLLEXPORT::Run
    
!    if (iclb /= 4) then
!      do iscen = 1, scenario
    
        !! simulate watershed processes
        call simulate

        !! perform summary calculations
        call finalbal
        call writeaa
        call pestw

        !!reinitialize for new scenario
 !       if (scenario > iscen) call rewind_init
 !     end do
 !        end if
 !     do i = 101, 109 
 !       close (i)
 !     end do
 !     close(124)
      
      end subroutine Run