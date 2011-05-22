
#include "FileFetcher.h"

#include <QCoreApplication>
#include <QDir>

#include "Global.h"




namespace Fogg {




FileFetcher::FileFetcher( QObject * const parent ) :
	QObject( parent )
{
	isRunning_ = false;

	thread_ = new FileFetcherThread( this );
}


FileFetcher::~FileFetcher()
{
	Q_ASSERT( !isRunning() );
}


void FileFetcher::setUrls( const QList<QUrl> & urls )
{
	Q_ASSERT( !isRunning() );

	urls_ = urls;
}


void FileFetcher::setFilters( const QStringList & filters )
{
	Q_ASSERT( !isRunning() );

	filters_ = filters;
}


void FileFetcher::start()
{
	Q_ASSERT( !isRunning() );

	QMutexLocker locker( &mutex_ );
	isAborted_ = false;
	thread_->start();
	waiter_.wait( &mutex_ );
	isRunning_ = true;
}


void FileFetcher::abort()
{
	if ( !isRunning() )
		return;

	{
		QMutexLocker locker( &mutex_ );
		isAborted_ = true;
		QCoreApplication::removePostedEvents( this, EventType_Fetched );
		QCoreApplication::removePostedEvents( this, EventType_CurrentDir );
		QCoreApplication::removePostedEvents( this, EventType_Finished );
	}

	thread_->wait();

	isRunning_ = false;
}


bool FileFetcher::event( QEvent * const e )
{
	switch ( e->type() )
	{
	case EventType_Fetched:
	{
		Q_ASSERT( !isAborted_ );
		Q_ASSERT( isRunning_ );
		const FetchedEvent * const fetchEvent = static_cast<const FetchedEvent*>( e );
		foreach ( const QString & filePath, fetchEvent->filePaths )
		{
			emit fetched( filePath, fetchEvent->basePath, fetchEvent->isExtensionRecognized );
			if ( isAborted_ )
				break;
		}
	}
		return true;

	case EventType_CurrentDir:
	{
		Q_ASSERT( !isAborted_ );
		Q_ASSERT( isRunning_ );
		const CurrentDirEvent * const currentDirEvent = static_cast<const CurrentDirEvent*>( e );
		emit currentDirChanged( currentDirEvent->dirPath );
	}
		return true;

	case EventType_Finished:
	{
		Q_ASSERT( !isAborted_ );
		Q_ASSERT( isRunning_ );
		thread_->wait();
		isRunning_ = false;
		emit finished();
	}
		return true;
	}

	return QObject::event( e );
}


void FileFetcher::_run()
{
	{
		QMutexLocker locker( &mutex_ );
		// at this point urls_ and filters_ variables are immutable until thread
		// will be fnished, so there is no need to copy them locally, just wake the main thread
		waiter_.wakeOne();
	}

	if ( !filters().isEmpty() )
		_processUrls();

	{
		QMutexLocker locker( &mutex_ );
		if ( isAborted_ )
			return;
		QCoreApplication::postEvent( this, new QEvent( QEvent::Type(EventType_Finished) ) );
	}
}


void FileFetcher::_processUrls()
{
	QList<SearchPath> pathsToSearch;

	{
		QMap<QString,QStringList> directFilePathsForBasePath;
		QMap<QString,QStringList> nonRecognizedDirectFilePathsForBasePath;

		foreach ( const QUrl & url, urls_ )
		{
			const QFileInfo pathInfo = QFileInfo( url.toLocalFile() );

			if ( !pathInfo.exists() )
				continue;

			if ( pathInfo.isDir() )
			{
				// given URL is a directory, add it to the search list
				const QString basePath = pathInfo.path();
				const QString path = pathInfo.absoluteFilePath();
				pathsToSearch << SearchPath( basePath, path );
				continue;
			}

			// given URL is a file, check whether it matches the given filters
			const QString basePath = pathInfo.path();
			const QString path = pathInfo.absoluteFilePath();
			if ( QDir::match( filters_, pathInfo.fileName() ) )
				directFilePathsForBasePath[ basePath ] << path;
			else
				nonRecognizedDirectFilePathsForBasePath[ basePath ] << path;
		}

		for ( QMapIterator<QString,QStringList> it( directFilePathsForBasePath ); it.hasNext(); )
		{
			it.next();
			if ( !_postFetchEvent( it.key(), it.value(), true ) )
				return;
		}

		for ( QMapIterator<QString,QStringList> it( nonRecognizedDirectFilePathsForBasePath ); it.hasNext(); )
		{
			it.next();
			if ( !_postFetchEvent( it.key(), it.value(), false ) )
				return;
		}
	}

	while ( !pathsToSearch.isEmpty() )
	{
		if ( isAborted_ )
			return;

		const SearchPath currentPath = pathsToSearch.takeFirst();
		const QDir currentDir = QDir( currentPath.path );

		{
			QMutexLocker locker( &mutex_ );
			if ( isAborted_ )
				return;
			QCoreApplication::postEvent( this, new CurrentDirEvent( currentPath.path ) );
		}

		foreach ( const QString & dirPath, currentDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot ) )
			pathsToSearch << SearchPath( currentPath.basePath, currentDir.absoluteFilePath( dirPath ) );

		const QStringList fileNames = currentDir.entryList( filters_, QDir::Files );
		QStringList filePaths;
		foreach ( const QString & fileName, fileNames )
			filePaths << currentDir.absoluteFilePath( fileName );
		if ( !_postFetchEvent( currentPath.basePath, filePaths, true ) )
			return;
	}
}


bool FileFetcher::_postFetchEvent( const QString & basePath, const QStringList & filePaths, const bool isExtensionRecognized )
{
	if ( filePaths.isEmpty() )
		return true;

	QMutexLocker locker( &mutex_ );
	if ( isAborted_ )
		return false;

	QCoreApplication::postEvent( this, new FetchedEvent( basePath, filePaths, isExtensionRecognized ) );

	return true;
}




FileFetcherThread::FileFetcherThread( FileFetcher * const fileFetcher ) :
	QThread( fileFetcher ),
	fileFetcher_( fileFetcher )
{
}


void FileFetcherThread::run()
{
	fileFetcher_->_run();
}




} // namespace Fogg
