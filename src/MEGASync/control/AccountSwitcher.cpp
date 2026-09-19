#include "AccountSwitcher.h"

#include "LoginController.h"
#include "MegaApplication.h"
#include "Preferences.h"
#include "megaapi.h"

#include <QMessageBox>

#include <memory>

namespace
{
bool emailsEqual(const QString& left, const QString& right)
{
    return QString::compare(left, right, Qt::CaseInsensitive) == 0;
}

bool confirmAction(const QString& text)
{
    return QMessageBox::question(nullptr,
                                 QString::fromUtf8("Accounts"),
                                 text,
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes;
}
}

void AccountSwitcher::persistCurrentSession(MegaApplication* app)
{
    if (!app)
    {
        return;
    }

    mega::MegaApi* api = app->getMegaApi();
    if (!api || !api->isLoggedIn())
    {
        return;
    }

    std::unique_ptr<char[]> session(api->dumpSession());
    if (!session)
    {
        return;
    }

    auto preferences = Preferences::instance();
    preferences->setSession(QString::fromUtf8(session.get()));
    if (preferences->logged())
    {
        preferences->rememberSavedAccount(preferences->email());
    }
}

void AccountSwitcher::switchTo(MegaApplication* app, const QString& email)
{
    if (!app || email.isEmpty())
    {
        return;
    }

    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : QString();
    if (!current.isEmpty() && emailsEqual(current, email))
    {
        return;
    }

    if (!preferences->hasSessionForAccount(email))
    {
        QMessageBox::warning(nullptr,
                             QString::fromUtf8("Accounts"),
                             QString::fromUtf8(
                                 "No saved session for %1. Log in again with Add account.")
                                 .arg(email));
        return;
    }

    const QString prompt =
        current.isEmpty() ?
            QString::fromUtf8("Switch to %1?").arg(email) :
            QString::fromUtf8("Switch to %1? %2 will stop syncing until you switch back.")
                .arg(email, current);
    if (!confirmAction(prompt))
    {
        return;
    }

    persistCurrentSession(app);
    preferences->prepareSwitchToAccount(email);
    app->rebootApplication(false);
}

void AccountSwitcher::addAccount(MegaApplication* app)
{
    if (!app)
    {
        return;
    }

    auto preferences = Preferences::instance();
    if (preferences->savedAccountEmails().size() >= kMaxAccounts)
    {
        QMessageBox::information(nullptr,
                                 QString::fromUtf8("Accounts"),
                                 QString::fromUtf8("Maximum of 5 accounts. Remove one first."));
        return;
    }

    if (!preferences->logged() && (!app->getMegaApi() || !app->getMegaApi()->isLoggedIn()))
    {
        return;
    }

    const QString current = preferences->email();
    if (!confirmAction(
            QString::fromUtf8(
                "Sign in to another account? Sync for %1 will pause. You can switch back later.")
                .arg(current)))
    {
        return;
    }

    persistCurrentSession(app);
    preferences->prepareAddAccount();
    app->rebootApplication(false);
}

void AccountSwitcher::forget(MegaApplication* app, const QString& email)
{
    Q_UNUSED(app);
    if (email.isEmpty())
    {
        return;
    }

    auto preferences = Preferences::instance();
    if (preferences->logged() && emailsEqual(preferences->email(), email))
    {
        QMessageBox::information(nullptr,
                                 QString::fromUtf8("Accounts"),
                                 QString::fromUtf8("Log out to remove the active account."));
        return;
    }

    preferences->forgetSavedAccount(email);
}

QString AccountSwitcher::trayStatusLine()
{
    auto preferences = Preferences::instance();
    const QStringList emails = preferences->savedAccountEmails();
    const QString current = preferences->logged() ? preferences->email() : QString();
    const int used = emails.size();
    QString line = QString::fromUtf8("MEGAsyncX");
    if (!current.isEmpty())
    {
        line += QString::fromUtf8(" — %1").arg(current);
    }
    line += QString::fromUtf8(" (%1 of %2)").arg(used).arg(kMaxAccounts);
    return line;
}

void AccountSwitcher::handleDeadSession(MegaApplication* app, const QString& email)
{
    if (!app)
    {
        return;
    }

    auto preferences = Preferences::instance();
    const QString keepEmail = email.isEmpty() && preferences->logged() ? preferences->email() : email;
    if (!keepEmail.isEmpty())
    {
        preferences->rememberSavedAccount(keepEmail);
    }
    preferences->prepareAddAccount();
    app->unlink(true);
    if (auto* login = app->getLoginController())
    {
        login->prefillEmail(keepEmail);
    }
}
