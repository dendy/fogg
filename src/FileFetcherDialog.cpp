
#include "FileFetcherDialog.h"

#include <QFileInfo>
#include <QCloseEvent>
#include <QTimer>
#include <QPushButton>

#include "Global.h"




namespace Fogg {




static const int kAutoHideTimeout = 2000;




FileFetcherDialog::FileFetcherDialog( QWidget * const parent ) :
	QDialog( parent )
{
	isFetchFinished_ = false;

	autoHideTimer_ = new QTimer( this );
	autoHideTimer_->setSingleShot( true );
	autoHideTimer_->setInterval( kAutoHideTimeout );
	connect( autoHideTimer_, SIGNAL(timeout()), SLOT(_autoHideTimerTriggered()) );

	ui_.setupUi( this );

	connect( ui_.buttonBox->button( QDialogButtonBox::Abort ), SIGNAL(clicked()), SLOT(_abortButtonClicked()) );

	_retranslateUi();
}


void FileFetcherDialog::setFetchFinished( const bool finished )
{
	isFetchFinished_ = finished;

	ui_.buttonBox->button( QDialogButtonBox::Ok )->setVisible( isFetchFinished_ );
	ui_.buttonBox->button( QDialogButtonBox::Abort )->setVisible( !isFetchFinished_ );

	if ( isFetchFinished_ && isVisible() )
		autoHideTimer_->start();
}


void FileFetcherDialog::setCurrentDir( const QString & dirPath )
{
	ui_.currentDirLabel->setText( QFileInfo( dirPath ).fileName() );
}


void FileFetcherDialog::setFileCount( const int count )
{
	ui_.fileCountLabel->setText( QString::number( count ) );
}


void FileFetcherDialog::closeEvent( QCloseEvent * const e )
{
	if ( !isFetchFinished_ )
	{
		e->ignore();
		return;
	}

	autoHideTimer_->stop();

	QDialog::closeEvent( e );
}


void FileFetcherDialog::changeEvent( QEvent * const e )
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


void FileFetcherDialog::_abortButtonClicked()
{
	emit aborted();
}


void FileFetcherDialog::_autoHideTimerTriggered()
{
	close();
}


void FileFetcherDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( tr( "Fetching files" ) ) );
}




} // namespace Fogg
