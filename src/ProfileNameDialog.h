
#pragma once

#include <QDialog>

#include "ui_ProfileNameDialog.h"




namespace Fogg {




class ProfileNameDialog : public QDialog
{
	Q_OBJECT

public:
	ProfileNameDialog( QWidget * parent = 0 );

	QString name() const;
	void setName( const QString & name );

protected:
	// reimplemented from QWidget
	void showEvent( QShowEvent * e );
	void changeEvent( QEvent * e );

private slots:
	void on_nameLineEdit_textChanged();

private:
	void _retranslateUi();

private:
	Ui::ProfileNameDialog ui_;
};




} // namespace Fogg
