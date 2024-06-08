#include "chapter-hotkeys.hpp"

#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QMenu>
#include <QUuid>

using namespace std;

ChapterHotkeyUI *hk_edit;

ChapterHotkeyUI::ChapterHotkeyUI(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui_HotkeyChaptersDialog)
{
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	ui->listWidget->setSortingEnabled(true);
}

void ChapterHotkeyUI::ShowHideDialog()
{
	if (!isVisible()) {
		setVisible(true);
	} else {
		close();
	}
}

void ChapterHotkeyUI::on_actionAddHotkey_triggered()
{
	string name;
	bool accepted = ChapterNameDialog::AskForName(
		this, obs_module_text("ChapterHotkey.AddDialog.Title"),
		obs_module_text("ChapterHotkey.AddDialog.Text"), name);
	if (!accepted || name.empty())
		return;

	/* Generate ID */
	auto uuid = QUuid::createUuid();
	QString id = "chapter_hotkey_" + uuid.toString(QUuid::WithoutBraces);

	auto item = new ChapterHotkeyItem(id, name.c_str(), nullptr);
	ui->listWidget->addItem(item);
	ui->listWidget->sortItems();
}

void ChapterHotkeyUI::on_aactionRemoveHotkey_triggered()
{
	auto item = ui->listWidget->currentItem();
	ui->listWidget->removeItemWidget(item);
	ui->listWidget->sortItems();
}

void ChapterHotkeyUI::on_actionRenameHotkey_triggered()
{
	auto item = ui->listWidget->currentItem();
	Qt::ItemFlags flags = item->flags();

	item->setFlags(flags | Qt::ItemIsEditable);
	ui->listWidget->editItem(item);
	item->setFlags(flags);
	ui->listWidget->sortItems();
}

void ChapterHotkeyUI::LoadHotkeys(obs_data_t *data)
{
	ui->listWidget->clear();

	obs_data_item *item = obs_data_first(data);

	while (item) {
		const char *id = obs_data_item_get_name(item);
		OBSDataAutoRelease hk = obs_data_item_get_obj(item);

		const char *name = obs_data_get_string(hk, "name");
		OBSDataArrayAutoRelease bindings =
			obs_data_get_array(hk, "bindings");

		auto hkItem = new ChapterHotkeyItem(id, name, bindings);
		ui->listWidget->addItem(hkItem);

		obs_data_item_next(&item);
	}

	ui->listWidget->sortItems();
}

void ChapterHotkeyUI::SaveHotkeys(obs_data_t *data)
{
	obs_data_clear(data);

	for (int i = 0; i < ui->listWidget->count(); i++) {
		auto item = ui->listWidget->item(i);

		auto name = item->data(Name).toString();
		auto uuid = item->data(HotkeyId).toString();
		OBSDataArrayAutoRelease bindings =
			static_cast<obs_data_array_t *>(
				item->data(Bindings).value<void *>());

		OBSDataAutoRelease hk = obs_data_create();
		obs_data_set_string(hk, "name", name.toUtf8().constData());
		obs_data_set_array(hk, "bindings", bindings);
		obs_data_set_obj(data, uuid.toUtf8().constData(), hk);
	}
}

/*
 * Hotkey item
 */

ChapterHotkeyItem::ChapterHotkeyItem(const QString &id, const char *name,
				     obs_data_array_t *bindings)
	: QListWidgetItem(nullptr),
	  chapterName(name),
	  hotkeyUUID(id)
{
	setText(name);

	QString translationName = obs_module_text("ChapterHotkey.Name");
	QString formattedName = translationName.arg(name);

	hotkey = obs_hotkey_register_frontend(
		id.toUtf8().constData(), formattedName.toUtf8().constData(),
		ChapterHotkeyItem::HotkeyPressed, this);

	if (bindings)
		obs_hotkey_load(hotkey, bindings);
}

ChapterHotkeyItem::~ChapterHotkeyItem()
{
	obs_hotkey_unregister(hotkey);
}

