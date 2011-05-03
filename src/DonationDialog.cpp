
#include "DonationDialog.h"

#include <QEvent>




namespace Fogg {




DonationDialog::DonationDialog( QWidget * const parent ) :
	QDialog( parent )
{
	ui_.setupUi( this );

	_retranslateUi();
}


void DonationDialog::changeEvent( QEvent * const e )
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


void DonationDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( tr( "Donation" ) ) );
}




} // namespace Fogg
