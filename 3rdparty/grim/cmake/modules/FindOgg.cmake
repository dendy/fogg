
# Looks up for the Ogg libraries.
#
# Possible CMake variables:
#
#   Ogg_ROOT_DIR - points where Ogg root directory exists
#
# Possible environment variables:
#
#   OGGDIR       - same as Ogg_ROOT_DIR
#
# Outputs:
#
#   Ogg_INLUDE_DIR         - directory to include (done automatically)
#   Ogg_LIBRARY            - path to Ogg interface library
#   Ogg_LIBRARIES          - list of all Ogg libraries
#   Ogg_ADVANCED_VARIABLES - list of variables to setup manually


set( Ogg_FOUND NO )


set( Ogg_ROOT_DIR "" CACHE PATH "Root directory for Ogg library" )


if ( Ogg_ROOT_DIR )
	unset( Ogg_INCLUDE_DIR CACHE )
	unset( Ogg_LIBRARY CACHE )
endif()


find_path( Ogg_INCLUDE_DIR
	NAMES
		"ogg/ogg.h"
	HINTS
		"${Ogg_ROOT_DIR}/include"
		"$ENV{OGGDIR}"
	PATHS
		"/usr/include"
	DOC
		"Path to Ogg include directory"
)


if ( WIN32 )
	if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		set( _ogg_lib_names "ogg_d" "ogg" "libogg" )
		set( _ogg_path_suffixes
			"win32/Dynamic_Debug"
			"win32/VS2008/Win32/Debug"
			"win32/VS2005/Win32/Debug"
			"win32/VS2003/Debug"
			)
	else()
		set( _ogg_lib_names "ogg" "libogg" )
		set( _ogg_path_suffixes
			"win32/Dynamic_Release"
			"win32/VS2008/Win32/Release"
			"win32/VS2005/Win32/Release"
			"win32/VS2003/Release"
			)
	endif()
else()
	set( _ogg_lib_names "ogg" "Ogg" "OGG" )
	set( _ogg_path_suffixes "lib" "lib64" )
endif()


find_library( Ogg_LIBRARY
	NAMES
		${_ogg_lib_names}
	PATH_SUFFIXES
		${_ogg_path_suffixes}
	HINTS
		"${Ogg_ROOT_DIR}"
		"$ENV{OGGDIR}"
		"$ENV{VORBISDIR}"
	PATHS
		"/usr"
		"/usr/local"
		"/sw"
		"/opt"
		"/opt/local"
		"/opt/csw"
	DOC
		"Path to Ogg library"
)


if ( Ogg_INCLUDE_DIR AND Ogg_LIBRARY )
	set( Ogg_FOUND YES )
endif()


set( Ogg_ADVANCED_VARIABLES Ogg_ROOT_DIR Ogg_INCLUDE_DIR Ogg_LIBRARY )


if ( NOT Ogg_FOUND )
	set( _message_common
		"Ogg library not found.\nPlease specify Ogg_ROOT_DIR variable or Ogg_INCLUDE_DIR and Ogg_LIBRARY separately." )

	if ( Ogg_FIND_REQUIRED )
		mark_as_advanced( CLEAR ${Ogg_ADVANCED_VARIABLES} )
		message( FATAL_ERROR "${_message_common}" )
	endif()

	mark_as_advanced( ${Ogg_ADVANCED_VARIABLES} )
	message( "${_message_common}\n"
		"You will find this variables in the advanced variables list." )
	set( Ogg_LIBRARIES "" CACHE STRING "Ogg libraries" FORCE )
else()
	mark_as_advanced( FORCE ${Ogg_ADVANCED_VARIABLES} )
	include_directories( "${Ogg_INCLUDE_DIR}" )
	set( Ogg_LIBRARIES "${Ogg_LIBRARY}" CACHE STRING "Ogg libraries" FORCE )
endif()


mark_as_advanced( Ogg_LIBRARIES )
