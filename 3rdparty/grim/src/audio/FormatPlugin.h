
#pragma once

#include <QObject>
#include <QtPlugin>
#include <QMultiMap>
#include <QStringList>

#include <grim/audio/Global.h>




class QIODevice;




namespace Grim {
namespace Audio {




class FormatFilePrivate;




class GRIM_AUDIO_EXPORT FormatFile
{
public:
	enum OpenFlag
	{
		OpenFlag_ReadTags = 0x01
	};
	Q_DECLARE_FLAGS( OpenFlags, OpenFlag )

	FormatFile( const QString & fileName, const QString & format, OpenFlags openFlags );
	virtual ~FormatFile();

	virtual QIODevice * device() = 0;

	OpenFlags openFlags() const;
	QString fileName() const;
	QString format() const;
	QString resolvedFormat() const;

	int channels() const;
	int frequency() const;
	int bitsPerSample() const;
	qint64 totalSamples() const;
	QMultiMap<QString,QString> tags() const;

	qint64 truncatedSize( qint64 size ) const;

	qint64 bytesToSamples( qint64 bytes ) const;
	qint64 samplesToBytes( qint64 samples ) const;

protected:
	void setResolvedFormat( const QString & format );
	void setChannels( int channels );
	void setFrequency( int frequency );
	void setBitsPerSample( int bitsPerSample );
	void setTotalSamples( qint64 totalSamples );
	void setTags( const QMultiMap<QString,QString> & tags );

private:
	QScopedPointer<FormatFilePrivate> d_;
};





#define GRIM_AUDIO_FORMAT_PLUGIN_INTERFACE_IID "ua.org.dendy/Grim/Audio/FormatPlugin/1.0"

class GRIM_AUDIO_EXPORT FormatPlugin
{
public:
	virtual ~FormatPlugin() {}

	virtual QStringList formats() const = 0;
	virtual QStringList extensions( const QString & format ) const = 0;

	virtual FormatFile * createFile( const QString & fileName, const QString & format = QString(),
		FormatFile::OpenFlags openFlags = FormatFile::OpenFlags() ) = 0;
};




} // namespace Audio
} // namespace Grim




Q_DECLARE_INTERFACE( Grim::Audio::FormatPlugin, GRIM_AUDIO_FORMAT_PLUGIN_INTERFACE_IID )