void ChapterHotkeyItem::HotkeyPressed(void *_this, obs_hotkey_id,
				      obs_hotkey_t *, bool pressed)
{
	auto hk = static_cast<ChapterHotkeyItem *>(_this);

	if (pressed)
		obs_frontend_recording_add_chapter(hk->chapterName.c_str());
}

QVariant ChapterHotkeyItem::data(int role) const
{
	if (role == Name)
		return QString::fromStdString(chapterName);
	if (role == HotkeyId)
		return hotkeyUUID;
	if (role == Bindings) {
		obs_data_array_t *hk = obs_hotkey_save(hotkey);
		return QVariant::fromValue(static_cast<void *>(hk));
	}

	return QListWidgetItem::data(role);
}

void ChapterHotkeyItem::setData(int role, const QVariant &value)
{
	if (role == Name || role == Qt::EditRole) {
		QString newName = value.toString();
		QString translationName = obs_module_text("ChapterHotkey.Name");
		QString formattedName = translationName.arg(newName);

		chapterName = newName.toStdString();
		obs_hotkey_set_description(hotkey,
					   formattedName.toUtf8().constData());

		setText(newName);
	} else {
		QListWidgetItem::setData(role, value);
	}
}

/*
 * Name Dialog from main UI
 */

ChapterNameDialog::ChapterNameDialog(QWidget *parent) : QDialog(parent)
{
	setModal(true);
	setWindowModality(Qt::WindowModality::WindowModal);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setFixedWidth(555);
	setMinimumHeight(100);
	QVBoxLayout *layout = new QVBoxLayout;
	setLayout(layout);

	label = new QLabel(this);
	layout->addWidget(label);
	label->setText("Set Text");

	userText = new QLineEdit(this);
	layout->addWidget(userText);

	checkbox = new QCheckBox(this);
	layout->addWidget(checkbox);

	QDialogButtonBox *buttonbox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addWidget(buttonbox);
	buttonbox->setCenterButtons(true);
	connect(buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

static bool IsWhitespace(char ch)
{
	return ch == ' ' || ch == '\t';
}

static void CleanWhitespace(std::string &str)
{
	while (str.size() && IsWhitespace(str.back()))
		str.erase(str.end() - 1);
	while (str.size() && IsWhitespace(str.front()))
		str.erase(str.begin());
}

bool ChapterNameDialog ::AskForName(QWidget *parent, const QString &title,
				    const QString &text,
				    std::string &userTextInput,
				    const QString &placeHolder)
{
	ChapterNameDialog dialog(parent);
	dialog.setWindowTitle(title);

	dialog.checkbox->setHidden(true);
	dialog.label->setText(text);
	dialog.userText->setText(placeHolder);
	dialog.userText->selectAll();

	if (dialog.exec() != DialogCode::Accepted) {
		return false;
	}

	userTextInput = dialog.userText->text().toUtf8().constData();
	CleanWhitespace(userTextInput);
	return true;
}

/*
 * Frontend Event Handlers
 */

static void LoadSaveHotkeys(obs_data_t *save_data, bool saving, void *)
{
	if (saving) {
		OBSDataAutoRelease obj = obs_data_create();
		hk_edit->SaveHotkeys(obj);
		obs_data_set_obj(save_data, "chapter_hotkeys", obj);
	} else {
		OBSDataAutoRelease obj =
			obs_data_get_obj(save_data, "chapter_hotkeys");
		if (obj)
			hk_edit->LoadHotkeys(obj);
	}
}

/*
 * C stuff
 */

extern "C" void InitChapterHotkeys()
{
	auto action =
		static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(
			obs_module_text("ChapterHotkeys")));
	auto window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());

	/* Push translation function so that strings in .ui file are translated */
	obs_frontend_push_ui_translation(obs_module_get_string);
	hk_edit = new ChapterHotkeyUI(window);
	obs_frontend_pop_ui_translation();

	obs_frontend_add_save_callback(LoadSaveHotkeys, nullptr);

	QAction::connect(action, &QAction::triggered, hk_edit,
			 &ChapterHotkeyUI::ShowHideDialog);
}
