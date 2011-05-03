
#include "FormatPluginFlac.h"

#include "VorbisComment.h"




namespace Grim {
namespace Audio {




static const QString kFlacFormatName = QLatin1String( "FLAC" );




inline static FlacFormatDevice * _takeFlacDevice( void * const client_data )
{
	return static_cast<FlacFormatDevice*>( client_data );
}


FLAC__StreamDecoderReadStatus FlacFormatDevice::flac_read( const FLAC__StreamDecoder * const decoder,
	FLAC__byte buffer[], size_t * const bytes, void * const client_data )
{
	QIODevice * const file = &_takeFlacDevice( client_data )->file_;

	const qint64 bytesRead = file->read( reinterpret_cast<char*>( buffer ), *bytes );
	if ( bytesRead == -1 )
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	*bytes = bytesRead;

	if ( bytesRead == 0 )
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}


FLAC__StreamDecoderSeekStatus FlacFormatDevice::flac_seek( const FLAC__StreamDecoder * const decoder,
	FLAC__uint64 absolute_byte_offset, void * const client_data )
{
	QIODevice * const file = &_takeFlacDevice( client_data )->file_;

	if ( file->isSequential() )
		return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;

	const bool done = file->seek( (qint64)absolute_byte_offset );

	if ( !done )
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}


FLAC__StreamDecoderTellStatus FlacFormatDevice::flac_tell( const FLAC__StreamDecoder * const decoder,
	FLAC__uint64 * const absolute_byte_offset, void * const client_data )
{
	QIODevice * const file = &_takeFlacDevice( client_data )->file_;

	if ( file->isSequential() )
		return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;

	const qint64 pos = file->pos();

	if ( pos < 0 )
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

	*absolute_byte_offset = (FLAC__uint64)pos;

	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}


FLAC__StreamDecoderLengthStatus FlacFormatDevice::flac_length( const FLAC__StreamDecoder * const decoder,
	FLAC__uint64 * const stream_length, void * const client_data )
{
	QIODevice * const file = &_takeFlacDevice( client_data )->file_;

	if ( file->isSequential() )
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

	const qint64 size = file->size();

	if ( size < 0 )
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)size;

	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}


FLAC__bool FlacFormatDevice::flac_eof( const FLAC__StreamDecoder * const decoder,
	void * const client_data )
{
	QIODevice * const file = &_takeFlacDevice( client_data )->file_;

	return (FLAC__bool)file->atEnd();
}


template<int channels, typename B>
static void _processFrameWorker( const FLAC__Frame * const frame, const FLAC__int32 * const buffer[],
	void * const data, const qint64 sampleCount )
{
	B * const b = static_cast<B*>( data );
	for ( int i = 0; i < sampleCount; ++i )
	{
		b[ i*channels + 0 ] = buffer[ 0 ][ i ];
		if ( channels > 1 )
			b[ i*channels + 1 ] = buffer[ 1 ][ i ];
	}
}


void FlacFormatDevice::_processFrame( const FLAC__Frame * const frame, const FLAC__int32 * const buffer[],
	void * const data, const qint64 sampleCount )
{
	if ( formatFile_->channels() == 1 )
	{
		if ( formatFile_->bitsPerSample() == 8 )
			_processFrameWorker<1,qint8>( frame, buffer, data, sampleCount );
		else // if ( formatFile_->bitsPerSample() == 16 )
			_processFrameWorker<1,qint16>( frame, buffer, data, sampleCount );
	}
	else
	{
		if ( formatFile_->bitsPerSample() == 8 )
			_processFrameWorker<2,qint8>( frame, buffer, data, sampleCount );
		else // if ( formatFile_->bitsPerSample() == 16 )
			_processFrameWorker<2,qint16>( frame, buffer, data, sampleCount );
	}
}


FLAC__StreamDecoderWriteStatus FlacFormatDevice::flac_write( const FLAC__StreamDecoder * const decoder,
	const FLAC__Frame * const frame, const FLAC__int32 * const buffer[], void * const client_data )
{
	FlacFormatDevice * const flacDevice = _takeFlacDevice( client_data );

	const qint64 currentSample = flacDevice->isSeeking_ ?
		flacDevice->seekSample_ :
		flacDevice->formatFile_->bytesToSamples( flacDevice->pos_ + flacDevice->readTotalBytes_ );

	if ( frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER )
	{
		if ( (qint64)frame->header.number.sample_number != currentSample )
		{
			// desynchronization after seeking happened
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	if ( flacDevice->formatFile_->channels() != (int)frame->header.channels )
	{
		// should not happen
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	qint64 remainSamples = frame->header.blocksize;
	qint64 samplesToReadToBuffer = 0;

	if ( !flacDevice->isSeeking_ )
	{
		// first write decoded samples into read buffer, if we in read mode

		const qint64 readMaxSamples = flacDevice->formatFile_->bytesToSamples( flacDevice->readMaxSize_ );

		samplesToReadToBuffer = qMin<qint64>( readMaxSamples, remainSamples );
		const qint64 bytesToReadToBuffer = flacDevice->formatFile_->samplesToBytes( samplesToReadToBuffer );

		flacDevice->_processFrame( frame, buffer, flacDevice->readData_, samplesToReadToBuffer );
		flacDevice->readBytes_ = bytesToReadToBuffer;
	}

	// then write remaining samples into cache
	remainSamples -= samplesToReadToBuffer;

	// at this point cache is always cleared either from read or seek
	Q_ASSERT( flacDevice->readCache_.isEmpty() );

	if ( remainSamples > 0 )
	{
		flacDevice->readCache_.resize( flacDevice->formatFile_->samplesToBytes( remainSamples ) );

		// create temporary buffer with shifted sample offsets
		const FLAC__int32 * shiftedBuffer[ 2 ]; // 2 == maximum number of channels, this value is checked in open()
		for ( int i = 0; i < flacDevice->formatFile_->channels(); ++i )
			shiftedBuffer[ i ] = buffer[ i ] + samplesToReadToBuffer;

		flacDevice->_processFrame( frame, shiftedBuffer, flacDevice->readCache_.data(), remainSamples );
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


void FlacFormatDevice::flac_metadata( const FLAC__StreamDecoder * const decoder,
	const FLAC__StreamMetadata * const metadata, void * const client_data )
{
	FlacFormatDevice * const flacDevice = _takeFlacDevice( client_data );

	switch ( metadata->type )
	{
	case FLAC__METADATA_TYPE_STREAMINFO:
	{
		const FLAC__StreamMetadata_StreamInfo & streamInfo = metadata->data.stream_info;

		// can process only mono or stereo
		if ( streamInfo.channels != 1 && streamInfo.channels != 2 )
			return;

		// can process only 8 or 16 bits per sample
		if ( streamInfo.bits_per_sample != 8 && streamInfo.bits_per_sample != 16 )
			return;

		flacDevice->formatFile_->setResolvedFormat( kFlacFormatName );
		flacDevice->formatFile_->setChannels( streamInfo.channels );
		flacDevice->formatFile_->setFrequency( streamInfo.sample_rate );
		flacDevice->formatFile_->setBitsPerSample( streamInfo.bits_per_sample );
		flacDevice->formatFile_->setTotalSamples( streamInfo.total_samples );

		flacDevice->isStreamMetadataProcessed_ = true;
	}
		break;
	}
}


void FlacFormatDevice::flac_error( const FLAC__StreamDecoder * const decoder,
	const FLAC__StreamDecoderErrorStatus status, void * const client_data )
{
	Q_UNUSED( decoder );
	Q_UNUSED( client_data );

	flacFormatDebug() << "Error status =" << status;
}




inline QIODevice * _takeFlacIODevice( const FLAC__IOHandle handle )
{
	return reinterpret_cast<QIODevice*>( handle );
}


static size_t flac_io_read( void * const ptr, const size_t size, const size_t count, const FLAC__IOHandle handle )
{
	// expect always size == 1
	Q_ASSERT( size == 1 );

	QIODevice * const device = _takeFlacIODevice( handle );

	const qint64 bytesRead = device->read( static_cast<char*>( ptr ), count );
	if ( bytesRead == -1 )
		return 0;

	return bytesRead;
}


static int flac_io_seek( const FLAC__IOHandle handle, const FLAC__int64 offset, const int whence )
{
	QIODevice * const device = _takeFlacIODevice( handle );

	qint64 absPos = -1;
	switch ( whence )
	{
	case SEEK_SET:
		absPos = offset;
		break;
	case SEEK_CUR:
		absPos = device->pos() + offset;
		break;
	case SEEK_END:
		absPos = device->size() + offset;
		break;
	default:
		return -1;
	}

	const bool done = device->seek( absPos );

	return done ? 0 : -1;
}


static FLAC__int64 flac_io_tell( const FLAC__IOHandle handle )
{
	QIODevice * const device = _takeFlacIODevice( handle );

	return device->pos();
}




FlacFormatDevice::FlacFormatDevice( FlacFormatFile * const formatFile ) :
	formatFile_( formatFile )
{
	flacDecoder_ = 0;
	pos_ = -1;
	isSeeking_ = false;
}


FlacFormatDevice::~FlacFormatDevice()
{
	close();
}


bool FlacFormatDevice::_readTags()
{
	FLAC__Metadata_Chain * const chain = FLAC__metadata_chain_new();

	FLAC__IOCallbacks callbacks;
	callbacks.read = flac_io_read;
	callbacks.seek = flac_io_seek;
	callbacks.tell = flac_io_tell;
	callbacks.write = 0;
	callbacks.eof = 0;
	callbacks.close = 0;

	if ( !FLAC__metadata_chain_read_with_callbacks( chain, &file_, callbacks ) )
	{
		FLAC__metadata_chain_delete( chain );
		return false;
	}

	FLAC__Metadata_Iterator * const iterator = FLAC__metadata_iterator_new();
	FLAC__metadata_iterator_init( iterator, chain );

	do
	{
		const FLAC__MetadataType metadataType = FLAC__metadata_iterator_get_block_type( iterator );
		if ( metadataType == FLAC__METADATA_TYPE_VORBIS_COMMENT )
		{
			QMultiMap<QString,QString> tags;

			const FLAC__StreamMetadata * metadata = FLAC__metadata_iterator_get_block( iterator );
			const FLAC__StreamMetadata_VorbisComment & vorbisComment = metadata->data.vorbis_comment;
			for ( int i = 0; i < vorbisComment.num_comments; ++i )
			{
				const FLAC__StreamMetadata_VorbisComment_Entry & comment = vorbisComment.comments[ i ];
				const VorbisComment parsedComment = VorbisComment( reinterpret_cast<const char*>( comment.entry ) );
				if ( !parsedComment.isValid() )
				{
					flacFormatDebug() << "Error reading Vorbis comment:" << reinterpret_cast<const char*>( comment.entry );
					continue;
				}

				flacFormatDebug() << "Found tag: key =" << parsedComment.key() << ", value =" << parsedComment.value();

				tags.insert( parsedComment.key(), parsedComment.value() );
			}

			formatFile_->setTags( tags );

			break;
		}
	}
	while ( FLAC__metadata_iterator_next( iterator ) );

	FLAC__metadata_iterator_delete( iterator );
	FLAC__metadata_chain_delete( chain );

	return true;
}


bool FlacFormatDevice::_open( const QString & format, const bool firstTime )
{
	// only FLAC format with native container is supported by this plugin
	Q_UNUSED( format );

	bool previousSequential;
	qint64 previousChannels;
	qint64 previousBitsPerSample;
	qint64 previousFrequency;
	qint64 previousTotalSamples;

	if ( !firstTime )
	{
		previousSequential = isSequential_;
		previousChannels = formatFile_->channels();
		previousBitsPerSample = formatFile_->bitsPerSample();
		previousFrequency = formatFile_->frequency();
		previousTotalSamples = formatFile_->totalSamples();
	}

	isSequential_ = file_.isSequential();

	if ( !firstTime && isSequential_ != previousSequential )
		return false;

	flacDecoder_ = FLAC__stream_decoder_new();
	if ( !flacDecoder_ )
	{
		file_.close();
		return false;
	}

	const FLAC__StreamDecoderInitStatus initStatus = isSequential_ ?
		FLAC__stream_decoder_init_stream( flacDecoder_,
			flac_read,
			0, // flac_seek
			0, // flac_tell
			0, // flac_length
			flac_eof,
			flac_write,
			flac_metadata,
			flac_error,
			this ) :
		FLAC__stream_decoder_init_stream( flacDecoder_,
			flac_read,
			flac_seek,
			flac_tell,
			flac_length,
			flac_eof,
			flac_write,
			flac_metadata,
			flac_error,
			this );

	if ( initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK )
	{
		FLAC__stream_decoder_delete( flacDecoder_ );
		flacDecoder_ = 0;
		file_.close();
		return false;
	}

	isStreamMetadataProcessed_ = false;

	const bool isProcessed = FLAC__stream_decoder_process_until_end_of_metadata( flacDecoder_ );
	if ( !isProcessed )
	{
		FLAC__stream_decoder_delete( flacDecoder_ );
		flacDecoder_ = 0;
		file_.close();
		return false;
	}

	if ( !isStreamMetadataProcessed_ )
	{
		FLAC__stream_decoder_delete( flacDecoder_ );
		flacDecoder_ = 0;
		file_.close();
		return false;
	}

	if ( !firstTime )
	{
		if ( formatFile_->channels() != previousChannels ||
			formatFile_->bitsPerSample() != previousBitsPerSample ||
			formatFile_->frequency() != previousFrequency ||
			formatFile_->totalSamples() != previousTotalSamples )
		{
			FLAC__stream_decoder_delete( flacDecoder_ );
			flacDecoder_ = 0;
			file_.close();
			return false;
		}
	}

	outputSize_ = formatFile_->samplesToBytes( formatFile_->totalSamples() );
	pos_ = 0;

	return true;
}


bool FlacFormatDevice::open( const QIODevice::OpenMode openMode )
{
	Q_ASSERT( !isOpen() );

	if ( openMode != QIODevice::ReadOnly )
		return false;

	file_.setFileName( formatFile_->fileName() );

	if ( !file_.open( QIODevice::ReadOnly ) )
		return false;

	if ( formatFile_->openFlags().testFlag( FormatFile::OpenFlag_ReadTags ) )
	{
		// read tags first
		if ( !_readTags() )
		{
			file_.close();
			return false;
		}

		// seek file back to beginning
		if ( file_.isSequential() )
		{
			file_.close();
			if ( !file_.open( QIODevice::ReadOnly ) )
				return false;
		}
		else
		{
			if ( !file_.seek( 0 ) )
				return false;
		}
	}

	if ( !_open( formatFile_->format(), true ) )
		return false;

	setOpenMode( openMode );

	return true;
}


void FlacFormatDevice::_close()
{
	if ( pos_ == -1 )
		return;

	FLAC__stream_decoder_delete( flacDecoder_ );
	flacDecoder_ = 0;

	readCache_ = QByteArray();

	pos_ = -1;
}


void FlacFormatDevice::close()
{
	if ( !isOpen() )
		return;

	QIODevice::close();

	_close();

	file_.close();
}


bool FlacFormatDevice::isSequential() const
{
	Q_ASSERT( isOpen() );

	return isSequential_;
}


qint64 FlacFormatDevice::pos() const
{
	Q_ASSERT( isOpen() );

	return pos_;
}


bool FlacFormatDevice::atEnd() const
{
	Q_ASSERT( isOpen() );

	return pos_ == outputSize_;
}


qint64 FlacFormatDevice::size() const
{
	if ( !isOpen() )
		return 0;

	return outputSize_;
}


qint64 FlacFormatDevice::bytesAvailable() const
{
	if ( isSequential_ )
		return 0 + QIODevice::bytesAvailable();
	else
		return outputSize_ - pos_;
}


qint64 FlacFormatDevice::readData( char * data, qint64 maxSize )
{
	Q_ASSERT( isOpen() );

	isSeeking_ = false;

	readTotalBytes_ = 0;

	readData_ = data;

#ifdef GRIM_AUDIO_FORMAT_FLAC_STRICT_OUTPUT_SIZE
	readMaxSize_ = qMin( formatFile_->truncatedSize( maxSize ), outputSize_ - pos_ );
#else
	readMaxSize_ = formatFile_->truncatedSize( maxSize );
#endif

	if ( readMaxSize_ == 0 )
		return readTotalBytes_;

	// first fill data with samples from cache
	if ( !readCache_.isEmpty() )
	{
		const qint64 bytesToCopy = qMin<qint64>( readCache_.size(), readMaxSize_ );

		memcpy( readData_, readCache_.constData(), bytesToCopy );
		readCache_ = readCache_.mid( bytesToCopy );

		readData_ += bytesToCopy;
		readMaxSize_ -= bytesToCopy;
		readTotalBytes_ += bytesToCopy;
	}

	if ( readMaxSize_ == 0 )
	{
#ifdef GRIM_AUDIO_FORMAT_FLAC_STRICT_OUTPUT_SIZE
		if ( pos_ + readTotalBytes_ == outputSize_ )
		{
			Q_ASSERT( readCache_.isEmpty() );
		}
#endif
		return readTotalBytes_;
	}

	// load rest bytes from decoder
	while ( readMaxSize_ > 0 )
	{
		readBytes_ = -1;

		if ( !FLAC__stream_decoder_process_single( flacDecoder_ ) )
		{
			readCache_ = QByteArray();
			return -1;
		}

		if ( readBytes_ == -1 )
		{
#ifdef GRIM_AUDIO_FORMAT_FLAC_STRICT_OUTPUT_SIZE
			Q_ASSERT( pos_ + readTotalBytes_ == outputSize_ );
#endif
			break;
		}

		readData_ += readBytes_;
		readMaxSize_ -= readBytes_;
		readTotalBytes_ += readBytes_;
	}

	pos_ += readTotalBytes_;

#ifndef GRIM_AUDIO_FORMAT_FLAC_STRICT_OUTPUT_SIZE
	outputSize_ = qMax( outputSize_, pos_ );
#endif

	const qint64 tmp = readTotalBytes_;
	readTotalBytes_ = 0;
	return tmp;
}


qint64 FlacFormatDevice::writeData( const char * const data, const qint64 maxSize )
{
	Q_UNUSED( data );
	Q_UNUSED( maxSize );

	return -1;
}


bool FlacFormatDevice::seek( const qint64 pos )
{
	Q_ASSERT( isOpen() );

	QIODevice::seek( pos );

	if ( pos < 0 )
		return false;

	if ( pos == pos_ )
		return true;

	if ( isSequential_ )
	{
		if ( pos != 0 || (outputSize_ != -1 && pos > outputSize_) )
			return false;

		_close();

		const QIODevice::OpenMode openMode = file_.openMode();
		if ( !file_.reset() )
		{
			// seeking to 0 byte is not implemented
			// close and reopen file to archive the same effect
			file_.close();
			if ( !file_.open( openMode ) )
				return false;
		}

		return _open( formatFile_->resolvedFormat(), false );
	}
	else
	{
		if ( pos > outputSize_ )
			return false;
	}

	// Same as in Vorbis plugin.
	// Currently there are no routines to reach here in sequential mode.
	// If somebody wants to seek thru the FLAC stream in sequential mode
	// this would be a complex task with internal buffers and hard decoding
	// sequentially thorough the whole stream.
	// The better idea is to operate on non compressed random-access files,
	// like ZIP-archive provides together with compressed ones in the same archive.
	Q_ASSERT( !isSequential_ );

	readTotalBytes_ = 0;
	readCache_ = QByteArray();

	// this also asserts correct pos value
	seekSample_ = formatFile_->bytesToSamples( pos );

	isSeeking_ = true;

	const bool done = FLAC__stream_decoder_seek_absolute( flacDecoder_, seekSample_ );

	if ( !done )
		return false;

	pos_ = pos;

	return true;
}




FlacFormatFile::FlacFormatFile( const QString & fileName, const QString & format,
	const OpenFlags openFlags ) :
	FormatFile( fileName, format, openFlags ),
	device_( this )
{
}


QIODevice * FlacFormatFile::device()
{
	return &device_;
}




QStringList FlacFormatPlugin::formats() const
{
	return QStringList()
			<< kFlacFormatName;
}


QStringList FlacFormatPlugin::extensions( const QString & format ) const
{
	Q_UNUSED( format );
	Q_ASSERT( format == kFlacFormatName );

	return QStringList()
			<< QLatin1String( "flac" );
}


FormatFile * FlacFormatPlugin::createFile( const QString & fileName, const QString & format,
	const FormatFile::OpenFlags openFlags )
{
	Q_ASSERT( format.isNull() || formats().contains( format ) );

	return new FlacFormatFile( fileName, format, openFlags );
}




} // namespace Audio
} // namespace Grim




Q_EXPORT_PLUGIN2( grim_audio_flac_format_plugin, Grim::Audio::FlacFormatPlugin )
