
#include "EncodingQualityWidget.h"




namespace Fogg {




EncodingQualityWidget::EncodingQualityWidget( QWidget * const parent ) :
	QWidget( parent )
{
	selfValueChanging_ = false;

	value_ = kMinimumQualityValue;

	ui_.setupUi( this );

	selfValueChanging_ = true;
	ui_.slider->setMinimum( qRound( kMinimumQualityValue / kQualitySliderStep ) );
	ui_.slider->setMaximum( qRound( kMaximumQualityValue / kQualitySliderStep ) );
	ui_.slider->setValue( _toSliderValue( value_ ) );
	selfValueChanging_ = false;

	_recalculateValueLabelSize();
	_updateValueLabel();
}


void EncodingQualityWidget::setValue( const qreal value )
{
	Q_ASSERT( value >= kMinimumQualityValue && value <= kMaximumQualityValue );

	if ( value == this->value() )
		return;

	value_ = value;

	selfValueChanging_ = true;
	ui_.slider->setValue( _toSliderValue( this->value() ) );
	selfValueChanging_ = false;

	_updateValueLabel();
}


void EncodingQualityWidget::changeEvent( QEvent * const e )
{
	switch ( e->type() )
	{
	case QEvent::FontChange:
		foggDebug();
		_recalculateValueLabelSize();
		break;
	}

	QWidget::changeEvent( e );
}


void EncodingQualityWidget::on_slider_valueChanged()
{
	if ( selfValueChanging_ )
		return;

	value_ = _fromSliderValue( ui_.slider->value() );
	_updateValueLabel();

	emit valueChanged( value() );
}


int EncodingQualityWidget::_toSliderValue( const qreal value ) const
{
	Q_ASSERT( value >= kMinimumQualityValue && value <= kMaximumQualityValue );
	return qRound( value / kQualitySliderStep );
}


qreal EncodingQualityWidget::_fromSliderValue( const int sliderValue ) const
{
	return qBound( kMinimumQualityValue, sliderValue * kQualitySliderStep, kMaximumQualityValue );
}


void EncodingQualityWidget::_recalculateValueLabelSize()
{
	const QFontMetrics fm = QFontMetrics( font() );
	ui_.valueLabel->setMinimumWidth( fm.width( QLatin1String( "00.00" ) ) );
}


void EncodingQualityWidget::_updateValueLabel()
{
	ui_.valueLabel->setText( QString::number( value(), 'f', 2 ) );
}




} // namespace Fogg
