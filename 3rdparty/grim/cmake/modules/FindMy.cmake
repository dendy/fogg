
set( My_FOUND YES )


# paths
get_filename_component( My_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH )

set( My_UTIL_SCRIPT "${My_DIR}/MyUtil.cmake" CACHE FILEPATH "" FORCE )
set( My_GENERATE_QT_TRANSLATIONS_PRO_SCRIPT "${My_DIR}/MyGenerateQtTranslationsPro.cmake" CACHE FILEPATH "" FORCE )
set( My_UPDATE_TS_FILE_SCRIPT "${My_DIR}/MyUpdateTsFile.cmake" CACHE FILEPATH "" FORCE )

mark_as_advanced(
	My_UTIL_SCRIPT
	My_GENERATE_QT_TRANSLATIONS_PRO_SCRIPT
	My_UPDATE_TS_FILE_SCRIPT
)




# util macros
include( "${My_UTIL_SCRIPT}" )




# Macro for creating rules to generate Qt translations.
#
# Usage:
#
#   my_create_translation( TS_FILE QM_FILE TS_TARGETS [source1 source2 ...] )
#
#   - TS_FILE    - in variable, pass .ts file to update and from which .qm file should be generated
#   - QM_FILE    - in variable, here will be generated .qm file
#   - TS_TARGETS - out variable, here will be added target to update .ts file
#
# Two steps here, each can be done separately:
#   1. Update .ts files from sources/forms
#   2. Generate .qm files from .ts files
#
# This macro also workarounds CMake issue, when .ts files treated as compilation
# product and will be physically removed from drive on "make clean" step.
#
# .ts files are not compilation products, they are not generated inside build
# directory. .qm files does - they could be completely removed on "make clean".
#
# Because .ts files should be created separately and should be not included
# into build step - separate target should be created by user for convenience.
#
# Example:
#
#   my_create_translation( qm_file ts_targets myapp.ts mysource.cpp myform.ui ... )
#   add_custom_target( update_my_translations DEPENDS ${ts_targets} )
#
# Updating .ts files can be issued in two ways:
# 1. run "make update_my_translations" (recommended)
# 2. add update_my_translations target as dependency to build process, this will issue .ts
#    updates automatically each time build runs.

macro( my_create_translation TS_FILE QM_FILE TS_TARGET )

	# check for lupdate and lrelease availability
	if ( NOT QT_LUPDATE_EXECUTABLE OR NOT QT_LRELEASE_EXECUTABLE )
		message( FATAL_ERROR "lupdate and lrelease executables not found. Your Qt is probably built with '-nomake tools' option" )
	endif()

	# paths
	get_filename_component( _qm_dir "${QM_FILE}" PATH )

	get_filename_component( _ts_file_absolute "${TS_FILE}" ABSOLUTE )
	get_filename_component( _ts_file_dir "${_ts_file_absolute}" PATH )
	get_filename_component( _ts_filename "${TS_FILE}" NAME )
	get_filename_component( _ts_filename_we "${TS_FILE}" NAME_WE )

	# targets
	set( _ts_file_target "${_qm_dir}/${_ts_filename}.target" )

	# rule to generate .pro file
	set( _pro_file "${_qm_dir}/${_ts_filename_we}.pro" )

	qt4_extract_options( _source_files _lupdate_options ${ARGN} )
	string( REPLACE ";" ":" _columned_source_files "${_source_files}" )

	add_custom_command( OUTPUT "${_pro_file}"
		COMMAND "${CMAKE_COMMAND}"
			-D "PRO_FILE=${_pro_file}"
			-D "SOURCE_FILES=${_columned_source_files}"
			-D "TS_FILE=${TS_FILE}"
			-D "My_UTIL_SCRIPT=${My_UTIL_SCRIPT}"
			-P "${My_GENERATE_QT_TRANSLATIONS_PRO_SCRIPT}"
		DEPENDS
			"${My_GENERATE_QT_TRANSLATIONS_PRO_SCRIPT}"
			${_source_files}
	)

	# rule to lupdate .ts file
	separate_arguments( _lupdate_options )
	string( REPLACE ";" ":" _columned_lupdate_options "${_lupdate_options}" )

	add_custom_command( OUTPUT "${_ts_file_target}"
		COMMAND "${CMAKE_COMMAND}"
			-D "PRO_FILE=${_pro_file}"
			-D "TS_FILE_TARGET=${_ts_file_target}"
			-D "TS_FILE=${TS_FILE}"
			-D "LUPDATE_COMMAND=${QT_LUPDATE_EXECUTABLE}"
			-D "LUPDATE_OPTIONS=${_columned_lupdate_options}"
			-D "My_UTIL_SCRIPT=${My_UTIL_SCRIPT}"
			-P "${My_UPDATE_TS_FILE_SCRIPT}"
		DEPENDS
			"${_pro_file}"
			${My_UPDATE_TS_FILE_SCRIPT}
	)

	set( ${TS_TARGET} "${_ts_file_target}" )

