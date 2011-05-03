
#include "SkippedFilesDialog.h"

#include <QStringListModel>
#include <QEvent>
#include <QStyle>

#include "Global.h"




namespace Fogg {




SkippedFilesDialog::SkippedFilesDialog( QWidget * const parent ) :
	QDialog( parent )
{
	ui_.setupUi( this );

	const int iconSize = style()->pixelMetric( QStyle::PM_MessageBoxIconSize );
	ui_.iconLabel->setPixmap( style()->standardIcon( QStyle::SP_MessageBoxInformation ).pixmap( iconSize, iconSize ) );

	filesModel_ = new QStringListModel( this );
	ui_.filesView->setModel( filesModel_ );

	_retranslateUi();
}


void SkippedFilesDialog::exec( const QStringList & list )
{
	filesModel_->setStringList( list );
	QDialog::exec();
	filesModel_->setStringList( QStringList() );
}


void SkippedFilesDialog::changeEvent( QEvent * const e )
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


void SkippedFilesDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( "Some files have been skipped" ) );
}




} // namespace Fogg
