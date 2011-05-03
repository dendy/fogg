
set( Mpg123_FOUND NO )


find_path( Mpg123_INCLUDE_DIR
	NAMES
		"mpg123.h"
	PATHS
		"${MPG123_ROOT_DIR}/include"
		"$ENV{MPG123DIR}"
		"/usr/include"
	DOC
		"Path to mpg123 include directory"
)


find_library( Mpg123_LIBRARY
	NAMES
		"mpg123"
	PATH_SUFFIXES
		"lib" "lib64"
	PATHS
		"${MPG123_ROOT_DIR}"
		"$ENV{MPG123DIR}"
		"/usr"
		"/usr/local"
)


if ( Mpg123_INCLUDE_DIR AND Mpg123_LIBRARY )
	set( Mpg123_FOUND YES )
endif()


set( _advanced_variables MPG123_ROOT_DIR Mpg123_INCLUDE_DIR Mpg123_LIBRARY )


if ( NOT Mpg123_FOUND )
	set( MPG123_ROOT_DIR "" CACHE PATH "Root directory of mpg123 library" )

	set( _message_common
		"mpg123 library not found.\nPlease specify MPG123_ROOT_DIR variable or Mpg123_INCLUDE_DIR and Mpg123_LIBRARY separately." )

	if ( Mpg123_FIND_REQUIRED )
		mark_as_advanced( CLEAR ${_advanced_variables} )
		message( FATAL_ERROR "${_message_common}" )
	endif()

	mark_as_advanced( ${_advanced_variables} )
	message( "${_message_common}\n"
		"You will find this variables in the advanced variables list." )
	set( Mpg123_LIBRARIES "" CACHE STRING "mpg123 libraries" FORCE )
else()
	mark_as_advanced( FORCE ${_advanced_variables} )
	include_directories( "${Mpg123_INCLUDE_DIR}" )
	set( Mpg123_LIBRARIES "${Mpg123_LIBRARY}" CACHE STRING "mpg123 libraries" FORCE )
endif()


mark_as_advanced( Mpg123_LIBRARIES )
