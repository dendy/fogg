
project( GrimAudioFormatPluginMp3 )


# Qt
find_package( Qt5 REQUIRED Core )


# mpg123
find_package( Mpg123 )


if ( NOT Mpg123_FOUND )
	if ( GrimAudio_BUILD_PLUGIN_MP3_OPTIONAL )
		set( _required_qualifier STATUS )
	else()
		mark_as_advanced( CLEAR ${Mpg123_ADVANCED_VARIABLES} )
		set( _required_qualifier FATAL_ERROR )
	endif()

	message( ${_required_qualifier}
		"\n"
		"mpg123 library is not found.\n"
		"Audio Mp3 plugin will be skipped from build.\n"
		"Resolve prerequisites for mpg123 library and rerun configuration."
		"\n" )
	return()
endif()


set( GrimAudio_FORMAT_PLUGIN_MP3_ENABLED YES CACHE BOOL "" FORCE )


# definitions
add_definitions( -DQT_STATICPLUGIN )


# sources
my_add_sources( GrimAudioFormatPluginMp3
	ROOT_DIR "${GrimAudio_DIR}/formats"
		FormatPluginMp3
)

qt5_wrap_cpp( GrimAudioFormatPluginMp3_MOC_SOURCES ${GrimAudioFormatPluginMp3_HEADERS} OPTIONS "-nw" )

set( GrimAudioFormatPluginMp3_ALL_SOURCES
	${GrimAudioFormatPluginMp3_SOURCES}
	${GrimAudioFormatPluginMp3_MOC_SOURCES}
)


# target
add_library( GrimAudioFormatPluginMp3 STATIC ${GrimAudioFormatPluginMp3_ALL_SOURCES} )
target_link_libraries(GrimAudioFormatPluginMp3 Qt5::Core)

if ( UNIX )
	set_target_properties( GrimAudioFormatPluginMp3 PROPERTIES COMPILE_FLAGS "-fPIC" )
endif()


# properties
set_property( TARGET GrimAudioFormatPluginMp3 PROPERTY
	LINK_INTERFACE_LIBRARIES ${Mpg123_LIBRARIES}
)
