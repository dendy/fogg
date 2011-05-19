
#pragma once

#include <QDialog>

#include "ui_FileFetcherDialog.h"




class QTimer;




namespace Fogg {




class FileFetcherDialog : public QDialog
{
	Q_OBJECT

public:
	FileFetcherDialog( QWidget * parent = 0 );

	void setFetchFinished( bool finished );
	void setCurrentDir( const QString & dirPath );
	void setFileCount( int count );

signals:
	void aborted();

protected:
	void closeEvent( QCloseEvent * e );
	void changeEvent( QEvent * e );

private slots:
	void _abortButtonClicked();
	void _autoHideTimerTriggered();

private:
	void _retranslateUi();

private:
	Ui::FileFetcherDialog ui_;
	bool isFetchFinished_;
	QTimer * autoHideTimer_;
};




} // namespace Fogg
