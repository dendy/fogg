
#pragma once

#include <qglobal.h>
#include <QDebug>




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




#ifdef GRIM_AUDIO_DEBUG
#	define grimAudioDebug() qDebug() << Q_FUNC_INFO
#else
#	define grimAudioDebug() QNoDebug()
#endif

#define grimAudioWarning() qWarning() << Q_FUNC_INFO