#	# run lupdate once if .ts file not exists
#	if ( NOT EXISTS "${_ts_file_absolute}" )
#		my_make_file_directory( "${_ts_file_absolute}" )
#		execute_process( COMMAND "${QT_LUPDATE_EXECUTABLE}" ${_lupdate_options} "${_pro_file}" -ts "${_ts_file_absolute}" )
#	endif()

	# rule to generate .qm from .ts
	add_custom_command( OUTPUT "${QM_FILE}"
		COMMAND
			"${CMAKE_COMMAND}" -E make_directory "${_qm_dir}"
		COMMAND
			"${QT_LRELEASE_EXECUTABLE}" -silent "${TS_FILE}" -qm "${QM_FILE}"
		DEPENDS
			"${TS_FILE}"
	)

endmacro()




# Macro that adds precompiled headers to the target.
#
# Usage:
#
#   my_add_precompiled_headers( TARGET PRECOMPILED_HEADER [source1 source2 ...] )
#
#   - TARGET             - in variable, target to add precompiled headers support to
#   - PRECOMPILED_HEADER - in variable, path to header to precompile
#   - [sources ...]      - in variables, list of sources to add precompiled header usage to,
#                          usually, these are all target sources, but could be exceptions.
#
# Example:
#
#   add_executable( myapp ${mysources} )
#   my_add_precompiled_headers( myapp precompiled_headers.h ${mysources} )

