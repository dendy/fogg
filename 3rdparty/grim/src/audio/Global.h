
#pragma once

#include <qglobal.h>




#if defined (Q_WS_WIN)
#	ifdef GRIM_AUDIO_STATIC
#		define GRIM_AUDIO_EXPORT
#	else
#		ifdef GRIM_AUDIO_BUILD
#			define GRIM_AUDIO_EXPORT Q_DECL_EXPORT
#		else
#			define GRIM_AUDIO_EXPORT Q_DECL_IMPORT
#		endif
#	endif
#else
#	define GRIM_AUDIO_EXPORT __attribute__ ((visibility("default")))
#endif
