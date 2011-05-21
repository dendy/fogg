
# Looks up for the FLAC libraries
#
# Possible environment variables:
#
# FLACDIR - points where FLAC root directory exists
#
# Outputs:
#
# FLAC_INLUDE_DIR
# FLAC_LIBRARY


set( Flac_FOUND NO )


set( FLAC_ROOT_DIR "" CACHE PATH "Root directory for FLAC library" )


if ( FLAC_ROOT_DIR )
	unset( FLAC_INCLUDE_DIR CACHE )
	unset( FLAC_LIBRARY CACHE )
endif()


find_path( FLAC_INCLUDE_DIR
	NAMES
		"FLAC/all.h"
	HINTS
		"${FLAC_ROOT_DIR}/include"
		"$ENV{FLACDIR}/include"
		"$ENV{FLACDIR}"
	PATHS
		"/usr/include"
	DOC
		"Path to FLAC include directory"
)


if ( WIN32 )
	if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
		set( _flac_path_suffixes "obj/debug/lib" )
	else()
		set( _flac_path_suffixes "obj/release/lib" )
	endif()
endif()


find_library( FLAC_LIBRARY
	NAMES
		flac FLAC libFLAC_dynamic
	PATH_SUFFIXES
		${_flac_path_suffixes}
	PATHS
		"${FLAC_ROOT_DIR}"
		"${FLAC_ROOT_DIR}/lib"
		"$ENV{FLACDIR}"
		"$ENV{FLACDIR}/lib"
		"/usr/local/lib"
		"/usr/lib"
		"/sw/lib"
		"/opt/local/lib"
		"/opt/csw/lib"
		"/opt/lib"
	DOC
		"Path to FLAC library"
)


if ( FLAC_INCLUDE_DIR AND FLAC_LIBRARY )
	set( Flac_FOUND YES )
endif()


set( FLAC_ADVANCED_VARIABLES FLAC_ROOT_DIR FLAC_INCLUDE_DIR FLAC_LIBRARY )


if ( NOT Flac_FOUND )
	set( _message_common
		"FLAC library not found.\nPlease specify FLAC_ROOT_DIR variable or FLAC_INCLUDE_DIR and FLAC_LIBRARY separately." )

	if ( FLAC_FIND_REQUIRED )
		mark_as_advanced( CLEAR ${FLAC_ADVANCED_VARIABLES} )
		message( FATAL_ERROR "${_message_common}" )
	endif()

	mark_as_advanced( ${FLAC_ADVANCED_VARIABLES} )
	message( "${_message_common}\n"
		"You will find this variables in the advanced variables list." )
	set( FLAC_LIBRARIES "" CACHE "FLAC libraries" STRING FORCE )
else()
	mark_as_advanced( FORCE ${FLAC_ADVANCED_VARIABLES} )
	include_directories( ${FLAC_INCLUDE_DIR} )
	set( FLAC_LIBRARIES "${FLAC_LIBRARY}" CACHE "FLAC libraries" STRING FORCE )
endif()


mark_as_advanced( FLAC_LIBRARIES )
