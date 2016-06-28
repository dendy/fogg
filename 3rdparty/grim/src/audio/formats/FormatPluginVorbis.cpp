
#include "FormatPluginVorbis.h"

#include "VorbisComment.h"




namespace Grim {
namespace Audio {




static const QString kVorbisFormatName = QLatin1String( "Ogg/Vorbis" );

static const int kVorbisEndianess =
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	0;
#else
	1;
#endif

static const int kVorbisBytesPerSample = 2;




inline static VorbisFormatDevice * _takeVorbisDevice( void * const data )
{
	return static_cast<VorbisFormatDevice*>( data );
}


size_t VorbisFormatDevice::ogg_read( void * const ptr, const size_t size, const size_t count, void * const datasource )
{
	// expect always size == 1
	Q_ASSERT( size == 1 );

	VorbisFormatDevice * const device = _takeVorbisDevice( datasource );

	const qint64 bytesRead = device->file_.read( static_cast<char*>( ptr ), count );
	Q_ASSERT( bytesRead >= -1 );

	if ( bytesRead == -1 )
	{
		// vorbisfile has no ability to detect aborted stream.
		// We have to save error flag somewhere else and tell vorbisfile
		// that stream just finished.
		device->readError_ = true;
		return 0;
	}

	return (size_t)bytesRead;
}


int VorbisFormatDevice::ogg_seek( void * const datasource, const ogg_int64_t offset, const int whence )
{
	VorbisFormatDevice * const device = _takeVorbisDevice( datasource );

	qint64 absPos = -1;
	switch ( whence )
	{
	case SEEK_SET:
		absPos = offset;
		break;
	case SEEK_CUR:
		absPos = device->file_.pos() + offset;
		break;
	case SEEK_END:
		absPos = device->file_.size() + offset;
		break;
	default:
		return -1;
	}

	const bool done = device->file_.seek( absPos );

	return done ? 0 : -1;
}


int VorbisFormatDevice::ogg_close( void * const datasource )
{
	VorbisFormatDevice * const device = _takeVorbisDevice( datasource );

	device->file_.close();

	return 0;
}


long VorbisFormatDevice::ogg_tell( void * const datasource )
{
	VorbisFormatDevice * const device = _takeVorbisDevice( datasource );

	return device->file_.pos();
}


static ov_callbacks ogg_create_callbacks()
{
	ov_callbacks c;
	c.read_func = VorbisFormatDevice::ogg_read;
	c.seek_func = VorbisFormatDevice::ogg_seek;
	c.close_func = VorbisFormatDevice::ogg_close;
	c.tell_func = VorbisFormatDevice::ogg_tell;
	return c;
}




VorbisFormatDevice::VorbisFormatDevice( VorbisFormatFile * const formatFile ) :
	formatFile_( formatFile )
{
	pos_ = -1;
}


VorbisFormatDevice::~VorbisFormatDevice()
{
	close();
}


bool VorbisFormatDevice::_open( const QString & format, const bool firstTime )
{
	// only Ogg/Vorbis format is supported by this plugin
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

	ov_callbacks ovCallbacks = ogg_create_callbacks();

	if ( isSequential_ )
	{
		ovCallbacks.seek_func = 0;
		ovCallbacks.close_func = 0;       // we will close file manually
		ovCallbacks.tell_func = 0;
	}
	else
	{
		ovCallbacks.close_func = 0;       // we will close file manually
	}

	if ( ov_open_callbacks( this, &oggVorbisFile_, 0, 0, ovCallbacks ) != 0 )
	{
		file_.close();
		return false;
	}

	if ( firstTime && formatFile_->openFlags().testFlag( FormatFile::OpenFlag_ReadTags ) )
	{
		QMultiMap<QString,QString> tags;

		const vorbis_comment * const vorbisComment = ov_comment( &oggVorbisFile_, -1 );
		for ( int i = 0; i < vorbisComment->comments; ++i )
		{
			const VorbisComment parsedComment = VorbisComment( vorbisComment->user_comments[ i ] );
			if ( !parsedComment.isValid() )
			{
				vorbisFormatDebug() << "Error reading Vorbis comment:" << vorbisComment->user_comments[ i ];
				continue;
			}

			vorbisFormatDebug() << "Found tag: key =" << parsedComment.key() << ", value =" << parsedComment.value();

			tags.insert( parsedComment.key(), parsedComment.value() );
		}

		formatFile_->setTags( tags );
	}

	vorbis_info * vorbisInfo = ov_info( &oggVorbisFile_, -1 );

	const int channels = vorbisInfo->channels;
	const int frequency = vorbisInfo->rate;
	const int bitsPerSample = kVorbisBytesPerSample * 8;

	if ( channels < 0 || frequency < 0 )
	{
		file_.close();
		return false;
	}

	qint64 totalSamples;

	if ( isSequential_ )
	{
		totalSamples = -1;
	}
	else
	{
		totalSamples = ov_pcm_total( &oggVorbisFile_, -1 );
		if ( totalSamples < 0 )
		{
			ov_clear( &oggVorbisFile_ );
			file_.close();
			return false;
		}
	}

	formatFile_->setResolvedFormat( kVorbisFormatName );
	formatFile_->setChannels( channels );
	formatFile_->setFrequency( frequency );
	formatFile_->setBitsPerSample( bitsPerSample );
	formatFile_->setTotalSamples( totalSamples );

	if ( !firstTime )
	{
		if ( formatFile_->channels() != previousChannels ||
			formatFile_->bitsPerSample() != previousBitsPerSample ||
			formatFile_->frequency() != previousFrequency ||
			formatFile_->totalSamples() != previousTotalSamples )
		{
			ov_clear( &oggVorbisFile_ );
			file_.close();
			return false;
		}
	}

	outputSize_ = isSequential_ ? -1 : formatFile_->samplesToBytes( formatFile_->totalSamples() );
	pos_ = 0;
	atEnd_ = false;
	readError_ = false;

	return true;
}


bool VorbisFormatDevice::open( const QIODevice::OpenMode openMode )
{
	Q_ASSERT( !isOpen() );

	if ( openMode != QIODevice::ReadOnly )
		return false;

	file_.setFileName( formatFile_->fileName() );

	if ( !file_.open( QIODevice::ReadOnly ) )
		return false;

	if ( !_open( formatFile_->format(), true ) )
		return false;

	setOpenMode( openMode );

	return true;
}


void VorbisFormatDevice::_close()
{
	if ( pos_ == -1 )
		return;

	ov_clear( &oggVorbisFile_ );

	pos_ = -1;
}


void VorbisFormatDevice::close()
{
	if ( !isOpen() )
		return;

	QIODevice::close();

	_close();

	file_.close();
}


bool VorbisFormatDevice::isSequential() const
{
	Q_ASSERT( isOpen() );

	return isSequential_;
}


qint64 VorbisFormatDevice::pos() const
{
	Q_ASSERT( isOpen() );

	return pos_;
}


bool VorbisFormatDevice::atEnd() const
{
	Q_ASSERT( isOpen() );

	return isSequential_ ? atEnd_ : pos_ == outputSize_;
}


qint64 VorbisFormatDevice::size() const
{
	if ( !isOpen() )
		return 0;

	return isSequential_ ? 0 : outputSize_;
}


qint64 VorbisFormatDevice::bytesAvailable() const
{
	if ( isSequential_ )
		return 0 + QIODevice::bytesAvailable();
	else
		return outputSize_ - pos_;
}


qint64 VorbisFormatDevice::readData( char * const data, const qint64 maxSize )
{
	Q_ASSERT( isOpen() );

	qint64 totalBytes = 0;

	if ( isSequential_ )
	{
		if ( atEnd_ )
			return 0;

		qint64 remainBytes = maxSize;

		while ( remainBytes > 0 )
		{
			int stream;
			const qint64 bytesRead = ov_read( &oggVorbisFile_, data + totalBytes, remainBytes,
				kVorbisEndianess, kVorbisBytesPerSample, 1, &stream );

			if ( bytesRead < 0 )
			{
				vorbisFormatDebug() << "ov_read() error on sequential, code =" << bytesRead;
				return -1;
			}

			Q_ASSERT( bytesRead <= remainBytes );

			if ( readError_ )
				return -1;

			if ( bytesRead == 0 )
			{
				atEnd_ = true;
				break;
			}

			totalBytes += bytesRead;
			remainBytes -= bytesRead;
		}
	}
	else
	{
		qint64 remainBytes = qMin( formatFile_->truncatedSize( maxSize ), outputSize_ - pos_ );

		while ( remainBytes > 0 )
		{
			int stream;
			const qint64 bytesRead = ov_read( &oggVorbisFile_, data + totalBytes, remainBytes,
				kVorbisEndianess, kVorbisBytesPerSample, 1, &stream );

			if ( bytesRead < 0 )
			{
				vorbisFormatDebug() << "ov_read() error on random-access, code =" << bytesRead;
				return -1;
			}

			Q_ASSERT( bytesRead <= remainBytes );

			if ( bytesRead == 0 )
			{
				// end of file reached
				if ( remainBytes > 0 )
					return -1;
			}

			totalBytes += bytesRead;
			remainBytes -= bytesRead;
		}
	}

	pos_ += totalBytes;

	return totalBytes;
}


qint64 VorbisFormatDevice::writeData( const char * const data, const qint64 maxSize )
{
	Q_UNUSED( data );
	Q_UNUSED( maxSize );

	return -1;
}


bool VorbisFormatDevice::seek( const qint64 pos )
{
	Q_ASSERT( isOpen() );

	QIODevice::seek( pos );

	if ( pos < 0 )
		return false;

	if ( pos == pos_ )
		return true;

	if ( isSequential_ )
	{
		if ( pos != 0 )
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

	// Same as in FLAC plugin.
	// Currently there are no routines to reach here in sequential mode.
	// If somebody wants to seek thru the Ogg/Vorbis stream in sequential mode
	// this would be a complex task with internal buffers and hard decoding
	// sequentially thorough the whole stream.
	// The better idea is to operate on non compressed random-access files,
	// like ZIP-archive provides together with compressed ones in the same archive.
	Q_ASSERT( !isSequential_ );

	// this also asserts correct pos value
	const qint64 sample = formatFile_->bytesToSamples( pos );

	const int ret = ov_pcm_seek( &oggVorbisFile_, sample );
	if ( ret != 0 )
		return false;

	pos_ = pos;

	return true;
}


VorbisFormatFile::VorbisFormatFile( const QString & fileName, const QString & format,
	const FormatFile::OpenFlags openFlags ) :
	FormatFile( fileName, format, openFlags ),
	device_( this )
{
}


QIODevice * VorbisFormatFile::device()
{
	return &device_;
}




QStringList VorbisFormatPlugin::formats() const
{
	return QStringList()
			<< kVorbisFormatName;
}


QStringList VorbisFormatPlugin::extensions( const QString & format ) const
{
	Q_UNUSED( format );
	Q_ASSERT( format == kVorbisFormatName );

	return QStringList()
			<< QLatin1String( "ogg" )
			<< QLatin1String( "oga" );
}


FormatFile * VorbisFormatPlugin::createFile( const QString & fileName, const QString & format,
	const FormatFile::OpenFlags openFlags )
{
	Q_ASSERT( format.isNull() || formats().contains( format ) );

	return new VorbisFormatFile( fileName, format, openFlags );
}




} // namespace Audio
} // namespace Grim
