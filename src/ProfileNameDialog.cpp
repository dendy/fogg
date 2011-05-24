
#include "ProfileNameDialog.h"

#include <QPushButton>

#include "Global.h"




namespace Fogg {




ProfileNameDialog::ProfileNameDialog( QWidget * const parent ) :
	QDialog( parent )
{
	ui_.setupUi( this );

	ui_.nameLineEdit->setText( QString() );
	on_nameLineEdit_textChanged();

	_retranslateUi();
}


QString ProfileNameDialog::name() const
{
	return ui_.nameLineEdit->text();
}


void ProfileNameDialog::setName( const QString & name )
{
	ui_.nameLineEdit->setText( name );
}


void ProfileNameDialog::showEvent( QShowEvent * const e )
{
	QDialog::showEvent( e );

	ui_.nameLineEdit->selectAll();
	ui_.nameLineEdit->setFocus();
}


void ProfileNameDialog::changeEvent( QEvent * const e )
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


void ProfileNameDialog::on_nameLineEdit_textChanged()
{
	QAbstractButton * const okButton = ui_.buttonBox->button( QDialogButtonBox::Ok );
	Q_ASSERT( okButton );
	okButton->setEnabled( !ui_.nameLineEdit->text().isEmpty() );
}


void ProfileNameDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( tr( "Rename profile" ) ) );
}




} // namespace Fogg
