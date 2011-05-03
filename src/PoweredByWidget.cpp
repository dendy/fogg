
#include "PoweredByWidget.h"

#include <QEvent>




namespace Fogg {




static const int kLogoSize = 64;




PoweredByWidget::PoweredByWidget( QWidget * const parent ) :
	QWidget( parent )
{
	ui_.setupUi( this );

	ui_.logoLabel->setFixedHeight( kLogoSize );
}


void PoweredByWidget::setName( const QString & name )
{
	name_ = name;
	ui_.nameLabel->setText( name_ );
}


void PoweredByWidget::setLogoFilePath( const QString & path )
{
	logoFilePath_ = path;
	ui_.logoLabel->setPixmap( QIcon( logoFilePath_ ).pixmap( kLogoSize, kLogoSize ) );
}


void PoweredByWidget::setUrl( const QUrl & url )
{
	static const QString kLinkUrlTemplate = QLatin1String(
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
		"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
		"p, li { white-space: pre-wrap; }"
		"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">"
		"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
		"<a href=\"%1\"><span style=\" text-decoration: underline; color:#0057ae;\">%2</span></a></p></body></html>" );

	url_ = url;
	ui_.urlLabel->setText( kLinkUrlTemplate.arg( url_.toString(), url_.host() + url_.path() ) );
}


void PoweredByWidget::changeEvent( QEvent * const e )
{
	switch ( e->type() )
	{
	case QEvent::LanguageChange:
		ui_.retranslateUi( this );
		break;
	}

	QWidget::changeEvent( e );
}




} // namespace Fogg
