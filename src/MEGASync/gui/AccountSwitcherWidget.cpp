#include "AccountSwitcherWidget.h"

#include "AccountListLogic.h"
#include "AccountSwitcher.h"
#include "MegaApplication.h"
#include "Preferences.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

namespace
{
void styleLikeMyAccount(QPushButton* button)
{
    button->setProperty("type", QStringLiteral("secondary"));
    button->setProperty("dimension", QStringLiteral("small"));
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoDefault(false);
    button->setFocusPolicy(Qt::StrongFocus);
}
}

AccountSwitcherWidget::AccountSwitcherWidget(MegaApplication* app, QWidget* parent):
    QWidget(parent),
    mApp(app),
    mHeading(new QLabel(this)),
    mFilter(new QLineEdit(this)),
    mList(new QListWidget(this)),
    mSwitchButton(new QPushButton(tr("Switch"), this)),
    mAddButton(new QPushButton(tr("Add account"), this)),
    mRemoveButton(new QPushButton(tr("Remove"), this)),
    mRemoveAllButton(new QPushButton(tr("Remove all except active"), this)),
    mImportButton(new QPushButton(tr("Import from File"), this)),
    mExportButton(new QPushButton(tr("Export"), this)),
    mForgetAllButton(new QPushButton(tr("Forget all"), this)),
    mConfirmSwitch(new QCheckBox(tr("Ask before switching accounts"), this)),
    mOpenFiles(new QCheckBox(tr("Open Files after switching accounts"), this)),
    mSortByFreeSpace(new QCheckBox(tr("Most free space first"), this))
{
    styleLikeMyAccount(mSwitchButton);
    styleLikeMyAccount(mAddButton);
    styleLikeMyAccount(mRemoveButton);
    styleLikeMyAccount(mRemoveAllButton);
    styleLikeMyAccount(mImportButton);
    styleLikeMyAccount(mExportButton);
    styleLikeMyAccount(mForgetAllButton);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 12);
    layout->addWidget(mHeading);

    auto* filterRow = new QHBoxLayout();
    mFilter->setPlaceholderText(tr("Filter accounts"));
    mFilter->setClearButtonEnabled(true);
    filterRow->addWidget(mFilter);
    filterRow->addWidget(mSortByFreeSpace);
    layout->addLayout(filterRow);
    mList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mList->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(mList);

    auto* listButtons = new QHBoxLayout();
    listButtons->addWidget(mSwitchButton);
    listButtons->addWidget(mAddButton);
    listButtons->addWidget(mRemoveButton);
    listButtons->addWidget(mRemoveAllButton);
    listButtons->addStretch();
    layout->addLayout(listButtons);

    auto* fileButtons = new QHBoxLayout();
    fileButtons->addWidget(mImportButton);
    fileButtons->addWidget(mExportButton);
    fileButtons->addWidget(mForgetAllButton);
    fileButtons->addStretch();
    layout->addLayout(fileButtons);

    auto preferences = Preferences::instance();
    mConfirmSwitch->setChecked(preferences->confirmAccountSwitch());
    mOpenFiles->setChecked(preferences->openFilesAfterSwitch());
    mSortByFreeSpace->setChecked(preferences->sortAccountsByFreeSpace());
    mSortByFreeSpace->setToolTip(
        tr("Sort by the last known free space. Accounts showing (? GB) go last."));
    layout->addWidget(mConfirmSwitch);
    layout->addWidget(mOpenFiles);

    mRemoveButton->setToolTip(
        tr("Remove the selected accounts. Ctrl+click or Shift+click selects several. The Delete "
           "key does the same."));
    mList->setToolTip(tr("Double-click an account to switch to it. Right-click for more."));
    mImportButton->setToolTip(
        tr("Read a plain text file of email:password lines, one account per line."));
    mExportButton->setToolTip(
        tr("Write stored passwords to a plain text file. The file is not encrypted."));
    mForgetAllButton->setToolTip(
        tr("Delete every stored session and password except the active account. Emails stay in "
           "the list and ask for a password on the next switch."));

    connect(mFilter, &QLineEdit::textChanged, this, &AccountSwitcherWidget::refresh);
    connect(mList,
            &QListWidget::itemSelectionChanged,
            this,
            &AccountSwitcherWidget::updateButtons);
    connect(mList,
            &QListWidget::itemDoubleClicked,
            this,
            &AccountSwitcherWidget::onItemDoubleClicked);
    connect(mList,
            &QListWidget::customContextMenuRequested,
            this,
            &AccountSwitcherWidget::onContextMenu);
    auto* deleteShortcut = new QShortcut(QKeySequence::Delete, mList);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, &AccountSwitcherWidget::onRemove);
    connect(preferences.get(),
            &Preferences::savedAccountsChanged,
            this,
            &AccountSwitcherWidget::refresh);
    connect(mSwitchButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onSwitch);
    connect(mAddButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onAdd);
    connect(mRemoveButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onRemove);
    connect(mRemoveAllButton,
            &QPushButton::clicked,
            this,
            &AccountSwitcherWidget::onRemoveAllInactive);
    connect(mImportButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onImport);
    connect(mExportButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onExport);
    connect(mForgetAllButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onForgetAll);
    connect(mConfirmSwitch,
            &QCheckBox::toggled,
            this,
            [](bool checked)
            {
                Preferences::instance()->setConfirmAccountSwitch(checked);
            });
    connect(mOpenFiles,
            &QCheckBox::toggled,
            this,
            [](bool checked)
            {
                Preferences::instance()->setOpenFilesAfterSwitch(checked);
            });
    connect(mSortByFreeSpace,
            &QCheckBox::toggled,
            this,
            [this](bool checked)
            {
                Preferences::instance()->setSortAccountsByFreeSpace(checked);
                refresh();
            });

    refresh();
}

