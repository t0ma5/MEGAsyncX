#include "AccountSwitcherWidget.h"

#include "AccountSwitcher.h"
#include "MegaApplication.h"
#include "Preferences.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
void styleLikeMyAccount(QPushButton* button)
{
    button->setProperty(QStringLiteral("type"), QStringLiteral("secondary"));
    button->setProperty(QStringLiteral("dimension"), QStringLiteral("small"));
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoDefault(false);
    button->setFocusPolicy(Qt::StrongFocus);
}
}

AccountSwitcherWidget::AccountSwitcherWidget(MegaApplication* app, QWidget* parent):
    QWidget(parent),
    mApp(app),
    mFilter(new QLineEdit(this)),
    mList(new QListWidget(this)),
    mSwitchButton(new QPushButton(tr("Switch"), this)),
    mAddButton(new QPushButton(tr("Add account"), this)),
    mRemoveButton(new QPushButton(tr("Remove"), this)),
    mImportButton(new QPushButton(tr("Import from File"), this)),
    mExportButton(new QPushButton(tr("Export"), this)),
    mForgetAllButton(new QPushButton(tr("Forget all"), this))
{
    styleLikeMyAccount(mSwitchButton);
    styleLikeMyAccount(mAddButton);
    styleLikeMyAccount(mRemoveButton);
    styleLikeMyAccount(mImportButton);
    styleLikeMyAccount(mExportButton);
    styleLikeMyAccount(mForgetAllButton);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 12);
    layout->addWidget(
        new QLabel(tr("Accounts (one active, max %1)").arg(AccountSwitcher::kMaxAccounts), this));

    mFilter->setPlaceholderText(tr("Filter accounts"));
    mFilter->setClearButtonEnabled(true);
    layout->addWidget(mFilter);
    layout->addWidget(mList);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(mSwitchButton);
    buttons->addWidget(mAddButton);
    buttons->addWidget(mRemoveButton);
    buttons->addWidget(mImportButton);
    buttons->addWidget(mExportButton);
    buttons->addWidget(mForgetAllButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    mImportButton->setToolTip(
        tr("Read a plain text file of email:password lines, one account per line."));
    mExportButton->setToolTip(
        tr("Write stored passwords to a plain text file. The file is not encrypted."));
    mForgetAllButton->setToolTip(
        tr("Delete every stored session and password except the active account. Emails stay in "
           "the list and ask for a password on the next switch."));

    connect(mFilter, &QLineEdit::textChanged, this, &AccountSwitcherWidget::refresh);
    connect(mSwitchButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onSwitch);
    connect(mAddButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onAdd);
    connect(mRemoveButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onRemove);
    connect(mImportButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onImport);
    connect(mExportButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onExport);
    connect(mForgetAllButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onForgetAll);

    refresh();
}

void AccountSwitcherWidget::refresh()
{
    mList->clear();
    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : QString();
    const QString filter = mFilter->text().trimmed();
    const auto emails = preferences->savedAccountEmails();
    for (const QString& email: emails)
    {
        if (!filter.isEmpty() && !email.contains(filter, Qt::CaseInsensitive))
        {
            continue;
        }

        auto* item = new QListWidgetItem(email, mList);
        if (email == current)
        {
            item->setText(email + QString::fromUtf8(" (active)"));
            item->setSelected(true);
        }
        item->setData(Qt::UserRole, email);
    }
    mAddButton->setEnabled(emails.size() < AccountSwitcher::kMaxAccounts);
}

QString AccountSwitcherWidget::selectedEmail() const
{
    auto* item = mList->currentItem();
    if (!item)
    {
        return QString();
    }
    return item->data(Qt::UserRole).toString();
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
    const QString email = selectedEmail();
    if (!email.isEmpty())
    {
        AccountSwitcher::forget(mApp, email);
        refresh();
    }
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
