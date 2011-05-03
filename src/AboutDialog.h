
#pragma once

#include <QDialog>

#include "ui_AboutDialog.h"




class QStringListModel;

namespace Grim {
namespace Audio {
	class FormatManager;
}
}




namespace Fogg {




class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	AboutDialog( Grim::Audio::FormatManager * audioFormatManager, QWidget * parent = 0 );

protected:
	void changeEvent( QEvent * e );

private:
	void _retranslateUi();

private:
	Ui::AboutDialog ui_;

	Grim::Audio::FormatManager * audioFormatManager_;

	QStringListModel * detailsItemModel_;
};




} // namespace Fogg
