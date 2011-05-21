
# Looks up for the mpg123 libraries.
#
# Possible CMake variables:
#
#   Mpg123_ROOT_DIR - points where mpg123 root directory exists
#
# Possible environment variables:
#
#   MPG123DIR       - same as Mpg123_ROOT_DIR
#
# Outputs:
#
#   Mpg123_INLUDE_DIR         - directory to include (done automatically)
#   Mpg123_LIBRARY            - path to mpg123 interface library
#   Mpg123_LIBRARIES          - list of all mpg123 libraries
#   Mpg123_ADVANCED_VARIABLES - list of variables to setup manually


set( Mpg123_FOUND NO )


set( Mpg123_ROOT_DIR "" CACHE PATH "Root directory of mpg123 library" )


if ( Mpg123_ROOT_DIR )
	unset( Mpg123_INCLUDE_DIR CACHE )
	unset( Mpg123_LIBRARY CACHE )
endif()


find_path( Mpg123_INCLUDE_DIR
	NAMES
		"mpg123.h"
	HINTS
		"${Mpg123_ROOT_DIR}/include"
		"$ENV{MPG123DIR}"
	PATHS
		"/usr/include"
	DOC
		"Path to mpg123 include directory"
)


find_library( Mpg123_LIBRARY
	NAMES
		"mpg123"
	PATH_SUFFIXES
		"lib" "lib64"
	HINTS
		"${Mpg123_ROOT_DIR}"
		"$ENV{MPG123DIR}"
	PATHS
		"/usr"
		"/usr/local"
)


if ( Mpg123_INCLUDE_DIR AND Mpg123_LIBRARY )
	set( Mpg123_FOUND YES )
endif()


set( Mpg123_ADVANCED_VARIABLES Mpg123_ROOT_DIR Mpg123_INCLUDE_DIR Mpg123_LIBRARY )


if ( NOT Mpg123_FOUND )
	set( _message_common
		"mpg123 library not found.\nPlease specify Mpg123_ROOT_DIR variable or Mpg123_INCLUDE_DIR and Mpg123_LIBRARY separately." )

	if ( Mpg123_FIND_REQUIRED )
		mark_as_advanced( CLEAR ${Mpg123_ADVANCED_VARIABLES} )
		message( FATAL_ERROR "${_message_common}" )
	endif()

	mark_as_advanced( ${Mpg123_ADVANCED_VARIABLES} )
	message( "${_message_common}\n"
		"You will find this variables in the advanced variables list." )
	set( Mpg123_LIBRARIES "" CACHE STRING "mpg123 libraries" FORCE )
else()
	mark_as_advanced( FORCE ${Mpg123_ADVANCED_VARIABLES} )
	include_directories( "${Mpg123_INCLUDE_DIR}" )
	set( Mpg123_LIBRARIES "${Mpg123_LIBRARY}" CACHE STRING "mpg123 libraries" FORCE )
endif()


mark_as_advanced( Mpg123_LIBRARIES )
