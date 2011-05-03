
#pragma once

#include <qglobal.h>
#include <QDebug>




#if defined (Q_OS_WIN)
#	ifdef GRIM_TOOLS_STATIC
#		define GRIM_TOOLS_EXPORT
#	else
#		ifdef GRIM_TOOLS_BUILD
#			define GRIM_TOOLS_EXPORT Q_DECL_EXPORT
#		else
#			define GRIM_TOOLS_EXPORT Q_DECL_IMPORT
#		endif
#	endif
#else
#	define GRIM_TOOLS_EXPORT __attribute__ ((visibility("default")))
#endif




#ifdef GRIM_TOOLS_DEBUG
#	define grimToolsDebug() qDebug() << Q_FUNC_INFO
#else
#	define grimToolsDebug() QNoDebug()
#endif

#define grimToolsWarning() qWarning() << Q_FUNC_INFO