macro( my_add_precompiled_headers TARGET PRECOMPILED_HEADER )

	# check that CMAKE_BUILD_TYPE was set
	if ( NOT CMAKE_BUILD_TYPE )
		message( FATAL_ERROR
			"No CMAKE_BUILD_TYPE specified for target: ${TARGET}\n" )
	endif()

	set( _additional_flags )

	# build type as suffix to cmake variables
	string( TOUPPER ${CMAKE_BUILD_TYPE} _build_type )

	# file names
	set( _pch_file_name "PrecompiledHeaders" )
	set( _pch_file_dir "${CMAKE_CURRENT_BINARY_DIR}/pch" )
	if ( MSVC )
		set( _pch_ext "pch" )
	else()
		set( _pch_ext "gch" )
	endif()
	set( _pch_file_path "${_pch_file_dir}/${_pch_file_name}.${_pch_ext}" )
	get_filename_component( _ph_absolute_path "${PRECOMPILED_HEADER}" ABSOLUTE )

	# MSVC requires additionally object file
	if ( MSVC )
		set( _pch_object_file_path "${_pch_file_dir}/${_pch_file_name}.obj" )
	endif()

	get_target_property( _target_type ${TARGET} TYPE )
	# TODO: Useless?
	if( ${_target_type} STREQUAL "SHARED_LIBRARY" )
		list( APPEND _additional_flags ${CMAKE_SHARED_LIBRARY_CXX_FLAGS} )
	endif()

	if ( APPLE )
		# add min deployment version
		list( APPEND _additional_flags -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} )

		# add architectures
		get_target_property( _archs ${TARGET} OSX_ARCHITECTURES )
		foreach ( _arch ${_archs} )
			list( APPEND _additional_flags -arch ${_arch} )
		endforeach()

		# add sysroot
		set( _additional_flags ${_additional_flags} -isysroot "${CMAKE_OSX_SYSROOT}" )
	endif()

	# gathering include information into _include_flags
	set( _include_flags )
	get_directory_property( _include_directories INCLUDE_DIRECTORIES )
	foreach ( _include ${_include_directories} )
		if ( MSVC )
			list( APPEND _include_flags "/I${_include}" )
		else()
			list( APPEND _include_flags "-I${_include}" )
		endif()
	endforeach()

	# gathering compile flags into _compile_flags
	set( _compile_flags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_build_type}} )
	separate_arguments( _compile_flags )
	get_target_property( _target_compile_flags ${TARGET} COMPILE_FLAGS )
	if ( _target_compile_flags )
		set( _compile_flags ${_compile_flags} ${_target_compile_flags} )
	endif()

	separate_arguments( _compile_flags )

	# Gathering definition flags into _definition_flags
	set( _definition_flags )
	get_directory_property( _definitions COMPILE_DEFINITIONS )
	foreach( _def ${_definitions} )
		string( REPLACE "\"" "\\\"" _def "${_def}" )
		list( APPEND _definition_flags "-D${_def}" )
	endforeach()
	get_directory_property( _definitions "COMPILE_DEFINITIONS_${_build_type}" )
	foreach( _def ${_definitions} )
		string( REPLACE "\"" "\\\"" _def "${_def}" )
		list( APPEND _definition_flags "-D${_def}" )
	endforeach()

	# generate precompiled header
	if ( MSVC )
		# HACK: Don't know how to obtain target's pdb file.
		get_target_property( _target_location ${TARGET} ${_build_type}_LOCATION )
		get_filename_component( _target_filename "${_target_location}" NAME )
		my_remove_extension( _target_filename_we "${_target_filename}" )

		my_get_target_output_directories( ${TARGET} _interface_directory _runtime_directory )
		if ( _target_type STREQUAL "EXECUTABLE" )
			set( _target_pdb_directory "${_runtime_directory}" )
		elseif ( _target_type STREQUAL "STATIC_LIBRARY" )
			set( _target_pdb_directory "${_interface_directory}" )
		else()
			set( _target_pdb_directory "${_runtime_directory}" )
		endif()

		set( _target_pdb "${_target_pdb_directory}/${_target_filename_we}.pdb" )

		add_custom_command(
			OUTPUT "${_pch_object_file_path}"
			COMMAND "${CMAKE_COMMAND}" -E remove -f "${_pch_object_file_path}"
			COMMAND "${CMAKE_COMMAND}" -E remove -f "${_target_pdb}"
			COMMAND "${CMAKE_COMMAND}" -E remove -f "${_pch_file_path}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${_pch_file_dir}"
			COMMAND "${CMAKE_CXX_COMPILER}" ${_include_flags} ${_compile_flags} ${_definition_flags} ${_additional_flags}
				-c /Yc /Fp${_pch_file_path} /Fo${_pch_object_file_path} /Fd${_target_pdb} /TP "${_ph_absolute_path}"
			IMPLICIT_DEPENDS CXX "${_ph_absolute_path}"
		)
