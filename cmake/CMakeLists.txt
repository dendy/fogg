
cmake_minimum_required( VERSION 2.8 )


project( Fogg )


if ( "${CMAKE_BUILD_TYPE}" STREQUAL "" )
	set( CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE )
endif()


# paths
get_filename_component( Fogg_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE )
set( Fogg_3RDPARTY_DIR "${Fogg_DIR}/3rdparty" )
set( Fogg_TRANSLATIONS_DIR "${Fogg_DIR}/translations" )
set( Fogg_QM_TRANSLATIONS_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations" )


# My
set( CMAKE_MODULE_PATH "${Fogg_3RDPARTY_DIR}/grim/cmake/modules" )
find_package( My )


# options
option( Fogg_DEBUG "Enable Fogg debugging" NO )
option( Fogg_USE_PRECOMPILED_HEADERS "Build using precompiled headers" YES )
set( Fogg_TRANSLATION_LOCALES "ALL" CACHE STRING "Space separated list of locales to build. Use word 'ALL' to build all locales from 'translations' directory." )

set( _plugin_build_options "Optional" "Yes" "No" )
set( _plugin_build_default_option "Optional" )
my_create_list_cache_entry( Fogg_BUILD_WAVE_PLUGIN   "${_plugin_build_default_option}" "Whether to build Wave plugin"   ${_plugin_build_options} )
my_create_list_cache_entry( Fogg_BUILD_VORBIS_PLUGIN "${_plugin_build_default_option}" "Whether to build Vorbis plugin" ${_plugin_build_options} )
my_create_list_cache_entry( Fogg_BUILD_FLAC_PLUGIN   "${_plugin_build_default_option}" "Whether to build FLAC plugin"   ${_plugin_build_options} )
my_create_list_cache_entry( Fogg_BUILD_MP3_PLUGIN    "${_plugin_build_default_option}" "Whether to build Mp3 plugin"    ${_plugin_build_options} )


# global cflags
if ( CMAKE_COMPILER_IS_GNUCXX )
	set( _cflags "-Wno-multichar -Werror=return-type" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_cflags}" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_cflags}" )
endif()


# Grim
macro( fogg_setup_audio_format_plugin PLUGIN )
	string( TOUPPER "${PLUGIN}" _plugin_upper )
	if ( "${Fogg_BUILD_${_plugin_upper}_PLUGIN}" STREQUAL "Yes" )
		set( _build_plugin YES )
		set( _build_is_optional NO )
	elseif ( "${Fogg_BUILD_${_plugin_upper}_PLUGIN}" STREQUAL "No" )
		set( _build_plugin NO )
		set( _build_is_optional NO )
	else()
		set( _build_plugin YES )
		set( _build_is_optional YES )
	endif()
	set( GrimAudio_BUILD_PLUGIN_${_plugin_upper} ${_build_plugin} CACHE BOOL "" FORCE )
	set( GrimAudio_BUILD_PLUGIN_${_plugin_upper}_OPTIONAL ${_build_is_optional} CACHE BOOL "" FORCE )
	mark_as_advanced( FORCE GrimAudio_BUILD_PLUGIN_${_plugin_upper} GrimAudio_BUILD_PLUGIN_${_plugin_upper}_OPTIONAL )
endmacro()

if ( Fogg_USE_PRECOMPILED_HEADERS )
	set( Grim_USE_PRECOMPILED_HEADERS YES CACHE BOOL "" FORCE )
endif()

if ( Fogg_DEBUG )
	set( GrimAudio_DEBUG YES CACHE BOOL "" FORCE )
	set( GrimTools_DEBUG YES CACHE BOOL "" FORCE )
endif()

set( Grim_BUILD_MODULE_AUDIO       YES CACHE BOOL "" FORCE )
set( GrimAudio_LIBRARY_SHARED      NO  CACHE BOOL "" FORCE )
set( GrimAudio_BUILD_OPENAL        NO  CACHE BOOL "" FORCE )
set( GrimAudio_BUILD_FORMATS       YES CACHE BOOL "" FORCE )
fogg_setup_audio_format_plugin( Wave )
fogg_setup_audio_format_plugin( Vorbis )
fogg_setup_audio_format_plugin( Flac )
fogg_setup_audio_format_plugin( Mp3 )

