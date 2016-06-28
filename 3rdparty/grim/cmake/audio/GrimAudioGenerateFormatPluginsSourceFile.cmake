#!/usr/bin/cmake -P


# Generates source file with list of Q_IMPORT_PLUGIN() records
# to automatically import static format plugins selected at configuration step.
#
# Arguments:
#    SOURCE_FILE    - where source file should be generated
#    PLUGINS        - list of plugin names
#    My_UTIL_SCRIPT - path to util macros to include


include( "${My_UTIL_SCRIPT}" )


# make output directory
my_make_file_directory( "${SOURCE_FILE}" )


# turn column separated list into normal list
string( REPLACE ":" ";" _plugins "${PLUGINS}" )


# write caption
file( WRITE "${SOURCE_FILE}"
	"\n"
	"// This file is automatically generated by CMake for the GrimAudio\n"
	"// to be included for importing static plugins.\n"
	"\n"
	"// All manual changes here will be lost.\n"
	"\n"
)

file( APPEND "${SOURCE_FILE}"
	"#include <QtPlugin>\n"
	"\n"
)


foreach ( _plugin ${_plugins} )
	string( TOLOWER "${_plugin}" _plugin_lower )
	file( APPEND "${SOURCE_FILE}" "Q_IMPORT_PLUGIN( ${_plugin}FormatPlugin )\n" )
endforeach()
