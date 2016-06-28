
project( GrimAudioFormatPluginWave )


# Qt
find_package( Qt5 REQUIRED Core )


set( GrimAudio_FORMAT_PLUGIN_WAVE_ENABLED YES CACHE BOOL "" FORCE )


# definitions
add_definitions( -DQT_STATICPLUGIN )


# sources
my_add_sources( GrimAudioFormatPluginWave
	ROOT_DIR "${GrimAudio_DIR}/formats"
		FormatPluginWave
)

qt5_wrap_cpp( GrimAudioFormatPluginWave_MOC_SOURCES ${GrimAudioFormatPluginWave_HEADERS} OPTIONS "-nw" )

set( GrimAudioFormatPluginWave_ALL_SOURCES
	${GrimAudioFormatPluginWave_SOURCES}
	${GrimAudioFormatPluginWave_MOC_SOURCES}
)


# target
add_library( GrimAudioFormatPluginWave STATIC ${GrimAudioFormatPluginWave_ALL_SOURCES} )
target_link_libraries(GrimAudioFormatPluginWave Qt5::Core)

if ( UNIX )
	set_target_properties( GrimAudioFormatPluginWave PROPERTIES COMPILE_FLAGS "-fPIC" )
endif()
