
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
		const QString & fileName, const QString & format )
{
	QWriteLocker fileFormatsLocker( &fileFormatsMutex_ );

	for ( QListIterator<FormatPlugin*> it( plugins ); it.hasNext(); )
	{
		FormatPlugin * plugin = it.next();
		FormatFile * file = plugin->createFile( fileName, format, FormatFile::OpenFlag_ReadTags );
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
		return _createFormatFileFromPlugins( audioFormatPluginsForFormat_.value( format ), fileName, format );

	// lookup plugin by the file name extension
	const int dotPos = fileName.lastIndexOf( QLatin1Char( '.' ) );
	const int slashPos = fileName.lastIndexOf( QLatin1Char( '/' ) );

	FormatPluginList restPlugins = audioFormatPlugins_;
	FormatPluginList extensionPlugins;

	if ( dotPos != -1 && dotPos != fileName.length()-1 && slashPos < dotPos )
	{
		// file name has extension
		const QString extension = fileName.mid( dotPos + 1 ).toLower();
		Q_ASSERT( !extension.isEmpty() );

		extensionPlugins = audioFormatPluginsForExtension_.value( extension );
		FormatFile * file = _createFormatFileFromPlugins( extensionPlugins, fileName, format );
		if ( file )
			return file;
	}

	// file name has no extension, try all plugins on by one, except we checked earlier by extension
	{
		for ( QListIterator<FormatPlugin*> it( extensionPlugins ); it.hasNext(); )
			restPlugins.removeOne( it.next() );
	}

	return _createFormatFileFromPlugins( restPlugins, fileName, format );
}


void FormatManager::_addAudioFormatPlugin( FormatPlugin * const plugin )
{
	foreach ( const QString & format, plugin->formats() )
	{
		if ( !availableFileFormats_.contains( format ) )
			availableFileFormats_ << format;

		audioFormatPluginsForFormat_[ format ] << plugin;

		foreach ( const QString & extension, plugin->extensions( format ) )
		{
			if ( !allAvailableFileExtensions_.contains( extension ) )
				allAvailableFileExtensions_ << extension;

			audioFormatPluginsForExtension_[ extension ] << plugin;
		}
	}
}




} // namespace Audio
} // namespace Grim
