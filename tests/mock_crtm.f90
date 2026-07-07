module crtm_parameters
  use iso_c_binding
  implicit none
  integer, parameter :: limit_exp = 1
  real, parameter :: toa_pressure = 0.1
  integer, parameter :: max_n_layers = 100
  real, parameter :: MAX_SENSOR_SCAN_ANGLE = 60.0
  type crtm_geometry_type
     real :: sensor_zenith_angle = 0.0
     real :: sensor_scan_angle = 0.0
     real :: source_zenith_angle = 0.0
  end type crtm_geometry_type
end module crtm_parameters

module crtm_channelinfo_define
  use iso_c_binding
  implicit none
  type crtm_channelinfo_type
     integer :: n_channels = 1
     character(len=20) :: sensor_id = 'mock'
     integer :: WMO_Satellite_Id = 0
     integer :: WMO_sensor_id = 0
     integer :: sensor_channel = 1
     logical :: Process_Channel(100) = .true.
     integer :: Channel_Index(100) = 0
  end type crtm_channelinfo_type
end module crtm_channelinfo_define

module crtm_rtsolution_define
  use iso_c_binding
  implicit none
  type crtm_rtsolution_type
     real :: brightness_temperature = 280.0
  end type crtm_rtsolution_type
contains
  subroutine crtm_rtsolution_create(r, n)
     type(crtm_rtsolution_type), allocatable, intent(out) :: r(:,:)
     integer, intent(in) :: n
     allocate(r(1,1))
  end subroutine
  subroutine crtm_rtsolution_destroy(r)
     type(crtm_rtsolution_type), allocatable, intent(inout) :: r(:,:)
     if (allocated(r)) deallocate(r)
  end subroutine
  function crtm_rtsolution_associated(r)
     type(crtm_rtsolution_type), intent(in) :: r(:,:)
     logical, dimension(size(r,1), size(r,2)) :: crtm_rtsolution_associated
     crtm_rtsolution_associated = .true.
  end function
end module crtm_rtsolution_define

module crtm_atmosphere_define
  implicit none
  integer, parameter :: h2o_id = 1
  integer, parameter :: volume_mixing_ratio_units = 1
  type crtm_cloud_type
     integer :: n_layers = 1
     integer :: Type = 1
     real, allocatable :: effective_radius(:)
     real, allocatable :: water_content(:)
  end type crtm_cloud_type
  type crtm_atmosphere_type
     integer :: n_layers = 1
     integer :: absorber_id(2) = 0
     integer :: absorber_units(2) = 0
     real, allocatable :: cloud_fraction(:)
     real, allocatable :: level_pressure(:)
     real, allocatable :: pressure(:)
     real, allocatable :: temperature(:)
     real, allocatable :: absorber(:,:)
     type(crtm_cloud_type) :: cloud(6)
  end type crtm_atmosphere_type
contains
  subroutine crtm_atmosphere_create(a, n_layers, n_absorbers, n_clouds, n_aerosols)
     type(crtm_atmosphere_type), intent(out) :: a
     integer, intent(in) :: n_layers, n_absorbers, n_clouds, n_aerosols
     integer :: i
     allocate(a%cloud_fraction(n_layers))
     allocate(a%level_pressure(0:n_layers))
     allocate(a%pressure(n_layers))
     allocate(a%temperature(n_layers))
     allocate(a%absorber(n_layers, 2))
     do i = 1, 6
        allocate(a%cloud(i)%effective_radius(n_layers))
        allocate(a%cloud(i)%water_content(n_layers))
     end do
  end subroutine
  subroutine crtm_atmosphere_destroy(a)
     type(crtm_atmosphere_type), intent(inout) :: a
     integer :: i
     if (allocated(a%cloud_fraction)) deallocate(a%cloud_fraction)
     if (allocated(a%level_pressure)) deallocate(a%level_pressure)
     if (allocated(a%pressure)) deallocate(a%pressure)
     if (allocated(a%temperature)) deallocate(a%temperature)
     if (allocated(a%absorber)) deallocate(a%absorber)
     do i = 1, 6
        if (allocated(a%cloud(i)%effective_radius)) deallocate(a%cloud(i)%effective_radius)
        if (allocated(a%cloud(i)%water_content)) deallocate(a%cloud(i)%water_content)
     end do
  end subroutine
  function crtm_atmosphere_associated(a)
     type(crtm_atmosphere_type), intent(in) :: a
     logical :: crtm_atmosphere_associated
     crtm_atmosphere_associated = .true.
  end function
  subroutine crtm_atmosphere_zero(a)
     type(crtm_atmosphere_type), intent(inout) :: a
  end subroutine
