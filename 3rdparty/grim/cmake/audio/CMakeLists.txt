
project( GrimAudio )


# paths
set( GrimAudio_DIR "${Grim_ROOT_DIR}/src/audio" )
set( GrimAudio_GENERATE_FORMAT_PLUGINS_SOURCE_FILE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/GrimAudioGenerateFormatPluginsSourceFile.cmake" )


# Qt
find_package( Qt5 REQUIRED Core )


# options
option( GrimAudio_DEBUG "Enable debugging for libGrimAudio" NO )
option( GrimAudio_LIBRARY_SHARED "Build as shared library" YES )
option( GrimAudio_BUILD_OPENAL  "Build OpenAL" YES )
option( GrimAudio_BUILD_FORMATS "Build audio formats" YES )
option( GrimAudio_BUILD_PLUGIN_WAVE   "Build Wave format support" YES )
option( GrimAudio_BUILD_PLUGIN_VORBIS "Build Ogg/Vorbis format support" YES )
option( GrimAudio_BUILD_PLUGIN_FLAC   "Build Flac format support" YES )
option( GrimAudio_BUILD_PLUGIN_MP3    "Build MP3 format support" YES )
option( GrimAudio_BUILD_PLUGIN_WAVE_OPTIONAL   "Whether Wave plugin is optional" YES )
option( GrimAudio_BUILD_PLUGIN_VORBIS_OPTIONAL "Whether Ogg/Vorbis plugin is optional" YES )
option( GrimAudio_BUILD_PLUGIN_FLAC_OPTIONAL   "Whether FLAC plugin is optional" YES )
option( GrimAudio_BUILD_PLUGIN_MP3_OPTIONAL    "Whether MP3 plugin is optional" YES )

if ( UNIX )
	option( GrimAudio_NULL_BUFFER_WORKAROUND "Enable workaround for openal-soft null buffer bug" ON )
	mark_as_advanced( FORCE GrimAudio_NULL_BUFFER_WORKAROUND )
endif()


# includes
include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )


# defines
if ( NOT GrimAudio_LIBRARY_SHARED )
	add_definitions( -DGRIM_AUDIO_STATIC )
else()
	add_definitions( -DGRIM_AUDIO_BUILD )
endif()
add_definitions( -DGRIM_TOOLS_STATIC )
add_definitions( -DGRIM_AUDIO_OPENAL_INCLUDE_DIR_PREFIX_${OPENAL_INCLUDE_DIR_PREFIX} )
if ( GrimAudio_DEBUG )
	add_definitions( -DGRIM_AUDIO_DEBUG )
endif()


# workaround for alSourcei( source_id, AL_BUFFER, 0 )
# openal-soft implementation fails to release processed buffers as OpenAL specification requires
if ( GrimAudio_NULL_BUFFER_WORKAROUND )
	add_definitions( -DGRIM_AUDIO_NULL_BUFFER_WORKAROUND )
endif()


# sources
my_add_sources( GrimAudioTools
	ROOT_DIR "${Grim_ROOT_DIR}/src/tools"
		IdGenerator
)

if ( GrimAudio_BUILD_OPENAL )
	# OpenAL
	find_package( OpenAL REQUIRED )

	# definitions
	add_definitions( -DGRIM_AUDIO_BUILD_OPENAL )

	my_add_sources( GrimAudioOpenAL
		ROOT_DIR "${GrimAudio_DIR}"
			Buffer
			BufferData
			BufferLoader
			CaptureDevice
			Context
			Device
			Listener
			Manager
			Math
			Source
			audio_p
	)
endif()

