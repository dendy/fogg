
#include "FormatPluginMp3.h"

#include <QTextCodec>




namespace Grim {
namespace Audio {




static const QByteArray kMp3FormatName = "Mp3";

static const char kCodecForRawStringsKey[] = "GRIM_AUDIO_MP3_FORMAT_PLUGIN_CODEC_FOR_RAW_STRINGS";

QReadWriteLock Mp3FormatMpg123Singlethon::sLock;
QAtomicInt Mp3FormatMpg123Singlethon::sRef;
bool Mp3FormatMpg123Singlethon::sIsValid = false;




Mp3FormatMpg123Singlethon::Mp3FormatMpg123Singlethon()
{
	QWriteLocker locker( &sLock );

	if ( sRef == 0 )
	{
		// try initialize first time
		sIsValid = mpg123_init() == MPG123_OK;
	}

	sRef.ref();
}


Mp3FormatMpg123Singlethon::~Mp3FormatMpg123Singlethon()
{
	QWriteLocker locker( &sLock );

	if ( !sRef.deref() )
	{
		if ( sIsValid )
		{
			mpg123_exit();
			sIsValid = false;
		}
	}
}


bool Mp3FormatMpg123Singlethon::isValid() const
{
	return sIsValid;
}




inline Mp3FormatDevice * _takeMp3DeviceForHandle( void * const handle )
{
	return static_cast<Mp3FormatDevice*>( handle );
}


ssize_t Mp3FormatDevice::mpg_read_handle( void * const handle, void * const ptr, const size_t size )
{
	Mp3FormatDevice * const device = _takeMp3DeviceForHandle( handle );

	const qint64 bytesRead = device->file_.read( static_cast<char*>( ptr ), size );
	if ( bytesRead == -1 )
		return -1;

	return bytesRead;
}


off_t Mp3FormatDevice::mpg_seek_handle( void * const handle, const off_t offset, const int whence )
{
	Mp3FormatDevice * const device = _takeMp3DeviceForHandle( handle );

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

	return done ? absPos : -1;
}




Mp3FormatDevice::Mp3FormatDevice( Mp3FormatFile * const formatFile ) :
	formatFile_( formatFile )
{
	codecForRawStrings_ = QTextCodec::codecForName( qgetenv( kCodecForRawStringsKey ) );

	pos_ = -1;
}


Mp3FormatDevice::~Mp3FormatDevice()
{
	close();
}


bool Mp3FormatDevice::_open( const QString & format, const bool firstTime )
{
	// only MP3 format is supported by this plugin
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

	int newErrorCode;
	mpgHandle_ = mpg123_new( 0, &newErrorCode );
	if ( !mpgHandle_ )
	{
		mp3FormatDebug() << "mpg123_new() error:" << newErrorCode << mpg123_plain_strerror( newErrorCode );
		return false;
	}

	const int callbacksErrorCode = mpg123_replace_reader_handle( mpgHandle_, mpg_read_handle, mpg_seek_handle, 0 );
	//const int callbacksErrorCode = mpg123_replace_reader( mpgHandle_, mpg_read_descriptor, mpg_seek_descriptor );
	Q_ASSERT( callbacksErrorCode == MPG123_OK );

	const int openErrorCode = mpg123_open_handle( mpgHandle_, this );
	//const int openErrorCode = mpg123_open( mpgHandle_, "/home/dendy/tmp/music_mp3/Amorphis/01 - Disment Of Soul (1990)/01 - Disment Of Soul.mp3" );
	//const int openErrorCode = mpg123_open_fd( mpgHandle_, generateDescriptorFordevice( this ) );
	if ( openErrorCode != MPG123_OK )
	{
		mp3FormatDebug() << "mpg123_open_handle() error:" << openErrorCode << mpg123_plain_strerror( openErrorCode );
		mpg123_delete( mpgHandle_ );
		return false;
	}

	long frequency;
	int channels;
	int encoding;
	const int formatErrorCode = mpg123_getformat( mpgHandle_, &frequency, &channels, &encoding );
	if ( formatErrorCode != MPG123_OK )
	{
		mp3FormatDebug() << "mpg123_getformat() error:" << formatErrorCode << mpg123_plain_strerror( formatErrorCode );
		mpg123_close( mpgHandle_ );
		mpg123_delete( mpgHandle_ );
		return false;
	}

	if ( encoding != MPG123_ENC_SIGNED_16 )
	{
		mp3FormatDebug() << "Encoding is not MPG123_ENC_SIGNED_16";
		mpg123_close( mpgHandle_ );
		mpg123_delete( mpgHandle_ );
		return false;
	}

	// prevent format from changing while decoding frames
	mpg123_format_none( mpgHandle_ );
	mpg123_format( mpgHandle_, frequency, channels, encoding );

	qint64 totalSamples;

	if ( isSequential_ )
	{
		totalSamples = -1;
	}
	else
	{
		//mpg123_scan( mpgHandle_ );
		totalSamples = mpg123_length( mpgHandle_ );
		if ( totalSamples < 0 )
		{
			mp3FormatDebug() << "mpg123_length error:" << totalSamples << mpg123_plain_strerror( totalSamples );
			mpg123_close( mpgHandle_ );
			mpg123_delete( mpgHandle_ );
			return false;
		}
	}

	// save recommended buffer size
	bufferSize_ = mpg123_outblock( mpgHandle_ );

	// setup format
	formatFile_->setResolvedFormat( kMp3FormatName );
	formatFile_->setChannels( channels );
	formatFile_->setFrequency( frequency );
	formatFile_->setBitsPerSample( 16 );
	formatFile_->setTotalSamples( totalSamples );

	if ( !firstTime )
	{
		if ( formatFile_->channels() != previousChannels ||
			formatFile_->bitsPerSample() != previousBitsPerSample ||
			formatFile_->frequency() != previousFrequency ||
			formatFile_->totalSamples() != previousTotalSamples )
		{
			mpg123_close( mpgHandle_ );
			mpg123_delete( mpgHandle_ );
			return false;
		}
	}

	if ( firstTime && formatFile_->openFlags().testFlag( FormatFile::OpenFlag_ReadTags ) )
	{
		QMultiMap<QString,QString> tags;

		const int metaFlags = mpg123_meta_check( mpgHandle_ );

		if ( metaFlags & MPG123_ID3 )
			_readId3Tags( tags );

		formatFile_->setTags( tags );
	}

	outputSize_ = isSequential_ ? -1 : formatFile_->samplesToBytes( formatFile_->totalSamples() );
	pos_ = 0;
	atEnd_ = false;

	return true;
}


QString Mp3FormatDevice::_fromMpgString( const mpg123_string * const string ) const
{
	if ( !string )
		return QString();

	if ( string->fill <= 0 )
		return QString();

	return QString::fromUtf8( string->p, string->fill );
}


QString Mp3FormatDevice::_fromRawString( const char * string, int size ) const
{
	Q_ASSERT( size >= 0 );

	if ( !string )
		return QString();

	const int length = qstrnlen( string, size );
	if ( length == 0 )
		return QString();

	return codecForRawStrings_ ?
		codecForRawStrings_->toUnicode( string, length ) :
		QString::fromLocal8Bit( string, length );
}


void Mp3FormatDevice::_readId3Tags( QMultiMap<QString,QString> & tags )
{
	static const QString kTitleTagKey       = QLatin1String( "TITLE" );
	static const QString kArtistTagKey      = QLatin1String( "ARTIST" );
	static const QString kAlbumTagKey       = QLatin1String( "ALBUM" );
	static const QString kGenreTagKey       = QLatin1String( "GENRE" );
	static const QString kTrackNumberTagKey = QLatin1String( "TRACKNUMBER" );
	static const QString kDateTagKey        = QLatin1String( "DATE" );

	//mpg123_scan( mpgHandle_ );

	mpg123_id3v1 * id3v1 = 0;
	mpg123_id3v2 * id3v2 = 0;

	const int id3ErrorCode = mpg123_id3( mpgHandle_, &id3v1, &id3v2 );

	if ( id3ErrorCode != MPG123_OK )
	{
		mp3FormatDebug() << "mpg123_id3() error:" << id3ErrorCode << mpg123_plain_strerror( id3ErrorCode );
		return;
	}

	mp3FormatDebug() << "id3v1 found =" << (id3v1 != 0);
	mp3FormatDebug() << "id3v2 found =" << (id3v2 != 0);

	QString title;
	QString artist;
	QString album;
	QString year;
	QString genre;
	int trackNumber = -1;

	if ( id3v2 )
	{
		title = _fromMpgString( id3v2->title );
		artist = _fromMpgString( id3v2->artist );
		album = _fromMpgString( id3v2->album );
		year = _fromMpgString( id3v2->year );
		genre = _fromMpgString( id3v2->genre );

		mp3FormatDebug() << "id3v2 tags:";
		mp3FormatDebug() << "title   :" << title;
		mp3FormatDebug() << "artist  :" << artist;
		mp3FormatDebug() << "album   :" << album;
		mp3FormatDebug() << "year    :" << year;
		mp3FormatDebug() << "genre   :" << genre;
	}

	if ( id3v1 )
	{
		if ( title.isNull() )
			title = _fromRawString( id3v1->title, 30 );
		if ( artist.isNull() )
			artist = _fromRawString( id3v1->artist, 30 );
		if ( album.isNull() )
			album = _fromRawString( id3v1->album, 30 );
		if ( year.isNull() )
			year = _fromRawString( id3v1->year, 4 );
		if ( trackNumber == -1 )
			trackNumber = static_cast<unsigned char>( id3v1->comment[29] );

		mp3FormatDebug() << "id3v1 tags:";
		mp3FormatDebug() << "title   :" << title;
		mp3FormatDebug() << "artist  :" << artist;
		mp3FormatDebug() << "album   :" << album;
		mp3FormatDebug() << "year    :" << year;
		mp3FormatDebug() << "track   :" << trackNumber;
	}

	if ( !title.isNull() )
		tags.insert( kTitleTagKey, title );
	if ( !artist.isNull() )
		tags.insert( kArtistTagKey, artist );
	if ( !album.isNull() )
		tags.insert( kAlbumTagKey, album );
	if ( !genre.isNull() )
		tags.insert( kGenreTagKey, genre );
	if ( !year.isNull() )
		tags.insert( kDateTagKey, year );
	if ( trackNumber != -1 )
		tags.insert( kTrackNumberTagKey, QString::number( trackNumber ) );
}


bool Mp3FormatDevice::open( const QIODevice::OpenMode openMode )
{
	Q_ASSERT( !isOpen() );

	if ( !mpgSinglethon_.isValid() )
	{
		// global mpg123_init() call failed
		return false;
	}

	if ( openMode != QIODevice::ReadOnly )
		return false;

	file_.setFileName( formatFile_->fileName() );

	if ( !file_.open( QIODevice::ReadOnly ) )
		return false;

	if ( !_open( formatFile_->format(), true ) )
	{
		file_.close();
		return false;
	}

	setOpenMode( openMode );

	return true;
}


void Mp3FormatDevice::_close()
{
	if ( pos_ == -1 )
		return;

	mpg123_close( mpgHandle_ );
	mpg123_delete( mpgHandle_ );

	pos_ = -1;
}


void Mp3FormatDevice::close()
{
	if ( !isOpen() )
		return;

	QIODevice::close();

	_close();

	file_.close();
}


bool Mp3FormatDevice::isSequential() const
{
	Q_ASSERT( isOpen() );

	return isSequential_;
}


qint64 Mp3FormatDevice::pos() const
{
	Q_ASSERT( isOpen() );

	return pos_;
}


bool Mp3FormatDevice::atEnd() const
{
	Q_ASSERT( isOpen() );

	return isSequential_ ? atEnd_ : pos_ == outputSize_;
}


qint64 Mp3FormatDevice::size() const
{
	if ( !isOpen() )
		return 0;

	return isSequential_ ? 0 : outputSize_;
}


qint64 Mp3FormatDevice::bytesAvailable() const
{
	if ( isSequential_ )
		return 0 + QIODevice::bytesAvailable();
	else
		return outputSize_ - pos_;
}


qint64 Mp3FormatDevice::readData( char * const data, const qint64 maxSize )
{
	Q_ASSERT( isOpen() );

	if ( atEnd_ )
		return 0;

	qint64 bytesRemain = maxSize;
	char * currentData = data;

	while ( bytesRemain > 0 )
	{
		const qint64 bytesToRead = qMin( bufferSize_, bytesRemain );

		size_t bytesRead;
		const int readErrorCode = mpg123_read( mpgHandle_, reinterpret_cast<unsigned char*>( currentData ), bytesToRead, &bytesRead );

//		mp3FormatDebug() << "readErrorCode =" << readErrorCode << ", bytesToRead =" << bytesToRead << ", bytesRead =" << bytesRead;

		if ( readErrorCode != MPG123_OK && readErrorCode != MPG123_DONE )
		{
			mp3FormatDebug() << "mpg123_read() error:" << readErrorCode << mpg123_plain_strerror( readErrorCode );
			return -1;
		}

		if ( bytesRead < 0 )
		{
			mp3FormatDebug() << "Read bytes < 0";
			return -1;
		}

		pos_ += bytesRead;
		bytesRemain -= bytesRead;
		currentData += bytesRead;

		if ( readErrorCode == MPG123_DONE )
		{
			atEnd_ = true;
			outputSize_ = pos_;
			break;
		}
	}

	return maxSize - bytesRemain;
}


qint64 Mp3FormatDevice::writeData( const char * const data, const qint64 maxSize )
{
	Q_UNUSED( data );
	Q_UNUSED( maxSize );

	return -1;
}


bool Mp3FormatDevice::seek( const qint64 pos )
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
	const qint64 sampleOffset = formatFile_->bytesToSamples( pos );
	const qint64 seekSampleOffset = mpg123_seek( mpgHandle_, sampleOffset, 0 );

