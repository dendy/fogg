
#include "TargetNameDialog.h"

#include <QPushButton>

#include "Global.h"




namespace Fogg {




TargetNameDialog::TargetNameDialog( QWidget * const parent ) :
	QDialog( parent )
{
	ui_.setupUi( this );

	ui_.nameLineEdit->setText( QString() );
	on_nameLineEdit_textChanged();

	_retranslateUi();
}


QString TargetNameDialog::name() const
{
	return ui_.nameLineEdit->text();
}


void TargetNameDialog::setName( const QString & name )
{
	ui_.nameLineEdit->setText( name );
}


void TargetNameDialog::showEvent( QShowEvent * const e )
{
	QDialog::showEvent( e );

	ui_.nameLineEdit->selectAll();
	ui_.nameLineEdit->setFocus();
}


void TargetNameDialog::changeEvent( QEvent * const e )
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


void TargetNameDialog::on_nameLineEdit_textChanged()
{
	QAbstractButton * const okButton = ui_.buttonBox->button( QDialogButtonBox::Ok );
	Q_ASSERT( okButton );
	okButton->setEnabled( !ui_.nameLineEdit->text().isEmpty() );
}


void TargetNameDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( tr( "Rename device" ) ) );
}




} // namespace Fogg
