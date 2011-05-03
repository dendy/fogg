
#include "Config.h"

#include <QSettings>
#include <QDir>

#include "Global.h"




namespace Fogg {




// constants
static const int kMaximumConcurrentThreadCount = 16;

// defaults
static const QString kDefaultLanguageValue = QString();

static const qreal   kDefaultQualityValue = 0.2;
static const bool    kDefaultPrependYearToAlbumValue = false;

static const bool    kDefaultMainWindowStayOnTop = true;
static const bool    kDefaultMainWindowMaximized = false;

// property keys
static const QString kLanguageKey                 = QLatin1String( "language" );
static const QString kConcurrentThreadCountKey    = QLatin1String( "concurrent-thread-count" );
static const QString kDefaultQualityKey           = QLatin1String( "default-quality" );
static const QString kCurrentDeviceTargetIndexKey = QLatin1String( "current-device-target-index" );
static const QString kFileSystemTargetKey         = QLatin1String( "file-system-target" );
static const QString kDeviceTargetsKey            = QLatin1String( "device-targets" );
static const QString kSourceDirsKey               = QLatin1String( "source-dirs" );
static const QString kMainWindowStayOnTopKey      = QLatin1String( "main-window-stay-on-top" );
static const QString kMainWindowGeometryKey       = QLatin1String( "main-window-geometry" );
static const QString kMainWindowMaximizedKey      = QLatin1String( "main-window-maximized" );
static const QString kLastUsedFilesDirPathKey     = QLatin1String( "last-user-files-dir-path" );
static const QString kJobViewHeaderStateKey       = QLatin1String( "job-view-header-state" );

// target property keys
static const QString kTargetNameKey               = QLatin1String( "name" );
static const QString kTargetPathKey               = QLatin1String( "path" );
static const QString kTargetQualityKey            = QLatin1String( "quality" );
static const QString kTargetPrependYearToAlbumKey = QLatin1String( "prepend-year-to-album" );

// source dir property keys
static const QString kSourceDirPathKey = QLatin1String( "path" );




Config::Config( QObject * const parent ) :
	QObject( parent )
{
	// this value is fixed
	maximumConcurrentThreadCount_ = kMaximumConcurrentThreadCount;

	_clear();
}


void Config::_clear()
{
	deviceTargetsIdGenerator_ = Grim::Tools::IdGenerator();
	deviceTargetIds_.clear();
	deviceTargets_.clear();

	defaultQuality_ = kDefaultQualityValue;
	currentDeviceTargetIndex_ = -1;

	sourceDirs_.clear();

	mainWindowStayOnTop_ = kDefaultMainWindowStayOnTop;
	mainWindowGeometry_ = QRect();
	mainWindowMaximized_ = kDefaultMainWindowMaximized;

	lastUsedFilesDirPath_ = QDir::homePath();

	jobViewHeaderState_ = QByteArray();
}


bool Config::load()
{
	_clear();

	QSettings settings;

	// load language
	language_ = settings.value( kLanguageKey, kDefaultLanguageValue ).toString();

	// load concurrent thread count
	concurrentThreadCount_ = settings.value( kConcurrentThreadCountKey, 0 ).toInt();
	if ( concurrentThreadCount() < 0 || concurrentThreadCount() > maximumConcurrentThreadCount() )
		concurrentThreadCount_ = 0;

	// load default quality
	defaultQuality_ = settings.value( kDefaultQualityKey, kDefaultQualityValue ).toReal();

	// load file system target
	settings.beginGroup( kFileSystemTargetKey );
	const Target fileSystemTarget = _loadTargetFromSettings( settings );
	setFileSystemTarget( fileSystemTarget );
	settings.endGroup();

	// load device targets
	const int deviceTargetCount = settings.beginReadArray( kDeviceTargetsKey );
	for ( int deviceTargetIndex = 0; deviceTargetIndex < deviceTargetCount; ++deviceTargetIndex )
	{
		settings.setArrayIndex( deviceTargetIndex );
		const Target deviceTarget = _loadTargetFromSettings( settings );
		const int deviceTargetId = addDeviceTarget();
		setDeviceTargetForId( deviceTargetId, deviceTarget );
	}
	settings.endArray();

	// load current device target index
	currentDeviceTargetIndex_ = settings.value( kCurrentDeviceTargetIndexKey, -1 ).toInt();
	if ( currentDeviceTargetIndex_ == -1 || (currentDeviceTargetIndex_ >= 0 && currentDeviceTargetIndex_ < deviceTargetCount) )
	{
		// index is valid
	}
	else
	{
		currentDeviceTargetIndex_ = -1;
	}

	// load source dirs
	const int sourceDirCount = settings.beginReadArray( kSourceDirsKey );
	for ( int sourceDirIndex = 0; sourceDirIndex < sourceDirCount; ++sourceDirIndex)
	{
		settings.setArrayIndex( sourceDirIndex );

		SourceDir sourceDir;
		sourceDir.path = settings.value( kSourceDirPathKey ).toString();

		if ( sourceDir.path.isEmpty() )
			continue;

		if ( sourceDirs().contains( sourceDir ) )
			continue;

		sourceDirs_ << sourceDir;
	}
	settings.endArray();

	// user interface
	mainWindowStayOnTop_ = settings.value( kMainWindowStayOnTopKey, kDefaultMainWindowStayOnTop ).toBool();
	mainWindowGeometry_ = settings.value( kMainWindowGeometryKey ).toRect();
	mainWindowMaximized_ = settings.value( kMainWindowMaximizedKey, kDefaultMainWindowMaximized ).toBool();

	lastUsedFilesDirPath_ = settings.value( kLastUsedFilesDirPathKey, lastUsedFilesDirPath() ).toString();

	jobViewHeaderState_ = settings.value( kJobViewHeaderStateKey ).toByteArray();

	return true;
}


bool Config::save()
{
	QSettings settings;

	// save language
	settings.setValue( kLanguageKey, language() );

	// save concurrent thread count
	settings.setValue( kConcurrentThreadCountKey, concurrentThreadCount() );

	// save default quality
	settings.setValue( kDefaultQualityKey, defaultQuality() );

	// save file system target
	settings.beginGroup( kFileSystemTargetKey );
	_saveTargetToSettings( settings, fileSystemTarget() );
	settings.endGroup();

	// save device targets
	settings.beginWriteArray( kDeviceTargetsKey, deviceTargetIds().count() );
	for ( int deviceTargetIndex = 0; deviceTargetIndex < deviceTargetIds().count(); ++deviceTargetIndex )
	{
		settings.setArrayIndex( deviceTargetIndex );
		const int deviceTargetId = deviceTargetIds_.at( deviceTargetIndex );
		_saveTargetToSettings( settings, deviceTargetForId( deviceTargetId ) );
	}
	settings.endArray();

	// save current device target index
	settings.setValue( kCurrentDeviceTargetIndexKey, currentDeviceTargetIndex() );

	// save source dirs
	settings.beginWriteArray( kSourceDirsKey );
	for ( int sourceDirIndex = 0; sourceDirIndex < sourceDirs().count(); ++sourceDirIndex )
	{
		settings.setArrayIndex( sourceDirIndex );
		settings.setValue( kSourceDirPathKey, sourceDirs().at( sourceDirIndex ).path );
	}
	settings.endArray();

	// user interface
	settings.setValue( kMainWindowStayOnTopKey, mainWindowStayOnTop() );
	settings.setValue( kMainWindowGeometryKey, mainWindowGeometry() );
	settings.setValue( kMainWindowMaximizedKey, mainWindowMaximized() );

	settings.setValue( kLastUsedFilesDirPathKey, lastUsedFilesDirPath() );

	settings.setValue( kJobViewHeaderStateKey, jobViewHeaderState() );

	return true;
}


void Config::setLanguage( const QString & language )
{
	language_ = language;
}


void Config::setConcurrentThreadCount( const int count )
{
	Q_ASSERT( count >= 0 && count <= maximumConcurrentThreadCount() );

	concurrentThreadCount_ = count;
}


void Config::setDefaultQuality( const qreal quality )
{
	Q_ASSERT( quality >= kMinimumQualityValue && quality <= kMaximumQualityValue );

	defaultQuality_ = quality;
}


int Config::addDeviceTarget()
{
	const int deviceTargetId = deviceTargetsIdGenerator_.take();

	if ( deviceTargets_.size() <= deviceTargetId )
		deviceTargets_.resize( deviceTargetId + 1 );

	Target & target = deviceTargets_[ deviceTargetId ];
	target.isNull_ = false;
	target.path = _defaultFileSystemPath();
	target.quality = defaultQuality_;
	target.prependYearToAlbum = kDefaultPrependYearToAlbumValue;

	deviceTargetIds_ << deviceTargetId;

	return deviceTargetId;
}


void Config::removeDeviceTarget( const int deviceTargetId )
{
	const int index = deviceTargetIndexForId( deviceTargetId );
	deviceTargetIds_.removeAt( index );
	deviceTargetsIdGenerator_.free( deviceTargetId );

	if ( currentDeviceTargetIndex() != -1 && currentDeviceTargetIndex() == deviceTargetIds().count() )
		setCurrentDeviceTargetIndex( deviceTargetIds().isEmpty() ? -1 : deviceTargetIds().count() - 1 );
}


void Config::setFileSystemTarget( const Target & target )
{
	_assertTarget( target );
	fileSystemTarget_ = target;
}


void Config::setDeviceTargetForId( const int deviceTargetId, const Target & target )
{
	_assertTarget( target );
	Q_ASSERT( !deviceTargetsIdGenerator_.isFree( deviceTargetId ) );
	deviceTargets_[ deviceTargetId ] = target;
}


int Config::deviceTargetIndexForId( const int deviceTargetId ) const
{
	const int index = deviceTargetIds().indexOf( deviceTargetId );
	Q_ASSERT( index != -1 );
	return index;
}


void Config::setCurrentDeviceTargetIndex( const int index )
{
	Q_ASSERT( index == -1 || (index >= 0 && index < deviceTargetIds().count() ) );
	currentDeviceTargetIndex_ = index;
}


void Config::setSourceDirs( const QList<SourceDir> & sourceDirs )
{
	sourceDirs_ = sourceDirs;
}


void Config::setMainWindowStayOnTop( const bool set )
{
	mainWindowStayOnTop_ = set;
}


void Config::setMainWindowGeometry( const QRect & geometry )
{
	mainWindowGeometry_ = geometry;
}

void Config::setMainWindowMaximized( const bool set )
{
	mainWindowMaximized_ = set;
}


void Config::setLastUsedFilesDirPath( const QString & path )
{
	lastUsedFilesDirPath_ = path;
}


void Config::setJobViewHeaderState( const QByteArray & state )
{
	jobViewHeaderState_ = state;
}


QString Config::_defaultFileSystemPath() const
{
	// use home dir by default as file system path
	return QDir::homePath();
}


void Config::_assertTarget( const Target & target )
{
	Q_ASSERT( !target.isNull() );
	Q_ASSERT( target.quality >= kMinimumQualityValue && target.quality <= kMaximumQualityValue );
}


Config::Target Config::_loadTargetFromSettings( QSettings & settings )
{
	Target target;
	target.isNull_ = false;
	target.name = settings.value( kTargetNameKey ).toString();
	foggDebug() << target.name;
	target.path = settings.value( kTargetPathKey, _defaultFileSystemPath() ).toString();
	target.quality = qBound<qreal>( 0.0, settings.value( kTargetQualityKey, kDefaultQualityValue ).toReal(), 1.0 );
	target.prependYearToAlbum = settings.value( kTargetPrependYearToAlbumKey, kDefaultPrependYearToAlbumValue ).toBool();
	return target;
}


void Config::_saveTargetToSettings( QSettings & settings, const Target & target )
{
	_assertTarget( target );
	settings.setValue( kTargetNameKey, target.name );
	settings.setValue( kTargetPathKey, target.path );
	settings.setValue( kTargetQualityKey, target.quality );
	settings.setValue( kTargetPrependYearToAlbumKey, target.prependYearToAlbum );
}




} // namespace Fogg
