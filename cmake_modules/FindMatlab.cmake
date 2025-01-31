# - this module looks for Matlab
# Defines:
#  MATLAB_INCLUDE_DIR: include path for mex.h
#  MATLAB_LIBRARIES:   required libraries: libmex, libmx
#  MATLAB_MEX_LIBRARY: path to libmex
#  MATLAB_MX_LIBRARY:  path to libmx

SET(MATLAB_FOUND 0)
IF( "$ENV{MATLAB_ROOT}" STREQUAL "" )
    MESSAGE(STATUS "MATLAB_ROOT environment variable not set." )
    MESSAGE(STATUS "In Linux this can be done in your user .bashrc file by appending the corresponding line, e.g:" )
    MESSAGE(STATUS "export MATLAB_ROOT=/usr/local/MATLAB/R2012b" )
    MESSAGE(STATUS "In Windows this can be done by adding system variable, e.g:" )
    MESSAGE(STATUS "MATLAB_ROOT=D:\\Program Files\\MATLAB\\R2011a" )
ELSE("$ENV{MATLAB_ROOT}" STREQUAL "" )

        FIND_PATH(MATLAB_INCLUDE_DIR mex.h
                  $ENV{MATLAB_ROOT}/extern/include)

        INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})

        FIND_LIBRARY( MATLAB_MEX_LIBRARY
                      NO_CMAKE_SYSTEM_PATH
                      NAMES libmex mex
                      PATHS $ENV{MATLAB_ROOT}/bin $ENV{MATLAB_ROOT}/extern/lib 
                      PATH_SUFFIXES glnxa64 glnx86 maci64 win64/microsoft win32/microsoft )

        FIND_LIBRARY( MATLAB_MX_LIBRARY
                      NO_CMAKE_SYSTEM_PATH
                      NAMES libmx mx
                      PATHS $ENV{MATLAB_ROOT}/bin $ENV{MATLAB_ROOT}/extern/lib 
                      PATH_SUFFIXES glnxa64 glnx86 maci64 win64/microsoft win32/microsoft)

    MESSAGE (STATUS "MATLAB_ROOT: $ENV{MATLAB_ROOT}")

ENDIF("$ENV{MATLAB_ROOT}" STREQUAL "" )

# This is common to UNIX and Win32:
SET(MATLAB_LIBRARIES
  ${MATLAB_MEX_LIBRARY}
  ${MATLAB_MX_LIBRARY}
)

IF(MATLAB_INCLUDE_DIR AND MATLAB_LIBRARIES)
  SET(MATLAB_FOUND 1)
ENDIF(MATLAB_INCLUDE_DIR AND MATLAB_LIBRARIES)

if(WIN32)
  if (CMAKE_CL_64)
    SET( MATLAB_MEX_SUFFIX .mexw64)
  else(CMAKE_CL_64)
    SET( MATLAB_MEX_SUFFIX .mexw32)
  endif(CMAKE_CL_64)
elseif(APPLE)
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET( MATLAB_MEX_SUFFIX .mexmaci64 )
  else(CMAKE_SIZEOF_VOID_P MATCHES "8")
      MESSAGE( FATAL, "Only 64-bit MacOS is supported." )
  endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
else()
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET( MATLAB_MEX_SUFFIX .mexa64 )
  else(CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET( MATLAB_MEX_SUFFIX .mexglx)
  endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
endif()

SET( MATLAB_MEX_SUFFIX ${MATLAB_MEX_SUFFIX} CACHE STRING "Matlab Mex file name extension" )

MARK_AS_ADVANCED(
  MATLAB_LIBRARIES
  MATLAB_MEX_LIBRARY
  MATLAB_MX_LIBRARY
  MATLAB_INCLUDE_DIR
  MATLAB_FOUND
  MATLAB_ROOT
  MATLAB_MEX_SUFFIX
)
