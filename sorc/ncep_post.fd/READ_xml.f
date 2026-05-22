!> @file
!> @brief read_postxconfig() reads the post available field XML file and post control XML file. 
!> Each set of output fields going to one output file will be saved and processed later. 
!> In other words, post control file will be read in whole once. 
!> 
!> ### Program history log:
!> Date | Programmer | Comments
!> -----|------------|---------
!>   2012-01-27 | Jun Wang | INITIAL CODE
!>   2015-03-10 | Lin Gan  | Replace XML file with flat file implementation with parameter marshalling
!>   2016-07-08 | J. Carley | Clean up prints 

      SUBROUTINE READ_xml()

       use xml_perl_data,only: post_avblflds,paramset,read_postxconfig
       use grib2_module, only: num_pset
       use rqstfld_mod,only: num_post_afld,MXLVL,lvlsxml
       use CTLBLK_mod, only: me
!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       implicit none
!
!     DECLARE VARIABLES.
!     
!******************************************************************************
!     START READCNTRL_XML HERE.
!     
!> @brief Read post available field table

      call read_postxconfig()
      num_pset=size(paramset)
      ! num_post_afld is already set in read_postxconfig for YAML or XML flat file

      RETURN
      end subroutine read_xml