set( Grim_BUILD_MODULE_TOOLS YES CACHE BOOL "" FORCE )
set( GrimTools_LIBRARY_SHARED NO CACHE BOOL "" FORCE )

mark_as_advanced( FORCE
	Grim_USE_PRECOMPILED_HEADERS
	Grim_TRANSLATIONS
	Grim_BUILD_MODULE_GAME
	Grim_BUILD_MODULE_ARCHIVE
	Grim_BUILD_MODULE_AUDIO
	Grim_BUILD_MODULE_TOOLS
	GrimAudio_DEBUG
	GrimTools_DEBUG
	GrimAudio_LIBRARY_SHARED
	GrimAudio_BUILD_OPENAL
	GrimAudio_BUILD_FORMATS
	GrimTools_LIBRARY_SHARED
)

add_subdirectory( "${Fogg_3RDPARTY_DIR}/grim/cmake" "grim" )


# Qt
find_package( Qt4 REQUIRED )
if ( WIN32 )
	set( QT_USE_QTMAIN YES )
endif()
include( "${QT_USE_FILE}" )


# Ogg/Vorbis, FLAC, mpg123
find_package( Ogg REQUIRED )
find_package( Vorbis REQUIRED )


# save build configuration header
macro( fogg_check_audio_format_plugin PLUGIN )
	string( TOUPPER "${PLUGIN}" _plugin_upper )
	if ( GrimAudio_FORMAT_PLUGIN_${_plugin_upper}_ENABLED )
		set( Fogg_HAS_AUDIO_${_plugin_upper}_FORMAT_PLUGIN "true" )
	else()
		set( Fogg_HAS_AUDIO_${_plugin_upper}_FORMAT_PLUGIN "false" )
	endif()
endmacro()

fogg_check_audio_format_plugin( Wave )
fogg_check_audio_format_plugin( Vorbis )
fogg_check_audio_format_plugin( Flac )
fogg_check_audio_format_plugin( Mp3 )

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/FoggBuildConfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/FoggBuildConfig.h" )


# includes
include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${Fogg_3RDPARTY_DIR}/grim/include"
	"${Fogg_DIR}/src/promoted"
)


# definitions
add_definitions( -DGRIM_TOOLS_STATIC -DGRIM_AUDIO_STATIC )

if ( Fogg_DEBUG )
	add_definitions( -DFOGG_DEBUG -DGRIM_AUDIO_DEBUG -DGRIM_TOOLS_DEBUG )
endif()


# sources
my_add_sources( Fogg
	ROOT_DIR "${Fogg_DIR}/src"
		AboutDialog
		Config
		Converter
		DonationDialog
		EncodingQualityWidget
		FileFetcher
		FileFetcherDialog
		Global
		JobItemModel
		main.cpp
		MainWindow
		NonRecognizedFilesDialog
		PoweredByWidget
		PreferencesDialog
		ButtonActionBinder
		SkippedFilesDialog
		TargetNameDialog

	ROOT_DIR "${Fogg_DIR}/ui"
		AboutDialog
		DonationDialog
		EncodingQualityWidget
		FileFetcherDialog
		MainWindow
		NonRecognizedFilesDialog
		PoweredByWidget
		PreferencesDialog
		SkippedFilesDialog
		TargetNameDialog

	ROOT_DIR "${Fogg_DIR}/res"
		fogg.qrc
)


# generated
qt4_wrap_cpp( Fogg_MOC_SOURCES ${Fogg_HEADERS} OPTIONS -nw )
qt4_wrap_ui( Fogg_UI_HEADERS ${Fogg_FORMS} )
qt4_add_resources( Fogg_RCC_SOURCES ${Fogg_RESOURCES} )


# all sources
set_source_files_properties( "${GrimAudio_FORMAT_PLUGINS_SOURCE_FILE}" PROPERTIES GENERATED YES )
set( Fogg_ALL_SOURCES ${Fogg_SOURCES} ${Fogg_MOC_SOURCES} ${Fogg_RCC_SOURCES} "${GrimAudio_FORMAT_PLUGINS_SOURCE_FILE}" )


