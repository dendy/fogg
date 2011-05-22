
#include "NonRecognizedFilesDialog.h"

#include <QEvent>
#include <QStringListModel>

#include "Global.h"




namespace Fogg {




NonRecognizedFilesDialog::NonRecognizedFilesDialog( QWidget * const parent ) :
	QDialog( parent )
{
	ui_.setupUi( this );

	const int iconSize = style()->pixelMetric( QStyle::PM_MessageBoxIconSize );
	ui_.iconLabel->setPixmap( style()->standardIcon( QStyle::SP_MessageBoxInformation ).pixmap( iconSize, iconSize ) );

	filesModel_ = new QStringListModel( this );
	ui_.filesView->setModel( filesModel_ );

	_retranslateUi();
}


int NonRecognizedFilesDialog::exec( const QStringList & filePaths )
{
	filesModel_->setStringList( filePaths );
	return QDialog::exec();
}


void NonRecognizedFilesDialog::changeEvent( QEvent * const e )
{
	switch ( e->type() )
	{
	case QEvent::LanguageChange:
		ui_.retranslateUi( this );
		_retranslateUi();
		break;
	}

	QDialog::changeEvent( e );
}


void NonRecognizedFilesDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( "Unrecognized files" ) );
}




} // namespace Fogg
