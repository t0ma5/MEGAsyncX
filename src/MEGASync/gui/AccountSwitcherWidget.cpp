#include "AccountSwitcherWidget.h"

#include "AccountSwitcher.h"
#include "MegaApplication.h"
#include "Preferences.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

AccountSwitcherWidget::AccountSwitcherWidget(MegaApplication* app, QWidget* parent):
    QWidget(parent),
    mApp(app),
    mList(new QListWidget(this)),
    mSwitchButton(new QPushButton(tr("Switch"), this)),
    mAddButton(new QPushButton(tr("Add account"), this)),
    mRemoveButton(new QPushButton(tr("Remove"), this))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 12);
    layout->addWidget(new QLabel(tr("Accounts (one active, max 5)"), this));
    layout->addWidget(mList);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(mSwitchButton);
    buttons->addWidget(mAddButton);
    buttons->addWidget(mRemoveButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(mSwitchButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onSwitch);
    connect(mAddButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onAdd);
    connect(mRemoveButton, &QPushButton::clicked, this, &AccountSwitcherWidget::onRemove);

    refresh();
}

void AccountSwitcherWidget::refresh()
{
    mList->clear();
    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : QString();
    const auto emails = preferences->savedAccountEmails();
    for (const QString& email: emails)
    {
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
