
#include "Global.h"

#include <QFile>




namespace Fogg {




extern const QString kLogoFilePath    = QLatin1String( ":/icons/fogg-logo.png" );
extern const QString kLicenseFilePath = QLatin1String( ":/license/LICENSE.GPL3" );




QString Global::applicationName()
{
	return tr( "Fogg", "application name" );
}


QString Global::applicationVersion()
{
	return QLatin1String( "0.1" );
}


QString Global::authorName()
{
	return tr( "Daniel Levin", "author" );
}


QString Global::copyrightDate()
{
	return QLatin1String( "2011" );
}


QString Global::makeWindowTitle( const QString & title )
{
	static const QString kTitleTemplate = QLatin1String( "%1 - %2" );
	return kTitleTemplate.arg( title ).arg( applicationName() );
}


bool Global::checkResources()
{
	const QStringList filePathsToExist = QStringList()
			<< kLogoFilePath
			<< kLicenseFilePath;

	foreach ( const QString & filePath, filePathsToExist )
	{
		if ( !QFile::exists( filePath ) )
		{
			foggWarning() << "Failed locating resource file:" << filePath;
			return false;
		}
	}

	return true;
}


QStringList Global::fileFiltersForExtensions( const QStringList & extensions )
{
	static const QString kExtensionFilterTemplate = QLatin1String( "*.%1" );

	QStringList filters;
	foreach ( const QString & extension, extensions )
		filters << kExtensionFilterTemplate.arg( extension );
	return filters;
}




} // namespace Fogg
