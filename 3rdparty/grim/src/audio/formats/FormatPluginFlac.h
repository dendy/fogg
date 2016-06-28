
#pragma once

#include <grim/audio/FormatPlugin.h>

#include <QIODevice>
#include <QFile>
#include <QDebug>

#include <FLAC/stream_decoder.h>
#include <FLAC/metadata.h>




namespace Grim {
namespace Audio {




#ifdef GRIM_AUDIO_DEBUG
#	define flacFormatDebug() qDebug() << Q_FUNC_INFO
#else
#	define flacFormatDebug() QNoDebug()
#endif




class FlacFormatFile;




class FlacFormatDevice : public QIODevice
{
public:
	static FLAC__StreamDecoderReadStatus flac_read( const FLAC__StreamDecoder * decoder,
		FLAC__byte buffer[], size_t * bytes, void * client_data );
	static FLAC__StreamDecoderSeekStatus flac_seek( const FLAC__StreamDecoder * decoder,
		FLAC__uint64 absolute_byte_offset, void * client_data );
	static FLAC__StreamDecoderTellStatus flac_tell( const FLAC__StreamDecoder * decoder,
		FLAC__uint64 * absolute_byte_offset, void * client_data );
	static FLAC__StreamDecoderLengthStatus flac_length( const FLAC__StreamDecoder * decoder,
		FLAC__uint64 * stream_length, void * client_data );
	static FLAC__bool flac_eof( const FLAC__StreamDecoder * decoder,
		void * client_data );
	static FLAC__StreamDecoderWriteStatus flac_write( const FLAC__StreamDecoder * decoder,
		const FLAC__Frame * frame, const FLAC__int32 * const buffer[], void * client_data );
	static void flac_metadata( const FLAC__StreamDecoder * decoder,
		const FLAC__StreamMetadata * metadata, void * client_data );
	static void flac_error( const FLAC__StreamDecoder * decoder,
		FLAC__StreamDecoderErrorStatus status, void * client_data );

	FlacFormatDevice( FlacFormatFile * formatFile );
	~FlacFormatDevice();

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

	void _processFrame( const FLAC__int32 * const * flacData, char * rawData, qint64 sampleCount );

	bool _readTags();

private:
	FlacFormatFile * formatFile_;

	QFile file_;

	bool isSequential_;

	qint64 pos_;
	qint64 outputSize_;

	FLAC__StreamDecoder * flacDecoder_;

	// for metadata callback
	bool isStreamMetadataProcessed_;

	// for write callback
	bool isSeeking_;          // indicates whether we are inside seek or read

	char * readData_;         // input data pointer
	qint64 readMaxSize_;      // input data max size
	qint64 readBytes_;      // output bytes count
	QByteArray readCache_;    // cache between readData() calls to store unhandled samples
	qint64 readTotalBytes_; // total bytes readed so far at current readData() call

	qint64 seekSample_;       // sample that was passed to seek

	friend class DecoderDestroyer;
};




class FlacFormatFile : public FormatFile
{
public:
	FlacFormatFile( const QString & fileName, const QString & format, OpenFlags openFlags );

	QIODevice * device();

private:
	FlacFormatDevice device_;

	friend class FlacFormatDevice;
};




class FlacFormatPlugin : public QObject, public FormatPlugin
{
	Q_OBJECT
	Q_INTERFACES( Grim::Audio::FormatPlugin )
	Q_PLUGIN_METADATA(IID GRIM_AUDIO_FORMAT_PLUGIN_INTERFACE_IID)

public:
	QStringList formats() const;
	QStringList extensions( const QString & format ) const;

	FormatFile * createFile( const QString & fileName, const QString & format, FormatFile::OpenFlags openFlags );
};




} // namespace Audio
} // namespace Grim
