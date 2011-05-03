
#pragma once

#include <grim/tools/Global.h>

#include <QObject>
#include <QLocale>
#include <QHash>
#include <QStringList>
#include <QSharedDataPointer>




class QTranslator;
class QDir;




namespace Grim {
namespace Tools {




class LocalizationInfoPrivate;
class LocalizationManagerPrivate;




class GRIM_TOOLS_EXPORT LocalizationInfo
{
public:
	LocalizationInfo();
	LocalizationInfo( const LocalizationInfo & other );
	LocalizationInfo & operator=( const LocalizationInfo & other );
	~LocalizationInfo();

	bool operator==( const LocalizationInfo & other ) const;

	bool isNull() const;

	QString name() const;
	QString nativeName() const;
	QLocale locale() const;

private:
	QSharedDataPointer<LocalizationInfoPrivate> d_;

	friend class LocalizationManager;
};




class GRIM_TOOLS_EXPORT LocalizationManager : public QObject
{
	Q_OBJECT

public:
	LocalizationManager( QObject * parent = 0 );
	~LocalizationManager();

	void loadTranslations( const QString & translationsDirPath );

	QStringList names() const;

	LocalizationInfo localizationForName( const QString & name ) const;
	LocalizationInfo localizationForLocale( const QLocale & locale ) const;

	LocalizationInfo fallbackLocalization() const;
	void setFallbackLocalization( const LocalizationInfo & localization );

	LocalizationInfo currentLocalization() const;
	void setCurrentLocalization( const LocalizationInfo & localization );

	void setCurrentLocalizationFromSystemLocale();

protected:
	bool eventFilter( QObject * o, QEvent * e );

private:
	LocalizationInfo _createLocalization( const QDir & dir ) const;

private:
	QScopedPointer<LocalizationManagerPrivate> d_;
};




} // namespace Tools
} // namespace Grim
