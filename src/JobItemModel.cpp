
#include "JobItemModel.h"

#include "Converter.h"




namespace Fogg {




JobItemModel::Item::Item( ItemType _type ) :
	type( _type )
{
	Q_ASSERT( type == Item_Dir || type == Item_File );

	parentItem = 0;
	row = -1;

	state = State_Null;
	result = Converter::JobResult_Null;
}


qreal JobItemModel::Item::progress() const
{
	switch ( type )
	{
	case Item_Dir:
		return asDir()->totalProgress;
	case Item_File:
		return asFile()->totalProgress();
	}

	Q_ASSERT( false );
	return 0;
}


void JobItemModel::Item::destroy()
{
	switch ( type )
	{
	case Item_Dir:
		Q_ASSERT( asDir()->childItems.isEmpty() );
		delete asDir();
		break;
	case Item_File:
		delete asFile();
		break;
	default:
		Q_ASSERT( false );
	}
}




JobItemModel::DirItem::DirItem() :
	Item( JobItemModel::Item_Dir )
{
	totalWeight = 0;
	totalProgress = 0;
}


void JobItemModel::DirItem::addChildItem( Item * const item )
{
	Q_ASSERT( !item->parentItem );
	Q_ASSERT( !childItems.contains( item ) );

	const int itemWeight = item->weight();
	const int newTotalWeight = totalWeight + itemWeight;
	const qreal itemProgress = item->progress();
	const qreal newProgress = newTotalWeight == 0 ? 0 :
			qMax<qreal>( 0, totalProgress*totalWeight + itemProgress*itemWeight ) / newTotalWeight;

	const int wasWeight = totalWeight;
	const qreal wasProgress = totalProgress;

	totalWeight = newTotalWeight;
	totalProgress = newProgress;

	childItems << item;
	item->parentItem = this;
	item->row = childItems.count() - 1;

	if ( parentItem )
		parentItem->asDir()->childItemChanged( this, wasWeight, wasProgress );
}


void JobItemModel::DirItem::removeChildItem( Item * const item )
{
	Q_ASSERT( item->parentItem == this );
	Q_ASSERT( childItems.contains( item ) );

	const int itemWeight = item->weight();
	const int newTotalWeight = totalWeight - itemWeight;
	const qreal itemProgress = item->progress();
	const qreal newProgress = newTotalWeight == 0 ? 0 :
			qMax<qreal>( 0, totalProgress*totalWeight - itemProgress*itemWeight ) / newTotalWeight;

	const int wasWeight = totalWeight;
	const qreal wasProgress = totalProgress;

	totalWeight = newTotalWeight;
	totalProgress = newProgress;

	for ( int i = item->row + 1; i < childItems.count(); ++i )
	{
		Q_ASSERT( childItems.at( i )->row == i );
		childItems[ i ]->row--;
	}

	childItems.removeOne( item );
	item->parentItem = 0;
	item->row = -1;

	if ( parentItem )
		parentItem->asDir()->childItemChanged( this, wasWeight, wasProgress );
}


void JobItemModel::DirItem::childItemChanged( const Item * const item, const int wasWeight, const qreal wasProgress )
{
	Q_ASSERT( item->parentItem == this );
	Q_ASSERT( childItems.contains( const_cast<Item*>( item ) ) );

	const int itemWeight = item->weight();
	const int newTotalWeight = totalWeight + itemWeight - wasWeight;
	const qreal itemProgress = item->progress();
	const qreal newProgress = newTotalWeight == 0 ? 0 :
			qMax<qreal>( 0, totalProgress*totalWeight + itemProgress*itemWeight - wasWeight*wasProgress ) / newTotalWeight;

	const int selfWasWeight = totalWeight;
	const qreal selfWasProgress = totalProgress;

	totalWeight = newTotalWeight;
	totalProgress = newProgress;

	if ( parentItem )
		parentItem->asDir()->childItemChanged( this, selfWasWeight, selfWasProgress );
}


void JobItemModel::DirItem::clearChildItems()
{
	QList<Item*> itemsToDestroy = childItems;

	while ( !itemsToDestroy.isEmpty() )
	{
		Item * const item = itemsToDestroy.takeFirst();
		if ( item->type == Item_Dir )
		{
			itemsToDestroy << item->asDir()->childItems;
			item->asDir()->childItems.clear();
		}
		item->destroy();
	}

	childItems.clear();

	const int wasWeight = totalWeight;
	const qreal wasProgress = totalProgress;

	totalWeight = 0;
	totalProgress = 0;

	if ( parentItem )
		parentItem->asDir()->childItemChanged( this, wasWeight, wasProgress );
}


JobItemModel::Item * JobItemModel::DirItem::findChildItemByName( const QString & name ) const
{
	foreach ( Item * const item, childItems )
		if ( item->name == name )
			return item;
	return 0;
}




JobItemModel::FileItem::FileItem() :
	Item( Item_File )
{
	jobId = 0;
	conversionProgress = 0;
}


qreal JobItemModel::FileItem::totalProgress() const
{
	return result == Converter::JobResult_Null ? conversionProgress : 1.0;
}




int JobItemModel::progressToPercents( const qreal progress )
{
	return qBound( 0, qRound( progress*100 ), 100 );
}


JobItemModel::JobItemModel( QObject * parent ) :
	QAbstractItemModel( parent )
{
	rootItem_ = new DirItem;
}


JobItemModel::~JobItemModel()
{
	rootItem_->clearChildItems();
	rootItem_->destroy();
}


QString JobItemModel::_nameForColumn( const ColumnType column )
{
	switch ( column )
	{
	case Column_Name:
		return tr( "Filename" );
	case Column_Progress:
		return tr( "Progress" );
	case Column_State:
		return tr( "State" );
	}

	Q_ASSERT( false );
	return QString();
}


QString JobItemModel::_nameForStateAndResult( const StateType state, const int result )
{
	Q_ASSERT( state == State_Null || result == Converter::JobResult_Null );

	switch ( state )
	{
	case State_Null:
		return result == Converter::JobResult_Null ? QString() : tr( "Finished" );
	case State_Idle:
		return tr( "Idle" );
	case State_Running:
		return tr( "Running" );
	}

	Q_ASSERT( false );
	return QString();
}


QString JobItemModel::_nameForJobResult( const int result )
{
	switch ( result )
	{
	case Converter::JobResult_Null:
		return QString();
	case Converter::JobResult_Done:
		return tr( "Done" );
	case Converter::JobResult_NotSupported:
		return tr( "Format not supported" );
	case Converter::JobResult_ConvertError:
		return tr( "Converting error" );
	case Converter::JobResult_ReadError:
		return tr( "Read error" );
	case Converter::JobResult_WriteError:
		return tr( "Write error" );
	}

	return tr( "Unknown" );
}


JobItemModel::Item * JobItemModel::_itemForIndex( const QModelIndex & index ) const
{
	if ( !index.isValid() )
		return rootItem_;

	Item * const item = static_cast<Item*>( index.internalPointer() );
	Q_ASSERT( item );
	return item;
}


QModelIndex JobItemModel::_indexForItem( Item * const item, const int column ) const
{
	Q_ASSERT( item );

	if ( !item->parentItem )
	{
		// this is a root item, no parent
		Q_ASSERT( item == rootItem_ );
		return QModelIndex();
	}

	return createIndex( item->row, column, item );
}


void JobItemModel::_setItemName( Item * const item, const QString & name )
{
	item->name = name;
	item->modelName = QVariant();
}


void JobItemModel::_setItemProgress( Item * const item )
{
	item->modelProgress = QVariant();
}


void JobItemModel::_setItemStateAndResult( Item * const item, const StateType state, const int result )
{
	Q_ASSERT( state == State_Null || result == Converter::JobResult_Null );

	const bool wasFinished = item->result != Converter::JobResult_Null;
	const qreal wasProgress = item->progress();

	item->state = state;
	item->result = result;
	item->modelState = QVariant();

	if ( item->type == Item_File )
	{
		if ( wasFinished && item->result == Converter::JobResult_Null )
		{
			Q_ASSERT( !allUnfinishedFileItems().contains( item->asFile() ) );
			allUnfinishedFileItems_ << item->asFile();

			if ( item->asFile()->jobId == 0 )
			{
				Q_ASSERT( !allInactiveUnfinishedFileItems().contains( item->asFile() ) );
				allInactiveUnfinishedFileItems_ << item->asFile();
			}
		}
		else if ( !wasFinished && item->result != Converter::JobResult_Null )
		{
			Q_ASSERT( allUnfinishedFileItems().contains( item->asFile() ) );
			allUnfinishedFileItems_.removeOne( item->asFile() );

			if ( item->asFile()->jobId == 0 )
			{
				Q_ASSERT( allInactiveUnfinishedFileItems().contains( item->asFile() ) );
				allInactiveUnfinishedFileItems_.removeOne( item->asFile() );
			}
		}
	}

	if ( item->parentItem )
		item->parentItem->childItemChanged( item, item->weight(), wasProgress );
}


void JobItemModel::_updateItemModelName( const Item * const item ) const
{
	if ( !item->modelName.isNull() )
		return;

	item->modelName = QVariant::fromValue( item->name );
}


void JobItemModel::_updateItemModelProgress( const Item * const item ) const
{
	if ( !item->modelProgress.isNull() )
		return;

	switch ( item->type )
	{
	case Item_File:
		if ( item->state == State_Null && item->result == Converter::JobResult_Null )
		{
			// hide progress indicator for unfinished file without assigned job
			item->modelProgress = QVariant::fromValue( QString() );
		}
		else
		{
			// show progress in percents: 0..100%
			const int percents = JobItemModel::progressToPercents( item->asFile()->conversionProgress );
			item->modelProgress = QVariant::fromValue( percents );
		}
		break;

	case Item_Dir:
		const int percents = JobItemModel::progressToPercents( item->asDir()->totalProgress );
		if ( percents == 0 )
			item->modelProgress = QVariant::fromValue( QString() );
		else
			item->modelProgress = QVariant::fromValue( percents );
		break;
	}
}


void JobItemModel::_updateItemModelState( const Item * const item ) const
{
	Q_ASSERT( item->state == State_Null || item->result == Converter::JobResult_Null );

	if ( !item->modelState.isNull() )
		return;

	static const QString kFinishedStateTemplate = QLatin1String( "%1 (%2)" );
	const QString stateName = _nameForStateAndResult( item->state, item->result );

	switch ( item->type )
	{
	case Item_Dir:
		item->modelState = QVariant::fromValue( stateName );
		break;

	case Item_File:
		if ( item->state == State_Null )
		{
			if ( item->result == Converter::JobResult_Null )
			{
				item->modelState = QVariant();
			}
			else
			{
				item->modelState = QVariant::fromValue( kFinishedStateTemplate
					.arg( stateName )
					.arg( _nameForJobResult( item->result ) ) );
			}
		}
		else
		{
			item->modelState = QVariant::fromValue( stateName );
		}
		break;

	default:
		Q_ASSERT( false );
	}
}


void JobItemModel::_changeFileItemProgress( FileItem * const fileItem, const qreal progress )
{
	const qreal wasProgress = fileItem->progress();
	fileItem->conversionProgress = progress;

	fileItem->parentItem->childItemChanged( fileItem, 1, wasProgress );

	for ( Item * item = fileItem; item; item = item->parentItem )
	{
		progressChangedItem_ = item;
		emit itemProgressChanged();
		progressChangedItem_ = 0;

		_setItemProgress( item );

		if ( item != rootItem_ )
		{
			const QModelIndex progressColumnIndex = _indexForItem( item, Column_Progress );
			emit dataChanged( progressColumnIndex, progressColumnIndex );
		}
	}
}


void JobItemModel::_changeFileItemState( FileItem * const fileItem, const StateType state )
{
	if ( fileItem->state == state )
		return;

	_setItemStateAndResult( fileItem, state, Converter::JobResult_Null );

	const QModelIndex stateColumnIndex = _indexForItem( fileItem, Column_State );
	emit dataChanged( stateColumnIndex, stateColumnIndex );
}


void JobItemModel::_changeFileItemResult( FileItem * const fileItem, const int result )
{
	if ( fileItem->result == result )
		return;

	_setItemStateAndResult( fileItem, State_Null, result );

	const QModelIndex stateColumnIndex = _indexForItem( fileItem, Column_State );
	emit dataChanged( stateColumnIndex, stateColumnIndex );
}


QString JobItemModel::_oggedFileName( const QString & filePath ) const
{
	static const QString kOggSuffix = QLatin1String( "ogg" );
	const QFileInfo fileInfo = QFileInfo( filePath );
	const QString suffix = fileInfo.suffix();
	return suffix.isEmpty() ?
		filePath + "." + kOggSuffix :
		filePath.left( filePath.length() - suffix.length() ) + kOggSuffix;
}


QString JobItemModel::_evaluateRelativeDestinationPathForFile( const QString & filePath, const QString & basePath ) const
{
	static const QString kRelativePathTestPrefix = QLatin1String( "../" );

	// try to find path relative to source directories
	foreach ( const QDir & sourceDir, sourceDirs_ )
	{
		const QString relativePath = sourceDir.relativeFilePath( filePath );
		if ( !relativePath.startsWith( kRelativePathTestPrefix ) )
			return _oggedFileName( relativePath );
	}

	// try to file path relative to basePath
	{
		const QString relativePath = QDir( basePath ).relativeFilePath( filePath );
		if ( !relativePath.startsWith( kRelativePathTestPrefix ) )
			return _oggedFileName( relativePath );
	}

	// try to find path relative to drives
	foreach ( const QFileInfo & driveFileInfo, QDir::drives() )
	{
		const QString relativePath = driveFileInfo.absoluteDir().relativeFilePath( filePath );
		if ( !relativePath.startsWith( kRelativePathTestPrefix ) )
			return _oggedFileName( relativePath );
	}

	// no relative path found, return file name
	return _oggedFileName( QFileInfo( filePath ).fileName() );
}


JobItemModel::DirItem * JobItemModel::_constructDirItem( const QString & dirName, DirItem * const parentDirItem )
{
	Item * item = parentDirItem->findChildItemByName( dirName );
	if ( item )
	{
		if ( item->type != Item_Dir )
			return 0;
		return item->asDir();
	}

	DirItem * dirItem = new DirItem;
	_setItemName( dirItem, dirName );

	const int dirItemIndex = parentDirItem->childItems.count();
	beginInsertRows( _indexForItem( parentDirItem, 0 ), dirItemIndex, dirItemIndex );
	parentDirItem->addChildItem( dirItem );
	endInsertRows();

	return dirItem;
}


JobItemModel::FileItem * JobItemModel::_constructFileItem( const QString & fileName, DirItem * const parentDirItem,
		const QString & sourcePath, const QString & basePath, const QString & relativeDestinationPath, bool & isAdded )
{
	isAdded = false;

	Item * const existedItem = parentDirItem->findChildItemByName( fileName );
	if ( existedItem )
	{
		if ( existedItem->type == Item_Dir )
			return 0;

		if ( existedItem->asFile()->sourcePath == sourcePath )
			return existedItem->asFile();

		return 0;
	}

	FileItem * fileItem = new FileItem;
	fileItem->basePath = basePath;

	_setItemName( fileItem, fileName );

	const int fileItemIndex = parentDirItem->childItems.count();
	beginInsertRows( _indexForItem( parentDirItem, 0 ), fileItemIndex, fileItemIndex );

	parentDirItem->addChildItem( fileItem );

	fileItem->sourcePath = sourcePath;
	fileItem->relativeDestinationPath = relativeDestinationPath;

	allFileItems_ << fileItem;
	allInactiveFileItems_ << fileItem;
	allUnfinishedFileItems_ << fileItem;
	allInactiveUnfinishedFileItems_ << fileItem;

	endInsertRows();

	isAdded = true;
	return fileItem;
}


void JobItemModel::_cleanupCachedItems( const QList<Item*> & items, const bool emitSignals )
{
	QList<Item*> itemsToRemove = items;

	while ( !itemsToRemove.isEmpty() )
	{
		Item * const item = itemsToRemove.takeFirst();
		if ( item->type == Item_File )
		{
			aboutToRemoveFileItem_ = item->asFile();
			const int wasJobId = item->asFile()->jobId;

			if ( emitSignals )
				emit fileItemAboutToRemove();

			Q_ASSERT( wasJobId == item->asFile()->jobId );
			aboutToRemoveFileItem_ = 0;

			Q_ASSERT( allFileItems().contains( item->asFile() ) );
			allFileItems_.removeOne( item->asFile() );

			if ( item->asFile()->jobId != 0 )
			{
				fileItemForJobId_.remove( item->asFile()->jobId );
				Q_ASSERT( allActiveFileItems().contains( item->asFile() ) );
				allActiveFileItems_.removeOne( item->asFile() );
			}
			else
			{
				Q_ASSERT( allInactiveFileItems().contains( item->asFile() ) );
				allInactiveFileItems_.removeOne( item->asFile() );
			}

			if ( item->asFile()->result == Converter::JobResult_Null )
			{
				Q_ASSERT( allUnfinishedFileItems().contains( item->asFile() ) );
				allUnfinishedFileItems_.removeOne( item->asFile() );

				if ( item->asFile()->jobId == 0 )
				{
					Q_ASSERT( allInactiveUnfinishedFileItems().contains( item->asFile() ) );
					allInactiveUnfinishedFileItems_.removeOne( item->asFile() );
				}
				else
				{
					Q_ASSERT( !allInactiveUnfinishedFileItems().contains( item->asFile() ) );
				}
			}
			else
			{
				Q_ASSERT( !allUnfinishedFileItems().contains( item->asFile() ) );
				Q_ASSERT( !allInactiveUnfinishedFileItems().contains( item->asFile() ) );
			}
		}
		else
		{
			itemsToRemove << item->asDir()->childItems;
		}
	}
}


void JobItemModel::setSourcePaths( const QStringList & paths )
{
	sourceDirs_.clear();

	foreach ( const QString & path, paths )
		sourceDirs_ << QDir( path );
}


QModelIndex JobItemModel::indexForItem( const Item * const item ) const
{
	return _indexForItem( const_cast<Item*>( item ), Column_Name );
}


const JobItemModel::FileItem * JobItemModel::fileItemForJobId( const int jobId ) const
{
	Q_ASSERT( jobId != 0 );

	FileItem * const fileItem = fileItemForJobId_.value( jobId );
	Q_ASSERT( fileItem );

	return fileItem;
}


const JobItemModel::Item * JobItemModel::itemForIndex( const QModelIndex & index ) const
{
	return _itemForIndex( index );
}


/**
  Returns invalid index on failure. This might happen if such item already exist in the tree.
  Returns added index for Column_Name on success.
  */

QModelIndex JobItemModel::addFile( const QString & filePath, const QString & basePath, bool & isAdded )
{
	isAdded = false;

	const QString relativeDestinationFilePath = _evaluateRelativeDestinationPathForFile( filePath, basePath );

	const QFileInfo relativeDestinationFileInfo = QFileInfo( relativeDestinationFilePath );

	QStringList dirChain;
	QString lastDirPath;
	for ( QDir dir = relativeDestinationFileInfo.dir(); ; )
	{
		const QString dirPath = dir.path();
		const QFileInfo pathInfo = QFileInfo( dirPath );
		const QString dirName = pathInfo.fileName();

		if ( dirName == "." || dirName.isEmpty() || dirPath == lastDirPath )
			break;

		dirChain.prepend( dirName );
		lastDirPath = dirPath;

		dir = pathInfo.dir();
	}

	DirItem * currentDirItem = rootItem_;
	foreach ( const QString & dirName, dirChain )
	{
		DirItem * const dirItem = _constructDirItem( dirName, currentDirItem );
		if ( !dirItem )
			return QModelIndex();
		currentDirItem = dirItem;
	}

	bool isFileItemAdded;
	FileItem * const fileItem = _constructFileItem( relativeDestinationFileInfo.fileName(), currentDirItem,
			filePath, basePath, relativeDestinationFilePath, isFileItemAdded );

	if ( !fileItem )
		return QModelIndex();

	isAdded = isFileItemAdded;
	return _indexForItem( fileItem, Column_Name );
}


void JobItemModel::setFileItemJobIdForIndex( const QModelIndex & index, const int jobId )
{
	FileItem * const fileItem = _itemForIndex( index )->asFile();

	if ( fileItem->jobId != 0 )
	{
		Q_ASSERT( jobId == 0 );
		fileItemForJobId_.remove( fileItem->jobId );

		fileItem->jobId = 0;
		_changeFileItemState( fileItem, State_Null );

		Q_ASSERT( allActiveFileItems().contains( fileItem ) );
		allActiveFileItems_.removeOne( fileItem );

		Q_ASSERT( !allInactiveFileItems().contains( fileItem ) );
		allInactiveFileItems_ << fileItem;

		if ( fileItem->result == Converter::JobResult_Null )
		{
			Q_ASSERT( !allInactiveUnfinishedFileItems().contains( fileItem ) );
			allInactiveUnfinishedFileItems_ << fileItem;
		}
	}
	else
	{
		if ( jobId != 0 )
		{
			fileItem->jobId = jobId;
			_changeFileItemState( fileItem, State_Idle );

			Q_ASSERT( !fileItemForJobId_.contains( jobId ) );
			fileItemForJobId_[ jobId ] = fileItem;

			Q_ASSERT( !allActiveFileItems().contains( fileItem ) );
			allActiveFileItems_ << fileItem;

			Q_ASSERT( allInactiveFileItems().contains( fileItem ) );
			allInactiveFileItems_.removeOne( fileItem );

			if ( fileItem->result == Converter::JobResult_Null )
			{
				Q_ASSERT( allInactiveUnfinishedFileItems().contains( fileItem ) );
				allInactiveUnfinishedFileItems_.removeOne( fileItem );
			}
		}
	}
}


void JobItemModel::setFileItemUnmarkedForIndex( const QModelIndex & index )
{
	FileItem * const fileItem = _itemForIndex( index )->asFile();
	Q_ASSERT( fileItem->jobId == 0 );

	_changeFileItemState( fileItem, State_Null );
	_changeFileItemResult( fileItem, Converter::JobResult_Null );
	_changeFileItemProgress( fileItem, 0 );
}


void JobItemModel::setFileItemProgressForIndex( const QModelIndex & index, const qreal progress )
{
	FileItem * const fileItem = _itemForIndex( index )->asFile();
	_changeFileItemProgress( fileItem, progress );
}


void JobItemModel::removeItemByIndex( const QModelIndex & index )
{
	Q_ASSERT( index.isValid() );

	// remove whole branch if it contains only one item
	Item * item = _itemForIndex( index );
	while ( item->parentItem != rootItem_ && item->parentItem->childItems.count() == 1 )
		item = item->parentItem;

	const QModelIndex parentIndex = _indexForItem( item->parentItem, Column_Name );

	beginRemoveRows( parentIndex, item->row, item->row );

	item->parentItem->removeChildItem( item );

	// cleanup cached items and emit signal for file items
	_cleanupCachedItems( QList<Item*>() << item, true );

	// clear items recursively
	if ( item->type == Item_Dir )
		item->asDir()->clearChildItems();

	item->destroy();

	endRemoveRows();
}


void JobItemModel::removeAllItems()
{
	if ( rootItem_->childItems.isEmpty() )
		return;

	beginRemoveRows( QModelIndex(), 0, rootItem_->childItems.count() - 1 );

	_cleanupCachedItems( rootItem_->childItems, true );

	rootItem_->clearChildItems();

	endRemoveRows();
}


void JobItemModel::setJobStarted( const int jobId )
{
	Q_ASSERT( fileItemForJobId_.contains( jobId ) );

	FileItem * const fileItem = fileItemForJobId_.value( jobId );
	Q_ASSERT( fileItem->jobId == jobId );

	_changeFileItemState( fileItem, State_Running );
}


void JobItemModel::setJobFinished( const int jobId, const int result )
{
	Q_ASSERT( fileItemForJobId_.contains( jobId ) );
	FileItem * const fileItem = fileItemForJobId_.take( jobId );
	Q_ASSERT( fileItem->jobId == jobId );

	Q_ASSERT( allActiveFileItems().contains( fileItem ) );
	allActiveFileItems_.removeOne( fileItem );

	Q_ASSERT( !allInactiveFileItems().contains( fileItem ) );
	allInactiveFileItems_ << fileItem;

	if ( fileItem->result == Converter::JobResult_Null )
	{
		Q_ASSERT( !allInactiveUnfinishedFileItems().contains( fileItem ) );
		allInactiveUnfinishedFileItems_ << fileItem;
	}

	fileItem->jobId = 0;
	_changeFileItemState( fileItem, State_Null );
	_changeFileItemResult( fileItem, result );

	if ( fileItem->result == Converter::JobResult_Done )
		_changeFileItemProgress( fileItem, 1.0 );
	else
		_changeFileItemProgress( fileItem, fileItem->conversionProgress );
}


int JobItemModel::rowCount( const QModelIndex & parent ) const
{
	const Item * const item = _itemForIndex( parent );
	return item->type == Item_Dir ? item->asDir()->childItems.count() : 0;
}


int JobItemModel::columnCount( const QModelIndex & parent ) const
{
	const Item * const item = _itemForIndex( parent );
	return Column_TotalColumns;
}


QModelIndex JobItemModel::parent( const QModelIndex & index ) const
{
	const Item * const item = _itemForIndex( index );
	return _indexForItem( item->parentItem, Column_Name );
}


QModelIndex JobItemModel::index( const int row, const int column, const QModelIndex & parent ) const
{
	const Item * const parentItem = _itemForIndex( parent );

	switch ( parentItem->type )
	{
	case Item_File:
		return QModelIndex();

	case Item_Dir:
		if ( row >= 0 && row < parentItem->asDir()->childItems.count() && column >= 0 && column < Column_TotalColumns )
			return _indexForItem( parentItem->asDir()->childItems.at( row ), column );
		return QModelIndex();
	}

	Q_ASSERT( false );
	return QModelIndex();
}


QVariant JobItemModel::data( const QModelIndex & index, const int role ) const
{
	if ( !index.isValid() )
		return QVariant();

	const Item * const item = _itemForIndex( index );

	switch ( index.column() )
	{
	case Column_Name:
		switch ( role )
		{
		case Qt::DisplayRole:
			_updateItemModelName( item );
			return item->modelName;
		}
		return QVariant();

	case Column_Progress:
		switch ( role )
		{
		case Qt::DisplayRole:
			_updateItemModelProgress( item );
			return item->modelProgress;
		}
		return QVariant();

	case Column_State:
		switch ( role )
		{
		case Qt::DisplayRole:
			_updateItemModelState( item );
			return item->modelState;
		}
		return QVariant();
	}

	return QVariant();
}


QVariant JobItemModel::headerData( const int column, const Qt::Orientation orientation, const int role ) const
{
	Q_UNUSED( orientation );

	switch ( role )
	{
	case Qt::DisplayRole:
		return QVariant::fromValue( _nameForColumn( ColumnType(column) ) );
	}

	return QVariant();
}




} // namespace Fogg
