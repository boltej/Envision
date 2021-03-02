
      subroutine endYear
      !DEC$ ATTRIBUTES DLLEXPORT::endYear
      !! perform end-of-year processes
      use parm          
        do isb = 1, msub
          !! Srin co2 (EPA)
          !! increment co2 concentrations 
          co2(isb) = co2_x2 * curyr **2 + co2_x * curyr + co2(isb)
        end do
        
        do j = 1, nhru
          !! compute biological mixing at the end of every year

!          if (biomix(j) > .001) call tillmix (j,biomix(j))
          if (biomix(j) > .001) call newtillmix (j,biomix(j))

          !! update sequence number for year in rotation to that of
          !! the next year and reset sequence numbers for operations
          if (idplt(j) > 0) then
            if (idc(idplt(j)) == 7) then
              curyr_mat(j) = curyr_mat(j) + 1
              curyr_mat(j) = Min(curyr_mat(j),mat_yrs(idplt(j)))
            end if
          end if

          !! update target nitrogen content of yield with data from
          !! year just simulated
          do ic = 1, mcr
            xx = 0.
            xx = dfloat(curyr)
            tnylda(j) = (tnylda(j) * xx + tnyld(j)) / (xx + 1.)
          end do

          if (idaf < 181) then
            if (mgtop(nop(j),j) /= 17) then
              dorm_flag = 1
              ihru = j
              call operatn
              dorm_flag = 0
            end if
            nop(j) = nop(j) + 1
          
            if (nop(j) > nopmx(j)) then
              nop(j) = 1
            end if
            
            phubase(j) = 0.
            yr_skip(j) = 0
          endif
          if (mgtop(nop(j),j) == 17) then
            nop(j) = nop(j) + 1
            if (mgtop(nop(j),j) == 17) then
              yr_skip(j) = 1
            end if
          endif

        end do

      !! update simulation year
      iyr = iyr + 1
   !   end do            !!     end annual loop

      return
1234 format (1x,' Executing year ', i4)
     end