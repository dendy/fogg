
#pragma once

#include <QAbstractItemModel>
#include <QDir>




namespace Fogg {




class JobItemModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum ColumnType
	{
		Column_Name         = 0,
		Column_Progress     = 1,
		Column_State        = 2,
		Column_Format       = 3,
		Column_TotalColumns = 4
	};

	enum ItemType
	{
		Item_Null = 0,
		Item_Dir  = 1,
		Item_File = 2
	};

	enum StateType
	{
		State_Null = 0,
		State_Idle,
		State_Running
	};


	class DirItem;
	class FileItem;

	class Item
	{
	public:
		Item( ItemType type );

		ItemType type;
		DirItem * parentItem;
		int row;

		// data
		QString name;
		StateType state;
		int result;

		// model data
		mutable QVariant modelName;
		mutable QVariant modelProgress;
		mutable QVariant modelState;
		mutable QVariant modelFormat;

	public:
		const DirItem * asDir() const;
		DirItem * asDir();
		const FileItem * asFile() const;
		FileItem * asFile();

		qreal progress() const;

		int weight() const;

		void destroy();
	};


	class DirItem : public Item
	{
	public:
		DirItem();

		qreal totalProgress;

		QList<Item*> childItems;
		int totalWeight;

		void addChildItem( Item * item );
		void removeChildItem( Item * item );
		void childItemChanged( const Item * item, int wasWeight, qreal wasProgress );
		void clearChildItems();
		Item * findChildItemByName( const QString & name ) const;
	};


	class FileItem : public Item
	{
	public:
		FileItem();

		qreal conversionProgress;

		QString sourcePath;
		QString basePath;
		QString format;

		QString relativeDestinationPath;
		QString resolvedFormat;

		int jobId;

	public:
		qreal totalProgress() const;
	};


	static int progressToPercents( qreal progress );

	JobItemModel( QObject * parent = 0 );
	~JobItemModel();

	void setSourcePaths( const QStringList & paths );

	QList<const FileItem*> allFileItems() const;
	QList<const FileItem*> allInactiveFileItems() const;
	QList<const FileItem*> allUnfinishedFileItems() const;
	QList<const FileItem*> allInactiveUnfinishedFileItems() const;
	QList<const FileItem*> allActiveFileItems() const;
	QModelIndex indexForItem( const Item * item ) const;
	const FileItem * fileItemForJobId( int jobId ) const;
	const Item * itemForIndex( const QModelIndex & index ) const;

	QModelIndex addFile( const QString & filePath, const QString & basePath, const QString & format, bool & isAdded );
	void setFileItemJobIdForIndex( const QModelIndex & index, int jobId );
	void setFileItemUnmarkedForIndex( const QModelIndex & index );
	void setFileItemProgressForIndex( const QModelIndex & index, qreal progress );

	void removeItemByIndex( const QModelIndex & index );
	void removeAllItems();

	void setJobStarted( int jobId );
	void setJobResolvedFormat( int jobId, const QString & format );
	void setJobFinished( int jobId, int result );

	const FileItem * aboutToRemoveFileItem() const;
	const Item * progressChangedItem() const;

	// reimplemented from QAbstractItemModel
	int rowCount( const QModelIndex & parent ) const;
	int columnCount( const QModelIndex & parent ) const;
	QModelIndex parent( const QModelIndex & index ) const;
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QVariant data( const QModelIndex & index, int role ) const;
	QVariant headerData( int column, Qt::Orientation orientation, int role ) const;

signals:
	void fileItemAboutToRemove();
	void itemProgressChanged();

protected:
	bool event( QEvent * e );

private:
	static QString _nameForColumn( ColumnType column );
	static QString _nameForStateAndResult( StateType state, int result );
	static QString _nameForJobResult( int result );

	void _retranslate();

	Item * _itemForIndex( const QModelIndex & index ) const;
	QModelIndex _indexForItem( Item * item, int column ) const;

	void _setItemName( Item * item, const QString & name );
	void _setItemProgress( Item * item );
	void _setItemStateAndResult( Item * item, StateType state, int result );
	void _setFileItemResolvedFormat( FileItem * fileItem, const QString & format );

