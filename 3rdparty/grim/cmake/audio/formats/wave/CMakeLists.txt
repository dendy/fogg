
project( GrimAudioFormatPluginWave )


# Qt
set( QT_MIN_VERSION "4.6.0" )
find_package( Qt4 REQUIRED )
set( QT_DONT_USE_QTGUI YES )
include( "${QT_USE_FILE}" )


set( GrimAudio_FORMAT_PLUGIN_WAVE_ENABLED YES CACHE BOOL "" FORCE )


# definitions
add_definitions( -DQT_STATICPLUGIN )


# sources
my_add_sources( GrimAudioFormatPluginWave
	ROOT_DIR "${GrimAudio_DIR}/formats"
		FormatPluginWave
)

qt4_wrap_cpp( GrimAudioFormatPluginWave_MOC_SOURCES ${GrimAudioFormatPluginWave_HEADERS} OPTIONS "-nw" )

set( GrimAudioFormatPluginWave_ALL_SOURCES
	${GrimAudioFormatPluginWave_SOURCES}
	${GrimAudioFormatPluginWave_MOC_SOURCES}
)


# target
add_library( GrimAudioFormatPluginWave STATIC ${GrimAudioFormatPluginWave_ALL_SOURCES} )

if ( UNIX )
	set_target_properties( GrimAudioFormatPluginWave PROPERTIES COMPILE_FLAGS "-fPIC" )
endif()