#		target_link_libraries( ${TARGET} "${_pch_object_file_path}" )
	else()
		add_custom_command(
			OUTPUT "${_pch_file_path}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${_pch_file_dir}"
			COMMAND "${CMAKE_CXX_COMPILER}" ${_include_flags} ${_compile_flags} ${_definition_flags} ${_additional_flags}
				-x c++-header -c "${_ph_absolute_path}" -o "${_pch_file_path}"
			IMPLICIT_DEPENDS CXX "${_ph_absolute_path}"
		)
	endif()

	# adding global compile flags to the target
	# if no sources was given than PrecompiledHeader file will not be created, so we should skip dependency
	foreach ( _source_file ${ARGN} )
		get_source_file_property( _compile_flags "${_source_file}" COMPILE_FLAGS )

		if ( NOT _compile_flags )
			set( _compile_flags )
		endif()

		if ( MSVC )
			set_source_files_properties( "${_source_file}" PROPERTIES
				COMPILE_FLAGS "${_compile_flags} -FI${PRECOMPILED_HEADER} -Yu${PRECOMPILED_HEADER} -Fp${_pch_file_path}"
				OBJECT_DEPENDS "${_pch_object_file_path}"
			)
		else()
			set_source_files_properties( "${_source_file}" PROPERTIES
				COMPILE_FLAGS "${_compile_flags} -include ${_pch_file_name} -Winvalid-pch"
				OBJECT_DEPENDS "${_pch_file_path}"
			)
		endif()
	endforeach()

	include_directories( BEFORE "${_pch_file_dir}" )

endmacro()




