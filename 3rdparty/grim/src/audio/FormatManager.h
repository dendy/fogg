
#pragma once

#include <QObject>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QReadWriteLock>




class QPluginLoader;




namespace Grim {
namespace Audio {




class FormatFile;
class FormatPlugin;




class FormatManager : public QObject
{
	Q_OBJECT

public:
	FormatManager( QObject * parent = 0 );

	QStringList availableFileFormats() const;
	QStringList availableFileExtensionsForFormat( const QString & format ) const;
	QStringList allAvailableFileExtensions() const;

	FormatFile * createFormatFile( const QString & fileName, const QString & format );

private:
	typedef QList<FormatPlugin*> FormatPluginList;

private:
	FormatFile * _createFormatFileFromPlugins( const FormatPluginList & plugins,
			const FormatPluginList & exceptPlugins,
			const QString & fileName, const QString & format );
	void _addAudioFormatPlugin( FormatPlugin * plugin );

private:
	mutable QReadWriteLock fileFormatsMutex_;
	QStringList availableFileFormats_;
	QStringList allAvailableFileExtensions_;
	QList<FormatPlugin*> audioFormatPlugins_;
	QHash<QString,FormatPluginList> audioFormatPluginsForFormat_;
	QHash<QString,FormatPluginList> audioFormatPluginsForExtension_;
};




} // namespace Audio
} // namespace Grim