# target
add_executable( Fogg ${Fogg_UI_HEADERS} ${Fogg_ALL_SOURCES} )


# link
target_link_libraries( Fogg ${Grim_LIBRARIES} ${QT_LIBRARIES} ${Ogg_LIBRARIES} ${Vorbis_LIBRARIES} )


# precompiled
if ( Fogg_USE_PRECOMPILED_HEADERS )
	set( Fogg_PRECOMPILED_SOURCES ${Fogg_SOURCES} ${Fogg_MOC_SOURCES} ${Fogg_RCC_SOURCES} )
	my_add_precompiled_headers( Fogg "${Fogg_DIR}/src/PrecompiledHeaders.h" ${Fogg_PRECOMPILED_SOURCES} )
endif()


# dependencies
add_dependencies( Fogg ${Grim_TARGETS} )


# localization
set( Fogg_RESOLVED_TRANSLATION_LOCALES )
set( Fogg_TS_TARGETS )
set( Fogg_TS_NO_OBSOLETED_TARGETS )
set( Fogg_QM_TARGETS )

# collect list of locales
if ( "${Fogg_TRANSLATION_LOCALES}" STREQUAL "ALL" )
	# locate all available locales manually by looking for info.ts files
	# under the translation/${locale} directories
	file( GLOB _locale_dirs RELATIVE "${Fogg_TRANSLATIONS_DIR}" "${Fogg_TRANSLATIONS_DIR}/*" )
	foreach ( _locale ${_locale_dirs} )
		if ( EXISTS "${Fogg_TRANSLATIONS_DIR}/${_locale}/info.ts" )
			list( APPEND Fogg_RESOLVED_TRANSLATION_LOCALES "${_locale}" )
		endif()
	endforeach()
else()
	set( _locales ${Fogg_TRANSLATION_LOCALES} )
	separate_arguments( _locales )
	foreach ( _locale ${_locales} )
		list( APPEND Fogg_RESOLVED_TRANSLATION_LOCALES "${_locale}" )
	endforeach()
endif()

# collect list of modules to translate
# currently Fogg contains of two such modules:
# 1. "info" - tiny modules that contains only information about locale
# 2. "fogg" - solid module with all application localization strings
set( TRANSLATION_SOURCES_FOR_MODULE_info "${Fogg_3RDPARTY_DIR}/grim/src/tools/LocalizationManager.cpp" )
set( TRANSLATION_SOURCES_FOR_MODULE_fogg ${Fogg_ALL_SOURCES} ${Fogg_FORMS} )
list( REMOVE_ITEM TRANSLATION_SOURCES_FOR_MODULE_fogg ${TRANSLATION_SOURCES_FOR_MODULE_info} )

# create rules for translations
foreach ( _locale ${Fogg_RESOLVED_TRANSLATION_LOCALES} )
	set( _ts_dir "${Fogg_TRANSLATIONS_DIR}/${_locale}" )
	set( _qm_dir "${Fogg_QM_TRANSLATIONS_DIR}/${_locale}" )

	foreach ( _module info fogg )
		set( _ts_file "${_ts_dir}/${_module}.ts" )
		set( _qm_file "${_qm_dir}/${_module}.qm" )

		my_create_translation( "${_ts_file}" "${_qm_file}"
			_ts_target
			_ts_no_obsoleted_target
			${TRANSLATION_SOURCES_FOR_MODULE_${_module}}
			OPTIONS -silent )

		list( APPEND Fogg_TS_TARGETS "${_ts_target}" )
		list( APPEND Fogg_TS_NO_OBSOLETED_TARGETS "${_ts_no_obsoleted_target}" )
		list( APPEND Fogg_QM_TARGETS "${_qm_file}" )
	endforeach()
endforeach()

# let .qm files update self automatically on .ts change
add_custom_target( FoggTranslations ALL DEPENDS ${Fogg_QM_TARGETS} )


# custom target to manually update .ts files from command line
# run manually 'make translations' to update .ts files from sources
add_custom_target( translations DEPENDS ${Fogg_TS_TARGETS} )
add_custom_target( translations-without-obsoleted DEPENDS ${Fogg_TS_NO_OBSOLETED_TARGETS} )
