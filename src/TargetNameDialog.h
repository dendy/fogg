
#pragma once

#include <QDialog>

#include "ui_TargetNameDialog.h"




namespace Fogg {




class TargetNameDialog : public QDialog
{
	Q_OBJECT

public:
	TargetNameDialog( QWidget * parent = 0 );

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
	Ui::TargetNameDialog ui_;
};




} // namespace Fogg
