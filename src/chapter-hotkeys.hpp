#pragma once

#include "ui_chapter-hotkeys.h"

#include "external/qt-wrappers.hpp"

#include <obs.hpp>
#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <string>
#include <memory>

Q_DECLARE_METATYPE(OBSDataArray);

class ChapterHotkeyUI : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui_HotkeyChaptersDialog> ui;

public:
	ChapterHotkeyUI(QWidget *parent);

	void ShowHideDialog();

	void SaveHotkeys(obs_data_t *data);
	void LoadHotkeys(obs_data_t *data);

private slots:
	void on_actionAddHotkey_triggered();
	void on_aactionRemoveHotkey_triggered();
	void on_actionRenameHotkey_triggered();
};

enum HotkeyDataRoles { Name = Qt::UserRole, HotkeyId, Bindings };

class ChapterHotkeyItem : public QListWidgetItem {

public:
	ChapterHotkeyItem(const QString &id, const char *name,
			  obs_data_array_t *bindings);
	~ChapterHotkeyItem() override;

	QVariant data(int role) const override;
	void setData(int role, const QVariant &value) override;

private:
	obs_hotkey_id hotkey;
	std::string chapterName;
	QString hotkeyUUID;

	static void HotkeyPressed(void *_this, obs_hotkey_id, obs_hotkey_t *,
				  bool pressed);
};

class ChapterNameDialog : public QDialog {
	Q_OBJECT

public:
	ChapterNameDialog(QWidget *parent);

	static bool AskForName(QWidget *parent, const QString &title,
			       const QString &text, std::string &userTextInput,
			       const QString &placeHolder = QString(""));

private:
	QLabel *label;
	QLineEdit *userText;
	QCheckBox *checkbox;
};
