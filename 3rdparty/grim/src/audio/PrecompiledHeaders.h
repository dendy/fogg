
#ifndef PRECOMPILEDHEADERS_H
#define PRECOMPILEDHEADERS_H




#include <QtCore>

#ifdef GRIM_AUDIO_BUILD_OPENAL
#if defined GRIM_AUDIO_OPENAL_INCLUDE_DIR_PREFIX_AL
#	include <AL/alc.h>
#	include <AL/al.h>
#elif defined GRIM_AUDIO_OPENAL_INCLUDE_DIR_PREFIX_OpenAL
#	include <OpenAL/alc.h>
#	include <OpenAL/al.h>
#else
#	include <alc.h>
#	include <al.h>
#endif
#endif




#endif
