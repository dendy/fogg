#!/usr/bin/cmake -P

# Updates specified .ts file with Qt lupdate tool.
#
# Arguments:
#    PRO_FILE        - .pro file used as list of source files
#    TS_FILE_TARGET  - where to generate target file
#    TS_FILE         - .ts file to lupdate
#    LUPDATE_OPTIONS - extra options for lupdate command line
#    LUPDATE_COMMAND - path to Qt lupdate executable
#    My_UTIL_SCRIPT  - path to util macros to include


include( "${My_UTIL_SCRIPT}" )


# convert lupdate options into normal list
string( REPLACE ":" ";" _lupdate_options "${LUPDATE_OPTIONS}" )


# run lupdate
execute_process(
	COMMAND
		"${LUPDATE_COMMAND}" ${_lupdate_options} "${PRO_FILE}" -ts "${TS_FILE}"
	RESULT_VARIABLE _result
	OUTPUT_VARIABLE _output
	ERROR_VARIABLE _error
)


# check for error
if ( NOT _result EQUAL 0 )
	message( FATAL_ERROR "Error updating .ts file: ${TS_FILE}" )
endif()


# save target file
my_make_file_directory( "${TS_FILE_TARGET}" )
my_touch_file( "${TS_FILE_TARGET}" )