	void _updateItemModelName( const Item * item ) const;
	void _updateItemModelProgress( const Item * item ) const;
	void _updateItemModelState( const Item * item ) const;
	void _updateItemModelFormat( const Item * item ) const;

	void _changeFileItemProgress( FileItem * fileItem, qreal progress );
	void _changeFileItemState( FileItem * fileItem, StateType state );
	void _changeFileItemResult( FileItem * fileItem, int result );
	void _changeFileItemResolvedFormat( FileItem * fileItem, const QString & format );

	QString _oggedFileName( const QString & filePath ) const;
	QString _evaluateRelativeDestinationPathForFile( const QString & filePath, const QString & basePath ) const;
	DirItem * _constructDirItem( const QString & dirName, DirItem * parentDirItem );
	FileItem * _constructFileItem( const QString & fileName, DirItem * parentDirItem,
			const QString & sourcePath, const QString & basePath, const QString & format,
			const QString & relativeDestinationPath, bool & isAdded );
	void _cleanupCachedItems( const QList<Item*> & items, bool emitSignals );

private:
	QList<QDir> sourceDirs_;
	DirItem * rootItem_;
	QList<FileItem*> allFileItems_;
	QList<FileItem*> allInactiveFileItems_;
	QList<FileItem*> allUnfinishedFileItems_;
	QList<FileItem*> allInactiveUnfinishedFileItems_;
	QList<FileItem*> allActiveFileItems_;
	QHash<int,FileItem*> fileItemForJobId_;

	// file item remove helper
	const FileItem * aboutToRemoveFileItem_;

	// item progress helper
	const Item * progressChangedItem_;

	// cached translations
	QString autoFormatTemplate_;
	QString resolvedAutoFormatTemplate_;
};




inline const JobItemModel::DirItem * JobItemModel::Item::asDir() const
{ Q_ASSERT( type == Item_Dir ); return static_cast<const DirItem*>( this ); }

inline JobItemModel::DirItem * JobItemModel::Item::asDir()
{ Q_ASSERT( type == Item_Dir ); return static_cast<DirItem*>( this ); }

inline const JobItemModel::FileItem * JobItemModel::Item::asFile() const
{ Q_ASSERT( type == Item_File ); return static_cast<const FileItem*>( this ); }

inline JobItemModel::FileItem * JobItemModel::Item::asFile()
{ Q_ASSERT( type == Item_File ); return static_cast<FileItem*>( this ); }

inline int JobItemModel::Item::weight() const
{
	switch ( type )
	{
	case Item_Dir:
		return asDir()->totalWeight;
	case Item_File:
		return 1;
	}

	Q_ASSERT( false );
	return 0;
}




inline QList<const JobItemModel::FileItem*> JobItemModel::allFileItems() const
{ return *reinterpret_cast<const QList<const FileItem*>*>( &allFileItems_ ); }

inline QList<const JobItemModel::FileItem*> JobItemModel::allInactiveFileItems() const
{ return *reinterpret_cast<const QList<const FileItem*>*>( &allInactiveFileItems_ ); }

inline QList<const JobItemModel::FileItem*> JobItemModel::allUnfinishedFileItems() const
{ return *reinterpret_cast<const QList<const FileItem*>*>( &allUnfinishedFileItems_ ); }

inline QList<const JobItemModel::FileItem*> JobItemModel::allInactiveUnfinishedFileItems() const
{ return *reinterpret_cast<const QList<const FileItem*>*>( &allInactiveUnfinishedFileItems_ ); }

inline QList<const JobItemModel::FileItem*> JobItemModel::allActiveFileItems() const
{ return *reinterpret_cast<const QList<const FileItem*>*>( &allActiveFileItems_ ); }

inline const JobItemModel::FileItem * JobItemModel::aboutToRemoveFileItem() const
{ return aboutToRemoveFileItem_; }

inline const JobItemModel::Item * JobItemModel::progressChangedItem() const
{ return progressChangedItem_; }




} // namespace Fogg
