
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
#include "TargetNameDialog.h"
#include "DonationDialog.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "SkippedFilesDialog.h"
#include "NonRecognizedFilesDialog.h"
#include "JobItemModel.h"
#include "ButtonActionBinder.h"




namespace Fogg {




static const QString kEnLanguageName = QLatin1String( "en" );
static const int kFileFetchDialogAppearTimeout = 1000;
static const int kMaxTotalProgressBarValue = 10000;




bool TargetItemModel::isDeviceTarget( const int targetId )
{
	return targetId > 0;
}


TargetItemModel::TargetItemModel( Config * const config, QObject * const parent ) :
	QAbstractItemModel( parent ),
	config_( config )
{
}


TargetItemModel::~TargetItemModel()
{
}


int TargetItemModel::targetIdForRow( const int row ) const
{
	Q_ASSERT( row >= 0 && row < _rowCount() );

	switch ( row )
	{
	case 0:
		return TargetId_FileSystem;
	case 1:
		return TargetId_Null;
	default:
		return config_->deviceTargetIds().at( row - 2 );
	}

	Q_ASSERT( false );
	return 0;
}


Config::Target TargetItemModel::targetForRow( const int row ) const
{
	const int targetId = targetIdForRow( row );

	switch ( targetId )
	{
	case TargetId_FileSystem:
		return config_->fileSystemTarget();
	case TargetId_Null:
		return Config::Target();
	default:
		return config_->deviceTargetForId( targetId );
	}

	Q_ASSERT( false );
	return Config::Target();
}


int TargetItemModel::rowForTargetId( const int targetId ) const
{
	switch ( targetId )
	{
	case TargetId_FileSystem:
		return 0;
	case TargetId_Null:
		return 1;
	default:
		return 2 + config_->deviceTargetIndexForId( targetId );
	}

	Q_ASSERT( false );
	return -1;
}


void TargetItemModel::beginAddTarget()
{
	const int lastRow = _rowForDeviceTargetIndex( config_->deviceTargetIds().count() );
	const int firstRow = config_->deviceTargetIds().isEmpty() ? rowForTargetId( TargetId_Null ) : lastRow;

	beginInsertRows( QModelIndex(), firstRow, lastRow );
}


void TargetItemModel::endAddTarget()
{
	endInsertRows();
}


void TargetItemModel::beginChangeTarget( const int targetId )
{
	targetIdBeingChanged_ = targetId;
}


void TargetItemModel::endChangeTarget()
{
	const int row = rowForTargetId( targetIdBeingChanged_ );
	const QModelIndex targetModelIndex = index( row, 0 );
	emit dataChanged( targetModelIndex, targetModelIndex );
}


void TargetItemModel::beginRemoveTarget( const int targetId )
{
	const int lastRow = rowForTargetId( targetId );
	const int firstRow = config_->deviceTargetIds().count() == 1 ? rowForTargetId( TargetId_Null ) : lastRow;

	beginRemoveRows( QModelIndex(), firstRow, lastRow );
}


void TargetItemModel::endRemoveTarget()
{
	endRemoveRows();
}


int TargetItemModel::rowCount( const QModelIndex & parent ) const
{
	if ( !parent.isValid() )
		return _rowCount();
	return 0;
}


int TargetItemModel::columnCount( const QModelIndex & parent ) const
{
	if ( !parent.isValid() )
		return 1;
	return 0;
}


QModelIndex TargetItemModel::parent( const QModelIndex & index ) const
{
	Q_UNUSED( index );
	return QModelIndex();
}


QModelIndex TargetItemModel::index( const int row, const int column, const QModelIndex & parent ) const
{
	if ( !parent.isValid() && row >= 0 && row < _rowCount() && column == 0 )
		return createIndex( row, column, targetIdForRow( row ) );
	return QModelIndex();
}


QVariant TargetItemModel::data( const QModelIndex & index, const int role ) const
{
	if ( !index.isValid() )
		return QVariant();

	const int targetId = targetIdForRow( index.row() );

	switch ( targetId )
	{
	case TargetId_FileSystem:
	{
		switch ( role )
		{
		case Qt::DisplayRole:
			return tr( "File System" );
		}
	}
		break;

	case TargetId_Null:
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
		const Config::Target target = config_->deviceTargetForId( targetId );
		switch ( role )
		{
		case Qt::DisplayRole:
			return target.name;
		}
	}
		break;
	}

	return QVariant();
}


