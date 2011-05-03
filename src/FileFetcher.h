
#pragma once

#include <QThread>
#include <QEvent>
#include <QMutex>
#include <QWaitCondition>
#include <QStringList>
#include <QUrl>




namespace Fogg {




class FileFetcherThread;




class FileFetcher : public QObject
{
	Q_OBJECT

public:
	enum EventType
	{
		EventType_Fetched = QEvent::User + 1,
		EventType_CurrentDir,
		EventType_Finished
	};

	FileFetcher( QObject * parent = 0 );
	~FileFetcher();

	QList<QUrl> urls() const;
	void setUrls( const QList<QUrl> & urls );

	QStringList filters() const;
	void setFilters( const QStringList & filters );

	bool isRunning() const;

public slots:
	void start();
	void abort();

signals:
	void currentDirChanged( const QString & dirPath );
	void fetched( const QString & filePath, const QString & basePath );
	void finished();

protected:
	bool event( QEvent * e );

private:
	class FetchedEvent : public QEvent
	{
	public:
		FetchedEvent( const QString & _basePath, const QStringList & _filePaths ) :
			QEvent( QEvent::Type(EventType_Fetched) ),
			basePath( _basePath ), filePaths( _filePaths )
		{}

		QString basePath;
		QStringList filePaths;
	};

	class CurrentDirEvent : public QEvent
	{
	public:
		CurrentDirEvent( const QString & _dirPath ) :
			QEvent( QEvent::Type(EventType_CurrentDir) ),
			dirPath( _dirPath )
		{}

		QString dirPath;
	};

	class SearchPath
	{
	public:
		SearchPath()
		{}

		SearchPath( const QString & _basePath, const QString & _path ) :
			basePath( _basePath ), path( _path )
		{}

		QString basePath;
		QString path;
	};

private:
	// called from FileFetcherThread::run()
	void _run();

	void _processUrls();
	bool _postFetchEvent( const QString & basePath, const QStringList & filePaths );

private:
	QList<QUrl> urls_;
	QStringList filters_;

	bool isRunning_;
	FileFetcherThread * thread_;
	mutable QMutex mutex_;
	mutable QWaitCondition waiter_;
	bool isAborted_;

	friend class FileFetcherThread;
};




class FileFetcherThread : public QThread
{
	Q_OBJECT

private:
	FileFetcherThread( FileFetcher * fileFetcher );

protected:
	// reimplemented from QThread
	void run();

private:
	FileFetcher * fileFetcher_;

	friend class FileFetcher;
};




inline bool FileFetcher::isRunning() const
{ return isRunning_; }

inline QList<QUrl> FileFetcher::urls() const
{ return urls_; }

inline QStringList FileFetcher::filters() const
{ return filters_; }




} // namespace Fogg
