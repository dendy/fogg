
#include "FormatPluginWave.h"

#include <QDataStream>




namespace Grim {
namespace Audio {




static const QString kRawFormatName = QLatin1String( "Raw" );
static const QString kWaveFormatName = QLatin1String( "Wave" );
static const QString kAuFormatName = QLatin1String( "Au" );


static const quint32 kRiffMagic = 0x52494646; // first 4 bytes of .wav file: "RIFF"
static const quint32 kWaveMagic = 0x57415645; // 4 bytes of .wav file: "WAVE"
static const quint32 kFmtMagic  = 0x666d7420; // 4 bytes of .wav file: "fmt"
static const quint32 kDataMagic = 0x64617461; // 4 bytes of .wav file: "data"

static const quint32 kAuMagic   = 0x2E736E64; // first 4 bytes of .au file:  ".snd"
static const int kAuHeaderSize = 24;

enum AuEncoding
{
	AuEncoding_ULaw_8   = 1,  // 8-bit ISDN u-law
	AuEncoding_Pcm_8    = 2,  // 8-bit linear PCM (signed)
	AuEncoding_Pcm_16   = 3,  // 16-bit linear PCM (signed, big-endian)
	AuEncoding_Pcm_24   = 4,  // 24-bit linear PCM
	AuEncoding_Pcm_32   = 5,  // 32-bit linear PCM
	AuEncoding_Float_32 = 6,  // 32-bit IEEE floating point
	AuEncoding_Float_64 = 7,  // 64-bit IEEE floating point
	AuEncoding_ALaw_8   = 27  // 8-bit ISDN a-law
};




static void _linearCodec( const void * data, int size, void * outData )
{
	memcpy( outData, data, size );
}


static void _pcm8sCodec( const void * data, int size, void * outData )
{
	const qint8 * d = (const qint8*)data;
	qint8 * outd = (qint8*)outData;

	for ( int i = 0; i < size; i++ )
		outd[ i ] = d[ i ] + qint8( 128 );
}


static void _pcm16Codec( const void * data, int size, void * outData )
{
	const qint16 * d = (const qint16*)data;
	const int isize = size / 2;
	qint16 * outd = (qint16*)outData;

	for ( int i = 0; i < isize; i++ )
	{
		qint16 x = d[ i ];
		outd[ i ] = ((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF);
	}
}


inline static qint16 _mulaw2linear( quint8 mulawbyte )
{
	static const qint16 exp_lut[8] = {
		0, 132, 396, 924, 1980, 4092, 8316, 16764
	};

	qint16 sign, exponent, mantissa, sample;
	mulawbyte = ~mulawbyte;
	sign = (mulawbyte & 0x80);
	exponent = (mulawbyte >> 4) & 0x07;
	mantissa = mulawbyte & 0x0F;
	sample = exp_lut[exponent] + (mantissa << (exponent + 3));
	if ( sign != 0 )
		sample = -sample;
	return sample;
}


static void _uLawCodec(  const void * data, int size, void * outData  )
{
	const qint8 * d = (const qint8*)data;
	qint16 * outd = (qint16*)outData;

	for ( int i = 0; i < size; i++ )
		outd[ i ] = _mulaw2linear( d[ i ] );
}


inline static qint16 _alaw2linear( quint8 a_val )
{
	static const quint8 SignBit   = 0x80; // Sign bit for a A-law byte.
	static const quint8 QuantMask = 0x0f; // Quantization field mask.
	static const int SegShift     = 4;    // Left shift for segment number.
	static const quint8 SegMask   = 0x70; // Segment field mask.

	qint16 t, seg;
	a_val ^= 0x55;
	t = (a_val & QuantMask) << 4;
	seg = ((qint16) a_val & SegMask) >> SegShift;
	switch ( seg )
	{
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return (a_val & SignBit) ? t : -t;
}


static void _aLawCodec(  const void * data, int size, void * outData  )
{
	const qint8 * d = (const qint8*)data;
	qint16 * outd = (qint16*)outData;

	for ( int i = 0; i < size; i++ )
		outd[ i ] = _alaw2linear( d[ i ] );
}




WaveFormatDevice::WaveFormatDevice( WaveFormatFile * const formatFile ) :
	formatFile_( formatFile )
{
	isWave_ = false;
	isAu_ = false;

	pos_ = -1;
}


WaveFormatDevice::~WaveFormatDevice()
{
	close();
}


inline int WaveFormatDevice::_codecMultiplier() const
{
	Q_ASSERT( codec_ );
	return codec_ == _linearCodec || codec_ == _pcm8sCodec || codec_ == _pcm16Codec ? 1 : 2;
}


bool WaveFormatDevice::_guessWave()
{
	QDataStream ds( &file_ );
	ds.setByteOrder( QDataStream::BigEndian );

	quint32 magic;
	ds >> magic;

	if ( ds.status() != QDataStream::Ok )
		return false;

	if ( magic != kRiffMagic )
		return false;

	isWave_ = true;
	formatFile_->setResolvedFormat( kWaveFormatName );

	return true;
}


bool WaveFormatDevice::_guessAu()
{
	QDataStream ds( &file_ );
	ds.setByteOrder( QDataStream::BigEndian );

	quint32 magic;
	ds >> magic;

	if ( ds.status() != QDataStream::Ok )
		return false;

	if ( magic != kAuMagic )
		return false;

	isAu_ = true;
	formatFile_->setResolvedFormat( kAuFormatName );

	return true;
}


void WaveFormatDevice::_setOutputSize()
{
	outputSize_ = formatFile_->truncatedSize( inputSize_ * _codecMultiplier() );
	formatFile_->setTotalSamples( outputSize_ / (formatFile_->channels() * (formatFile_->bitsPerSample() >> 3)) );
}


bool WaveFormatDevice::_openRaw()
{
	// hardcoded properties for raw data
	formatFile_->setResolvedFormat( kRawFormatName );
	formatFile_->setChannels( 1 );
	formatFile_->setBitsPerSample( 8 );
	formatFile_->setFrequency( 8000 );

	codec_ = _linearCodec;
	inputSize_ = file_.size();

	_setOutputSize();

	dataPos_ = file_.pos();

	return true;
}


bool WaveFormatDevice::_openWave()
{
	QDataStream ds( &file_ );

	quint32 chunkLength;
	ds.setByteOrder( QDataStream::LittleEndian );
	ds >> chunkLength;

	{
		ds.setByteOrder( QDataStream::BigEndian );
		quint32 magic;
		ds >> magic;
		if ( magic != kWaveMagic )
			return false;
	}

	if ( ds.status() != QDataStream::Ok )
		return false;

	quint16 audioFormat;
	quint16 channels;
	quint32 frequency;
	quint32 byteRate;
	quint16 blockAlign;
	quint16 bitsPerSample;

	bool foundHeader = false;

	while ( true )
	{
		quint32 magic;
		quint32 chunkLength;

		ds.setByteOrder( QDataStream::BigEndian );
		ds >> magic;

		ds.setByteOrder( QDataStream::LittleEndian );
		ds >> chunkLength;

		if ( ds.status() != QDataStream::Ok )
			return false;

		if ( magic == kFmtMagic )
		{
			foundHeader = true;

			if ( chunkLength < 16 )
				return false;

			ds.setByteOrder( QDataStream::LittleEndian );
			ds >> audioFormat;
			ds >> channels;
			ds >> frequency;
			ds >> byteRate;
			ds >> blockAlign;
			ds >> bitsPerSample;

			if ( ds.status() != QDataStream::Ok )
				return false;

			if ( !file_.seek( file_.pos() + chunkLength - 16 ) )
				return false;

			switch ( audioFormat )
			{
			case 1: // PCM
				codec_ =
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
					_linearCodec;
#else
					bitsPerSample == 8 ? _linearCodec : _pcm16Codec;
#endif
				break;

			case 7: // uLaw
				bitsPerSample *= 2;
				codec_ = _uLawCodec;
				break;

			default:
				return false;
			}
		}
		else if ( magic == kDataMagic )
		{
			if ( !foundHeader )
				return false;

			Q_ASSERT( codec_ );
			inputSize_ = chunkLength;

			formatFile_->setResolvedFormat( kWaveFormatName );
			formatFile_->setChannels( channels );
			formatFile_->setFrequency( frequency );
			formatFile_->setBitsPerSample( bitsPerSample );

			_setOutputSize();

			dataPos_ = file_.pos();

			return true;
		}
		else
		{
			if ( !file_.seek( file_.pos() + chunkLength ) )
				return false;
		}

		if ( (chunkLength & 1) && !file_.atEnd() )
		{
			if ( !file_.seek( file_.pos() + 1 ) )
				return false;
		}
	}

	return false;
}


bool WaveFormatDevice::_openAu()
{
	QDataStream ds( &file_ );

	qint32 dataOffset;    // byte offset to data part, minimum 24
	qint32 dataLength;    // number of bytes in the data part, -1 = not known
	qint32 dataEncoding;  // encoding of the data part, see AUEncoding
	qint32 frequency;     // number of samples per second
	qint32 channels;      // number of interleaved channels

	int bitsPerSample = 0;

	ds.setByteOrder( QDataStream::BigEndian );
	ds >> dataOffset;
	ds >> dataLength;
	ds >> dataEncoding;
	ds >> frequency;
	ds >> channels;

	if ( ds.status() != QDataStream::Ok )
		return false;

	if ( dataLength == -1 )
		dataLength = file_.bytesAvailable() - kAuHeaderSize - dataOffset;

	if ( dataOffset < kAuHeaderSize || dataLength <= 0 || frequency < 1 || channels < 1 )
		return false;

	if ( !file_.seek( file_.pos() + dataOffset - kAuHeaderSize ) )
		return false;

	switch ( dataEncoding )
	{
	case AuEncoding_ULaw_8:
		bitsPerSample = 16;
		codec_ = _uLawCodec;
		break;

	case AuEncoding_Pcm_8:
		bitsPerSample = 8;
		codec_ = _pcm8sCodec;
		break;

	case AuEncoding_Pcm_16:
		bitsPerSample = 16;
		codec_ =
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
			_pcm16Codec;
#else
			_linearCodec;
#endif
		break;

	case AuEncoding_ALaw_8:
		bitsPerSample = 16;
		codec_ = _aLawCodec;
		break;

	default:
		return false;
	}

	inputSize_ = dataLength;

	formatFile_->setResolvedFormat( kAuFormatName );
	formatFile_->setChannels( channels );
	formatFile_->setFrequency( frequency );
	formatFile_->setBitsPerSample( bitsPerSample );

	_setOutputSize();

	dataPos_ = file_.pos();

	return true;
}


qint64 WaveFormatDevice::_readData( char * const data, const qint64 maxSize )
{
	qint64 bytesToRead = qMin( outputSize_ - pos_, maxSize );

	QByteArray buffer;
	buffer.resize( bytesToRead / _codecMultiplier() );
	if ( file_.read( buffer.data(), buffer.size() ) != buffer.size() )
		return -1;

	codec_( buffer.constData(), buffer.size(), data );

	pos_ += bytesToRead;

	return bytesToRead;
}


bool WaveFormatDevice::_open( const QString & format, const bool firstTime )
{
	if ( !firstTime )
		Q_ASSERT( !format.isEmpty() );

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

	isWave_ = false;
	isAu_ = false;

	// resolve actual format
	if ( format.isNull() )
	{
		bool isWaveChecked = false;
		bool isAuChecked = false;

		// first check formats by extension
		if ( file_.fileName().endsWith( QLatin1String( ".wav" ), Qt::CaseInsensitive ) )
		{
			isWaveChecked = true;
			if ( _guessWave() )
				goto formatIsChecked;
		}

		if ( file_.fileName().endsWith( QLatin1String( ".au" ), Qt::CaseInsensitive ) )
		{
			isAuChecked = true;
			if ( _guessAu() )
				goto formatIsChecked;
		}

		// not extensions or extension not accorded to file contents
		// ignore extension and check by contents only
		if ( !isWaveChecked && _guessWave() )
			goto formatIsChecked;

		if ( !isAuChecked && _guessAu() )
			goto formatIsChecked;

		// unknown file format, we will not treat it as raw
		file_.close();
		return false;
	}

	formatIsChecked:

	const QString actualFormat = !formatFile_->format().isNull() ? formatFile_->format() : formatFile_->resolvedFormat();
	Q_ASSERT( !actualFormat.isNull() );

	bool isOpened = false;
	if ( actualFormat == kRawFormatName )
	{
		isOpened = _openRaw();
	}
	else if ( actualFormat == kWaveFormatName )
	{
		isWave_ = true;
		if ( !format.isNull() && !_guessWave() )
		{
			file_.close();
			return false;
		}

		isOpened = _openWave();
	}
	else if ( actualFormat == kAuFormatName )
	{
		isAu_ = true;
		if ( !format.isNull() && !_guessAu() )
		{
			file_.close();
			return false;
		}
		isOpened = _openAu();
	}
	else
	{
		Q_ASSERT( false );
	}

	if ( !isOpened )
	{
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
			file_.close();
			return false;
		}
	}

	pos_ = 0;

	return true;
}


bool WaveFormatDevice::open( const QIODevice::OpenMode openMode )
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


void WaveFormatDevice::_close()
{
	if ( pos_ == -1 )
		return;

	pos_ = -1;
}


void WaveFormatDevice::close()
{
	if ( !isOpen() )
		return;

	QIODevice::close();

	_close();

	file_.close();
}


bool WaveFormatDevice::isSequential() const
{
	Q_ASSERT( isOpen() );
	return isSequential_;
}


qint64 WaveFormatDevice::pos() const
{
	return pos_;
}


bool WaveFormatDevice::atEnd() const
{
	Q_ASSERT( isOpen() );

	return pos_ == outputSize_;
}


qint64 WaveFormatDevice::size() const
{
	if ( !isOpen() )
		return 0;

	return outputSize_;
}


qint64 WaveFormatDevice::bytesAvailable() const
{
	if ( isSequential_ )
		return outputSize_ - pos_ + QIODevice::bytesAvailable();
	else
		return outputSize_ - pos_;
}


qint64 WaveFormatDevice::readData( char * const data, const qint64 maxSize )
{
	Q_ASSERT( isOpen() );

	return _readData( data, formatFile_->truncatedSize( maxSize ) );
}


qint64 WaveFormatDevice::writeData( const char * data, qint64 maxSize )
{
	return -1;
}


bool WaveFormatDevice::seek( qint64 pos )
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

	// Same as for Vorbis and FLAC.
	Q_ASSERT( !isSequential_ );

	// this just asserts correct pos value
	formatFile_->bytesToSamples( pos );

	const qint64 outputPos = dataPos_ + pos / _codecMultiplier();

	if ( !file_.seek( outputPos ) )
		return false;

	pos_ = pos;

	return true;
}




WaveFormatFile::WaveFormatFile( const QString & fileName, const QString & format,
	const OpenFlags openFlags ) :
	FormatFile( fileName, format, openFlags ),
	device_( this )
{
}


QIODevice * WaveFormatFile::device()
{
	return &device_;
}




QStringList WaveFormatPlugin::formats() const
{
	return QStringList()
			<< kRawFormatName
			<< kWaveFormatName
			<< kAuFormatName;
}


QStringList WaveFormatPlugin::extensions( const QString & format ) const
{
	Q_ASSERT( formats().contains( format ) );

	if ( format == kRawFormatName )
		return QStringList() << QLatin1String( "raw" );
	else if ( format == kWaveFormatName )
		return QStringList() << QLatin1String( "wav" );
	else if ( format == kAuFormatName )
		return QStringList() << QLatin1String( "au" );

	Q_ASSERT( false );
	return QStringList();
}


FormatFile * WaveFormatPlugin::createFile( const QString & fileName, const QString & format,
	const FormatFile::OpenFlags openFlags )
{
	Q_ASSERT( format.isNull() || formats().contains( format ) );

	return new WaveFormatFile( fileName, format, openFlags );
}




} // namespace Audio
} // namespace Grim
