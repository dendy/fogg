
#pragma once

#include <grim/audio/FormatPlugin.h>

#include <QIODevice>
#include <QFile>
#include <QReadWriteLock>
#include <QDebug>

#include <mpg123.h>




class QTextCodec;




namespace Grim {
namespace Audio {




#ifdef GRIM_AUDIO_DEBUG
#	define mp3FormatDebug() qDebug() << Q_FUNC_INFO
#else
#	define mp3FormatDebug() QNoDebug()
#endif




class Mp3FormatFile;




class Mp3FormatMpg123Singlethon
{
public:
	Mp3FormatMpg123Singlethon();
	~Mp3FormatMpg123Singlethon();

	bool isValid() const;

private:
	static QReadWriteLock sLock;
	static QAtomicInt sRef;
	static bool sIsValid;
};




class Mp3FormatDevice : public QIODevice
{
public:
	static ssize_t mpg_read_handle( void * handle, void * ptr, size_t size );
	static off_t mpg_seek_handle( void * handle, off_t offset, int whence );

	Mp3FormatDevice( Mp3FormatFile * formatFile );
	~Mp3FormatDevice();

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

	QString _fromMpgString( const mpg123_string * string ) const;
	QString _fromRawString( const char * string, int size ) const;
	void _readId3Tags( QMultiMap<QString,QString> & tags );

private:
	Mp3FormatFile * formatFile_;

	QFile file_;

	bool isSequential_;

	qint64 pos_;
	qint64 outputSize_;
	bool atEnd_;

	Mp3FormatMpg123Singlethon mpgSinglethon_;
	mpg123_handle * mpgHandle_;
	qint64 bufferSize_;
	const QTextCodec * codecForRawStrings_;
};




class Mp3FormatFile : public FormatFile
{
public:
	Mp3FormatFile( const QString & fileName, const QString & format, OpenFlags openFlags );

	QIODevice * device();

private:
	Mp3FormatDevice device_;

	friend class Mp3FormatDevice;
};




class Mp3FormatPlugin : public QObject, public FormatPlugin
{
	Q_OBJECT
	Q_INTERFACES( Grim::Audio::FormatPlugin )
	Q_PLUGIN_METADATA(IID GRIM_AUDIO_FORMAT_PLUGIN_INTERFACE_IID)

public:
	QStringList formats() const;
	QStringList extensions( const QString & format ) const;

	FormatFile * createFile( const QString & fileName, const QString & format,
		FormatFile::OpenFlags openFlags );
};




} // namespace Audio
} // namespace Grim
