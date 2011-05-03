
#pragma once

#include <QDialog>

#include "Global.h"

#include "ui_DonationDialog.h"




namespace Fogg {




class DonationDialog : public QDialog
{
	Q_OBJECT

public:
	DonationDialog( QWidget * parent = 0 );

protected:
	void changeEvent( QEvent * e );

private:
	void _retranslateUi();

private:
	Ui::DonationDialog ui_;
};




} // namespace Fogg
