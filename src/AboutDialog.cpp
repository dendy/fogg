
#include "AboutDialog.h"

#include <QEvent>
#include <QFile>
#include <QStyle>
#include <QStringListModel>

#include "Global.h"
#include "PoweredByWidget.h"

#include <grim/audio/FormatManager.h>




namespace Fogg {




AboutDialog::AboutDialog( Grim::Audio::FormatManager * const audioFormatManager, QWidget * const parent ) :
	QDialog( parent ),
	audioFormatManager_( audioFormatManager )
{
	ui_.setupUi( this );

	// powered by
	{
		QHBoxLayout * const poweredByLayout = new QHBoxLayout;

		poweredByLayout->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed ) );

		PoweredByWidget * const poweredByQtWidget = new PoweredByWidget( this );
		poweredByQtWidget->setName( QLatin1String( "Qt" ) );
		poweredByQtWidget->setLogoFilePath( ":/3rdparty/qt-logo.png" );
		poweredByQtWidget->setUrl( QUrl( "http://qt.nokia.com" ) );
		poweredByLayout->addWidget( poweredByQtWidget );

		PoweredByWidget * const poweredByOggVorbisWidget = new PoweredByWidget( this );
		poweredByOggVorbisWidget->setName( QLatin1String( "Ogg/Vorbis" ) );
		poweredByOggVorbisWidget->setLogoFilePath( ":/3rdparty/ogg-vorbis-logo.png" );
		poweredByOggVorbisWidget->setUrl( QUrl( "http://xiph.org/vorbis" ) );
		poweredByLayout->addWidget( poweredByOggVorbisWidget );

		PoweredByWidget * const poweredByFlacWidget = new PoweredByWidget( this );
		poweredByFlacWidget->setName( QLatin1String( "FLAC" ) );
		poweredByFlacWidget->setLogoFilePath( ":/3rdparty/flac-logo.png" );
		poweredByFlacWidget->setUrl( QUrl( "http://flac.sourceforge.net" ) );
		poweredByLayout->addWidget( poweredByFlacWidget );

		PoweredByWidget * const poweredByMpg123Widget = new PoweredByWidget( this );
		poweredByMpg123Widget->setName( QLatin1String( "mpg123" ) );
		poweredByMpg123Widget->setLogoFilePath( ":/3rdparty/mpg123-logo.png" );
		poweredByMpg123Widget->setUrl( QUrl( "http://mpg123.org" ) );
		poweredByLayout->addWidget( poweredByMpg123Widget );

		poweredByLayout->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed ) );

		ui_.poweredByWidget->setLayout( poweredByLayout );
	}

	// details
	detailsItemModel_ = new QStringListModel( this );
	ui_.detailsView->setModel( detailsItemModel_ );

	// license
	{
		QFont monospaceFont = font();
		monospaceFont.setWeight( QFont::Normal );
		monospaceFont.setStyleHint( QFont::TypeWriter );
		monospaceFont.setFamily( "Monospace" );
		ui_.licenseTextBrowser->setFont( monospaceFont );

		const QFontMetrics monospaceFontMetrics = QFontMetrics( monospaceFont );
		ui_.licenseTextBrowser->setMinimumWidth( monospaceFontMetrics.width( '0' )*80
				+ style()->pixelMetric( QStyle::PM_ScrollBarExtent ) );

		QFile file( kLicenseFilePath );
		if ( file.open( QIODevice::ReadOnly ) )
		{
			QTextStream ts( &file );
			ts.setCodec( "UTF-8" );
			const QString text = ts.readAll();
			ui_.licenseTextBrowser->setPlainText( text );
		}
	}

	_retranslateUi();
}


void AboutDialog::changeEvent( QEvent * const e )
{
	switch ( e->type() )
	{
	case QEvent::LanguageChange:
		ui_.retranslateUi( this );
		_retranslateUi();
		break;
	}

	return QDialog::changeEvent( e );
}


void AboutDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( tr( "About" ) ) );

	ui_.titleLabel->setText( QString::fromLatin1( "%1 %2" )
			.arg( Global::applicationName() )
			.arg( Global::applicationVersion() ) );

	ui_.descriptionLabel->setText(
		tr( "%1 is a music converter application, "
			"designed for easy compression from various formats into Ogg/Vorbis." )
				.arg( Global::applicationName() ) );

	ui_.copyrightLabel->setText( tr( "Copyright %1 %2" )
			.arg( Global::copyrightDate() )
			.arg( Global::authorName() ) );

	// update details view
	QStringList details;
	foreach ( const QString & format, audioFormatManager_->availableFileFormats() )
	{
		const QString pluginString = QString::fromLatin1( "%1 (%2)" )
				.arg( format )
				.arg( Global::fileFiltersForExtensions( audioFormatManager_->availableFileExtensionsForFormat( format ) ).join( " " ) );

		const QString detailString = tr( "Built with audio plugin: %1" ).arg( pluginString );

		details << detailString;
	}

	detailsItemModel_->setStringList( details );
}




} // namespace Fogg
