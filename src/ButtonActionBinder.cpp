
#include "ButtonActionBinder.h"

#include <QAction>
#include <QAbstractButton>




namespace Fogg {




ButtonActionBinder::ButtonActionBinder( QAction * const action, QAbstractButton * const button, QObject * const parent ) :
	QObject( parent ),
	action_( action ),
	button_( button )
{
	Q_ASSERT( action_ );
	Q_ASSERT( button_ );

	connect( action_, SIGNAL(destroyed()), SLOT(_actionDestroyed()) );
	connect( button_, SIGNAL(destroyed()), SLOT(_buttonDestroyed()) );

	connect( action_, SIGNAL(changed()), SLOT(_actionChanged()) );

	connect( button_, SIGNAL(clicked()), action_, SLOT(trigger()) );

	_updateButton();
}


void ButtonActionBinder::_actionChanged()
{
	Q_ASSERT( button_ );
	_updateButton();
}


void ButtonActionBinder::_actionDestroyed()
{
	_unbind();
	action_ = 0;
}


void ButtonActionBinder::_buttonDestroyed()
{
	_unbind();
	button_ = 0;
}


void ButtonActionBinder::_updateButton()
{
	if ( action_->text() != actionText_ )
	{
		actionText_ = action_->text();

		// remove ampersand (&) from action text, since it useless in buttons
		QString buttonText = actionText_;
		buttonText.remove( QLatin1Char( '&' ) );
		button_->setText( buttonText );
	}

	button_->setCheckable( action_->isCheckable() );
	button_->setChecked( action_->isChecked() );
	button_->setEnabled( action_->isEnabled() );
	button_->setIcon( action_->icon() );
	button_->setVisible( action_->isVisible() );
}


void ButtonActionBinder::_unbind()
{
	if ( action_ )
		action_->disconnect( this );

	if ( button_ )
		button_->disconnect( this );

	if ( action_ && button_ )
		button_->disconnect( action_ );
}




} // namespace Fogg