void AccountSwitcherWidget::refresh()
{
    mList->clear();
    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : QString();
    const QString filter = mFilter->text().trimmed();
    const auto emails = preferences->savedAccountEmails();
    QStringList shown;
    for (const QString& email: emails)
    {
        if (filter.isEmpty() || email.contains(filter, Qt::CaseInsensitive))
        {
            shown.append(email);
        }
    }

    const auto storage = preferences->savedAccountStorage(shown);
    if (mSortByFreeSpace->isChecked())
    {
        shown = AccountListLogic::sortedByFreeSpace(shown, storage);
    }
    for (const QString& email: shown)
    {
        auto* item = new QListWidgetItem(AccountSwitcher::accountLabel(email, storage), mList);
        if (QString::compare(email, current, Qt::CaseInsensitive) == 0)
        {
            item->setText(tr("%1 - active").arg(item->text()));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        item->setData(Qt::UserRole, email);
    }
    mHeading->setText(tr("Accounts: %1 of %2, one active at a time")
                          .arg(emails.size())
                          .arg(AccountSwitcher::kMaxAccounts));
    mAddButton->setEnabled(emails.size() < AccountSwitcher::kMaxAccounts);
    mRemoveAllButton->setEnabled(emails.size() > (current.isEmpty() ? 0 : 1));
    updateButtons();
}

void AccountSwitcherWidget::updateButtons()
{
    const QStringList selected = selectedEmails();
    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : QString();
    mSwitchButton->setEnabled(selected.size() == 1 &&
                              QString::compare(selected.first(), current, Qt::CaseInsensitive) != 0);
    mRemoveButton->setEnabled(!selected.isEmpty());
}

QStringList AccountSwitcherWidget::selectedEmails() const
{
    QStringList emails;
    for (const QListWidgetItem* item: mList->selectedItems())
    {
        emails.append(item->data(Qt::UserRole).toString());
    }
    return emails;
}

void AccountSwitcherWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    if (item)
    {
        AccountSwitcher::switchTo(mApp, item->data(Qt::UserRole).toString());
    }
}

void AccountSwitcherWidget::onContextMenu(const QPoint& position)
{
    QListWidgetItem* item = mList->itemAt(position);
    if (!item)
    {
        return;
    }
    if (!item->isSelected())
    {
        mList->clearSelection();
        item->setSelected(true);
    }

    const QString email = item->data(Qt::UserRole).toString();
    QMenu menu(this);
    QAction* switchAction = menu.addAction(tr("Switch"), this, &AccountSwitcherWidget::onSwitch);
    switchAction->setEnabled(mSwitchButton->isEnabled());
    menu.addAction(tr("Copy email"),
                   this,
                   [email]()
                   {
                       QApplication::clipboard()->setText(email);
                   });
    menu.addSeparator();
    menu.addAction(tr("Remove"), this, &AccountSwitcherWidget::onRemove);
    menu.exec(mList->viewport()->mapToGlobal(position));
}

QString AccountSwitcherWidget::selectedEmail() const
{
    const QStringList selected = selectedEmails();
    return selected.size() == 1 ? selected.first() : QString();
}

void AccountSwitcherWidget::onSwitch()
{
    const QString email = selectedEmail();
    if (!email.isEmpty())
    {
        AccountSwitcher::switchTo(mApp, email);
    }
}

void AccountSwitcherWidget::onAdd()
{
    AccountSwitcher::addAccount(mApp);
}

void AccountSwitcherWidget::onRemove()
{
    const QStringList emails = selectedEmails();
    if (emails.isEmpty())
    {
        return;
    }
    if (emails.size() == 1)
    {
        // Single removal keeps the old behaviour, including the notice for the active account.
        AccountSwitcher::forget(mApp, emails.first());
        refresh();
        return;
    }
    removeAccounts(emails);
}

void AccountSwitcherWidget::onRemoveAllInactive()
{
    removeAccounts(Preferences::instance()->savedAccountEmails());
}

void AccountSwitcherWidget::removeAccounts(const QStringList& emails)
{
    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : QString();
    int count = 0;
    for (const QString& email: emails)
    {
        if (QString::compare(email, current, Qt::CaseInsensitive) != 0)
        {
            ++count;
        }
    }
    if (count == 0)
    {
        return;
    }

    if (!AccountSwitcher::confirm(
            tr("Remove %1 account(s)? Their stored sessions and passwords are deleted. The active "
               "account stays.")
                .arg(count)))
    {
        return;
    }

    AccountSwitcher::forgetMany(mApp, emails);
    refresh();
}

void AccountSwitcherWidget::onImport()
{
    AccountSwitcher::importFromFile(this);
    refresh();
}

void AccountSwitcherWidget::onExport()
{
    AccountSwitcher::exportToFile(this);
}

void AccountSwitcherWidget::onForgetAll()
{
    if (!AccountSwitcher::confirm(
            tr("Delete every stored session and password except the active account? Each account will "
               "ask for a password on the next switch.")))
    {
        return;
    }

    Preferences::instance()->forgetAllSavedSessions();
    refresh();
}