if ( GrimAudio_BUILD_FORMATS )
	my_add_sources( GrimAudioFormats
		ROOT_DIR "${GrimAudio_DIR}"
			FormatManager
			FormatPlugin
	)

	set( _audio_format_plugins )

	macro ( grim_audio_add_format_plugin PLUGIN )
		string( TOUPPER "${PLUGIN}" _plugin_upper )
		string( TOLOWER "${PLUGIN}" _plugin_lower )

		set( GrimAudio_FORMAT_PLUGIN_${_plugin_upper}_ENABLED NO CACHE BOOL "" FORCE )
		mark_as_advanced( FORCE GrimAudio_FORMAT_PLUGIN_${_plugin_upper}_ENABLED )

		if ( GrimAudio_BUILD_PLUGIN_${_plugin_upper} )
			add_subdirectory( "formats/${_plugin_lower}" )

			if ( GrimAudio_FORMAT_PLUGIN_${_plugin_upper}_ENABLED )
				list( APPEND _audio_format_plugins ${PLUGIN} )
			endif()
		endif()
	endmacro()

	grim_audio_add_format_plugin( Wave )
	grim_audio_add_format_plugin( Vorbis )
	grim_audio_add_format_plugin( Flac )
	grim_audio_add_format_plugin( Mp3 )

	set( GrimAudio_FORMAT_PLUGINS_SOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/grim_audio_format_plugins.cpp" CACHE FILEPATH "" FORCE )
	mark_as_advanced( GrimAudio_FORMAT_PLUGINS_SOURCE_FILE )

	string( REPLACE ";" ":" _colunmed_audio_format_plugins "${_audio_format_plugins}" )

	# rule to generate source file with list of Q_IMPORT_PLUGIN()
	add_custom_command( OUTPUT "${GrimAudio_FORMAT_PLUGINS_SOURCE_FILE}"
		COMMAND "${CMAKE_COMMAND}"
			-D "SOURCE_FILE=${GrimAudio_FORMAT_PLUGINS_SOURCE_FILE}"
			-D "PLUGINS=${_colunmed_audio_format_plugins}"
			-D "My_UTIL_SCRIPT=${My_UTIL_SCRIPT}"
			-P "${GrimAudio_GENERATE_FORMAT_PLUGINS_SOURCE_FILE_SCRIPT}"
		DEPENDS
			"${GrimAudio_GENERATE_FORMAT_PLUGINS_SOURCE_FILE_SCRIPT}"
	)

	add_custom_target( GrimAudioGenerateFormatPluginsSourceFile DEPENDS "${GrimAudio_FORMAT_PLUGINS_SOURCE_FILE}" )
endif()


qt5_wrap_cpp( GrimAudio_MOC_SOURCES ${GrimAudioTools_HEADERS} ${GrimAudioOpenAL_HEADERS} ${GrimAudioFormats_HEADERS} OPTIONS "-nw" )

set( GrimAudio_ALL_SOURCES ${GrimTools_SOURCES} ${GrimAudioOpenAL_SOURCES} ${GrimAudioFormats_SOURCES} ${GrimAudio_MOC_SOURCES} )

if ( GrimAudio_BUILD_FORMATS AND GrimAudio_LIBRARY_SHARED )
	list( APPEND GrimAudio_ALL_SOURCES "${GrimAudio_FORMAT_PLUGINS_SOURCE_FILE}" )
endif()


# target
if ( GrimAudio_LIBRARY_SHARED )
	set( _library_type "SHARED" )
else()
	set( _library_type "STATIC" )
endif()
add_library( GrimAudio ${_library_type} ${GrimAudio_ALL_SOURCES} )
target_link_libraries( GrimAudio Qt5::Core ${OPENAL_LIBRARY} )


# precompiled
grim_add_precompiled_headers_wrapper( GrimAudio "${GrimAudio_DIR}/PrecompiledHeaders.h" ${GrimAudio_ALL_SOURCES} )


# properties
set_target_properties( GrimAudio PROPERTIES
	VERSION "${Grim_VERSION}"
	SOVERSION "${Grim_SOVERSION}"
	OUTPUT_NAME GrimAudio
)


# export
my_get_target_libraries( GrimAudio _interface_library _runtime_library )
set( GrimAudio_LIBRARY "${_interface_library}" CACHE FILEPATH "" FORCE )


if ( GrimAudio_BUILD_FORMATS )
	get_target_property( _link_interface_libraries GrimAudio LINK_INTERFACE_LIBRARIES )
	if ( NOT _link_interface_libraries )
		set( _link_interface_libraries )
	endif()

	foreach( _audio_format_plugin ${_audio_format_plugins} )
		set( _target GrimAudioFormatPlugin${_audio_format_plugin} )
		target_link_libraries( GrimAudio "${_target}" )

		my_get_target_libraries( ${_target} _interface_library _runtime_library )
		list( APPEND _link_interface_libraries "${_interface_library}" )

		get_target_property( _plugin_link_interface_libraries ${_target} LINK_INTERFACE_LIBRARIES )
		if ( _plugin_link_interface_libraries )
			foreach( _plugin_link_interface_library ${_plugin_link_interface_libraries} )
				target_link_libraries( GrimAudio "${_plugin_link_interface_library}" )
				list( APPEND _link_interface_libraries "${_plugin_link_interface_library}" )
			endforeach()
		endif()
	endforeach()

	if ( NOT GrimAudio_LIBRARY_SHARED )
		set_target_properties( GrimAudio PROPERTIES LINK_INTERFACE_LIBRARIES "${_link_interface_libraries}" )
	endif()

	add_dependencies( GrimAudio GrimAudioGenerateFormatPluginsSourceFile )
endif()


# fill all grim sources
grim_add_sources( ${GrimAudio_SOURCES} )


# installation
install( TARGETS GrimAudio
	ARCHIVE DESTINATION "${Grim_ARCHIVE_DESTINATION}"
	LIBRARY DESTINATION "${Grim_LIBRARY_DESTINATION}"
	RUNTIME DESTINATION "${Grim_RUNTIME_DESTINATION}"
)
