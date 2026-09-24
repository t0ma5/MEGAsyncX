#include "AccountSwitcher.h"

#include "LoginController.h"
#include "MegaApplication.h"
#include "Preferences.h"
#include "TokenParserWidgetManager.h"
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

void styleAccountDialog(QMessageBox& box)
{
    auto* tokens = TokenParserWidgetManager::instance().get();
    const auto css = [tokens](const char* name)
    {
        return tokens->getColor(QLatin1String(name)).name(QColor::HexArgb);
    };
    const QString page = css("page-background");
    const QString text = css("text-primary");
    const QString button = css("button-secondary");
    const QString hover = css("button-secondary-hover");
    const QString pressed = css("button-secondary-pressed");
    box.setTextFormat(Qt::PlainText);
    box.setStyleSheet(QStringLiteral(
        "QMessageBox { background-color: %1; }"
        "QLabel { color: %2; background-color: transparent; font-family: 'Segoe UI'; font-size: 13px; }"
        "QPushButton { color: %2; background-color: %3; border: none; border-radius: 6px; "
        "min-height: 26px; padding: 0 16px; font-weight: 500; }"
        "QPushButton:hover { background-color: %4; }"
        "QPushButton:pressed { background-color: %5; }")
                           .arg(page, text, button, hover, pressed));
}

bool confirmAction(const QString& text)
{
    QMessageBox box(QMessageBox::Question,
                    AccountSwitcher::tr("Accounts"),
                    text,
                    QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    styleAccountDialog(box);
    return box.exec() == QMessageBox::Yes;
}

void showAccountsNotice(const QString& text, bool problem)
{
    const QString body = text.isEmpty() ?
                             AccountSwitcher::tr("Import finished, but there is no detail to show.") :
                             text;
    QMessageBox box(problem ? QMessageBox::Warning : QMessageBox::Information,
                    AccountSwitcher::tr("Accounts"),
                    body,
                    QMessageBox::Ok);
    styleAccountDialog(box);
    box.exec();
}
}

bool AccountSwitcher::confirm(const QString& text)
{
    return confirmAction(text);
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
        showAccountsNotice(
            tr("No saved session or password for %1. Log in again with Add account.").arg(email),
            true);
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
        showAccountsNotice(tr("Maximum of %1 accounts. Remove one first.").arg(kMaxAccounts), false);
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
        showAccountsNotice(tr("Log out to remove the active account."), false);
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
        Q_UNUSED(parent);
        showAccountsNotice(tr("Could not open %1.").arg(path), true);
        return;
    }

    auto preferences = Preferences::instance();
    QStringList known = preferences->savedAccountEmails();
    int added = 0;
    int updated = 0;
    int syntaxErrors = 0;
    int missingPassword = 0;
    int overflow = 0;
    QStringList details;

    auto addDetail = [&details](const QString& line)
    {
        if (details.size() < 5)
        {
            details.append(line);
        }
    };

    QTextStream stream(&file);
    int lineNumber = 0;
    while (!stream.atEnd())
    {
        ++lineNumber;
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
        {
            continue;
        }

        // Split on the first colon only: passwords may contain colons.
        const int separator = line.indexOf(QLatin1Char(':'));
        if (separator <= 0)
        {
            ++syntaxErrors;
            addDetail(tr("Line %1: expected email:password.").arg(lineNumber));
            continue;
        }

        const QString email = line.left(separator).trimmed();
        const QString password = line.mid(separator + 1);
        if (password.trimmed().isEmpty())
        {
            ++missingPassword;
            addDetail(tr("Line %1: %2 has no password.").arg(lineNumber).arg(email));
            continue;
        }
        if (!email.contains(QLatin1Char('@')))
        {
            ++syntaxErrors;
            addDetail(tr("Line %1: \"%2\" is not an email.").arg(lineNumber).arg(email));
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
            addDetail(tr("Line %1: %2 skipped, account limit reached.").arg(lineNumber).arg(email));
            continue;
        }

        preferences->setAccountPassword(email, password);
        if (isKnown)
        {
            ++updated;
            addDetail(tr("%1 is already in the list. Stored password was replaced.").arg(email));
        }
        else
        {
            preferences->rememberSavedAccount(email);
            known.append(email);
            ++added;
        }
    }

    QString report = tr("Added %1. Password updated for %2 already in the list.")
                         .arg(added)
                         .arg(updated);
    if (syntaxErrors > 0)
    {
        report += QLatin1Char('\n');
        report += tr("Skipped %1 line(s) that were not email:password.").arg(syntaxErrors);
    }
    if (missingPassword > 0)
    {
        report += QLatin1Char('\n');
        report += tr("Skipped %1 line(s) with a missing password.").arg(missingPassword);
    }
    if (overflow > 0)
    {
        report += QLatin1Char('\n');
        report +=
            tr("Skipped %1 account(s): the limit of %2 was reached.").arg(overflow).arg(kMaxAccounts);
    }
    if (!details.isEmpty())
    {
        report += QLatin1Char('\n');
        report += details.join(QLatin1Char('\n'));
    }
    const bool problem = syntaxErrors > 0 || missingPassword > 0 || overflow > 0;
    showAccountsNotice(report, problem);
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
        showAccountsNotice(tr("No stored passwords to export. Only accounts added through "
                              "Import from File have one."),
                           false);
        return;
    }

    if (!confirmAction(tr("Write %1 password(s) to a plain text file?\n\n"
                          "The file is NOT encrypted. Anyone who reads it gets full access to those MEGA "
                          "accounts. Store it somewhere safe or delete it when you are done.")
                           .arg(withPassword)))
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
        showAccountsNotice(tr("Could not write %1.").arg(path), true);
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
    showAccountsNotice(report, false);
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
