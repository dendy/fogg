
#pragma once

#include <grim/audio/FormatPlugin.h>

#include <QIODevice>
#include <QFile>
#include <QDebug>

#include <vorbis/vorbisfile.h>




namespace Grim {
namespace Audio {




#ifdef GRIM_AUDIO_DEBUG
#	define vorbisFormatDebug() qDebug() << Q_FUNC_INFO
#else
#	define vorbisFormatDebug() QNoDebug()
#endif




class VorbisFormatFile;




class VorbisFormatDevice : public QIODevice
{
public:
	static size_t ogg_read( void * ptr, size_t size, size_t nmemb, void * datasource );
	static int ogg_seek( void * datasource, ogg_int64_t offset, int whence );
	static int ogg_close( void * datasource );
	static long ogg_tell( void * datasource );

	VorbisFormatDevice( VorbisFormatFile * formatFile );
	~VorbisFormatDevice();

	bool open( QIODevice::OpenMode openMode );
	void close();

	bool isSequential() const;

	qint64 pos() const;
	bool atEnd() const;
	qint64 size() const;
	qint64 bytesAvailable() const;

	qint64 readData( char * data, qint64 maxSize );
	qint64 writeData( const char * data, qint64 maxSize );

	bool seek( qint64 pos );

private:
	bool _open( const QString & format, bool firstTime );
	void _close();

private:
	VorbisFormatFile * formatFile_;

	QFile file_;

	bool isSequential_;

	qint64 pos_;
	qint64 outputSize_;
	bool readError_;
	bool atEnd_;

	OggVorbis_File oggVorbisFile_;
};




class VorbisFormatFile : public FormatFile
{
public:
	VorbisFormatFile( const QString & fileName, const QString & format, OpenFlags openFlags );

	QIODevice * device();

private:
	VorbisFormatDevice device_;

	friend class VorbisFormatDevice;
};




class VorbisFormatPlugin : public QObject, public FormatPlugin
{
	Q_OBJECT
	Q_INTERFACES( Grim::Audio::FormatPlugin )

public:
	QStringList formats() const;
	QStringList extensions( const QString & format ) const;

	FormatFile * createFile( const QString & fileName, const QString & format,
		FormatFile::OpenFlags openFlags );
};




} // namespace Audio
} // namespace Grim
