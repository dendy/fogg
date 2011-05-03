
#pragma once

#include <QString>




namespace Grim {
namespace Audio {




class VorbisComment
{
public:
	VorbisComment( const char * data );

	bool isValid() const;

	QString key() const;
	QString value() const;

private:
	QString key_;
	QString value_;
};




inline VorbisComment::VorbisComment( const char * const data )
{
	const char * const equalPosPointer = strchr( data, '=' );
	if ( !equalPosPointer || equalPosPointer == data )
	{
		// no equal sign or key length is 0
		return;
	}

	const int equalPos = equalPosPointer - data;

	key_ = QString::fromLatin1( data, equalPos );
	value_ = QString::fromUtf8( data + equalPos + 1 );
}


inline bool VorbisComment::isValid() const
{ return !key_.isNull(); }

inline QString VorbisComment::key() const
{ return key_; }

inline QString VorbisComment::value() const
{ return value_; }




} // namespace Audio
} // namespace Grim
