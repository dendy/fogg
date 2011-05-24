
#pragma once

#include <QObject>
#include <QVector>
#include <QRect>

#include <grim/tools/IdGenerator.h>

#include "Global.h"




class QSettings;




namespace Fogg {




class Config : public QObject
{
	Q_OBJECT

public:
	class Profile
	{
	public:
		Profile() :
			isNull_( true )
		{}

		bool isNull() const
		{ return isNull_; }

		QString name;
		qreal quality;
		QString path;
		bool prependYearToAlbum;

	private:
		bool isNull_;

		friend class Config;
	};


	class SourceDir
	{
	public:
		SourceDir()
		{}

		SourceDir( const QString & _path ) :
			path( _path )
		{}

		QString path;

		bool operator==( const SourceDir & other ) const
		{ return path == other.path; }
	};


	Config( QObject * parent = 0 );

	bool load();
	bool save();

	QString language() const;
	void setLanguage( const QString & language );

	int maximumConcurrentThreadCount() const;
	int concurrentThreadCount() const;
	void setConcurrentThreadCount( int count );

	qreal defaultQuality() const;
	void setDefaultQuality( qreal quality );

	int addCustomProfile();
	void removeCustomProfile( int customProfileId );

	Profile fileSystemProfile() const;
	void setFileSystemProfile( const Profile & profile );

	QList<int> customProfileIds() const;
	Profile customProfileForId( int customProfileId ) const;
	void setCustomProfileForId( int customProfileId, const Profile & profile );
	int customProfileIndexForId( int customProfileId ) const;

	int currentCustomProfileIndex() const;
	void setCurrentCustomProfileIndex( int index );

	QList<SourceDir> sourceDirs() const;
	void setSourceDirs( const QList<SourceDir> & sourceDirs );

	bool mainWindowStayOnTop() const;
	void setMainWindowStayOnTop( bool set );

	QRect mainWindowGeometry() const;
	void setMainWindowGeometry( const QRect & geometry );

	bool mainWindowMaximized() const;
	void setMainWindowMaximized( bool set );

	QString lastUsedFilesDirPath() const;
	void setLastUsedFilesDirPath( const QString & path );

	QByteArray jobViewHeaderState() const;
	void setJobViewHeaderState( const QByteArray & state );

private:
	void _clear();

	QString _defaultFileSystemPath() const;
	void _assertProfile( const Profile & profile );
	Profile _loadProfileFromSettings( QSettings & settings );
	void _saveProfileToSettings( QSettings & settings, const Profile & profile );

private:
	// general
	QString language_;
	int maximumConcurrentThreadCount_;
	int concurrentThreadCount_;
	qreal defaultQuality_;

	// profiles
	Profile fileSystemProfile_;
	Grim::Tools::IdGenerator customProfileIdGenerator_;
	QVector<Profile> customProfiles_;
	QList<int> customProfileIds_;
	int currentCustomProfileIndex_;

	// sources
	QList<SourceDir> sourceDirs_;

	// user interface
	bool mainWindowStayOnTop_;
	QRect mainWindowGeometry_;
	bool mainWindowMaximized_;

	QString lastUsedFilesDirPath_;

	QByteArray jobViewHeaderState_;
};




inline QString Config::language() const
{ return language_; }

inline int Config::maximumConcurrentThreadCount() const
{ return maximumConcurrentThreadCount_; }

inline int Config::concurrentThreadCount() const
{ return concurrentThreadCount_; }

inline qreal Config::defaultQuality() const
{ return defaultQuality_; }

inline QList<int> Config::customProfileIds() const
{ return customProfileIds_; }

inline Config::Profile Config::fileSystemProfile() const
{ return fileSystemProfile_; }

inline Config::Profile Config::customProfileForId( const int customProfileId ) const
{ return customProfiles_.at( customProfileId ); }

inline int Config::currentCustomProfileIndex() const
{ return currentCustomProfileIndex_; }

inline QList<Config::SourceDir> Config::sourceDirs() const
{  return sourceDirs_; }

inline bool Config::mainWindowStayOnTop() const
{ return mainWindowStayOnTop_; }

inline QRect Config::mainWindowGeometry() const
{ return mainWindowGeometry_; }

inline bool Config::mainWindowMaximized() const
{ return mainWindowMaximized_; }

inline QString Config::lastUsedFilesDirPath() const
{ return lastUsedFilesDirPath_; }

inline QByteArray Config::jobViewHeaderState() const
{ return jobViewHeaderState_; }




} // namespace Fogg
