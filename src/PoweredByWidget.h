
#pragma once

#include <QWidget>
#include <QUrl>

#include "ui_PoweredByWidget.h"




namespace Fogg {




class PoweredByWidget : public QWidget
{
	Q_OBJECT

public:
	PoweredByWidget( QWidget * parent = 0 );

	QString name() const;
	void setName( const QString & name );

	QString logoFilePath() const;
	void setLogoFilePath( const QString & path );

	QUrl url() const;
	void setUrl( const QUrl & url );

protected:
	void changeEvent( QEvent * e );

private:
	Ui::PoweredByWidget ui_;

	QString name_;
	QString logoFilePath_;
	QUrl url_;
};




inline QString PoweredByWidget::name() const
{ return name_; }

inline QString PoweredByWidget::logoFilePath() const
{ return logoFilePath_; }

inline QUrl PoweredByWidget::url() const
{ return url_; }




} // namespace Fogg
