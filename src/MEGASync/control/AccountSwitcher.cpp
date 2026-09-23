#include "AccountSwitcher.h"

#include "LoginController.h"
#include "MegaApplication.h"
#include "Preferences.h"
#include "megaapi.h"

#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>

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
                                 AccountSwitcher::tr("Accounts"),
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

    if (!preferences->hasSessionForAccount(email) &&
        preferences->accountPassword(email).isEmpty())
    {
        QMessageBox::warning(
            nullptr,
            tr("Accounts"),
            tr("No saved session or password for %1. Log in again with Add account.").arg(email));
        return;
    }

    const QString prompt =
        current.isEmpty() ?
            tr("Switch to %1?").arg(email) :
            tr("Switch to %1? %2 will stop syncing until you switch back.").arg(email, current);
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
                                 tr("Accounts"),
                                 tr("Maximum of %1 accounts. Remove one first.").arg(kMaxAccounts));
        return;
    }

    if (!preferences->logged() && (!app->getMegaApi() || !app->getMegaApi()->isLoggedIn()))
    {
        return;
    }

    const QString current = preferences->email();
    if (!confirmAction(
            tr("Sign in to another account? Sync for %1 will pause. You can switch back later.")
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
                                 tr("Accounts"),
                                 tr("Log out to remove the active account."));
        return;
    }

    preferences->forgetSavedAccount(email);
}

QString AccountSwitcher::trayTooltipHeader()
{
    auto preferences = Preferences::instance();
    const QString current = preferences->logged() ? preferences->email() : tr("Not signed in");

    QString header = QString::fromUtf8("MEGAsyncX ");
    header += Preferences::reportedVersionString();
    header += QLatin1Char('\n');
    header += tr("%1 (%2/%3)")
                  .arg(current)
                  .arg(preferences->savedAccountEmails().size())
                  .arg(kMaxAccounts);
    return header;
}

void AccountSwitcher::importFromFile(QWidget* parent)
{
    const QString path =
        QFileDialog::getOpenFileName(parent,
                                     tr("Import accounts"),
                                     QString(),
                                     tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(parent, tr("Accounts"), tr("Could not open %1.").arg(path));
        return;
    }

    auto preferences = Preferences::instance();
    QStringList known = preferences->savedAccountEmails();
    int added = 0;
    int updated = 0;
    int malformed = 0;
    int overflow = 0;

    QTextStream stream(&file);
    while (!stream.atEnd())
    {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
        {
            continue;
        }

        // Split on the first colon only: passwords may contain colons.
        const int separator = line.indexOf(QLatin1Char(':'));
        if (separator <= 0 || separator == line.size() - 1)
        {
            ++malformed;
            continue;
        }

        const QString email = line.left(separator).trimmed();
        const QString password = line.mid(separator + 1);
        if (!email.contains(QLatin1Char('@')))
        {
            ++malformed;
            continue;
        }

        bool isKnown = false;
        for (const QString& storedEmail: known)
        {
            if (emailsEqual(storedEmail, email))
            {
                isKnown = true;
                break;
            }
        }

        if (!isKnown && known.size() >= kMaxAccounts)
        {
            ++overflow;
            continue;
        }

        preferences->setAccountPassword(email, password);
        if (isKnown)
        {
            ++updated;
        }
        else
        {
            preferences->rememberSavedAccount(email);
            known.append(email);
            ++added;
        }
    }

    QString report = tr("Added %1, updated %2.").arg(added).arg(updated);
    if (malformed > 0)
    {
        report += QLatin1Char('\n');
        report += tr("Skipped %1 line(s) that were not email:password.").arg(malformed);
    }
    if (overflow > 0)
    {
        report += QLatin1Char('\n');
        report +=
            tr("Skipped %1 account(s): the limit of %2 was reached.").arg(overflow).arg(kMaxAccounts);
    }
    QMessageBox::information(parent, tr("Accounts"), report);
}

void AccountSwitcher::exportToFile(QWidget* parent)
{
    auto preferences = Preferences::instance();
    const QStringList emails = preferences->savedAccountEmails();

    int withPassword = 0;
    for (const QString& email: emails)
    {
        if (!preferences->accountPassword(email).isEmpty())
        {
            ++withPassword;
        }
    }

    if (withPassword == 0)
    {
        QMessageBox::information(parent,
                                 tr("Accounts"),
                                 tr("No stored passwords to export. Only accounts added through "
                                    "Import from File have one."));
        return;
    }

    const auto answer = QMessageBox::warning(
        parent,
        tr("Accounts"),
        tr("Write %1 password(s) to a plain text file?\n\n"
           "The file is NOT encrypted. Anyone who reads it gets full access to those MEGA "
           "accounts. Store it somewhere safe or delete it when you are done.")
            .arg(withPassword),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    const QString path =
        QFileDialog::getSaveFileName(parent,
                                     tr("Export accounts"),
                                     QString::fromLatin1("megasyncx-accounts.txt"),
                                     tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QMessageBox::warning(parent, tr("Accounts"), tr("Could not write %1.").arg(path));
        return;
    }

    QTextStream stream(&file);
    int written = 0;
    for (const QString& email: emails)
    {
        const QString password = preferences->accountPassword(email);
        if (password.isEmpty())
        {
            continue;
        }
        QString line = email;
        line += QLatin1Char(':');
        line += password;
        line += QLatin1Char('\n');
        stream << line;
        ++written;
    }
    file.close();

    QString report = tr("Exported %1 account(s) to %2.").arg(written).arg(path);
    if (written < emails.size())
    {
        report += QLatin1Char('\n');
        report += tr("Skipped %1 account(s) with no stored password.").arg(emails.size() - written);
    }
    QMessageBox::information(parent, tr("Accounts"), report);
}

void AccountSwitcher::resumePendingLogin(MegaApplication* app)
{
    if (!app)
    {
        return;
    }

    auto preferences = Preferences::instance();
    const QString email = preferences->takePendingLoginEmail();
    if (email.isEmpty())
    {
        return;
    }

    auto* login = app->getLoginController();
    if (!login)
    {
        return;
    }

    login->prefillEmail(email);

    const QString password = preferences->accountPassword(email);
    if (password.isEmpty())
    {
        return;
    }

    QTimer::singleShot(0,
                       app,
                       [login, email, password]()
                       {
                           login->login(email, password);
                       });
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