	if ( seekSampleOffset != sampleOffset )
	{
		mp3FormatDebug() << "seekSampleOffset != sampleOffset";
		return false;
	}

	pos_ = pos;

	return true;
}




Mp3FormatFile::Mp3FormatFile( const QString & fileName, const QString & format,
	const OpenFlags openFlags ) :
	FormatFile( fileName, format, openFlags ),
	device_( this )
{
}


QIODevice * Mp3FormatFile::device()
{
	return &device_;
}




QStringList Mp3FormatPlugin::formats() const
{
	return QStringList()
			<< kMp3FormatName;
}


QStringList Mp3FormatPlugin::extensions( const QString & format ) const
{
	Q_UNUSED( format );
	Q_ASSERT( format == kMp3FormatName );

	return QStringList()
			<< QLatin1String( "mp3" );
}


FormatFile * Mp3FormatPlugin::createFile( const QString & fileName, const QString & format,
	const FormatFile::OpenFlags openFlags )
{
	Q_ASSERT( format.isNull() || formats().contains( format ) );

	return new Mp3FormatFile( fileName, format, openFlags );
}




} // namespace Audio
} // namespace Grim




Q_EXPORT_PLUGIN2( grim_audio_mp3_format_plugin, Grim::Audio::Mp3FormatPlugin )
