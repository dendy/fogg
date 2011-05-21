
project( GrimAudioFormatPluginFlac )


# Qt
set( QT_MIN_VERSION "4.6.0" )
find_package( Qt4 REQUIRED )
set( QT_DONT_USE_QTGUI YES )
include( "${QT_USE_FILE}" )


# FLAC
find_package( FLAC )


if ( NOT FLAC_FOUND )
	if ( GrimAudio_BUILD_PLUGIN_FLAC_OPTIONAL )
		set( _required_qualifier STATUS )
	else()
		mark_as_advanced( CLEAR ${FLAC_ADVANCED_VARIABLES} )
		set( _required_qualifier FATAL_ERROR )
	endif()

	message( ${_required_qualifier}
		"\n"
		"FLAC libraries are not found.\n"
		"Audio FLAC plugin will be skipped from build.\n"
		"Resolve prerequisites for FLAC libraries and rerun configuration."
		"\n" )
	return()
endif()


set( GrimAudio_FORMAT_PLUGIN_FLAC_ENABLED YES CACHE BOOL "" FORCE )


# definitions
add_definitions( -DQT_STATICPLUGIN )


# sources
my_add_sources( GrimAudioFormatPluginFlac
	ROOT_DIR "${GrimAudio_DIR}/formats"
		FormatPluginFlac
)

qt4_wrap_cpp( GrimAudioFormatPluginFlac_MOC_SOURCES ${GrimAudioFormatPluginFlac_HEADERS} OPTIONS "-nw" )

set( GrimAudioFormatPluginFlac_ALL_SOURCES
	${GrimAudioFormatPluginFlac_SOURCES}
	${GrimAudioFormatPluginFlac_MOC_SOURCES}
)


# target
add_library( GrimAudioFormatPluginFlac STATIC ${GrimAudioFormatPluginFlac_ALL_SOURCES} )

if ( UNIX )
	set_target_properties( GrimAudioFormatPluginFlac PROPERTIES COMPILE_FLAGS "-fPIC" )
endif()


# properties
set_property( TARGET GrimAudioFormatPluginFlac PROPERTY
	LINK_INTERFACE_LIBRARIES ${FLAC_LIBRARY}
)
