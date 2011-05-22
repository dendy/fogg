
#pragma once

#include <QDialog>
#include <QStringList>

#include "ui_NonRecognizedFilesDialog.h"




class QStringListModel;




namespace Fogg {




class NonRecognizedFilesDialog : public QDialog
{
	Q_OBJECT

public:
	NonRecognizedFilesDialog( QWidget * parent = 0 );

	int exec( const QStringList & filePaths );

protected:
	void changeEvent( QEvent * e );

private:
	void _retranslateUi();

private:
	Ui::NonRecognizedFilesDialog ui_;

	QStringListModel * filesModel_;
};




} // namespace Fogg
