
#include "LocalizationManager.h"

#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <QDebug>




namespace Grim {
namespace Tools {




static const QString InfoQmFileName = QLatin1String( "info.qm" );




typedef quint64 LocalizationLocaleKey;




class LocalizationInfoPrivate : public QSharedData
{
public:
	QString name_;

	QString nativeName_;

	QLocale::Language language_;
	QLocale::Country country_;

	QStringList translatorFileNames_;
	QList<QTranslator*> translators_;
};




class LocalizationManagerPrivate
{
public:
	const char * localizationNameInEnglish_;
	const char * localizationNameInNative_;
	const char * localizationLanguage_;
	const char * localizationCountry_;

	QStringList names_;
	QHash<QString,LocalizationInfo> localizationForName_;
	QHash<LocalizationLocaleKey,LocalizationInfo> localizationForLocaleKey;
	LocalizationInfo fallbackLocalization_;

	LocalizationInfo currentLocalization_;

	bool isSelfChangeLanguage_;
};




inline static LocalizationLocaleKey _localeKeyForLocale( const QLocale & locale )
{
	return (quint64(locale.language()) << 32) | quint64(locale.country());
}




LocalizationInfo::LocalizationInfo()
{
}


LocalizationInfo::LocalizationInfo( const LocalizationInfo & other ) :
	d_( other.d_ )
{
}


LocalizationInfo & LocalizationInfo::operator=( const LocalizationInfo & other )
{
	d_ = other.d_;
	return *this;
}


LocalizationInfo::~LocalizationInfo()
{
}


bool LocalizationInfo::operator==( const LocalizationInfo & other ) const
{
	if ( d_ == other.d_ )
		return true;

	if ( !d_ || !other.d_ )
		return false;

	return d_->name_ == other.d_->name_;
}


bool LocalizationInfo::isNull() const
{
	return !d_;
}


QString LocalizationInfo::name() const
{
	return d_ ? d_->name_ : QString();
}


QString LocalizationInfo::nativeName() const
{
	return d_ ? d_->nativeName_ : QString();
}


QLocale LocalizationInfo::locale() const
{
	return d_ ? QLocale( d_->language_, d_->country_ ) : QLocale();
}




LocalizationManager::LocalizationManager( QObject * parent ) :
	QObject( parent ), d_( new LocalizationManagerPrivate )
{
	d_->isSelfChangeLanguage_ = false;

	d_->localizationNameInEnglish_ = QT_TR_NOOP( "Localization Name In English" );
	d_->localizationNameInNative_  = QT_TR_NOOP( "Localization Name In Native" );
	d_->localizationLanguage_      = QT_TR_NOOP( "Language" );
	d_->localizationCountry_       = QT_TR_NOOP( "Country" );

	QCoreApplication::instance()->installEventFilter( this );
}


LocalizationManager::~LocalizationManager()
{
	Q_ASSERT( d_->currentLocalization_.isNull() );
}


QStringList LocalizationManager::names() const
{
	return d_->names_;
}


LocalizationInfo LocalizationManager::localizationForName( const QString & name ) const
{
	return d_->localizationForName_.value( name );
}


LocalizationInfo LocalizationManager::localizationForLocale( const QLocale & locale ) const
{
	return d_->localizationForLocaleKey.value( _localeKeyForLocale( locale ) );
}


void LocalizationManager::loadTranslations( const QString & translationsDirPath )
{
	// reset
	setCurrentLocalization( LocalizationInfo() );

	d_->names_.clear();
	d_->localizationForName_.clear();
	d_->localizationForLocaleKey.clear();
	d_->fallbackLocalization_ = LocalizationInfo();

	// load translations
	const QDir translationsDir = QDir( translationsDirPath );
	for ( QListIterator<QFileInfo> translationIt( translationsDir.entryInfoList( QDir::Dirs | QDir::NoDotAndDotDot ) ); translationIt.hasNext(); )
	{
		const QDir dir = QDir( translationIt.next().absoluteFilePath() );

		const LocalizationInfo info = _createLocalization( dir );
		if ( info.isNull() )
			continue;

		if ( d_->localizationForName_.contains( info.name() ) )
		{
			grimToolsWarning() << "Duplicated translation name:" << info.name();
			continue;
		}

		if ( d_->localizationForLocaleKey.contains( _localeKeyForLocale( info.locale() ) ) )
		{
			grimToolsWarning() << "Duplicated translation locale:" << info.locale().name();
			continue;
		}

		d_->names_ << info.name();
		d_->localizationForName_[ info.name() ] = info;
		d_->localizationForLocaleKey[ _localeKeyForLocale( info.locale() ) ] = info;
	}
}


LocalizationInfo LocalizationManager::_createLocalization( const QDir & dir ) const
{
	QTranslator * infoTranslator = new QTranslator;
	if ( !infoTranslator->load( dir.filePath( InfoQmFileName ) ) )
	{
		delete infoTranslator;
		return LocalizationInfo();
	}

	const QString englishName = infoTranslator->translate( metaObject()->className(), d_->localizationNameInEnglish_ );
	const QString nativeName = infoTranslator->translate( metaObject()->className(), d_->localizationNameInNative_ );
	const QString languageString = infoTranslator->translate( metaObject()->className(), d_->localizationLanguage_ );
	const QString countryString = infoTranslator->translate( metaObject()->className(), d_->localizationCountry_ );

#ifdef GRIM_TOOLS_DEBUG
	grimToolsDebug() << englishName << nativeName << languageString << countryString;
#endif

	delete infoTranslator;

	if ( englishName.isEmpty() || nativeName.isEmpty() )
		return LocalizationInfo();

	if ( languageString.isEmpty() )
		return LocalizationInfo();

	const QString localeName = countryString.isEmpty() ?
		languageString :
		QString( "%1_%2" ).arg( languageString ).arg( countryString );

	const QLocale locale = QLocale( localeName );
	if ( locale.language() == QLocale::C )
	{
		// language or country violates ISO rules
		return LocalizationInfo();
	}

	LocalizationInfo info;
	info.d_ = new LocalizationInfoPrivate;

	info.d_->name_ = englishName;
	info.d_->nativeName_ = nativeName;
	info.d_->language_ = locale.language();
	info.d_->country_ = locale.country();

	for ( QListIterator<QFileInfo> modulesIt( dir.entryInfoList( QStringList() << "*.qm", QDir::Files ) ); modulesIt.hasNext(); )
	{
		const QFileInfo & fileInfo = modulesIt.next();

		// skip info.qm
		if ( fileInfo.fileName() == InfoQmFileName )
			continue;

		info.d_->translatorFileNames_ << fileInfo.absoluteFilePath();
	}

	return info;
}


LocalizationInfo LocalizationManager::fallbackLocalization() const
{
	return d_->fallbackLocalization_;
}


void LocalizationManager::setFallbackLocalization( const LocalizationInfo & localization )
{
	d_->fallbackLocalization_ = localization;
}


LocalizationInfo LocalizationManager::currentLocalization() const
{
	return d_->currentLocalization_;
}


void LocalizationManager::setCurrentLocalization( const LocalizationInfo & localization )
{
	if ( localization == d_->currentLocalization_ )
		return;

	d_->isSelfChangeLanguage_ = true;

	// cleanup current translators
	if ( !d_->currentLocalization_.isNull() )
	{
		for ( QListIterator<QTranslator*> it( d_->currentLocalization_.d_->translators_ ); it.hasNext(); )
			QCoreApplication::instance()->removeTranslator( it.next() );

		qDeleteAll( d_->currentLocalization_.d_->translators_ );
		d_->currentLocalization_.d_->translators_.clear();
	}

	d_->currentLocalization_ = localization;

	// load translators
	if ( !d_->currentLocalization_.isNull() )
	{
		// TODO: Load translation from QLibraryInfo::TranslationsPath
		for ( QStringListIterator it( d_->currentLocalization_.d_->translatorFileNames_ ); it.hasNext(); )
		{
			const QString & qmFilePath = it.next();

			QTranslator * translator = new QTranslator;
			if ( !translator->load( qmFilePath ) )
			{
				delete translator;
				continue;
			}

			d_->currentLocalization_.d_->translators_ << translator;

			QCoreApplication::installTranslator( translator );
		}
	}

	d_->isSelfChangeLanguage_ = false;

	QEvent event( QEvent::LanguageChange );
	QCoreApplication::sendEvent( QCoreApplication::instance(), &event );
}


void LocalizationManager::setCurrentLocalizationFromSystemLocale()
{
	const LocalizationInfo localization = localizationForLocale( QLocale::system() );
	if ( localization.isNull() )
	{
		grimToolsWarning() << "No translation found for system locale:" << QLocale::system().name();
		setCurrentLocalization( d_->fallbackLocalization_ );
	}
	else
	{
		grimToolsDebug() << localization.name();
		setCurrentLocalization( localization );
	}
}


bool LocalizationManager::eventFilter( QObject * o, QEvent * e )
{
	if ( d_->isSelfChangeLanguage_ && o == QCoreApplication::instance() && e->type() == QEvent::LanguageChange )
		return true;

	return false;
}




} // namespace Tools
} // namespace Grim
