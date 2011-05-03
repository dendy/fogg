
#pragma once

#include <QWidget>

#include "Global.h"

#include "ui_EncodingQualityWidget.h"




namespace Fogg {




// use Vorbis quality with specified interval
static const qreal kQualitySliderStep = 0.05;




class EncodingQualityWidget : public QWidget
{
	Q_OBJECT

public:
	EncodingQualityWidget( QWidget * parent = 0 );

	qreal value() const;
	void setValue( qreal value );

signals:
	void valueChanged( qreal value );

protected:
	void changeEvent( QEvent * e );

private slots:
	void on_slider_valueChanged();

private:
	int _toSliderValue( qreal value ) const;
	qreal _fromSliderValue( int sliderValue ) const;
	void _recalculateValueLabelSize();
	void _updateValueLabel();

private:
	Ui::EncodingQualityWidget ui_;

	qreal value_;

	// helper
	bool selfValueChanging_;
};




inline qreal EncodingQualityWidget::value() const
{ return value_; }




} // namespace Fogg
