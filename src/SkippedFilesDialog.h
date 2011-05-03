
#pragma once

#include <QDialog>

#include "ui_SkippedFilesDialog.h"




class QStringListModel;




namespace Fogg {




class SkippedFilesDialog : public QDialog
{
	Q_OBJECT

public:
	SkippedFilesDialog( QWidget * parent = 0 );

	void exec( const QStringList & list );

protected:
	void changeEvent( QEvent * e );

private:
	void _retranslateUi();

private:
	Ui::SkippedFilesDialog ui_;
	QStringListModel * filesModel_;
};




} // namespace Fogg
