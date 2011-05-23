
#pragma once

#include <QObject>
#include <QEvent>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QHash>
#include <QRunnable>
#include <QFile>
#include <QTime>

#include <grim/tools/IdGenerator.h>

#include "Global.h"




class QThreadPool;

namespace Grim {
namespace Audio {
	class FormatFile;
	class FormatManager;
}
}




namespace Fogg {




class Job;




class Converter : public QObject
{
	Q_OBJECT

public:
	enum JobResultType
	{
		JobResult_Null         = 0,
		JobResult_Done         = 1,
		JobResult_ReadError    = 2,
		JobResult_NotSupported = 3,
		JobResult_ConvertError = 4,
		JobResult_WriteError   = 5
	};

	Converter( QObject * parent = 0 );

	Grim::Audio::FormatManager * audioFormatManager() const;

	void setConcurrentThreadCount( int count );

	int addJob( const QString & sourceFilePath, const QString & format,
			const QString & destinationFilePath, qreal quality, bool prependYearToAlbum );
	void abortJob( int jobId );
	void abortAllJobs();
	void wait();

signals:
	void jobStarted( int jobId );
	void jobResolvedFormat( int jobId, const QString & format );
	void jobProgress( int jobId, qreal progress );
	void jobFinished( int jobId, int result );

protected:
	bool event( QEvent * e );

private:
	enum EventType
	{
		EventType_JobStarted = QEvent::User + 1,
		EventType_JobResolvedFormat,
		EventType_JobProgress,
		EventType_JobFinished
	};

	class JobEvent : public QEvent
	{
	public:
		JobEvent( Job * const _job, const EventType eventType ) :
			QEvent( QEvent::Type(eventType) ),
			job( _job )
		{}

		Job * const job;
	};

	class JobResolvedFormatEvent : public JobEvent
	{
	public:
		JobResolvedFormatEvent( Job * const job, const QString & _format ) :
			JobEvent( job, EventType_JobResolvedFormat ),
			format( _format )
		{}

		QString format;
	};

private:
	Grim::Audio::FormatManager * audioFormatManager_;

	int concurrentThreadCount_;
	QThreadPool * jobThreadPool_;

	Grim::Tools::IdGenerator jobIdGenerator_;
	QHash<int,Job*> jobForId_;

	friend class Job;
};




class Job : public QRunnable
{
public:
	int id() const;

	QString sourceFilePath() const;
	QString format() const;
	QString destinationFilePath() const;
	Converter::JobResultType result() const;

	QReadWriteLock * lock() const;
	QWaitCondition * waiter() const;

	qreal progress() const;

	bool isStarted() const;
	bool isAborted() const;

	void abort();

protected:
	// reimplemented from QRunnable
	void run();

private:
	Job( Converter * converter, int id, const QString & sourceFilePath, const QString & format,
			const QString & destinationFilePath, qreal quality, bool prependYearToAlbum );

	Converter::JobResultType _runBody();
	QString _findDateTag( const QMultiMap<QString,QString> & tags ) const;

private:
	Converter * converter_;

	int id_;

	QString sourceFilePath_;
	QString format_;
	QString destinationFilePath_;
	qreal quality_;
	bool prependYearToAlbum_;

	Converter::JobResultType result_;
	bool isStarted_;
	bool isAborted_;

	mutable QReadWriteLock lock_;
	mutable QWaitCondition waiter_;

	Grim::Audio::FormatFile * sourceAudioFile_;
	QFile destinationFile_;

	qreal progress_;
	QTime sentProgressTime_;
	int sentProgressValue_;

	friend class Converter;
};




inline Grim::Audio::FormatManager * Converter::audioFormatManager() const
{ return audioFormatManager_; }




inline int Job::id() const
{ return id_; }

inline QString Job::sourceFilePath() const
{ return sourceFilePath_; }

inline QString Job::format() const
{ return format_; }

inline QString Job::destinationFilePath() const
{ return destinationFilePath_; }

inline Converter::JobResultType Job::result() const
{ return result_; }

inline QReadWriteLock * Job::lock() const
{ return &lock_; }

inline QWaitCondition * Job::waiter() const
{ return &waiter_; }

inline qreal Job::progress() const
{ return progress_; }

inline bool Job::isStarted() const
{ return isStarted_; }

inline bool Job::isAborted() const
{ return isAborted_; }




} // namespace Fogg
