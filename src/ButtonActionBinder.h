
#pragma once

#include <QObject>




class QAction;
class QAbstractButton;




namespace Fogg {




class ButtonActionBinder : public QObject
{
	Q_OBJECT

public:
	ButtonActionBinder( QAction * action, QAbstractButton * button, QObject * parent = 0 );

private slots:
	void _actionChanged();
	void _actionDestroyed();
	void _buttonDestroyed();

private:
	void _updateButton();
	void _unbind();

private:
	QAction * action_;
	QAbstractButton * button_;

	// cache
	QString actionText_;
};




} // namespace Fogg