Qt::ItemFlags TargetItemModel::flags( const QModelIndex & index ) const
{
	if ( !index.isValid() )
		return Qt::NoItemFlags;

	const int targetId = targetIdForRow( index.row() );

	switch ( targetId )
	{
	case TargetId_Null:
		return Qt::NoItemFlags;
	case TargetId_FileSystem:
	default:
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	Q_ASSERT( false );
	return Qt::NoItemFlags;
}


int TargetItemModel::_rowCount() const
{
	// file system + separator + devices
	const int deviceTargetCount = config_->deviceTargetIds().count();
	return 1 + deviceTargetCount + (deviceTargetCount == 0 ? 0 : 1);
}


int TargetItemModel::_rowForDeviceTargetIndex( const int deviceTargetIndex ) const
{
	Q_ASSERT( deviceTargetIndex >= 0 );

	// 2 == file system index + separator index
	return deviceTargetIndex + 2;
}




MainWindow::MainWindow( Config * const config, Converter * const converter ) :
	config_( config ),
	converter_( converter )
{
	selfChangeCurrentTargetIndex_ = false;
	underDragging_ = false;

	// localization
	localizationManager_ = new Grim::Tools::LocalizationManager( this );

	translationsDirPath_ = QString::fromUtf8( qgetenv( "FOGG_TRANSLATIONS_DIR" ).constData() );
	localizationManager_->loadTranslations( translationsDirPath_ );
	foggDebug() << "Found locales:" << localizationManager_->names();

	// save 'en' localization
	localizationManager_->setFallbackLocalization( localizationManager_->localizationForName( kEnLanguageName ) );
	if ( localizationManager_->fallbackLocalization().isNull() )
		foggWarning() << "No 'en' translation found";

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
			foggWarning() << "No translation found for locale:" << config_->language();
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
	new ButtonActionBinder( ui_.actionAddTarget, ui_.addTargetButton, this );
	new ButtonActionBinder( ui_.actionRemoveTarget, ui_.removeTargetButton, this );
	new ButtonActionBinder( ui_.actionRenameTarget, ui_.renameTargetButton, this );

	// drop hint label
	ui_.jobView->viewport()->installEventFilter( this );
	dropHintLabel_ = new QLabel( ui_.jobView->viewport() );
	dropHintLabel_->setAlignment( Qt::AlignCenter );
	dropHintLabel_->setTextFormat( Qt::RichText );

	// target
	targetItemModel_ = new TargetItemModel( config_, this );

	selfChangeCurrentTargetIndex_ = true;
	ui_.targetComboBox->setModel( targetItemModel_ );
	ui_.targetComboBox->setCurrentIndex( targetItemModel_->rowForTargetId( config_->currentDeviceTargetIndex() == -1 ?
			TargetItemModel::TargetId_FileSystem : config_->deviceTargetIds().at( config_->currentDeviceTargetIndex() ) ) );
	selfChangeCurrentTargetIndex_ = false;

	QCompleter * const targetPathCompleter = new QCompleter( ui_.targetPathLineEdit );
	QDirModel * targetPathDirModel = new QDirModel( QStringList(), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name, targetPathCompleter );
	targetPathCompleter->setModel( targetPathDirModel );
	ui_.targetPathLineEdit->setCompleter( targetPathCompleter );

	_updateCurrentTarget();

	// jobs
	jobItemModel_ = new JobItemModel( this );
	ui_.jobView->setModel( jobItemModel_ );

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


int MainWindow::currentTargetId() const
{
	return targetItemModel_->targetIdForRow( ui_.targetComboBox->currentIndex() );
}


Config::Target MainWindow::currentTarget() const
{
	const int currentTargetIndex = ui_.targetComboBox->currentIndex();
	Q_ASSERT( currentTargetIndex != -1 );

	const Config::Target target = targetItemModel_->targetForRow( currentTargetIndex );
	Q_ASSERT( !target.isNull() );

	return target;
}


void MainWindow::setCurrentTarget( const Config::Target & target )
{
	const int currentTargetId = this->currentTargetId();
	if ( TargetItemModel::isDeviceTarget( currentTargetId ) )
		config_->setDeviceTargetForId( currentTargetId, target );
	else
		config_->setFileSystemTarget( target );
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


void MainWindow::_updateCurrentTarget()
{
	const int currentTargetId = this->currentTargetId();
	const bool isDeviceTarget = TargetItemModel::isDeviceTarget( currentTargetId );
	config_->setCurrentDeviceTargetIndex( isDeviceTarget ?
			config_->deviceTargetIndexForId( currentTargetId ) : -1 );

	const Config::Target currentTarget = this->currentTarget();
	ui_.targetPathLineEdit->setText( currentTarget.path );
	ui_.targetQualityWidget->setValue( currentTarget.quality );
	ui_.targetPrependYearToAlbumCheckBox->setChecked( currentTarget.prependYearToAlbum );

	ui_.actionRemoveTarget->setEnabled( isDeviceTarget );
	ui_.actionRenameTarget->setEnabled( isDeviceTarget );
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


void MainWindow::on_targetComboBox_currentIndexChanged()
{
	if ( selfChangeCurrentTargetIndex_ )
		return;

	_updateCurrentTarget();
}


void MainWindow::on_targetPathBrowseButton_clicked()
{
	Config::Target currentTarget = this->currentTarget();

	const QString path = QFileDialog::getExistingDirectory( this,
			Global::makeWindowTitle( tr( "Select directory for device <b>%1</b>" ).arg( currentTarget.name ) ),
			currentTarget.path );
	if ( path.isNull() )
		return;

	currentTarget.path = path;

	setCurrentTarget( currentTarget );

	ui_.targetPathLineEdit->setText( currentTarget.path );
}


void MainWindow::on_targetPathLineEdit_textChanged()
{
	Config::Target currentTarget = this->currentTarget();
	currentTarget.path = ui_.targetPathLineEdit->text();
	setCurrentTarget( currentTarget );
}


void MainWindow::on_targetPrependYearToAlbumCheckBox_toggled()
{
	Config::Target currentTarget = this->currentTarget();
	currentTarget.prependYearToAlbum = ui_.targetPrependYearToAlbumCheckBox->isChecked();
	setCurrentTarget( currentTarget );
}


void MainWindow::on_targetQualityWidget_valueChanged()
{
	Config::Target currentTarget = this->currentTarget();
	currentTarget.quality = ui_.targetQualityWidget->value();
	setCurrentTarget( currentTarget );
}


void MainWindow::on_actionAddFiles_triggered()
{
	if ( fileFetcher_->isRunning() )
		return;

	const QStringList filePaths = QFileDialog::getOpenFileNames( this,
			Global::makeWindowTitle( tr( "Select audio files" ) ),
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
			Global::makeWindowTitle( tr( "Select directory with audio files" ) ),
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


void MainWindow::on_actionAddTarget_triggered()
{
	if ( !targetNameDialog_ )
		targetNameDialog_ = new TargetNameDialog( this );

	targetNameDialog_->setName( QString() );
	if ( targetNameDialog_->exec() != QDialog::Accepted )
		return;

	targetItemModel_->beginAddTarget();

	const int targetId = config_->addDeviceTarget();
	Config::Target target = config_->deviceTargetForId( targetId );
	target.name = targetNameDialog_->name();
	config_->setDeviceTargetForId( targetId, target );

	targetItemModel_->endAddTarget();

	const int currentTargetIndex = targetItemModel_->rowForTargetId( targetId );
	ui_.targetComboBox->setCurrentIndex( currentTargetIndex );
}


void MainWindow::on_actionRemoveTarget_triggered()
{
	Config::Target currentTarget = this->currentTarget();

	if ( QMessageBox::question( this,
			Global::makeWindowTitle( tr( "Removing device" ) ),
			tr( "Do you really want to remove device <b>%1</b>?" ).arg( currentTarget.name ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		return;

	targetItemModel_->beginRemoveTarget( currentTargetId() );
	config_->removeDeviceTarget( currentTargetId() );
	targetItemModel_->endRemoveTarget();
}


void MainWindow::on_actionRenameTarget_triggered()
{
	if ( !targetNameDialog_ )
		targetNameDialog_ = new TargetNameDialog( this );

	Config::Target currentTarget = this->currentTarget();

	targetNameDialog_->setName( currentTarget.name );
	if ( targetNameDialog_->exec() != QDialog::Accepted )
		return;

	targetItemModel_->beginChangeTarget( currentTargetId() );

	currentTarget.name = targetNameDialog_->name();
	config_->setDeviceTargetForId( currentTargetId(), currentTarget );

	targetItemModel_->endChangeTarget();
}


void MainWindow::on_actionStartConversion_triggered()
{
	const Config::Target currentTarget = this->currentTarget();

	const QDir targetDir = QDir( currentTarget.path );

	foreach ( const JobItemModel::FileItem * const fileItem, jobItemModel_->allInactiveFileItems() )
	{
		if ( fileItem->result != Converter::JobResult_Null )
		{
			// already finished, skipping
			continue;
		}

		const int jobId = converter_->addJob( fileItem->sourcePath, fileItem->format,
				targetDir.absoluteFilePath( fileItem->relativeDestinationPath ),
				currentTarget.quality, currentTarget.prependYearToAlbum );

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
