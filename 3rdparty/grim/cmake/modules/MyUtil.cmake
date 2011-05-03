
# Useful My macros go here.




# adds LINUX token
if ( WIN32 )
elseif ( APPLE )
elseif ( UNIX )
	set( LINUX YES )
else()
endif()




function( my_make_file_directory FILE )

	get_filename_component( _dir "${FILE}" PATH )

	execute_process(
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${_dir}"
		RESULT_VARIABLE _result
	)

	if ( NOT _result EQUAL 0 )
		message( FATAL_ERROR "Error creating directory: ${_dir}" )
	endif()

endfunction()




macro( my_touch_file FILEPATH )
	# FIXME: CMake -E touch does not set time stamp correctly in CMake 2.8.4.
	#        If file already exists it uses fopen() to change modification time.
	#        But that actually does not work, because fopen() truncates current time to seconds,
	#        while touch should preserve precise time, like utime() does:
	#            fopen(): 15:04:13.000000000
	#            utime(): 15:04:13.528446529

	set( Android_USE_CMAKE_TOUCH_WORKAROUND YES )
	if ( UNIX AND Android_USE_CMAKE_TOUCH_WORKAROUND )
		execute_process( COMMAND "touch" "${FILEPATH}" )
	else()
		execute_process( COMMAND "${CMAKE_COMMAND}" -E touch "${FILEPATH}" )
	endif()

endmacro()




macro( my_remove_extension FILENAME_WE FILENAME )
	string( LENGTH "${FILENAME}" _length )
	math( EXPR _pos "${_length} - 1" )

	set( ${FILENAME_WE} )

	while ( 1 )
		if ( _pos LESS 0 )
			break()
		endif()

		string( SUBSTRING "${FILENAME}" ${_pos} 1 _char )

		if ( _char STREQUAL "/" )
			break()
		endif()

		if ( _char STREQUAL "." )
			string( SUBSTRING "${FILENAME}" 0 ${_pos} ${FILENAME_WE} )
			break()
		endif()

		math( EXPR _pos "${_pos} - 1" )
	endwhile()

	if ( NOT ${FILENAME_WE} )
		set( ${FILENAME_WE} "${FILENAME}" )
	endif()
endmacro()




function( my_create_list_cache_entry VAR DEFAULT_OPTION DOC_STRING )

	set( _list ${ARGN} )

	# ensure that list is not empty
	list( LENGTH _list _list_length )
	if ( _list_length EQUAL 0 )
		message( FATAL_ERROR "Entry list is empty" )
	endif()

	# ensure that default value is in the list
	list( FIND _list "${DEFAULT_OPTION}" _default_option_index )
	if ( _default_option_index EQUAL -1 )
		message( FATAL_ERROR "Default entry value is not in the list" )
	endif()

	# create entry in cache
	set( ${VAR} "${DEFAULT_OPTION}" CACHE STRING "${DOC_STRING}" )
	set_property( CACHE ${VAR} PROPERTY STRINGS ${ARGN} )

endfunction()
