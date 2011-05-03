
#include <QApplication>

#include "Global.h"
#include "Config.h"
#include "Converter.h"
#include "MainWindow.h"




int main( int argc, char ** argv )
{
	QApplication app( argc, argv );

	if ( !Fogg::Global::checkResources() )
	{
		foggWarning() << "Failed to locate application resources, exiting.";
		return 1;
	}

	app.setOrganizationName( "dendy.org" );
	app.setOrganizationDomain( "www.dendy.org" );
	app.setApplicationName( "fogg" );
	app.setWindowIcon( QIcon( Fogg::kLogoFilePath ) );

	Fogg::Config config;
	config.load();

	Fogg::Converter converter;
	converter.setConcurrentThreadCount( config.concurrentThreadCount() );

	Fogg::MainWindow mainWindow( &config, &converter );

	if ( config.mainWindowMaximized() )
		mainWindow.showMaximized();
	else
		mainWindow.show();

	const int exitCode = app.exec();

	mainWindow.abort();

	converter.abortAllJobs();
	converter.wait();

	config.save();

	return exitCode;
}
