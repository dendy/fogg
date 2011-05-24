
#pragma once

#include <QMainWindow>
#include <QAbstractItemModel>
#include <QPointer>

#include <grim/tools/LocalizationManager.h>

#include "Config.h"

#include "ui_MainWindow.h"




class QTimer;
class QLabel;

namespace Grim {
namespace Tools {
	class LocalizationManager;
}
}




namespace Fogg {




class Config;
class Converter;
class FileFetcher;
class FileFetcherDialog;
class ProfileNameDialog;
class DonationDialog;
class AboutDialog;
class PreferencesDialog;
class SkippedFilesDialog;
class NonRecognizedFilesDialog;
class JobItemModel;




class ProfileItemModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum ProfileId
	{
		ProfileId_Null = 0,
		ProfileId_FileSystem = -1
	};

	static bool isCustomProfile( int profileId );

	ProfileItemModel( Config * config, QObject * parent = 0 );
	~ProfileItemModel();

	int profileIdForRow( int row ) const;
	Config::Profile profileForRow( int row ) const;
	int rowForProfileId( int profileId ) const;

	void beginAddProfile();
	void endAddProfile();

	void beginChangeProfile( int profileId );
	void endChangeProfile();

	void beginRemoveProfile( int profileId );
	void endRemoveProfile();

	// reimplemented from QAbstractItemModel
	int rowCount( const QModelIndex & parent ) const;
	int columnCount( const QModelIndex & parent ) const;
	QModelIndex parent( const QModelIndex & index ) const;
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QVariant data( const QModelIndex & index, int role ) const;
	Qt::ItemFlags flags( const QModelIndex & index ) const;

private:
	int _rowCount() const;
	int _rowForCustomProfileIndex( int customProfileIndex ) const;

private:
	Config * config_;

	// helpers
	int profileIdBeingChanged_;

	friend class MainWindow;
};




class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow( Config * config, Converter * converter );
	~MainWindow();

	void abort();

	int currentProfileId() const;

	Config::Profile currentProfile() const;
	void setCurrentProfile( const Config::Profile & profile );

protected:
	// reimplemented from QWidget
	void closeEvent( QCloseEvent * e );
	void dragEnterEvent( QDragEnterEvent * e );
	void dragLeaveEvent( QDragLeaveEvent * e );
	void dropEvent( QDropEvent * e );
	void changeEvent( QEvent * e );
	bool eventFilter( QObject * o, QEvent * e );

private:
	class FetchedFileInfo
	{
	public:
		FetchedFileInfo( const QString & _filePath, const QString & _basePath ) :
			filePath( _filePath ), basePath( _basePath )
		{}

		QString filePath;
		QString basePath;
	};

private:
	void _retranslateUi();
	void _retranslateDropHintLabel();

	QString _collectMediaFileFilterString() const;
	QStringList _collectMediaFileFilters() const;
	void _updateTotalProgressBar();
	void _updateJobActions();
	void _adjustDropHintLabel();
	void _updateFileFetcherActions();
	void _updateCurrentProfile();
	void _setJobItemModelSourcePaths();
	void _startFileFetch( const QList<QUrl> & urls );
	void _finishFileFetch();
	bool _tryAddFile( const QString & filePath, const QString & basePath, const QString & format );

private slots:
	void _aboutToQuit();

	void _jobItemModelContentsChanged();

	void _jobStarted( int jobId );
	void _jobResolvedFormat( int jobId, const QString & format );
	void _jobProgress( int jobId, qreal progress );
	void _jobFinished( int jobId, int result );

	void _currentFetchDirChanged( const QString & dirPath );
	void _fetched( const QString & filePath, const QString & basePath, bool extensionRecognized );
	void _fetchFinished();

	void _showFileFetcherDialog();
	void _fileFetchDialogAborted();

	void _jobSelectionChanged();

	void _jobFileItemAboutToRemove();
	void _jobItemProgressChanged();

	void on_profileComboBox_currentIndexChanged();
	void on_profilePathBrowseButton_clicked();
	void on_profilePathLineEdit_textChanged();
	void on_profilePrependYearToAlbumCheckBox_toggled();
	void on_profileQualityWidget_valueChanged();

	// actions
	void on_actionAddFiles_triggered();
	void on_actionAddDirectory_triggered();
	void on_actionRemoveSelected_triggered();
	void on_actionRemoveAll_triggered();
	void on_actionUnmark_triggered();

	void on_actionAddProfile_triggered();
	void on_actionRemoveProfile_triggered();
	void on_actionRenameProfile_triggered();

	void on_actionStartConversion_triggered();
	void on_actionStopConversion_triggered();

	void on_actionQuit_triggered();

	void on_actionPreferences_triggered();
	void on_actionStayOnTop_toggled();
	void on_actionExpandAll_triggered();
	void on_actionCollapseAll_triggered();
	void on_actionDonate_triggered();
	void on_actionAbout_triggered();

private:
	Config * config_;
	Converter * converter_;

	Ui::MainWindow ui_;

	// localization
	QString translationsDirPath_;
	QPointer<Grim::Tools::LocalizationManager> localizationManager_;

	// profile combo box
	QPointer<ProfileItemModel> profileItemModel_;

	// job view
	QPointer<JobItemModel> jobItemModel_;

	// file fetcher
	QPointer<FileFetcher> fileFetcher_;
	QPointer<FileFetcherDialog> fileFetcherDialog_;
	QPointer<QTimer> fileFetcherDialogAppearTimer_;

	// dialogs
	QPointer<ProfileNameDialog> profileNameDialog_;
	QPointer<DonationDialog> donationDialog_;
	QPointer<AboutDialog> aboutDialog_;
	QPointer<PreferencesDialog> preferencesDialog_;
	QPointer<SkippedFilesDialog> skippedFilesDialog_;
	QPointer<NonRecognizedFilesDialog> nonRecognizedFilesDialog_;

	// drop hint
	QPointer<QLabel> dropHintLabel_;
	bool underDragging_;

	// helpers
	int fetchedFileCount_;
	QStringList failedFetchedFilePaths_;
	QList<FetchedFileInfo> nonRecognizedFetchedFileInfos_;

	bool selfChangeCurrentProfileIndex_;
};




} // namespace Fogg
