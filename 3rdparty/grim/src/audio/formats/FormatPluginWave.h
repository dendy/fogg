
#pragma once

#include <grim/audio/FormatPlugin.h>

#include <QIODevice>
#include <QFile>




namespace Grim {
namespace Audio {




class WaveFormatFile;




class WaveFormatDevice : public QIODevice
{
public:
	typedef void (*Codec)( const void * data, int size, void * outData );

	WaveFormatDevice( WaveFormatFile * formatFile );
	~WaveFormatDevice();

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
	int _codecMultiplier() const;

	bool _guessWave();
	bool _guessAu();

	bool _openRaw();
	bool _openWave();
	bool _openAu();

	void _setOutputSize();

	qint64 _readData( char * data, qint64 maxSize );

	bool _open( const QString & format, bool firstTime );
	void _close();

private:
	WaveFormatFile * formatFile_;

	QFile file_;

	bool isSequential_;

	bool isWave_;
	bool isAu_;

	Codec codec_;
	qint64 dataPos_;
	qint64 inputSize_;
	qint64 outputSize_;

	qint64 pos_;
};




class WaveFormatFile : public FormatFile
{
public:
	WaveFormatFile( const QString & fileName, const QString & format, OpenFlags openFlags );

	QIODevice * device();

private:
	WaveFormatDevice device_;

	friend class WaveFormatDevice;
};




class WaveFormatPlugin : public QObject, public FormatPlugin
{
	Q_OBJECT
	Q_INTERFACES( Grim::Audio::FormatPlugin )

public:
	QStringList formats() const;
	QStringList extensions( const QString & format ) const;

	FormatFile * createFile( const QString & fileName, const QString & format, FormatFile::OpenFlags openFlags );
};




} // namespace Audio
} // namespace Grim
