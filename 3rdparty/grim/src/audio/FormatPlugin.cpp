
#include "FormatPlugin.h"




namespace Grim {
namespace Audio {




class FormatFilePrivate
{
public:
	void setSampleChunkSize()
	{
		if ( channels == -1 || bitsPerSample == -1 )
			return;

		sampleChunkSize = channels * bytesPerSample;
	}

	// given
	QString fileName;
	QString format;
	FormatFile::OpenFlags openFlags;

	// resolved
	QString resolvedFormat;
	int channels;
	int frequency;
	int bitsPerSample;
	qint64 totalSamples;
	QMultiMap<QString,QString> tags;

	// generated
	int bytesPerSample;
	int sampleChunkSize;
};




FormatFile::FormatFile( const QString & fileName, const QString & format, const OpenFlags openFlags ) :
	d_( new FormatFilePrivate )
{
	d_->fileName = fileName;
	d_->format = format;
	d_->openFlags = openFlags;

	d_->channels = -1;
	d_->frequency = -1;
	d_->bitsPerSample = -1;
	d_->totalSamples = -1;

	d_->bytesPerSample = -1;
	d_->sampleChunkSize = -1;
}


FormatFile::~FormatFile()
{
}


FormatFile::OpenFlags FormatFile::openFlags() const
{
	return d_->openFlags;
}


QString FormatFile::fileName() const
{
	return d_->fileName;
}


QString FormatFile::format() const
{
	return d_->format;
}


QString FormatFile::resolvedFormat() const
{
	return d_->resolvedFormat;
}


int FormatFile::channels() const
{
	return d_->channels;
}


int FormatFile::frequency() const
{
	return d_->frequency;
}


int FormatFile::bitsPerSample() const
{
	return d_->bitsPerSample;
}


qint64 FormatFile::totalSamples() const
{
	return d_->totalSamples;
}


QMultiMap<QString,QString> FormatFile::tags() const
{
	return d_->tags;
}


qint64 FormatFile::truncatedSize( qint64 size ) const
{
	Q_ASSERT( d_->channels != -1 && d_->frequency != -1 && d_->bitsPerSample != -1 );

	const int oddSize = size % d_->sampleChunkSize;

	return size - oddSize;
}


qint64 FormatFile::bytesToSamples( qint64 bytes ) const
{
	Q_ASSERT( d_->channels != -1 && d_->frequency != -1 && d_->bitsPerSample != -1 );
	Q_ASSERT( bytes >= 0 && (bytes % d_->sampleChunkSize) == 0 );

	return bytes / d_->sampleChunkSize;
}


qint64 FormatFile::samplesToBytes( qint64 samples ) const
{
	Q_ASSERT( d_->channels != -1 && d_->frequency != -1 && d_->bitsPerSample != -1 );
	Q_ASSERT( samples >= 0 );

	return samples * d_->sampleChunkSize;
}


void FormatFile::setResolvedFormat( const QString & format )
{
	d_->resolvedFormat = format;
}


void FormatFile::setChannels( int channels )
{
	Q_ASSERT( channels > 0 );

	d_->channels = channels;

	d_->setSampleChunkSize();
}


void FormatFile::setFrequency( int frequency )
{
	Q_ASSERT( frequency >= 0 );

	d_->frequency = frequency;
}


void FormatFile::setBitsPerSample( int bitsPerSample )
{
	Q_ASSERT( bitsPerSample >= 0 && (bitsPerSample & 0x07) == 0 );

	d_->bitsPerSample = bitsPerSample;
	d_->bytesPerSample = bitsPerSample >> 3;

	d_->setSampleChunkSize();
}


void FormatFile::setTotalSamples( qint64 totalSamples )
{
	Q_ASSERT( totalSamples >= 0 || totalSamples == -1 );

	d_->totalSamples = totalSamples;
}


void FormatFile::setTags( const QMultiMap<QString,QString> & tags )
{
	Q_ASSERT( openFlags().testFlag( OpenFlag_ReadTags ) );

	d_->tags = tags;
}




} // namespace Audio
} // namespace Grim