end module crtm_atmosphere_define

module crtm_surface_define
  use crtm_channelinfo_define, only: crtm_channelinfo_type
  implicit none
  type crtm_sensordata_type
     character(len=20) :: sensor_id = 'mock'
     integer :: WMO_sensor_id = 0
     integer :: WMO_Satellite_id = 0
     integer :: sensor_channel = 1
     integer :: n_channels = 1
  end type crtm_sensordata_type
  type crtm_surface_type
     integer :: land_type = 1
     real :: wind_speed = 0.0
     real :: water_temperature = 280.0
     real :: ice_temperature = 270.0
     real :: soil_moisture_content = 0.1
     real :: vegetation_fraction = 0.5
     real :: soil_temperature = 280.0
     real :: land_temperature = 280.0
     real :: snow_temperature = 270.0
     real :: snow_depth = 0.0
     real :: water_coverage = 0.0
     real :: land_coverage = 0.0
     real :: ice_coverage = 0.0
     real :: snow_coverage = 0.0
     type(crtm_sensordata_type) :: sensordata
  end type crtm_surface_type
contains
  subroutine crtm_surface_create(s, n)
     type(crtm_surface_type), intent(out) :: s
     integer, intent(in) :: n
  end subroutine
  subroutine crtm_surface_destroy(s)
     type(crtm_surface_type), intent(inout) :: s
  end subroutine
  function crtm_surface_associated(s)
     type(crtm_surface_type), intent(in) :: s
     logical :: crtm_surface_associated
     crtm_surface_associated = .true.
  end function
  subroutine crtm_surface_zero(s)
     type(crtm_surface_type), intent(inout) :: s
  end subroutine
end module crtm_surface_define

module crtm_spccoeff
  implicit none
  integer :: sc = 1
end module crtm_spccoeff

module crtm_cloud_define
  implicit none
  integer, parameter :: water_cloud = 1
  integer, parameter :: ice_cloud = 2
  integer, parameter :: rain_cloud = 3
  integer, parameter :: snow_cloud = 4
  integer, parameter :: graupel_cloud = 5
  integer, parameter :: hail_cloud = 6
end module crtm_cloud_define

module message_handler
  implicit none
  integer, parameter :: success = 0
  integer, parameter :: warning = 1
contains
  subroutine display_message(r, msg, level)
     integer, intent(in) :: r
     character(len=*), intent(in) :: msg
     integer, intent(in) :: level
  end subroutine
end module message_handler

module crtm_module
  use crtm_parameters
  use crtm_channelinfo_define
  use crtm_atmosphere_define
  use crtm_surface_define
  use crtm_rtsolution_define
  use message_handler, only: success
  implicit none
  type crtm_options_type
     integer :: dummy = 1
  end type crtm_options_type
  integer, parameter :: o3_id = 2
  integer, parameter :: co2_id = 3
  integer, parameter :: mass_mixing_ratio_units = 1
  integer, parameter :: SPECIFIC_AMOUNT_UNITS = 2
contains
  subroutine crtm_options_create(o)
     type(crtm_options_type), intent(out) :: o
  end subroutine
  subroutine crtm_options_destroy(o)
     type(crtm_options_type), intent(inout) :: o
  end subroutine
  function crtm_options_associated(o)
     type(crtm_options_type), intent(in) :: o
     logical :: crtm_options_associated
     crtm_options_associated = .true.
  end function
  function crtm_init(sensors, info, Process_ID, Output_Process_ID)
     character(len=*), intent(in) :: sensors(:)
     type(crtm_channelinfo_type), intent(inout) :: info(:)
     integer, intent(in), optional :: Process_ID, Output_Process_ID
     integer :: crtm_init
     crtm_init = 0
  end function
  function crtm_forward(a, s, g, info, r)
     type(crtm_atmosphere_type), intent(in) :: a(:)
     type(crtm_surface_type), intent(in) :: s(:)
     type(crtm_rtsolution_type), intent(inout) :: r(:,:)
     type(crtm_channelinfo_type), intent(in) :: info(:)
     type(crtm_geometry_type), intent(in) :: g(:)
     integer :: crtm_forward
     crtm_forward = 0
  end function
  function crtm_destroy(info)
     type(crtm_channelinfo_type), intent(inout) :: info(:)
     integer :: crtm_destroy
     crtm_destroy = 0
  end function
end module crtm_module