macro( my_get_target_output_directories TARGET INTERFACE_DIRECTORY RUNTIME_DIRECTORY )
	get_target_property( _target_type ${TARGET} TYPE )

	if ( "${_target_type}" STREQUAL "STATIC_LIBRARY" )
		get_target_property( _target_interface_output_dir ${TARGET} ARCHIVE_OUTPUT_DIRECTORY )
		if ( NOT _target_interface_output_dir )
			set( _target_interface_output_dir ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
		endif()

		set( ${INTERFACE_DIRECTORY} "${_target_interface_output_dir}" )
		set( ${RUNTIME_DIRECTORY} "" )
	elseif ( "${_target_type}" STREQUAL "SHARED_LIBRARY" OR "${_target_type}" STREQUAL "EXECUTABLE" )
		if ( WIN32 )
			get_target_property( _target_runtime_output_dir ${TARGET} RUNTIME_OUTPUT_DIRECTORY )
			if ( NOT _target_runtime_output_dir )
				set( _target_runtime_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
			endif()
			if ( NOT _target_runtime_output_dir )
				set( _target_runtime_output_dir ${CMAKE_CURRENT_BINARY_DIR} )
			endif()

			get_target_property( _target_interface_output_dir ${TARGET} ARCHIVE_OUTPUT_DIRECTORY )
			if ( NOT _target_interface_output_dir )
				set( _target_interface_output_dir ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
			endif()
			if ( NOT _target_interface_output_dir )
				set( _target_interface_output_dir ${CMAKE_CURRENT_BINARY_DIR} )
			endif()
		else ( WIN32 )
			get_target_property( _target_runtime_output_dir ${TARGET} LIBRARY_OUTPUT_DIRECTORY )
			if ( NOT _target_runtime_output_dir )
				set( _target_runtime_output_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
			endif()

			set( _target_interface_output_dir ${_target_runtime_output_dir} )
		endif()

		set( ${INTERFACE_DIRECTORY} "${_target_interface_output_dir}" )
		set( ${RUNTIME_DIRECTORY} "${_target_runtime_output_dir}" )
	else()
		message( FATAL_ERROR "Resolving output directories allowed only for static, shared libraries and executables" )
	endif()
endmacro()




macro( my_get_target_libraries TARGET INTERFACE_LIBRARY RUNTIME_LIBRARY )
	get_target_property( _target_type ${TARGET} TYPE )
	get_target_property( _target_output_name ${TARGET} OUTPUT_NAME )
	get_target_property( _target_prefix ${TARGET} PREFIX )
	get_target_property( _target_suffix ${TARGET} SUFFIX )

	if ( NOT _target_output_name )
		set( _target_output_name ${TARGET} )
	endif()

	if ( "${_target_type}" STREQUAL "STATIC_LIBRARY" )
		set( _target_interface_prefix ${_target_prefix} )
		if ( NOT _target_interface_prefix )
			set( _target_interface_prefix ${CMAKE_STATIC_LIBRARY_PREFIX} )
		endif()

		set( _target_interface_suffix ${_target_suffix} )
		if ( NOT _target_interface_suffix )
			set( _target_interface_suffix ${CMAKE_STATIC_LIBRARY_SUFFIX} )
		endif()

		get_target_property( _target_interface_output_dir ${TARGET} ARCHIVE_OUTPUT_DIRECTORY )
		if ( NOT _target_interface_output_dir )
			set( _target_interface_output_dir ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
		endif()

		set( ${INTERFACE_LIBRARY} "${_target_interface_output_dir}/${_target_interface_prefix}${_target_output_name}${_target_interface_suffix}" )
		set( ${RUNTIME_LIBRARY} "" )
	elseif ( "${_target_type}" STREQUAL "SHARED_LIBRARY" )
		set( _target_runtime_prefix ${_target_prefix} )
		if ( NOT _target_runtime_prefix )
			set( _target_runtime_prefix ${CMAKE_SHARED_LIBRARY_PREFIX} )
		endif()

		get_target_property( _target_runtime_suffix ${TARGET} SUFFIX )
		if ( NOT _target_runtime_suffix )
			set( _target_runtime_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX} )
		endif()

		if ( WIN32 )
			get_target_property( _target_runtime_output_dir ${TARGET} RUNTIME_OUTPUT_DIRECTORY )
			if ( NOT _target_runtime_output_dir )
				set( _target_runtime_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
			endif()

			if ( MSVC )
				set( _target_interface_prefix "${_target_runtime_prefix}" )
				set( _target_interface_suffix "${CMAKE_STATIC_LIBRARY_SUFFIX}" )
			else()
				set( _target_interface_prefix "${CMAKE_SHARED_LIBRARY_PREFIX}" )
				set( _target_interface_suffix "${CMAKE_SHARED_LIBRARY_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}" )
			endif()

			get_target_property( _target_interface_output_dir ${TARGET} ARCHIVE_OUTPUT_DIRECTORY )
			if ( NOT _target_interface_output_dir )
				set( _target_interface_output_dir ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
			endif ( NOT _target_interface_output_dir )
		else ( WIN32 )
			get_target_property( _target_runtime_output_dir ${TARGET} LIBRARY_OUTPUT_DIRECTORY )
			if ( NOT _target_runtime_output_dir )
				set( _target_runtime_output_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
			endif()

			set( _target_interface_prefix ${_target_runtime_prefix} )
			set( _target_interface_suffix ${_target_runtime_suffix} )
			set( _target_interface_output_dir ${_target_runtime_output_dir} )
		endif()

		set( ${INTERFACE_LIBRARY} "${_target_interface_output_dir}/${_target_interface_prefix}${_target_output_name}${_target_interface_suffix}" )
		set( ${RUNTIME_LIBRARY} "${_target_runtime_output_dir}/${_target_runtime_prefix}${_target_output_name}${_target_runtime_suffix}" )
	else()
		message( FATAL_ERROR "Resolving library name allowed only for static and shared library targets" )
	endif()
endmacro()




# Locate source files with filename FILENAME and place them into
# appropriate lists with prefix PREFIX:
#
#   *.h         - ${PREFIX}_HEADERS and ${PREFIX}_H_HEADERS
#   *.hpp       - ${PREFIX}_HEADERS and ${PREFIX}_HPP_HEADERS
#   *.c         - ${PREFIX}_SOURCES and ${PREFIX}_C_SOURCES
#   *.cpp *.cxx - ${PREFIX}_SOURCES and ${PREFIX}_CPP_SOURCES
#   *.m *.mm    - ${PREFIX}_SOURCES and ${PREFIX}_OBJC_SOURCES
#   *.ui        - ${PREFIX}_FORMS
#   *.qrc       - ${PREFIX}_RESOURCES

macro( my_add_sources PREFIX )

	set( _is_setting_root_dir )
	set( _is_reading_files )
	set( _current_root_dir )

	set( _h_lists   "h"   HEADERS H_HEADERS )
	set( _hpp_lists "hpp" HEADERS_HPP_HEADERS )
	set( _c_lists   "c"   SOURCES C_SOURCES )
	set( _cpp_lists "cpp" SOURCES CPP_SOURCES )
	set( _cxx_lists "cxx" SOURCES CPP_SOURCES )
	set( _m_lists   "m"   SOURCES OBJC_SOURCES )
	set( _mm_lists  "mm"  SOURCES OBJC_SOURCES )
	set( _ui_lists  "ui"  FORMS )
	set( _qrc_lists "qrc" RESOURCES )
	set( _lists _h_lists _hpp_lists _c_lists _cpp_lists _cxx_lists _m_lists _mm_lists
		_ui_lists _qrc_lists )

	foreach( _cmd ${ARGN} )
		set( _continue )

		if ( "${_cmd}" STREQUAL "ROOT_DIR" )
			set( _continue YES )
			set( _is_setting_root_dir YES )
		endif()

		if ( NOT _continue )
			if ( _is_setting_root_dir )
				set( _continue YES )
				set( _is_setting_root_dir )
				set( _current_root_dir "${_cmd}" )
				if ( NOT EXISTS "${_current_root_dir}" )
					message( FATAL_ERROR "Directory not exists: ${_current_root_dir}" )
				endif()
			endif()
		endif()

		if ( NOT _continue )
			if ( NOT _current_root_dir )
				message( FATAL_ERROR "ROOT_DIR was not specified" )
			endif()

			set( _file "${_current_root_dir}/${_cmd}" )
			get_filename_component( _file_ext "${_file}" EXT )

			if ( NOT _file_ext )
				# no extension given, find all possible files
				set( _found )
				foreach ( _list ${_lists} )
					list( GET ${_list} 0 _list_ext )
					set( _file_path "${_file}.${_list_ext}" )

					if ( EXISTS "${_file_path}" )
						set( _found YES )

						list( LENGTH ${_list} _list_length )
						set( _list_index 1 )
						while ( _list_index LESS _list_length )
							list( GET ${_list} ${_list_index} _list_name )
							list( APPEND ${PREFIX}_${_list_name} "${_file_path}" )
							math( EXPR _list_index "${_list_index} + 1" )
						endwhile()
					endif()
				endforeach()
				if ( NOT _found )
					message( FATAL_ERROR "No source files found for file name: ${_file}" )
				endif()
			else()
				set( _found )
				foreach ( _list ${_lists} )
					if ( NOT _found )
						list( GET ${_list} 0 _list_ext )
						if ( "${_file_ext}" STREQUAL ".${_list_ext}" )
							set( _found YES )

							if ( NOT EXISTS "${_file}" )
								message( FATAL_ERROR "File not found: ${_file}" )
							endif()

							list( LENGTH ${_list} _list_length )
							set( _list_index 1 )
							while ( _list_index LESS _list_length )
								list( GET ${_list} ${_list_index} _list_name )
								list( APPEND ${PREFIX}_${_list_name} "${_file}" )
								math( EXPR _list_index "${_list_index} + 1" )
							endwhile()
						endif()
					endif()
				endforeach()

				if ( NOT _found )
					message( FATAL_ERROR "Source file not recognized: ${_file}" )
				endif()
			endif()
		endif()
	endforeach()

endmacro()




macro( my_check_for_valid_project )
	if ( NOT PROJECT_NAME )
		message( FATAL_ERROR "PROJECT_NAME variable not set, use project() command on parent scope to specify project name." )
	endif()
endmacro()




macro( my_remove_include_deps TARGET )

	set( _deps_to_remove )

	set( _directory )
	set( _hasDirectoryFlag NO )
	foreach ( _var ${ARGN} )
		set( _continue )

		if ( _hasDirectoryFlag )
			set( _directory "${_var}" )
			set( _hasDirectoryFlag NO )
			set( _continue YES )
		endif()

		if ( NOT _continue )
			if ( "${_var}" STREQUAL "DIRECTORY" )
				set( _hasDirectoryFlag YES )
				set( _continue YES )
			endif()
		endif()

		if ( NOT _continue )
			list( APPEND _deps_to_remove "${_var}" )
		endif()
	endforeach()

	if ( NOT _directory )
		set( _directory "." )
	endif()

	# do nothing if list is empty
	list( LENGTH _deps_to_remove _deps_count )
	if ( _deps_count EQUAL 0 )
		return()
	endif()

	# get original includes from directory
	get_directory_property( _include_directories DIRECTORY "${_directory}" INCLUDE_DIRECTORIES )

	# list all matched includes into _deps_to_add
	set( _deps_to_add )
	foreach ( _dep_to_remove ${_deps_to_remove} )
		list( FIND _include_directories "${_dep_to_remove}" _index )
		if ( NOT _index EQUAL -1 )
			list( REMOVE_AT _include_directories ${_index} )
			list( APPEND _deps_to_add "${_dep_to_remove}" )
		endif()
	endforeach()

	# do nothing if no matched includes
	list( LENGTH _deps_to_remove _deps_count )
	if ( _deps_count EQUAL 0 )
		return()
	endif()

	# replace directory includes with list cleaned from _deps_to_remove
	set_property( DIRECTORY "${_directory}" PROPERTY INCLUDE_DIRECTORIES "${_include_directories}" )

	# collect string of includes compatible to pass into COMPILE_FLAGS property of target
	set( _cflags )
	set( _is_first_dep YES )
	foreach( _dep_to_add ${_deps_to_add} )
		if ( _is_first_dep )
			set( _cflags "-I${_dep_to_add}" )
			set( _is_first_dep NO )
		else()
			set( _cflags "${_cflags} -I${_dep_to_add}" )
		endif()
	endforeach()

	# append original target COMPILE_FLAGS property
	get_target_property( _target_cflags ${TARGET} COMPILE_FLAGS )
	if ( _target_cflags )
		set( _cflags "${_target_cflags} ${_cflags}" )
	endif()

	# replace target COMPILE_FLAGS propertiy
	set_target_properties( ${TARGET} PROPERTIES COMPILE_FLAGS "${_cflags}" )
endmacro()




function( my_create_macosx_bundle TARGET )

	# do nothing on non-apple platforms
	if ( NOT APPLE )
		return()
	endif()

	get_target_property( _target_type ${TARGET} TYPE )

	if ( "${_target_type}" STREQUAL EXECUTABLE )
	elseif( "${_target_type}" STREQUAL SHARED_LIBRARY )
	else()
		message( FATAL_ERROR "Target type is not supported to create bundle. Target: ${TARGET}" )
	endif()

	# resolve version
	set( _target_version "1" )

	# resolve output name
	get_target_property( _target_output_name ${TARGET} OUTPUT_NAME )
	if ( NOT _target_output_name )
		set( _target_output_name "${TARGET}" )
	endif()

	# resolve suffix
	get_target_property( _target_suffix ${TARGET} SUFFIX )

	# resolve bundle dir base name
	get_target_property( _bundle_dir_base_name ${TARGET} MACOSX_BUNDLE_DIR_BASENAME )
	if ( NOT _bundle_dir_base_name )
		set( _bundle_dir_base_name "${_target_output_name}" )
	endif()

	# resolve bundle dir suffix
	get_target_property( _bundle_dir_suffix ${TARGET} MACOSX_BUNDLE_DIR_SUFFIX )
	if ( NOT _bundle_dir_suffix )
		if ( "${_target_type}" STREQUAL EXECUTABLE )
			set( _bundle_dir_suffix ".app" )
		elseif( "${_target_type}" STREQUAL SHARED_LIBRARY )
			set( _bundle_dir_suffix ".plugin" )
		endif()
	endif()

	# resolve output dir
	if ( "${_target_type}" STREQUAL EXECUTABLE )
		set( _bundle_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" )
	elseif( "${_target_type}" STREQUAL SHARED_LIBRARY )
		set( _bundle_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" )
	endif()

	set( _bundle_dir_name "${_bundle_dir_base_name}${_bundle_dir_suffix}" )
	set( _bundle_dir_path "${_bundle_output_dir}/${_bundle_dir_name}" )

	# delete previous bundle
	file( REMOVE_RECURSE "${_bundle_dir_path}" )

	# create bundle directory
	file( MAKE_DIRECTORY "${_bundle_dir_path}" )

	file( MAKE_DIRECTORY "${_bundle_dir_path}/Contents" )
	file( MAKE_DIRECTORY "${_bundle_dir_path}/Contents/Resources" )
	if ( "${_target_type}" STREQUAL EXECUTABLE )
		file( MAKE_DIRECTORY "${_bundle_dir_path}/Contents/MacOS" )
	elseif( "${_target_type}" STREQUAL SHARED_LIBRARY )
		file( MAKE_DIRECTORY "${_bundle_dir_path}/Versions" )
		file( MAKE_DIRECTORY "${_bundle_dir_path}/Versions/${_target_version}" )
	endif()

	# extract icon file name from properties
	get_target_property( _bundle_icon_file_path ${TARGET} MACOSX_BUNDLE_ICON_FILE_PATH )
	if ( _bundle_icon_file_path )
		# create rule to update icon in bundle
		get_filename_component( _icon_file_name "${_bundle_icon_file_path}" NAME )
		get_filename_component( _icon_file_name_we "${_bundle_icon_file_path}" NAME_WE )
		set( _icon_file_path "${_bundle_dir_path}/Contents/Resources/${_icon_file_name}" )

		add_custom_command(
			OUTPUT "${_icon_file_path}"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bundle_icon_file_path}" "${_icon_file_path}"
			DEPENDS "${_bundle_icon_file_path}"
		)

		set( _copy_icon_target "${TARGET}_CopyBundleIcon" )
		add_custom_target( ${_copy_icon_target} DEPENDS "${_icon_file_path}" )
		add_dependencies( ${TARGET} ${_copy_icon_target} )

		set( MACOSX_BUNDLE_ICON_FILE "${_icon_file_name_we}" )
	endif()

	# configure Info.plist into the bundle
	set( MACOSX_BUNDLE_EXECUTABLE_NAME "${_target_output_name}" )
	configure_file( "${CMAKE_ROOT}/Modules/MacOSXBundleInfo.plist.in" "${_bundle_dir_path}/Contents/Info.plist" )

	# set target output directory
	if ( "${_target_type}" STREQUAL EXECUTABLE )
		set_target_properties( ${TARGET} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${_bundle_dir_path}/Contents/MacOS"
			LIBRARY_OUTPUT_DIRECTORY "${_bundle_dir_path}/Contents/MacOS"
		)
	elseif( "${_target_type}" STREQUAL SHARED_LIBRARY )
		set_target_properties( ${TARGET} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${_bundle_dir_path}/Versions/${_target_version}"
			LIBRARY_OUTPUT_DIRECTORY "${_bundle_dir_path}/Versions/${_target_version}"
		)

		# create symlink in the root bundle directory to target
		execute_process( COMMAND "${CMAKE_COMMAND}" -E create_symlink "${_bundle_dir_path}/Versions/${_target_version}/${_target_output_name}" "${_bundle_dir_path}/${_target_output_name}" )
	endif()

endfunction()
