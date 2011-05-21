
# Looks up for the FLAC libraries.
#
# Possible CMake variables:
#
#   Flac_ROOT_DIR - points where FLAC root directory exists
#
# Possible environment variables:
#
#   FLACDIR       - same as Flac_ROOT_DIR
#
# Outputs:
#
#   Flac_INLUDE_DIR         - directory to include (done automatically)
#   Flac_LIBRARY            - path to FLAC interface library
#   Flac_LIBRARIES          - list of all FLAC libraries
#   Flac_ADVANCED_VARIABLED - list of variables to setup manually


set( Flac_FOUND NO )


set( Flac_ROOT_DIR "" CACHE PATH "Root directory for FLAC library" )


if ( Flac_ROOT_DIR )
	unset( Flac_INCLUDE_DIR CACHE )
	unset( Flac_LIBRARY CACHE )
endif()


find_path( Flac_INCLUDE_DIR
	NAMES
		"FLAC/all.h"
	HINTS
		"${Flac_ROOT_DIR}/include"
		"$ENV{FLACDIR}/include"
		"$ENV{FLACDIR}"
	PATHS
		"/usr/include"
	DOC
		"Path to FLAC include directory"
)


if ( WIN32 )
	if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		set( _flac_path_suffixes "obj/debug/lib" )
	else()
		set( _flac_path_suffixes "obj/release/lib" )
	endif()
endif()


find_library( Flac_LIBRARY
	NAMES
		flac FLAC libFLAC_dynamic
	PATH_SUFFIXES
		${_flac_path_suffixes}
	HINTS
		"${Flac_ROOT_DIR}/lib"
		"${Flac_ROOT_DIR}"
		"$ENV{FLACDIR}/lib"
		"$ENV{FLACDIR}"
	PATHS
		"/usr/local/lib"
		"/usr/lib"
		"/sw/lib"
		"/opt/local/lib"
		"/opt/csw/lib"
		"/opt/lib"
	DOC
		"Path to FLAC library"
)


if ( Flac_INCLUDE_DIR AND Flac_LIBRARY )
	set( Flac_FOUND YES )
endif()


set( Flac_ADVANCED_VARIABLES Flac_ROOT_DIR Flac_INCLUDE_DIR Flac_LIBRARY )


if ( NOT Flac_FOUND )
	set( _message_common
		"FLAC library not found.\nPlease specify Flac_ROOT_DIR variable or Flac_INCLUDE_DIR and Flac_LIBRARY separately." )

	if ( FLAC_FIND_REQUIRED )
		mark_as_advanced( CLEAR ${Flac_ADVANCED_VARIABLES} )
		message( FATAL_ERROR "${_message_common}" )
	endif()

	mark_as_advanced( ${Flac_ADVANCED_VARIABLES} )
	message( "${_message_common}\n"
		"You will find this variables in the advanced variables list." )
	set( Flac_LIBRARIES "" CACHE "FLAC libraries" STRING FORCE )
else()
	mark_as_advanced( FORCE ${Flac_ADVANCED_VARIABLES} )
	include_directories( ${Flac_INCLUDE_DIR} )
	set( Flac_LIBRARIES "${Flac_LIBRARY}" CACHE "FLAC libraries" STRING FORCE )
endif()


mark_as_advanced( Flac_LIBRARIES )
