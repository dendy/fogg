
#pragma once

#include <QObject>
#include <QStringList>
#include <QDebug>




namespace Fogg {




#ifdef FOGG_DEBUG
#	define foggDebug() qDebug() << Q_FUNC_INFO
#else
#	define foggDebug() QNoDebug()
#endif

#define foggWarning() qWarning() << Q_FUNC_INFO




// standard Vorbis quality range
static const qreal kMinimumQualityValue = -0.1;
static const qreal kMaximumQualityValue =  1.0;

extern const QString kLogoFilePath;
extern const QString kLicenseFilePath;




class Global : public QObject
{
	Q_OBJECT

public:
	static QString applicationName();
	static QString applicationVersion();
	static QString authorName();
	static QString copyrightDate();

	static QString makeWindowTitle( const QString & title );

	static bool checkResources();

	static QStringList fileFiltersForExtensions( const QStringList & extensions );

	static void msleep( int ms );
};




} // namespace Fogg
