
#include "PreferencesDialog.h"

#include <QFileDialog>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QThread>

#include <grim/tools/LocalizationManager.h>

#include "Config.h"




namespace Fogg {




SourcePathItemModel::SourcePathItemModel( Config * const config, QObject * const parent ) :
	QAbstractListModel( parent ),
	config_( config )
{
}


int SourcePathItemModel::rowCount( const QModelIndex & parent ) const
{
	if ( parent.isValid() )
		return 0;

	return config_->sourceDirs().count();
}


QVariant SourcePathItemModel::data( const QModelIndex & index, const int role ) const
{
	if ( !index.isValid() )
		return QVariant();

	const Config::SourceDir sourceDir = config_->sourceDirs().at( index.row() );

	switch ( role )
	{
	case Qt::DisplayRole:
		return sourceDir.path;
	}

	return QVariant();
}


QVariant SourcePathItemModel::headerData( const int column, const Qt::Orientation orientation, const int role ) const
{
	if ( orientation != Qt::Horizontal )
		return QVariant();

	if ( column != 0 )
		return QVariant();

	switch ( role )
	{
	case Qt::DisplayRole:
		return tr( "Path" );
	}

	return QVariant();
}




PreferencesDialog::PreferencesDialog( Config * const config, Grim::Tools::LocalizationManager * const localizationManager,
		QWidget * const parent ) :
	QDialog( parent ),
	config_( config ),
	localizationManager_( localizationManager )
{
	sourcePathHasChanged_ = false;
	selfChangingLanguage_ = false;

	lastUsedSourceDirPath_ = QDir::homePath();

	ui_.setupUi( this );

	// apply preferences on change
	ui_.buttonBox->hide();

	// languages
	ui_.languageComboBox->addItem( tr( "System default" ), QVariant::fromValue( QString() ) );
	foreach ( const QString & localizationName, localizationManager_->names() )
	{
		const Grim::Tools::LocalizationInfo localization = localizationManager_->localizationForName( localizationName );
		ui_.languageComboBox->addItem( localization.nativeName(), QVariant::fromValue( localization.name() ) );
	}

	const int currentLanguageIndex = config_->language().isNull() ? 0 :
			ui_.languageComboBox->findData( QVariant::fromValue( config_->language() ) );
	Q_ASSERT( currentLanguageIndex != -1 );

	selfChangingLanguage_ = true;
	ui_.languageComboBox->setCurrentIndex( currentLanguageIndex );
	selfChangingLanguage_ = false;

	// concurrent threads
	ui_.concurrentThreadCountSpinBox->setMinimum( 0 );
	ui_.concurrentThreadCountSpinBox->setMaximum( config_->maximumConcurrentThreadCount() );

	// default quality
	ui_.defaultEncodingQualityWidget->setValue( config_->defaultQuality() );

	// source paths
	sourcePathItemModel_ = new SourcePathItemModel( config_, this );
	ui_.sourcePathView->setModel( sourcePathItemModel_ );
	ui_.sourcePathView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	connect( sourcePathItemModel_, SIGNAL(modelReset()), SLOT(_sourcePathViewSelectionChanged()) );
	connect( ui_.sourcePathView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
			SLOT(_sourcePathViewSelectionChanged()) );

	_sourcePathViewSelectionChanged();

	_retranslateUi();
}


void PreferencesDialog::done( const int result )
{


	QDialog::done( result );
}


void PreferencesDialog::changeEvent( QEvent * const e )
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


void PreferencesDialog::showEvent( QShowEvent * const e )
{
	sourcePathHasChanged_ = false;

	QDialog::showEvent( e );
}


void PreferencesDialog::_retranslateUi()
{
	setWindowTitle( Global::makeWindowTitle( tr( "Preferences" ) ) );

	const int idealThreadCount = QThread::idealThreadCount();
	const QString autoThreadCountText = tr( "Auto" );
	ui_.concurrentThreadCountSpinBox->setSpecialValueText( idealThreadCount == -1 ?
			autoThreadCountText :
			QString::fromLatin1( "%1 (%2)" ).arg( autoThreadCountText ).arg( idealThreadCount ) );
}


void PreferencesDialog::on_languageComboBox_activated( const int index )
{
	if ( selfChangingLanguage_ )
		return;

	config_->setLanguage( ui_.languageComboBox->itemData( index ).toString() );
	if ( config_->language().isNull() )
		localizationManager_->setCurrentLocalizationFromSystemLocale();
	else
		localizationManager_->setCurrentLocalization( localizationManager_->localizationForName( config_->language() ) );
}


void PreferencesDialog::on_concurrentThreadCountSpinBox_valueChanged()
{
	config_->setConcurrentThreadCount( ui_.concurrentThreadCountSpinBox->value() );
}


void PreferencesDialog::on_defaultEncodingQualityWidget_valueChanged()
{
	config_->setDefaultQuality( ui_.defaultEncodingQualityWidget->value() );
}


void PreferencesDialog::on_addSourcePathButton_clicked()
{
	const QString path = QFileDialog::getExistingDirectory( this,
			Global::makeWindowTitle( tr( "Select music directory" ) ), lastUsedSourceDirPath_ );

	if ( path.isNull() )
		return;

	if ( config_->sourceDirs().contains( Config::SourceDir( path ) ) )
		return;

	Config::SourceDir sourceDir;
	sourceDir.path = path;

	QList<Config::SourceDir> sourceDirs = config_->sourceDirs();
	sourceDirs << sourceDir;

	const int sourceDirIndex = config_->sourceDirs().count();
	sourcePathItemModel_->beginInsertRows( QModelIndex(), sourceDirIndex, sourceDirIndex );
	config_->setSourceDirs( sourceDirs );
	sourcePathItemModel_->endInsertRows();

	lastUsedSourceDirPath_ = path;

	sourcePathHasChanged_ = true;
}


void PreferencesDialog::on_removeSourcePathButton_clicked()
{
	if ( !ui_.sourcePathView->selectionModel()->hasSelection() )
		return;

	if ( QMessageBox::question( this, Global::makeWindowTitle( "Removing music source" ),
			tr( "Do you really want to remove selected music sources?" ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		return;

	QList<int> rows;
	foreach ( const QModelIndex & index, ui_.sourcePathView->selectionModel()->selectedRows() )
		rows << index.row();
	qSort( rows );

	QList<Config::SourceDir> sourceDirs = config_->sourceDirs();
	int removedCount = 0;
	foreach ( const int row, rows )
	{
		const int index = row - removedCount;
		sourcePathItemModel_->beginRemoveRows( QModelIndex(), index, index );
		sourceDirs.removeAt( index );
		config_->setSourceDirs( sourceDirs );
		sourcePathItemModel_->endRemoveRows();
		removedCount++;
	}

	sourcePathHasChanged_ = true;
}


void PreferencesDialog::_sourcePathViewSelectionChanged()
{
	const bool hasSelection = ui_.sourcePathView->selectionModel()->hasSelection();
	ui_.removeSourcePathButton->setEnabled( hasSelection );
}





} // namespace Fogg
