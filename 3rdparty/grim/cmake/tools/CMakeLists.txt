
project( GrimTools )


# paths
set( GrimTools_DIR "${Grim_ROOT_DIR}/src/tools" )


# Qt
set( QT_MIN_VERSION "4.6.0" )
find_package( Qt4 REQUIRED )
set( QT_USE_QTOPENGL YES )
include( "${QT_USE_FILE}" )


# OpenGL
find_package( OpenGL REQUIRED )


# options
option( GrimTools_DEBUG "Enable debugging for libGrimTools" NO )
option( GrimTools_LIBRARY_SHARED "Build as shared library" YES )
option( GrimTools_USE_TEXTURE_BORDERS "Enable OpenGL texture borders" NO )
mark_as_advanced( FROCE GrimTools_USE_TEXTURE_BORDERS )


# includes
include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )


# defines
if ( GrimTools_LIBRARY_SHARED )
	add_definitions( -DGRIM_TOOLS_STATIC )
else()
	add_definitions( -DGRIM_TOOLS_BUILD )
endif()
if ( GrimTools_DEBUG )
	add_definitions( -DGRIM_TOOLS_DEBUG )
endif()
if ( GrimTools_USE_TEXTURE_BORDERS )
	add_definitions( -DGRIM_TOOLS_USE_TEXTURE_BORDERS )
endif()


# sources
my_add_sources( GrimTools
	ROOT_DIR "${GrimTools_DIR}"
		IdGenerator
		LocalizationManager
)

qt4_wrap_cpp( GrimTools_MOC_SOURCES ${GrimTools_HEADERS} OPTIONS "-nw" )

set( GrimTools_ALL_SOURCES ${GrimTools_SOURCES} ${GrimTools_MOC_SOURCES} )


# target
if ( GrimTools_LIBRARY_SHARED )
	set( _library_type "SHARED" )
else()
	set( _library_type "STATIC" )
endif()
add_library( GrimTools ${_library_type} ${GrimTools_ALL_SOURCES} )
target_link_libraries( GrimTools ${QT_LIBRARIES} ${OPENGL_LIBRARIES} )
if ( WIN32 )
	target_link_libraries( GrimTools Winmm )
endif()
if ( NOT WIN32 AND NOT APPLE AND UNIX )
	target_link_libraries( GrimTools Xrandr )
endif()
if ( APPLE )
	target_link_libraries( GrimTools "-framework ApplicationServices" )
endif()


# properties
set_target_properties( GrimTools PROPERTIES
	VERSION "${Grim_VERSION}"
	SOVERSION "${Grim_SOVERSION}"
	OUTPUT_NAME GrimTools
)


# precompiled
grim_add_precompiled_headers_wrapper( GrimTools "${GrimTools_DIR}/PrecompiledHeaders.h" ${GrimTools_ALL_SOURCES} )


# export
my_get_target_libraries( GrimTools _interface_library _runtime_library )
set( GrimTools_LIBRARY "${_interface_library}" CACHE FILEPATH "" FORCE )


# fill all grim sources
grim_add_sources( ${GrimTools_SOURCES} )


# installation
install( TARGETS GrimTools
	ARCHIVE DESTINATION "${Grim_ARCHIVE_DESTINATION}"
	LIBRARY DESTINATION "${Grim_LIBRARY_DESTINATION}"
	RUNTIME DESTINATION "${Grim_RUNTIME_DESTINATION}"
)
