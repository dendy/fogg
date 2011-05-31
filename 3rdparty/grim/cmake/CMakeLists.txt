
cmake_minimum_required( VERSION 2.8 )


project( Grim )


# paths
get_filename_component( _grim_root_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE )
set( Grim_ROOT_DIR "${_grim_root_dir}" CACHE PATH "Root directory of Grim library" )
set( Grim_INCLUDE_DIR "${Grim_ROOT_DIR}/include" CACHE PATH "Path to Grim include directory" FORCE )
mark_as_advanced( Grim_ROOT_DIR Grim_INCLUDE_DIR )


# version
set( Grim_VERSION "0.0.1" )
set( Grim_SOVERSION "0.0" )


# additional directory with modules
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/modules" )


if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE Release )
endif()


# additional macroses
find_package( My )


set( CMAKE_SHARED_LIBRARY_PREFIX "lib" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib" )


# global options
option( Grim_USE_PRECOMPILED_HEADERS "Enable Precompiled Headers support" ON )

# options to build particular module
option( Grim_BUILD_MODULE_GAME       "Build libGrimGame"    OFF )
option( Grim_BUILD_MODULE_ARCHIVE    "Build libGrimArchive" OFF )
option( Grim_BUILD_MODULE_AUDIO      "Build libGrimAudio"   OFF )
option( Grim_BUILD_MODULE_TOOLS      "Build libGrimTools"   OFF )

# translation options
set( Grim_TRANSLATIONS "ALL" CACHE STRING "Space separated list of languages to build translations for." )


# helper macro for adding precompiled headers
macro ( grim_add_precompiled_headers_wrapper TARGET PRECOMPILED_HEADER )
	if ( Grim_USE_PRECOMPILED_HEADERS )
		my_add_precompiled_headers( ${TARGET} ${PRECOMPILED_HEADER} ${ARGN} )
	endif()
endmacro()


# add include directory for cross-references between modules
include_directories( "${Grim_ROOT_DIR}/include" )


# check if build is for 64-bit system and set library suffix appropriate
set( Grim_ARCHIVE_DESTINATION "lib" )
set( Grim_RUNTIME_DESTINATION "bin" )
set( Grim_LIBRARY_DESTINATION "lib" )
# TODO: USE CMAKE_SIZEOF_VOID_P instead?
if ( "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" )
	set( Grim_LIBRARY_DESTINATION "lib64" )
endif()


# mark global variable that we are inside Grim library build
set( Grim_INSIDE_BUILD ON )


# macro to collect all grim sources from modules to create solid translation file later
set( Grim_ALL_SOURCES )
macro( grim_add_sources )
	set( Grim_ALL_SOURCES ${Grim_ALL_SOURCES} ${ARGN} PARENT_SCOPE )
endmacro()


# list of Grim modules
set( _grim_modules
	Game
	Archive
	Audio
	Tools
)


set( _grim_libraries )
set( _grim_targets )


# subdirectories
foreach ( _grim_module ${_grim_modules} )
	string( TOUPPER ${_grim_module} _grim_module_upper )
	string( TOLOWER ${_grim_module} _grim_module_lower )
	if ( Grim_BUILD_MODULE_${_grim_module_upper} )
		add_subdirectory( "${_grim_module_lower}" )

		set( _grim_target Grim${_grim_module} )
		list( APPEND _grim_targets ${_grim_target} )

		get_target_property( _interface_libraries ${_grim_target} LINK_INTERFACE_LIBRARIES )
		if ( _interface_libraries )
			list( APPEND _grim_libraries ${_interface_libraries} )
		endif()

		set( _library_variable Grim${_grim_module}_LIBRARY )
		list( APPEND _grim_libraries ${${_library_variable}} )
		mark_as_advanced( FORCE ${_library_variable} )
	endif()
endforeach()


set( Grim_LIBRARIES "${_grim_libraries}" CACHE STRING "List of all Grim libraries" FORCE )
mark_as_advanced( FORCE Grim_LIBRARIES )

set( Grim_TARGETS "${_grim_targets}" CACHE STRING "List of all Grim module targets" FORCE )
mark_as_advanced( FORCE Grim_TARGETS )
