
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
static const QString kLanguageKey                  = QLatin1String( "language" );
static const QString kConcurrentThreadCountKey     = QLatin1String( "concurrent-thread-count" );
static const QString kDefaultQualityKey            = QLatin1String( "default-quality" );
static const QString kCurrentCustomProfileIndexKey = QLatin1String( "current-custom-profile-index" );
static const QString kFileSystemProfileKey         = QLatin1String( "file-system-profile" );
static const QString kCustomProfilesKey            = QLatin1String( "custom-profiles" );
static const QString kSourceDirsKey                = QLatin1String( "source-dirs" );
static const QString kMainWindowStayOnTopKey       = QLatin1String( "main-window-stay-on-top" );
static const QString kMainWindowGeometryKey        = QLatin1String( "main-window-geometry" );
static const QString kMainWindowMaximizedKey       = QLatin1String( "main-window-maximized" );
static const QString kLastUsedFilesDirPathKey      = QLatin1String( "last-user-files-dir-path" );
static const QString kJobViewHeaderStateKey        = QLatin1String( "job-view-header-state" );

// profile property keys
static const QString kProfileNameKey               = QLatin1String( "name" );
static const QString kProfilePathKey               = QLatin1String( "path" );
static const QString kProfileQualityKey            = QLatin1String( "quality" );
static const QString kProfilePrependYearToAlbumKey = QLatin1String( "prepend-year-to-album" );

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
	customProfileIdGenerator_ = Grim::Tools::IdGenerator();
	customProfileIds_.clear();
	customProfiles_.clear();

	defaultQuality_ = kDefaultQualityValue;
	currentCustomProfileIndex_ = -1;

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

	// load file system profile
	settings.beginGroup( kFileSystemProfileKey );
	const Profile fileSystemProfile = _loadProfileFromSettings( settings );
	setFileSystemProfile( fileSystemProfile );
	settings.endGroup();

	// load custom profiles
	const int customProfileCount = settings.beginReadArray( kCustomProfilesKey );
	for ( int customProfileIndex = 0; customProfileIndex < customProfileCount; ++customProfileIndex )
	{
		settings.setArrayIndex( customProfileIndex );
		const Profile customProfile = _loadProfileFromSettings( settings );
		const int customProfileId = addCustomProfile();
		setCustomProfileForId( customProfileId, customProfile );
	}
	settings.endArray();

	// load current custom profile index
	currentCustomProfileIndex_ = settings.value( kCurrentCustomProfileIndexKey, -1 ).toInt();
	if ( currentCustomProfileIndex_ == -1 || (currentCustomProfileIndex_ >= 0 && currentCustomProfileIndex_ < customProfileCount) )
	{
		// index is valid
	}
	else
	{
		currentCustomProfileIndex_ = -1;
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

	// save file system profile
	settings.beginGroup( kFileSystemProfileKey );
	_saveProfileToSettings( settings, fileSystemProfile() );
	settings.endGroup();

	// save custom profiles
	settings.beginWriteArray( kCustomProfilesKey, customProfileIds().count() );
	for ( int customProfileIndex = 0; customProfileIndex < customProfileIds().count(); ++customProfileIndex )
	{
		settings.setArrayIndex( customProfileIndex );
		const int customProfileId = customProfileIds_.at( customProfileIndex );
		_saveProfileToSettings( settings, customProfileForId( customProfileId ) );
	}
	settings.endArray();

	// save current custom profile index
	settings.setValue( kCurrentCustomProfileIndexKey, currentCustomProfileIndex() );

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


int Config::addCustomProfile()
{
	const int customProfileId = customProfileIdGenerator_.take();

	if ( customProfiles_.size() <= customProfileId )
		customProfiles_.resize( customProfileId + 1 );

	Profile & profile = customProfiles_[ customProfileId ];
	profile.isNull_ = false;
	profile.path = _defaultFileSystemPath();
	profile.quality = defaultQuality_;
	profile.prependYearToAlbum = kDefaultPrependYearToAlbumValue;

	customProfileIds_ << customProfileId;

	return customProfileId;
}


void Config::removeCustomProfile( const int customProfileId )
{
	const int index = customProfileIndexForId( customProfileId );
	customProfileIds_.removeAt( index );
	customProfileIdGenerator_.free( customProfileId );

	if ( currentCustomProfileIndex() != -1 && currentCustomProfileIndex() == customProfileIds().count() )
		setCurrentCustomProfileIndex( customProfileIds().isEmpty() ? -1 : customProfileIds().count() - 1 );
}


void Config::setFileSystemProfile( const Profile & profile )
{
	_assertProfile( profile );
	fileSystemProfile_ = profile;
}


void Config::setCustomProfileForId( const int customProfileId, const Profile & profile )
{
	_assertProfile( profile );
	Q_ASSERT( !customProfileIdGenerator_.isFree( customProfileId ) );
	customProfiles_[ customProfileId ] = profile;
}


int Config::customProfileIndexForId( const int customProfileId ) const
{
	const int index = customProfileIds().indexOf( customProfileId );
	Q_ASSERT( index != -1 );
	return index;
}


void Config::setCurrentCustomProfileIndex( const int index )
{
	Q_ASSERT( index == -1 || (index >= 0 && index < customProfileIds().count() ) );
	currentCustomProfileIndex_ = index;
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


void Config::_assertProfile( const Profile & profile )
{
	Q_ASSERT( !profile.isNull() );
	Q_ASSERT( profile.quality >= kMinimumQualityValue && profile.quality <= kMaximumQualityValue );
}


Config::Profile Config::_loadProfileFromSettings( QSettings & settings )
{
	Profile profile;
	profile.isNull_ = false;
	profile.name = settings.value( kProfileNameKey ).toString();
	profile.path = settings.value( kProfilePathKey, _defaultFileSystemPath() ).toString();
	profile.quality = qBound<qreal>( 0.0, settings.value( kProfileQualityKey, kDefaultQualityValue ).toReal(), 1.0 );
	profile.prependYearToAlbum = settings.value( kProfilePrependYearToAlbumKey, kDefaultPrependYearToAlbumValue ).toBool();
	return profile;
}


void Config::_saveProfileToSettings( QSettings & settings, const Profile & profile )
{
	_assertProfile( profile );
	settings.setValue( kProfileNameKey, profile.name );
	settings.setValue( kProfilePathKey, profile.path );
	settings.setValue( kProfileQualityKey, profile.quality );
	settings.setValue( kProfilePrependYearToAlbumKey, profile.prependYearToAlbum );
}




} // namespace Fogg
