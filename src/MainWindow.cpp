
#include "MainWindow.h"

#include <QSettings>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QDir>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QCompleter>
#include <QDirModel>
#include <QDesktopWidget>

#include <grim/audio/FormatManager.h>

#include "Converter.h"
#include "FileFetcher.h"
#include "FileFetcherDialog.h"
#include "ProfileNameDialog.h"
#include "DonationDialog.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "SkippedFilesDialog.h"
#include "NonRecognizedFilesDialog.h"
#include "JobItemModel.h"
#include "ButtonActionBinder.h"




namespace Fogg {




static const QString kEnLocaleName = QLatin1String( "en" );
static const int kFileFetchDialogAppearTimeout = 1000;
static const int kMaxTotalProgressBarValue = 10000;




bool ProfileItemModel::isCustomProfile( const int profileId )
{
	return profileId > 0;
}


ProfileItemModel::ProfileItemModel( Config * const config, QObject * const parent ) :
	QAbstractItemModel( parent ),
	config_( config )
{
}


ProfileItemModel::~ProfileItemModel()
{
}


int ProfileItemModel::profileIdForRow( const int row ) const
{
	Q_ASSERT( row >= 0 && row < _rowCount() );

	switch ( row )
	{
	case 0:
		return ProfileId_FileSystem;
	case 1:
		return ProfileId_Null;
	default:
		return config_->customProfileIds().at( row - 2 );
	}

	Q_ASSERT( false );
	return 0;
}


Config::Profile ProfileItemModel::profileForRow( const int row ) const
{
	const int profileId = profileIdForRow( row );

	switch ( profileId )
	{
	case ProfileId_FileSystem:
		return config_->fileSystemProfile();
	case ProfileId_Null:
		return Config::Profile();
	default:
		return config_->customProfileForId( profileId );
	}

	Q_ASSERT( false );
	return Config::Profile();
}


int ProfileItemModel::rowForProfileId( const int profileId ) const
{
	switch ( profileId )
	{
	case ProfileId_FileSystem:
		return 0;
	case ProfileId_Null:
		return 1;
	default:
		return 2 + config_->customProfileIndexForId( profileId );
	}

	Q_ASSERT( false );
	return -1;
}


void ProfileItemModel::beginAddProfile()
{
	const int lastRow = _rowForCustomProfileIndex( config_->customProfileIds().count() );
	const int firstRow = config_->customProfileIds().isEmpty() ? rowForProfileId( ProfileId_Null ) : lastRow;

	beginInsertRows( QModelIndex(), firstRow, lastRow );
}


void ProfileItemModel::endAddProfile()
{
	endInsertRows();
}


void ProfileItemModel::beginChangeProfile( const int profileId )
{
	profileIdBeingChanged_ = profileId;
}


void ProfileItemModel::endChangeProfile()
{
	const int row = rowForProfileId( profileIdBeingChanged_ );
	const QModelIndex profileModelIndex = index( row, 0 );
	emit dataChanged( profileModelIndex, profileModelIndex );
}


void ProfileItemModel::beginRemoveProfile( const int profileId )
{
	const int lastRow = rowForProfileId( profileId );
	const int firstRow = config_->customProfileIds().count() == 1 ? rowForProfileId( ProfileId_Null ) : lastRow;

	beginRemoveRows( QModelIndex(), firstRow, lastRow );
}


void ProfileItemModel::endRemoveProfile()
{
	endRemoveRows();
}


int ProfileItemModel::rowCount( const QModelIndex & parent ) const
{
	if ( !parent.isValid() )
		return _rowCount();
	return 0;
}


int ProfileItemModel::columnCount( const QModelIndex & parent ) const
{
	if ( !parent.isValid() )
		return 1;
	return 0;
}


QModelIndex ProfileItemModel::parent( const QModelIndex & index ) const
{
	Q_UNUSED( index );
	return QModelIndex();
}


QModelIndex ProfileItemModel::index( const int row, const int column, const QModelIndex & parent ) const
{
	if ( !parent.isValid() && row >= 0 && row < _rowCount() && column == 0 )
		return createIndex( row, column, profileIdForRow( row ) );
	return QModelIndex();
}


QVariant ProfileItemModel::data( const QModelIndex & index, const int role ) const
{
	if ( !index.isValid() )
		return QVariant();

	const int profileId = profileIdForRow( index.row() );

	switch ( profileId )
	{
	case ProfileId_FileSystem:
	{
		switch ( role )
		{
		case Qt::DisplayRole:
			return tr( "File System" );
		}
	}
		break;

	case ProfileId_Null:
	{
		switch ( role )
		{
		case Qt::DisplayRole:
			return QLatin1String( "========" );
		}
	}
		break;

	default:
	{
		const Config::Profile profile = config_->customProfileForId( profileId );
		switch ( role )
		{
		case Qt::DisplayRole:
			return profile.name;
		}
	}
		break;
	}

	return QVariant();
}


Qt::ItemFlags ProfileItemModel::flags( const QModelIndex & index ) const
{
	if ( !index.isValid() )
		return Qt::NoItemFlags;

	const int profileId = profileIdForRow( index.row() );

	switch ( profileId )
	{
	case ProfileId_Null:
		return Qt::NoItemFlags;
	case ProfileId_FileSystem:
	default:
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	Q_ASSERT( false );
	return Qt::NoItemFlags;
}


int ProfileItemModel::_rowCount() const
{
	// file system profile + separator + custom profile
	const int customProfileCount = config_->customProfileIds().count();
	return 1 + customProfileCount + (customProfileCount == 0 ? 0 : 1);
}


int ProfileItemModel::_rowForCustomProfileIndex( const int customProfileIndex ) const
{
	Q_ASSERT( customProfileIndex >= 0 );

	// 2 == file system index + separator index
	return customProfileIndex + 2;
}




MainWindow::MainWindow( Config * const config, Converter * const converter ) :
	config_( config ),
	converter_( converter )
{
	selfChangeCurrentProfileIndex_ = false;
	underDragging_ = false;

	// localization
	localizationManager_ = new Grim::Tools::LocalizationManager( this );

	translationsDirPath_ = QString::fromUtf8( qgetenv( "FOGG_TRANSLATIONS_DIR" ).constData() );
	localizationManager_->loadTranslations( translationsDirPath_ );
	foggDebug() << "Found locales:" << localizationManager_->names();

	// set 'en' localization as fallback
	const QLocale enLocale = QLocale( kEnLocaleName );
	localizationManager_->setFallbackLocalization( localizationManager_->localizationForLocale( enLocale ) );
	if ( localizationManager_->fallbackLocalization().isNull() )
		foggWarning() << "No 'en' localization found";

	if ( config_->language().isEmpty() )
	{
		localizationManager_->setCurrentLocalizationFromSystemLocale();
	}
	else
	{
		// use last used locale
		const Grim::Tools::LocalizationInfo localization = localizationManager_->localizationForName( config_->language() );
		if ( localization.isNull() )
		{
			foggWarning() << "No localization found for locale:" << config_->language();
			config_->setLanguage( QString() );
			localizationManager_->setCurrentLocalizationFromSystemLocale();
		}
		else
		{
			localizationManager_->setCurrentLocalization( localization );
		}
	}

	const Grim::Tools::LocalizationInfo currentLocalization = localizationManager_->localizationForName( config_->language() );
	if ( currentLocalization.isNull() )
	{
		foggWarning() << "Language not found:" << config_->language();
	}

	// quit handler
	connect( QCoreApplication::instance(), SIGNAL(aboutToQuit()), SLOT(_aboutToQuit()) );

	// file fetcher
	fileFetcherDialogAppearTimer_ = new QTimer( this );
	fileFetcherDialogAppearTimer_->setSingleShot( true );
	fileFetcherDialogAppearTimer_->setInterval( kFileFetchDialogAppearTimeout );
	connect( fileFetcherDialogAppearTimer_, SIGNAL(timeout()), SLOT(_showFileFetcherDialog()) );

	fileFetcherDialog_ = new FileFetcherDialog( this );
	connect( fileFetcherDialog_, SIGNAL(aborted()), SLOT(_fileFetchDialogAborted()) );

	// converter
	connect( converter_, SIGNAL(jobStarted(int)), SLOT(_jobStarted(int)) );
	connect( converter_, SIGNAL(jobResolvedFormat(int,QString)), SLOT(_jobResolvedFormat(int,QString)) );
	connect( converter_, SIGNAL(jobProgress(int,qreal)), SLOT(_jobProgress(int,qreal)) );
	connect( converter_, SIGNAL(jobFinished(int,int)), SLOT(_jobFinished(int,int)) );

	fileFetcher_ = new FileFetcher( this );
	fileFetcher_->setFilters( _collectMediaFileFilters() );
	connect( fileFetcher_, SIGNAL(currentDirChanged(QString)), SLOT(_currentFetchDirChanged(QString)) );
	connect( fileFetcher_, SIGNAL(fetched(QString,QString,bool)), SLOT(_fetched(QString,QString,bool)) );
	connect( fileFetcher_, SIGNAL(finished()), SLOT(_fetchFinished()) );

	// user interface
	ui_.setupUi( this );

	// global
	if ( config_->mainWindowStayOnTop() )
	{
		ui_.actionStayOnTop->setChecked( true );
		on_actionStayOnTop_toggled();
	}

	if ( !config_->mainWindowGeometry().isNull() )
	{
		const QDesktopWidget * desktopWidget = qApp->desktop();
		const QRect availableGeometry = desktopWidget->availableGeometry( config_->mainWindowGeometry().topLeft() );
		if ( !availableGeometry.isEmpty() )
			setGeometry( config_->mainWindowGeometry() );
	}

	setStatusBar( 0 );

	// total progress bar
	ui_.totalProgressBar->setMinimum( 0 );
	ui_.totalProgressBar->setMaximum( kMaxTotalProgressBarValue );

	// action buttons
	new ButtonActionBinder( ui_.actionAddFiles, ui_.addFilesButton, this );
	new ButtonActionBinder( ui_.actionAddDirectory, ui_.addDirectoryButton, this );
	new ButtonActionBinder( ui_.actionRemoveSelected, ui_.removeSelectedButton, this );
	new ButtonActionBinder( ui_.actionRemoveAll, ui_.removeAllButton, this );
	new ButtonActionBinder( ui_.actionUnmark, ui_.unmarkButton, this );
	new ButtonActionBinder( ui_.actionStartConversion, ui_.startConversionButton, this );
	new ButtonActionBinder( ui_.actionStopConversion, ui_.stopConversionButton, this );
	new ButtonActionBinder( ui_.actionAddProfile, ui_.addProfileButton, this );
	new ButtonActionBinder( ui_.actionRemoveProfile, ui_.removeProfileButton, this );
	new ButtonActionBinder( ui_.actionRenameProfile, ui_.renameProfileButton, this );

	// drop hint label
	ui_.jobView->viewport()->installEventFilter( this );
	dropHintLabel_ = new QLabel( ui_.jobView->viewport() );
	dropHintLabel_->setAlignment( Qt::AlignCenter );
	dropHintLabel_->setTextFormat( Qt::RichText );

	// profiles
	profileItemModel_ = new ProfileItemModel( config_, this );

	selfChangeCurrentProfileIndex_ = true;
	ui_.profileComboBox->setModel( profileItemModel_ );
	ui_.profileComboBox->setCurrentIndex( profileItemModel_->rowForProfileId( config_->currentCustomProfileIndex() == -1 ?
			ProfileItemModel::ProfileId_FileSystem : config_->customProfileIds().at( config_->currentCustomProfileIndex() ) ) );
	selfChangeCurrentProfileIndex_ = false;

	QCompleter * const profilePathCompleter = new QCompleter( ui_.profilePathLineEdit );
	QDirModel * const profilePathDirModel = new QDirModel( QStringList(), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name,
			profilePathCompleter );
	profilePathCompleter->setModel( profilePathDirModel );
	ui_.profilePathLineEdit->setCompleter( profilePathCompleter );

	_updateCurrentProfile();

	// jobs
	jobItemModel_ = new JobItemModel( this );
	ui_.jobView->setModel( jobItemModel_ );

	const QList<int> visualColumnOrder = QList<int>()
			<< JobItemModel::Column_Name
			<< JobItemModel::Column_Format
			<< JobItemModel::Column_Progress
			<< JobItemModel::Column_State;
	for ( int visualColumnIndex = 0; visualColumnIndex < visualColumnOrder.count(); ++visualColumnIndex )
	{
		const int currentVisualIndex = ui_.jobView->header()->visualIndex( visualColumnOrder.at( visualColumnIndex ) );
		ui_.jobView->header()->moveSection( currentVisualIndex, visualColumnIndex );
	}

	connect( jobItemModel_, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(_jobItemModelContentsChanged()) );
	connect( jobItemModel_, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(_jobItemModelContentsChanged()) );
	connect( jobItemModel_, SIGNAL(modelReset()), SLOT(_jobItemModelContentsChanged()) );
	connect( jobItemModel_, SIGNAL(fileItemAboutToRemove()), SLOT(_jobFileItemAboutToRemove()) );
	connect( jobItemModel_, SIGNAL(itemProgressChanged()), SLOT(_jobItemProgressChanged()) );

	_setJobItemModelSourcePaths();

	ui_.jobView->header()->restoreState( config_->jobViewHeaderState() );
	ui_.jobView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	connect( ui_.jobView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(_jobSelectionChanged()) );
	connect( jobItemModel_, SIGNAL(modelReset()), SLOT(_jobSelectionChanged()) );
	_jobSelectionChanged();

	_updateTotalProgressBar();
	_updateJobActions();

	_retranslateUi();
}


MainWindow::~MainWindow()
{
	Q_ASSERT( !fileFetcher_->isRunning() );
}


void MainWindow::abort()
{
	fileFetcher_->abort();

	localizationManager_->setCurrentLocalization( Grim::Tools::LocalizationInfo() );
}


int MainWindow::currentProfileId() const
{
	return profileItemModel_->profileIdForRow( ui_.profileComboBox->currentIndex() );
}


Config::Profile MainWindow::currentProfile() const
{
	const int currentProfileIndex = ui_.profileComboBox->currentIndex();
	Q_ASSERT( currentProfileIndex != -1 );

	const Config::Profile profile = profileItemModel_->profileForRow( currentProfileIndex );
	Q_ASSERT( !profile.isNull() );

	return profile;
}


void MainWindow::setCurrentProfile( const Config::Profile & profile )
{
	const int currentProfileId = this->currentProfileId();
	if ( ProfileItemModel::isCustomProfile( currentProfileId ) )
		config_->setCustomProfileForId( currentProfileId, profile );
	else
		config_->setFileSystemProfile( profile );
}


void MainWindow::_retranslateUi()
{
	setWindowTitle( Global::applicationName() );

	_retranslateDropHintLabel();
}


void MainWindow::_retranslateDropHintLabel()
{
	if ( !dropHintLabel_ )
		return;

	if ( underDragging_ )
		dropHintLabel_->setText( tr( "Go on, drop it!" ) );
	else
		dropHintLabel_->setText( tr( "Drag'n'drop files here you want to convert." ) );
}


QString MainWindow::_collectMediaFileFilterString() const
{
	QString string;

	// all media files
	string += tr( "All media files" ) + QString::fromLatin1( "(%1)" )
			.arg( Global::fileFiltersForExtensions( converter_->audioFormatManager()->allAvailableFileExtensions() ).join( " " ) );

	// all files
	string += ";;" + tr( "All files" ) + QString::fromLatin1( "(*.*)" );

	// separate plugins
	foreach ( const QString & format, converter_->audioFormatManager()->availableFileFormats() )
	{
		string += ";;" + QString::fromLatin1( "%1 (%2)" )
				.arg( format )
				.arg( Global::fileFiltersForExtensions( converter_->audioFormatManager()->availableFileExtensionsForFormat( format ) ).join( " " ) );
	}

	return string;
}


QStringList MainWindow::_collectMediaFileFilters() const
{
	return Global::fileFiltersForExtensions( converter_->audioFormatManager()->allAvailableFileExtensions() );
}


void MainWindow::_updateTotalProgressBar()
{
	static const QString kProgressBarTemplate = QLatin1String( "(%1 / %2) %3 %" );

	if ( jobItemModel_->allFileItems().isEmpty() )
	{
		ui_.totalProgressBar->setValue( 0 );
		ui_.totalProgressBar->setFormat( QString() );
	}
	else
	{
		const JobItemModel::DirItem * const rootItem = jobItemModel_->itemForIndex( QModelIndex() )->asDir();
		const int value = qBound( 0, qRound( rootItem->totalProgress*kMaxTotalProgressBarValue ), kMaxTotalProgressBarValue );
		const int percents = JobItemModel::progressToPercents( rootItem->totalProgress );

		const int totalCount = jobItemModel_->allFileItems().count();
		const int unfinishedCount = jobItemModel_->allUnfinishedFileItems().count();
		const int finishedCount = totalCount - unfinishedCount;

		// show 99% unless all jobs will be finished
		const int truncatedPercents = unfinishedCount == 0 ?
				JobItemModel::progressToPercents( 1.0 ) :
				qMin( percents, JobItemModel::progressToPercents( 1.0 ) - 1 );

		ui_.totalProgressBar->setValue( value );
		ui_.totalProgressBar->setFormat( kProgressBarTemplate
				.arg( finishedCount )
				.arg( totalCount )
				.arg( truncatedPercents ) );
	}
}


void MainWindow::_updateJobActions()
{
	const bool hasFileItems = !jobItemModel_->allFileItems().isEmpty();
	const bool hasActiveItems = !jobItemModel_->allActiveFileItems().isEmpty();
	const bool hasInactiveUnfinishedItems = !jobItemModel_->allInactiveUnfinishedFileItems().isEmpty();

	ui_.actionStartConversion->setEnabled( hasFileItems && hasInactiveUnfinishedItems );
	ui_.actionStopConversion->setEnabled( hasFileItems && hasActiveItems );
	ui_.actionUnmark->setEnabled( hasFileItems && !hasActiveItems );
	ui_.actionRemoveAll->setEnabled( hasFileItems );
}


void MainWindow::_adjustDropHintLabel()
{
	if ( !dropHintLabel_ )
		return;

	dropHintLabel_->setGeometry( ui_.jobView->viewport()->rect() );

	if ( jobItemModel_->rowCount( QModelIndex() ) == 0 )
	{
		dropHintLabel_->show();
		dropHintLabel_->raise();
	}
	else
	{
		dropHintLabel_->hide();
	}
}


void MainWindow::_updateFileFetcherActions()
{
	ui_.actionPreferences->setEnabled( !fileFetcher_->isRunning() );
}


void MainWindow::_updateCurrentProfile()
{
	const int currentProfileId = this->currentProfileId();
	const bool isCustomProfile = ProfileItemModel::isCustomProfile( currentProfileId );
	config_->setCurrentCustomProfileIndex( isCustomProfile ?
			config_->customProfileIndexForId( currentProfileId ) : -1 );

	const Config::Profile currentProfile = this->currentProfile();
	ui_.profilePathLineEdit->setText( currentProfile.path );
	ui_.profileQualityWidget->setValue( currentProfile.quality );
	ui_.profilePrependYearToAlbumCheckBox->setChecked( currentProfile.prependYearToAlbum );

	ui_.actionRemoveProfile->setEnabled( isCustomProfile );
	ui_.actionRenameProfile->setEnabled( isCustomProfile );
}


void MainWindow::_setJobItemModelSourcePaths()
{
	QStringList sourcePaths;
	foreach ( const Config::SourceDir & sourceDir, config_->sourceDirs() )
		sourcePaths << sourceDir.path;
	jobItemModel_->setSourcePaths( sourcePaths );
}


void MainWindow::_startFileFetch( const QList<QUrl> & urls )
{
	Q_ASSERT( !fileFetcher_->isRunning() );

	fetchedFileCount_ = 0;
	failedFetchedFilePaths_.clear();
	nonRecognizedFetchedFileInfos_.clear();

	fileFetcherDialog_->setFetchFinished( false );
	fileFetcher_->setUrls( urls );
	fileFetcher_->start();
	fileFetcherDialogAppearTimer_->start();

	_updateFileFetcherActions();
}


void MainWindow::_finishFileFetch()
{
	if ( !nonRecognizedFetchedFileInfos_.isEmpty() )
	{
		fileFetcherDialog_->hide();

		if ( !nonRecognizedFilesDialog_ )
			nonRecognizedFilesDialog_ = new NonRecognizedFilesDialog( this );

		QStringList filePaths;
		foreach ( const FetchedFileInfo & fetchFileInfo, nonRecognizedFetchedFileInfos_ )
			filePaths << fetchFileInfo.filePath;

		if ( nonRecognizedFilesDialog_->exec( filePaths ) == QDialog::Accepted )
		{
			foreach ( const FetchedFileInfo & fetchFileInfo, nonRecognizedFetchedFileInfos_ )
				_tryAddFile( fetchFileInfo.filePath, fetchFileInfo.basePath, QString() );
		}
	}

	if ( !failedFetchedFilePaths_.isEmpty() )
	{
		fileFetcherDialog_->hide();

		if ( !skippedFilesDialog_ )
			skippedFilesDialog_ = new SkippedFilesDialog( this );

		skippedFilesDialog_->exec( failedFetchedFilePaths_ );
	}

	failedFetchedFilePaths_.clear();
	nonRecognizedFetchedFileInfos_.clear();

	_updateFileFetcherActions();
}


bool MainWindow::_tryAddFile( const QString & filePath, const QString & basePath, const QString & format )
{
	bool isAdded;
	const QModelIndex index = jobItemModel_->addFile( filePath, basePath, format, isAdded );

	if ( !index.isValid() )
	{
		failedFetchedFilePaths_ << filePath;
		return false;
	}

	// expand index branch to make it visible
	for ( QModelIndex parentIndex = index.parent(); parentIndex.isValid(); parentIndex = parentIndex.parent() )
		ui_.jobView->expand( parentIndex );

	return true;
}


void MainWindow::closeEvent( QCloseEvent * const e )
{
	e->ignore();

	ui_.actionQuit->trigger();
}


void MainWindow::dragEnterEvent( QDragEnterEvent * const e )
{
	if ( fileFetcher_->isRunning() )
	{
		e->ignore();
		return;
	}

	const QMimeData * mimeData = e->mimeData();
	const QStringList formats = mimeData->formats();
	if ( !formats.contains( "text/uri-list" ) )
		return;

	e->accept();

	underDragging_ = true;

	_retranslateDropHintLabel();
	_adjustDropHintLabel();
}


void MainWindow::dragLeaveEvent( QDragLeaveEvent * const e )
{
	underDragging_ = false;

	_retranslateDropHintLabel();
	_adjustDropHintLabel();
}


void MainWindow::dropEvent( QDropEvent * const e )
{
	underDragging_ = false;
	_retranslateDropHintLabel();
	_adjustDropHintLabel();

	if ( fileFetcher_->isRunning() )
	{
		e->ignore();
		return;
	}

	const QMimeData * mimeData = e->mimeData();
	const QStringList formats = mimeData->formats();
	if ( !formats.contains( "text/uri-list" ) )
		return;

	e->accept();

	_startFileFetch( mimeData->urls() );
}


void MainWindow::changeEvent( QEvent * const e )
{
	switch ( e->type() )
	{
	case QEvent::LanguageChange:
		ui_.retranslateUi( this );
		_retranslateUi();
		break;
	}

	QMainWindow::changeEvent( e );
}


bool MainWindow::eventFilter( QObject * const o, QEvent * const e )
{
	if ( o == ui_.jobView->viewport() )
	{
		switch ( e->type() )
		{
		case QEvent::Resize:
			_adjustDropHintLabel();
			break;

		case QEvent::DragEnter:
		{
		}
			break;

		case QEvent::Drop:
		{
		}
			break;
		}
	}

	return QMainWindow::eventFilter( o, e );
}


void MainWindow::_aboutToQuit()
{
	config_->setMainWindowStayOnTop( windowFlags() & Qt::WindowStaysOnTopHint );
	config_->setMainWindowGeometry( geometry() );
	config_->setMainWindowMaximized( isMaximized() );

	config_->setJobViewHeaderState( ui_.jobView->header()->saveState() );
}


void MainWindow::_jobItemModelContentsChanged()
{
	_adjustDropHintLabel();
	_updateTotalProgressBar();
	_updateJobActions();
}


void MainWindow::_jobStarted( const int jobId )
{
	jobItemModel_->setJobStarted( jobId );
}


void MainWindow::_jobResolvedFormat( const int jobId, const QString & format )
{
	jobItemModel_->setJobResolvedFormat( jobId, format );
}


void MainWindow::_jobProgress( const int jobId, const qreal progress )
{
	const JobItemModel::FileItem * const fileItem = jobItemModel_->fileItemForJobId( jobId );
	const QModelIndex index = jobItemModel_->indexForItem( fileItem );
	jobItemModel_->setFileItemProgressForIndex( index, progress );
}


void MainWindow::_jobFinished( const int jobId, const int result )
{
	jobItemModel_->setJobFinished( jobId, result );
	_updateJobActions();
}


void MainWindow::_currentFetchDirChanged( const QString & dirPath )
{
	fileFetcherDialog_->setCurrentDir( dirPath );
}


void MainWindow::_fetched( const QString & filePath, const QString & basePath, const bool extensionRecognized )
{
	fetchedFileCount_++;
	fileFetcherDialog_->setFileCount( fetchedFileCount_ );

	if ( !extensionRecognized )
	{
		nonRecognizedFetchedFileInfos_ << FetchedFileInfo( filePath, basePath );
		return;
	}

	const QString extension = QFileInfo( filePath ).suffix();
	const QStringList formats = converter_->audioFormatManager()->formatsForExtension( extension );
	Q_ASSERT( !formats.isEmpty() );

	if ( !_tryAddFile( filePath, basePath, formats.first() ) )
		return;
}


void MainWindow::_fetchFinished()
{
	fileFetcherDialog_->setFetchFinished( true );
	fileFetcherDialogAppearTimer_->stop();

	_finishFileFetch();
}


void MainWindow::_showFileFetcherDialog()
{
	fileFetcherDialog_->setFileCount( fetchedFileCount_ );
	fileFetcherDialog_->show();
}


void MainWindow::_fileFetchDialogAborted()
{
	fileFetcherDialog_->setFetchFinished( true );
	fileFetcherDialog_->close();

	fileFetcherDialogAppearTimer_->stop();

	fileFetcher_->abort();

	_finishFileFetch();
}


void MainWindow::_jobSelectionChanged()
{
	ui_.actionRemoveSelected->setEnabled( ui_.jobView->selectionModel()->hasSelection() );
}


void MainWindow::_jobFileItemAboutToRemove()
{
	const JobItemModel::FileItem * const fileItem = jobItemModel_->aboutToRemoveFileItem();

	if ( fileItem->jobId != 0 )
		converter_->abortJob( fileItem->jobId );
}


void MainWindow::_jobItemProgressChanged()
{
	const JobItemModel::Item * const item = jobItemModel_->progressChangedItem();
	const QModelIndex index = jobItemModel_->indexForItem( item );

	if ( !index.isValid() )
		_updateTotalProgressBar();
}


void MainWindow::on_profileComboBox_currentIndexChanged()
{
	if ( selfChangeCurrentProfileIndex_ )
		return;

	_updateCurrentProfile();
}


void MainWindow::on_profilePathBrowseButton_clicked()
{
	Config::Profile currentProfile = this->currentProfile();

	const QString path = QFileDialog::getExistingDirectory( this,
			Global::makeWindowTitle( tr( "Select directory for profile <b>%1</b>" ).arg( currentProfile.name ) ),
			currentProfile.path );
	if ( path.isNull() )
		return;

	currentProfile.path = path;

	setCurrentProfile( currentProfile );

	ui_.profilePathLineEdit->setText( currentProfile.path );
}


void MainWindow::on_profilePathLineEdit_textChanged()
{
	Config::Profile currentProfile = this->currentProfile();
	currentProfile.path = ui_.profilePathLineEdit->text();
	setCurrentProfile( currentProfile );
}


void MainWindow::on_profilePrependYearToAlbumCheckBox_toggled()
{
	Config::Profile currentProfile = this->currentProfile();
	currentProfile.prependYearToAlbum = ui_.profilePrependYearToAlbumCheckBox->isChecked();
	setCurrentProfile( currentProfile );
}


void MainWindow::on_profileQualityWidget_valueChanged()
{
	Config::Profile currentProfile = this->currentProfile();
	currentProfile.quality = ui_.profileQualityWidget->value();
	setCurrentProfile( currentProfile );
}


void MainWindow::on_actionAddFiles_triggered()
{
	if ( fileFetcher_->isRunning() )
		return;

	const QStringList filePaths = QFileDialog::getOpenFileNames( this,
			Global::makeWindowTitle( tr( "Select media files" ) ),
			config_->lastUsedFilesDirPath(), _collectMediaFileFilterString() );

	if ( filePaths.isEmpty() )
		return;

	config_->setLastUsedFilesDirPath( QFileInfo( filePaths.first() ).path() );

	QList<QUrl> urls;
	foreach ( const QString & filePath, filePaths )
		urls << QUrl::fromLocalFile( filePath );

	_startFileFetch( urls );
}


void MainWindow::on_actionAddDirectory_triggered()
{
	if ( fileFetcher_->isRunning() )
		return;

	const QString dirPath = QFileDialog::getExistingDirectory( this,
			Global::makeWindowTitle( tr( "Select directory with media files" ) ),
			config_->lastUsedFilesDirPath() );

	if ( dirPath.isEmpty() )
		return;

	config_->setLastUsedFilesDirPath( dirPath );

	_startFileFetch( QList<QUrl>() << QUrl::fromLocalFile( dirPath ) );
}


void MainWindow::on_actionRemoveSelected_triggered()
{
	QList<QPersistentModelIndex> selectedIndexes;
	foreach ( const QModelIndex & index, ui_.jobView->selectionModel()->selectedRows( JobItemModel::Column_Name ) )
		selectedIndexes << index;

	foreach ( const QPersistentModelIndex & index, selectedIndexes )
	{
		if ( !index.isValid() )
			continue;

		jobItemModel_->removeItemByIndex( index );
	}
}


void MainWindow::on_actionRemoveAll_triggered()
{
	if ( !jobItemModel_->allActiveFileItems().isEmpty() )
	{
		if ( QMessageBox::question( this, Global::makeWindowTitle( "Removing all files" ),
				tr( "Conversion is ongoing, do you really want to stop it and clear the list?" ),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
			return;
	}

	jobItemModel_->removeAllItems();
}


void MainWindow::on_actionUnmark_triggered()
{
	foreach ( const JobItemModel::FileItem * const fileItem, jobItemModel_->allFileItems() )
	{
		if ( fileItem->jobId != 0 )
			continue;

		const QModelIndex index = jobItemModel_->indexForItem( fileItem );
		jobItemModel_->setFileItemUnmarkedForIndex( index );
	}

	_updateJobActions();
}


void MainWindow::on_actionAddProfile_triggered()
{
	if ( !profileNameDialog_ )
		profileNameDialog_ = new ProfileNameDialog( this );

	profileNameDialog_->setName( QString() );
	if ( profileNameDialog_->exec() != QDialog::Accepted )
		return;

	profileItemModel_->beginAddProfile();

	const int profileId = config_->addCustomProfile();
	Config::Profile profile = config_->customProfileForId( profileId );
	profile.name = profileNameDialog_->name();
	config_->setCustomProfileForId( profileId, profile );

	profileItemModel_->endAddProfile();

	const int currentProfileIndex = profileItemModel_->rowForProfileId( profileId );
	ui_.profileComboBox->setCurrentIndex( currentProfileIndex );
}


void MainWindow::on_actionRemoveProfile_triggered()
{
	const Config::Profile currentProfile = this->currentProfile();

	if ( QMessageBox::question( this,
			Global::makeWindowTitle( tr( "Removing profile" ) ),
			tr( "Do you really want to remove profile <b>%1</b>?" ).arg( currentProfile.name ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		return;

	profileItemModel_->beginRemoveProfile( currentProfileId() );
	config_->removeCustomProfile( currentProfileId() );
	profileItemModel_->endRemoveProfile();
}


void MainWindow::on_actionRenameProfile_triggered()
{
	if ( !profileNameDialog_ )
		profileNameDialog_ = new ProfileNameDialog( this );

	Config::Profile currentProfile = this->currentProfile();

	profileNameDialog_->setName( currentProfile.name );
	if ( profileNameDialog_->exec() != QDialog::Accepted )
		return;

	profileItemModel_->beginChangeProfile( currentProfileId() );

	currentProfile.name = profileNameDialog_->name();
	config_->setCustomProfileForId( currentProfileId(), currentProfile );

	profileItemModel_->endChangeProfile();
}


void MainWindow::on_actionStartConversion_triggered()
{
	const Config::Profile currentProfile = this->currentProfile();

	const QDir profileDir = QDir( currentProfile.path );

	foreach ( const JobItemModel::FileItem * const fileItem, jobItemModel_->allInactiveFileItems() )
	{
		if ( fileItem->result != Converter::JobResult_Null )
		{
			// already finished, skipping
			continue;
		}

		const int jobId = converter_->addJob( fileItem->sourcePath, fileItem->format,
				profileDir.absoluteFilePath( fileItem->relativeDestinationPath ),
				currentProfile.quality, currentProfile.prependYearToAlbum );

		const QModelIndex index = jobItemModel_->indexForItem( fileItem );
		jobItemModel_->setFileItemJobIdForIndex( index, jobId );
	}

	_updateJobActions();
}


void MainWindow::on_actionStopConversion_triggered()
{
	converter_->abortAllJobs();

	foreach ( const JobItemModel::FileItem * const fileItem, jobItemModel_->allActiveFileItems() )
	{
		const QModelIndex index = jobItemModel_->indexForItem( fileItem );
		jobItemModel_->setFileItemJobIdForIndex( index, 0 );
		jobItemModel_->setFileItemProgressForIndex( index, 0 );
	}

	Q_ASSERT( jobItemModel_->allActiveFileItems().isEmpty() );
	Q_ASSERT( jobItemModel_->allFileItems().count() == jobItemModel_->allInactiveFileItems().count() );

	_updateJobActions();
}


void MainWindow::on_actionQuit_triggered()
{
	if ( !jobItemModel_->allActiveFileItems().isEmpty() )
	{
		if ( QMessageBox::question( this,
				Global::makeWindowTitle( tr( "Quit confirmation" ) ),
				tr( "Conversion is still active, do you really want to abort it and quit anyway?" ),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
			return;
	}

	QCoreApplication::quit();
}


void MainWindow::on_actionPreferences_triggered()
{
	if ( !preferencesDialog_ )
		preferencesDialog_ = new PreferencesDialog( config_, localizationManager_, this );

	preferencesDialog_->exec();

	converter_->setConcurrentThreadCount( config_->concurrentThreadCount() );

	if ( preferencesDialog_->hasSourcePathChanged() )
		_setJobItemModelSourcePaths();
}


void MainWindow::on_actionStayOnTop_toggled()
{
	const Qt::WindowType flag = Qt::WindowStaysOnTopHint;

	const Qt::WindowFlags flags = ui_.actionStayOnTop->isChecked() ?
			windowFlags() | flag :
			windowFlags() & ~flag;

	foggDebug() << flag << windowFlags() << flags;

	// FIXME: By some reason this causes window to hide on Linux.
#if 0
	setWindowFlags( flags );
#endif
}


void MainWindow::on_actionExpandAll_triggered()
{
	ui_.jobView->expandAll();
}


void MainWindow::on_actionCollapseAll_triggered()
{
	ui_.jobView->collapseAll();
}


void MainWindow::on_actionDonate_triggered()
{
	if ( !donationDialog_ )
		donationDialog_ = new DonationDialog( this );
	donationDialog_->exec();
}


void MainWindow::on_actionAbout_triggered()
{
	if ( !aboutDialog_ )
		aboutDialog_ = new AboutDialog( converter_->audioFormatManager(), this );
	aboutDialog_->exec();
}




} // namespace Fogg
