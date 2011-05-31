
project( GrimAudioFormatPluginVorbis )


# Qt
set( QT_MIN_VERSION "4.6.0" )
find_package( Qt4 REQUIRED )
set( QT_DONT_USE_QTGUI YES )
include( "${QT_USE_FILE}" )


# Ogg & Vorbis
find_package( Ogg )
find_package( Vorbis )


if ( NOT Ogg_FOUND OR NOT Vorbis_FOUND )
	if ( GrimAudio_BUILD_PLUGIN_VORBIS_OPTIONAL )
		set( _required_qualifier STATUS )
	else()
		mark_as_advanced( CLEAR Ogg_ADVANCED_VARIABLES )
		mark_as_advanced( CLEAR Vorbis_ADVANCED_VARIABLES )
		set( _required_qualifier FATAL_ERROR )
	endif()

	message( ${_required_qualifier}
		"\n"
		"Ogg/Vorbis libraries are not found.\n"
		"Audio Vorbis plugin will be skipped from build.\n"
		"Resolve prerequisites for Ogg/Vorbis libraries and rerun configuration."
		"\n" )
	return()
endif()


set( GrimAudio_FORMAT_PLUGIN_VORBIS_ENABLED YES CACHE BOOL "" FORCE )


# definitions
add_definitions( -DQT_STATICPLUGIN )


# sources
my_add_sources( GrimAudioFormatPluginVorbis
	ROOT_DIR "${GrimAudio_DIR}/formats"
		FormatPluginVorbis
)

qt4_wrap_cpp( GrimAudioFormatPluginVorbis_MOC_SOURCES ${GrimAudioFormatPluginVorbis_HEADERS} OPTIONS "-nw" )

set( GrimAudioFormatPluginVorbis_ALL_SOURCES
	${GrimAudioFormatPluginVorbis_SOURCES}
	${GrimAudioFormatPluginVorbis_MOC_SOURCES}
)


# target
add_library( GrimAudioFormatPluginVorbis STATIC ${GrimAudioFormatPluginVorbis_ALL_SOURCES} )

if ( UNIX )
	set_target_properties( GrimAudioFormatPluginVorbis PROPERTIES COMPILE_FLAGS "-fPIC" )
endif()


# properties
set_property( TARGET GrimAudioFormatPluginVorbis PROPERTY
	LINK_INTERFACE_LIBRARIES ${Vorbis_LIBRARY} ${VorbisFile_LIBRARY} ${Ogg_LIBRARY}
)
