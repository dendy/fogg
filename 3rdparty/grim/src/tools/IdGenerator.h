
#pragma once

#include <grim/tools/Global.h>

#include <QSharedDataPointer>




namespace Grim {
namespace Tools {




class IdGeneratorPrivate;
class IdGeneratorIteratorPrivate;




class GRIM_TOOLS_EXPORT IdGenerator
{
public:
	IdGenerator( int limit = -1 );
	IdGenerator( const IdGenerator & generator );
	IdGenerator & operator=( const IdGenerator & generator );
	~IdGenerator();

	int limit() const;

	int take();
	void free( int id );
	void reserve( int id );
	bool isFree( int id ) const;

	bool isEmpty() const;
	int count() const;

private:
	QSharedDataPointer<IdGeneratorPrivate> d_;

	friend class IdGeneratorIterator;
};




class GRIM_TOOLS_EXPORT IdGeneratorIterator
{
public:
	IdGeneratorIterator( const IdGenerator & idGenerator );
	~IdGeneratorIterator();

	bool hasNext() const;
	int next();

private:
	IdGeneratorIteratorPrivate * d_;
};




} // namespace Tools
} // namespace Grim
