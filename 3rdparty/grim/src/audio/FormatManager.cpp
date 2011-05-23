
#include "FormatManager.h"

#include <QPluginLoader>

#include "FormatPlugin.h"




namespace Grim {
namespace Audio {




FormatManager::FormatManager( QObject * const parent ) :
	QObject( parent )
{
	// load static format plugins
	{
		for ( QListIterator<QObject*> pluginsIt( QPluginLoader::staticInstances() ); pluginsIt.hasNext(); )
		{
			FormatPlugin * plugin = qobject_cast<FormatPlugin*>( pluginsIt.next() );
			if ( !plugin )
				continue;

			_addAudioFormatPlugin( plugin );
		}
	}
}


QStringList FormatManager::availableFileFormats() const
{
	QReadLocker fileFormatsLocker( &fileFormatsMutex_ );
	return availableFileFormats_;
}


QStringList FormatManager::availableFileExtensionsForFormat( const QString & format ) const
{
	QReadLocker fileFormatsLocker( &fileFormatsMutex_ );

	Q_ASSERT( audioFormatPluginsForFormat_.contains( format ) );

	QStringList extensions;
	foreach ( const FormatPlugin * const plugin, audioFormatPluginsForFormat_.value( format ) )
		extensions << plugin->extensions( format );
	return extensions;
}


QStringList FormatManager::allAvailableFileExtensions() const
{
	QReadLocker fileFormatsLocker( &fileFormatsMutex_ );
	return allAvailableFileExtensions_;
}


FormatFile * FormatManager::_createFormatFileFromPlugins( const FormatPluginList & plugins,
		const FormatPluginList & exceptPlugins,
		const QString & fileName, const QString & format )
{
	QWriteLocker fileFormatsLocker( &fileFormatsMutex_ );

	for ( QListIterator<FormatPlugin*> it( plugins ); it.hasNext(); )
	{
		FormatPlugin * const plugin = it.next();
		if ( exceptPlugins.contains( plugin ) )
			continue;
		FormatFile * const file = plugin->createFile( fileName, format, FormatFile::OpenFlag_ReadTags );
		Q_ASSERT( file );
		if ( file->device()->open( QIODevice::ReadOnly ) )
			return file;
		delete file;
	}

	return 0;
}


FormatFile * FormatManager::createFormatFile( const QString & fileName, const QString & format )
{
	// lookup plugin by the given format name
	if ( !format.isNull() )
	{
		return _createFormatFileFromPlugins( audioFormatPluginsForFormat_.value( format ),
				FormatPluginList(), fileName, format );
	}

	// lookup plugin by the file name extension
	const QFileInfo fileInfo = QFileInfo( fileName );
	const QString suffix = fileInfo.suffix();

	FormatPluginList extensionPlugins;

	if ( !suffix.isEmpty() )
	{
		// file name has extension
		extensionPlugins = audioFormatPluginsForExtension_.value( suffix );
		FormatFile * const file = _createFormatFileFromPlugins( extensionPlugins,
				FormatPluginList(), fileName, format );
		if ( file )
			return file;
	}

	// file name has no extension, try all plugins on by one, except we checked earlier by extension
	return _createFormatFileFromPlugins( audioFormatPlugins_, extensionPlugins, fileName, format );
}


QStringList FormatManager::formatsForExtension( const QString & extension ) const
{
	return audioFormatsForExtension_.value( extension.toLower() );
}


void FormatManager::_addAudioFormatPlugin( FormatPlugin * const plugin )
{
	audioFormatPlugins_ << plugin;

	foreach ( const QString & format, plugin->formats() )
	{
		if ( !availableFileFormats_.contains( format ) )
			availableFileFormats_ << format;

		audioFormatPluginsForFormat_[ format ] << plugin;

		foreach ( const QString & extension, plugin->extensions( format ) )
		{
			Q_ASSERT( extension.toLower() == extension );

			if ( !allAvailableFileExtensions_.contains( extension ) )
				allAvailableFileExtensions_ << extension;

			audioFormatPluginsForExtension_[ extension ] << plugin;
			audioFormatsForExtension_[ extension ] << format;
		}
	}
}




} // namespace Audio
} // namespace Grim
