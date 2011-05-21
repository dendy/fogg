
# Looks up for the Vorbis libraries.
#
# Possible CMake variables:
#
#   Vorbis_ROOT_DIR - points where Vorbis root directory exists
#
# Possible environment variables:
#
#   VORBISDIR       - same as Vorbis_ROOT_DIR
#
# Outputs:
#
#   Vorbis_INCLUDE_DIR        - directory to include (done automatically)
#   Vorbis_LIBRARY            - path to Vorbis library
#   VorbisFile_LIBRARY        - path to VorbisFile library
#   VorbisEnc_LIBRARY         - path to VorbisEnc library
#   Vorbis_LIBRARIES          - list of all Vorbis libraries
#   Vorbis_ADVANCED_VARIABLES - list of variables to setup manually


set( Vorbis_FOUND NO )


set( Vorbis_ROOT_DIR "" CACHE PATH "Root directory for Vorbis library" )


if ( Vorbis_ROOT_DIR )
	unset( Vorbis_INCLUDE_DIR CACHE )
	unset( Vorbis_LIBRARY CACHE )
	unset( VorbisFile_LIBRARY CACHE )
	unset( VorbisEnc_LIBRARY CACHE )
endif()


find_path( Vorbis_INCLUDE_DIR
	NAMES
		"vorbis/codec.h"
		"vorbis/vorbisenc.h"
		"vorbis/vorbisfile.h"
	HINTS
		"${Vorbis_ROOT_DIR}/include"
		"$ENV{VORBISDIR}/include"
	PATHS
		"/usr/include"
	DOC
		"Path to Vorbis include directory"
)


if ( WIN32 )
	if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		set( _vorbis_lib_names "vorbis_d" "vorbis" "libvorbis" )
		set( _vorbisfile_lib_names "vorbisfile_d" "vorbisfile" "libvorbisfile" )
		set( _vorbisenc_lib_names "vorbisenc_d" "vorbisenc" "libvorbisenc" )
		set( _vorbis_path_suffixes
			"win32/Vorbis_Dynamic_Debug"
			"win32/VorbisFile_Dynamic_Debug"
			"win32/VS2008/Win32/Debug"
			"win32/VS2005/Win32/Debug"
			)
	else()
		set( _vorbis_lib_names "vorbis" "libvorbis" )
		set( _vorbisfile_lib_names "vorbisfile" "libvorbisfile" )
		set( _vorbisenc_lib_names "vorbisenc" "libvorbisenc" )
		set( _vorbis_path_suffixes
			"win32/Vorbis_Dynamic_Release"
			"win32/VorbisFile_Dynamic_Release"
			"win32/VS2008/Win32/Release"
			"win32/VS2005/Win32/Release"
			)
	endif()
else()
	set( _vorbis_lib_names "vorbis" "Vorbis" "VORBIS" )
	set( _vorbisfile_lib_names "vorbisfile" "VorbisFile" "VORBISFILE" )
	set( _vorbisenc_lib_names "vorbisenc" "VorbisEnc" "VORBISENC" )
endif()

set( _vorbis_hints
	"${Vorbis_ROOT_DIR}/lib"
	"${Virbis_ROOT_DIR}"
	"$ENV{VORBISDIR}/lib"
	"$ENV{VORBISDIR}"
	"$ENV{OGGDIR}/lib"
	"$ENV{OGGDIR}"
)

set( _vorbis_paths
	"/usr/local/lib"
	"/usr/lib"
	"/sw/lib"
	"/opt/local/lib"
	"/opt/csw/lib"
	"/opt/lib"
)


find_library( Vorbis_LIBRARY
	NAMES
		${_vorbis_lib_names}
	PATH_SUFFIXES
		${_vorbis_path_suffixes}
	HINTS
		${_vorbis_hints}
	PATHS
		${_vorbis_paths}
	DOC
		"Path to Vorbis library"
)


find_library( VorbisFile_LIBRARY
	NAMES
		${_vorbisfile_lib_names}
	PATH_SUFFIXES
		${_vorbis_path_suffixes}
	PATHS
		${_vorbis_paths}
	DOC
		"Path to VorbisFile library"
)


find_library( VorbisEnc_LIBRARY
	NAMES
		${_vorbisenc_lib_names}
	PATH_SUFFIXES
		${_vorbis_path_suffixes}
	PATHS
		${_vorbis_paths}
	DOC
		"Path to VorbisEnc library"
)


if ( Vorbis_INCLUDE_DIR AND Vorbis_LIBRARY AND VorbisFile_LIBRARY AND VorbisEnc_LIBRARY )
	set( Vorbis_FOUND YES )
endif()


set( Vorbis_ADVANCED_VARIABLES Vorbis_ROOT_DIR Vorbis_INCLUDE_DIR Vorbis_LIBRARY VorbisFile_LIBRARY VorbisEnc_LIBRARY )


if ( NOT Vorbis_FOUND )
	set( _message_common
		"Vorbis library not found.\nPlease specify Vorbis_ROOT_DIR variable or Vorbis_INCLUDE_DIR and Vorbis_LIBRARY and VorbisFile_LIBRARY and VorbisEnc_LIBRARY separately." )

	if ( Vorbis_FIND_REQUIRED )
		mark_as_advanced( CLEAR ${Vorbis_ADVANCED_VARIABLES} )
		message( FATAL_ERROR "${_message_common}" )
	endif()

	mark_as_advanced( ${Vorbis_ADVANCED_VARIABLES} )
	message( "${_message_common}\n"
		"You will find this variables in the advanced variables list." )
	set( Vorbis_LIBRARIES "" CACHE STRING "Vorbis libraries" FORCE )
else()
	mark_as_advanced( FORCE ${Vorbis_ADVANCED_VARIABLES} )
	include_directories( "${Vorbis_INCLUDE_DIR}" )
	set( Vorbis_LIBRARIES "${Vorbis_LIBRARY};${VorbisFile_LIBRARY};${VorbisEnc_LIBRARY}" CACHE STRING "Vorbis libraries" FORCE )
endif()


mark_as_advanced( Vorbis_LIBRARIES )
