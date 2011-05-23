
#include "Converter.h"

#include <QCoreApplication>
#include <QThreadPool>
#include <QMutexLocker>
#include <QFileInfo>
#include <QDir>
#include <QThread>

#include <grim/audio/FormatPlugin.h>
#include <grim/audio/FormatManager.h>

#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>




namespace Fogg {




// emit progress each 1% and each 1 second
static const int kMaxProgress = 100;
static const int kProgressInterval = 200;

// Vorbis tags
static const QString kVorbisTagAlbum = QLatin1String( "ALBUM" );
static const QString kVorbisTagDate  = QLatin1String( "DATE" );




Converter::Converter( QObject * const parent ) :
	QObject( parent )
{
	audioFormatManager_ = new Grim::Audio::FormatManager( this );

	concurrentThreadCount_ = 0;
	jobThreadPool_ = new QThreadPool( this );
}


void Converter::setConcurrentThreadCount( const int count )
{
	if ( count == concurrentThreadCount_ )
		return;

	concurrentThreadCount_ = count;
	jobThreadPool_->setMaxThreadCount( concurrentThreadCount_ == 0 ?
			QThread::idealThreadCount() : concurrentThreadCount_ );
}


int Converter::addJob( const QString & sourceFilePath, const QString & format,
		const QString & destinationFilePath, const qreal quality, const bool prependYearToAlbum )
{
	const int jobId = jobIdGenerator_.take();

	Job * const job = new Job( this, jobId, sourceFilePath, format, destinationFilePath, quality, prependYearToAlbum );
	jobForId_[ jobId ] = job;

	jobThreadPool_->start( job );

	return jobId;
}


void Converter::abortJob( const int jobId )
{
	Job * const job = jobForId_.value( jobId );
	Q_ASSERT( job );
	Q_ASSERT( !job->isAborted() );
	Q_ASSERT( job->id() == jobId );

	{
		QWriteLocker locker( job->lock() );

		job->abort();

		if ( job->isStarted() )
		{
			// job is running, wait for finished event, then remove it
		}
		else
		{
			jobForId_.remove( job->id() );
			jobIdGenerator_.free( job->id() );
		}
	}
}


void Converter::abortAllJobs()
{
	foreach ( const int jobId, jobForId_.keys() )
		abortJob( jobId );
}


void Converter::wait()
{
	// collect all finished events from jobs to wake them
	while ( !jobForId_.isEmpty() )
	{
		QCoreApplication::sendPostedEvents( this, EventType_JobStarted );
		QCoreApplication::sendPostedEvents( this, EventType_JobResolvedFormat );
		QCoreApplication::sendPostedEvents( this, EventType_JobProgress );
		QCoreApplication::sendPostedEvents( this, EventType_JobFinished );
	}

	// explicitly wait for thread pool
	jobThreadPool_->waitForDone();
}


bool Converter::event( QEvent * const e )
{
	switch ( e->type() )
	{
	case EventType_JobStarted:
	case EventType_JobResolvedFormat:
	case EventType_JobProgress:
	case EventType_JobFinished:
		const JobEvent * jobEvent = static_cast<JobEvent*>( e );
		QWriteLocker jobLocker( jobEvent->job->lock() );
		switch ( e->type() )
		{
		case EventType_JobStarted:
			if ( !jobEvent->job->isAborted() )
				emit jobStarted( jobEvent->job->id() );
			break;

		case EventType_JobResolvedFormat:
			if ( !jobEvent->job->isAborted() )
				emit jobResolvedFormat( jobEvent->job->id(), static_cast<const JobResolvedFormatEvent*>( jobEvent )->format );
			break;

		case EventType_JobProgress:
			if ( !jobEvent->job->isAborted() )
				emit jobProgress( jobEvent->job->id(), jobEvent->job->progress() );
			break;

		case EventType_JobFinished:
			if ( !jobEvent->job->isAborted() )
				emit jobFinished( jobEvent->job->id(), jobEvent->job->result() );

			jobForId_.remove( jobEvent->job->id() );
			jobIdGenerator_.free( jobEvent->job->id() );
			break;
		}
		jobEvent->job->waiter()->wakeOne();
		return true;
	}

	return QObject::event( e );
}




Job::Job( Converter * const converter, const int id, const QString & sourceFilePath, const QString & format,
		const QString & destinationFilePath, const qreal quality, const bool prependYearToAlbum )
{
	converter_ = converter;

	id_ = id;

	sourceFilePath_ = sourceFilePath;
	format_ = format;
	destinationFilePath_ = destinationFilePath;
	quality_ = quality;
	prependYearToAlbum_ = prependYearToAlbum;

	result_ = Converter::JobResult_Null;
	isStarted_ = false;
	isAborted_ = false;

	progress_ = 0;
	sentProgressValue_ = 0;

	sourceAudioFile_ = 0;
}


void Job::run()
{
	// send started event
	{
		QWriteLocker locker( &lock_ );
		if ( isAborted_ )
		{
			// do not emit finished event
			return;
		}
		isStarted_ = true;
		QCoreApplication::postEvent( converter_, new Converter::JobEvent( this, Converter::EventType_JobStarted ) );
		waiter_.wait( &lock_ );
	}

	result_ = _runBody();

	// send finished event
	{
		QWriteLocker locker( &lock_ );
		QCoreApplication::postEvent( converter_, new Converter::JobEvent( this, Converter::EventType_JobFinished ) );
		waiter_.wait( &lock_ );
	}

	// At this point all references to this Job instance are lost
	// and we can safely use isAborted flag not only for quick exiting from _runBody(),
	// but also for cleanup if conversion has been aborted.
	if ( isAborted() || result_ != Converter::JobResult_Done )
	{
		if ( destinationFile_.isOpen() )
			destinationFile_.close();
		destinationFile_.remove();
	}

	if ( sourceAudioFile_ )
	{
		delete sourceAudioFile_;
		sourceAudioFile_ = 0;
	}
}


Converter::JobResultType Job::_runBody()
{
	if ( isAborted() )
		return Converter::JobResult_Null;

	const QFileInfo destinationFileInfo = QFileInfo( destinationFilePath() );
	destinationFile_.setFileName( destinationFilePath() );

	static const int kOpenDestinationFileTryCount = 4;
	static const int kOpenDestinationFileDelay = 500;

	for ( int i = 0; i < kOpenDestinationFileTryCount; ++i )
	{
		QDir().mkpath( destinationFileInfo.path() );

		if ( destinationFile_.open( QIODevice::WriteOnly ) )
			break;

		foggWarning() << "Error opening destination file for write:" << destinationFilePath();

		Global::msleep( kOpenDestinationFileDelay );
	}

	if ( !destinationFile_.isOpen() )
		return Converter::JobResult_WriteError;

	sourceAudioFile_ = converter_->audioFormatManager()->createFormatFile( sourceFilePath(), format() );
	if ( !sourceAudioFile_ )
		return Converter::JobResult_NotSupported;

	{
		QWriteLocker locker( &lock_ );
		QCoreApplication::postEvent( converter_,
				new Converter::JobResolvedFormatEvent( this, sourceAudioFile_->resolvedFormat() ) );
		waiter_.wait( &lock_ );
	}

	const int channelCount = sourceAudioFile_->channels();
	if ( channelCount != 1 && channelCount != 2 )
	{
		// only mono and stereo audio files supported and this moment
		return Converter::JobResult_NotSupported;
	}

	vorbis_info vi;
	vorbis_info_init( &vi );

	if ( vorbis_encode_init_vbr( &vi, sourceAudioFile_->channels(), sourceAudioFile_->frequency(), quality_ ) )
	{
		vorbis_info_clear( &vi );
		return Converter::JobResult_ConvertError;
	}

	// save original tags
	vorbis_comment vc;
	vorbis_comment_init( &vc );

	const QString dateTagValue = _findDateTag( sourceAudioFile_->tags() );

	for ( QMapIterator<QString,QString> tagIterator( sourceAudioFile_->tags() ); tagIterator.hasNext(); )
	{
		tagIterator.next();

		const QString tagKey = tagIterator.key().toUpper();
		const QString tagValue = (prependYearToAlbum_ && tagKey == kVorbisTagAlbum) ?
				QString::fromLatin1( "%1 - %2" ).arg( dateTagValue.toInt(), 4, 10, QLatin1Char( '0' ) ).arg( tagIterator.value() ) :
				tagIterator.value();

		vorbis_comment_add_tag( &vc,
			tagKey.toLatin1().constData(),
			tagValue.toUtf8().constData() );
	}

	vorbis_dsp_state vd;
	vorbis_analysis_init( &vd, &vi );

	vorbis_block vb;
	vorbis_block_init( &vd, &vb );

	ogg_stream_state os;
	ogg_page og;
	ogg_packet op;

	ogg_stream_init( &os, 1 );

	ogg_packet header, header_comm, header_code;
	vorbis_analysis_headerout( &vd, &vc, &header, &header_comm, &header_code );
	ogg_stream_packetin( &os, &header );
	ogg_stream_packetin( &os, &header_comm );
	ogg_stream_packetin( &os, &header_code );

	bool readError = false;
	bool writeError = false;

	int eos = 0;

	while ( !eos )
	{
		int result = ogg_stream_flush( &os, &og );
		if ( result == 0 )
			break;
		if ( destinationFile_.write( (const char *)og.header, og.header_len ) != og.header_len )
		{
			writeError = true;
			break;
		}
		if ( destinationFile_.write( (const char *)og.body, og.body_len ) != og.body_len )
		{
			writeError = true;
			break;
		}
	}

	if ( !writeError )
	{
		while ( !eos )
		{
			static const int kBufferSize = 1024*128;
			char buf[ kBufferSize ];

			const qint64 bytes = sourceAudioFile_->device()->read( buf, sizeof(buf) );

			if ( bytes == -1 )
			{
				readError = true;
				break;
			}

			if ( bytes == 0 )
			{
				vorbis_analysis_wrote( &vd, 0 );
			}
			else
			{
				// each Vorbis sample is 2 byte float value
				const int sampleCount = bytes/(2*channelCount);

				// uninterleave samples
				float ** const buffer = vorbis_analysis_buffer( &vd, sampleCount );

				if ( channelCount == 2 )
				{
					for ( int i = 0; i < sampleCount; i++ )
					{
						buffer[ 0 ][ i ] = ((buf[i*4+1]<<8)|(0x00ff&(int)buf[i*4+0]))/32768.f;
						buffer[ 1 ][ i ] = ((buf[i*4+3]<<8)|(0x00ff&(int)buf[i*4+2]))/32768.f;
					}
				}
				else
				{
					for ( int i = 0; i < sampleCount; i++ )
					{
						buffer[ 0 ][ i ] = ((buf[i*2+1]<<8)|(0x00ff&(int)buf[i*2]))/32768.f;
					}
				}

				// tell the library how much we actually submitted
				vorbis_analysis_wrote( &vd, sampleCount );
			}

			while ( vorbis_analysis_blockout( &vd, &vb ) == 1 )
			{
				// analysis, assume we want to use bitrate management
				vorbis_analysis( &vb, 0 );
				vorbis_bitrate_addblock( &vb );

				while ( vorbis_bitrate_flushpacket( &vd, &op ) )
				{
					// weld the packet into the bitstream
					ogg_stream_packetin( &os, &op );

					// write out pages (if any)
					while ( !eos )
					{
						int result = ogg_stream_pageout( &os, &og );
						if ( result == 0 )
							break;

						if ( destinationFile_.write( (const char *)og.header, og.header_len ) != og.header_len )
						{
							writeError = true;
							break;
						}
						if ( destinationFile_.write( (const char *)og.body, og.body_len ) != og.body_len )
						{
							writeError = true;
							break;
						}

						if ( ogg_page_eos( &og ) )
							eos = 1;
					}

					if ( writeError || isAborted_ )
						break;
				}

				if ( writeError || isAborted_ )
					break;
			}

			if ( writeError || isAborted_ )
				break;

			// calculate progress
			progress_ = (qreal)sourceAudioFile_->bytesToSamples( sourceAudioFile_->device()->pos() ) /
				sourceAudioFile_->totalSamples();

			const int currentProgressValue = qBound( 0, static_cast<int>( progress_*kMaxProgress ), kMaxProgress );
			if ( currentProgressValue > sentProgressValue_ )
			{
				const QTime currentTime = QTime::currentTime();
				const int timeAfterPreviousSent = sentProgressTime_.msecsTo( currentTime );
				if ( timeAfterPreviousSent >= kProgressInterval )
				{
					sentProgressTime_ = currentTime;
					sentProgressValue_ = currentProgressValue;

					{
						QWriteLocker locker( &lock_ );
						QCoreApplication::postEvent( converter_,
							new Converter::JobEvent( this, Converter::EventType_JobProgress) );
						waiter_.wait( &lock_ );
					}
				}
			}
		}
	}

	// cleanup
	ogg_stream_clear( &os );
	vorbis_block_clear( &vb );
	vorbis_dsp_clear( &vd );
	vorbis_comment_clear( &vc );
	vorbis_info_clear( &vi );

	destinationFile_.close();

	if ( readError )
		return Converter::JobResult_ReadError;

	if ( writeError )
		return Converter::JobResult_WriteError;

	return Converter::JobResult_Done;
}


QString Job::_findDateTag( const QMultiMap<QString,QString> & tags ) const
{
	if ( !prependYearToAlbum_ )
		return QString();

	for ( QMapIterator<QString,QString> it( tags ); it.hasNext(); )
	{
		it.next();
		if ( it.key().toUpper() == kVorbisTagDate )
			return it.value();
	}

	return QString();
}


void Job::abort()
{
	isAborted_ = true;
}




} // namespace Foog
