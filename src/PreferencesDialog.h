
#pragma once

#include <QDialog>
#include <QAbstractListModel>

#include "Global.h"

#include "ui_PreferencesDialog.h"




namespace Grim {
namespace Tools {
	class LocalizationManager;
}
}




namespace Fogg {




class Config;




class SourcePathItemModel : public QAbstractListModel
{
	Q_OBJECT

public:
	SourcePathItemModel( Config * config, QObject * parent = 0 );

	// reimplemented from QAbstractItemModel
	int rowCount( const QModelIndex & parent ) const;
	QVariant data( const QModelIndex & index, int role ) const;
	QVariant headerData( int column, Qt::Orientation orientation, int role ) const;

private:
	Config * config_;

	friend class PreferencesDialog;
};




class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	PreferencesDialog( Config * config, Grim::Tools::LocalizationManager * localizationManager, QWidget * parent = 0 );

	bool hasSourcePathChanged() const;

	// reimplemented from QDialog
	void done( int result );

protected:
	void changeEvent( QEvent * e );
	void showEvent( QShowEvent * e );

private:
	void _retranslateUi();

private slots:
	void on_languageComboBox_activated( int index );
	void on_concurrentThreadCountSpinBox_valueChanged();
	void on_defaultEncodingQualityWidget_valueChanged();
	void on_addSourcePathButton_clicked();
	void on_removeSourcePathButton_clicked();

	void _sourcePathViewSelectionChanged();

private:
	Ui_PreferencesDialog ui_;

	Config * config_;
	Grim::Tools::LocalizationManager * localizationManager_;

	SourcePathItemModel * sourcePathItemModel_;
	QString lastUsedSourceDirPath_;

	bool sourcePathHasChanged_;

	// language combo box helper
	bool selfChangingLanguage_;
};




inline bool PreferencesDialog::hasSourcePathChanged() const
{ return sourcePathHasChanged_; }




} // namespace Fogg
