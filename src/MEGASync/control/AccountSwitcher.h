#ifndef ACCOUNT_SWITCHER_H
#define ACCOUNT_SWITCHER_H

#include "Preferences.h"

#include <QCoreApplication>
#include <QString>

class MegaApplication;
class QWidget;

class AccountSwitcher
{
    Q_DECLARE_TR_FUNCTIONS(AccountSwitcher)

public:
    static constexpr int kMaxAccounts = Preferences::MAX_SAVED_ACCOUNTS;
    static constexpr int kMenuAccountLimit = 10;

    static void persistCurrentSession(MegaApplication* app);
    static void switchTo(MegaApplication* app, const QString& email);
    static void addAccount(MegaApplication* app);
    static void forget(MegaApplication* app, const QString& email);
    static bool confirm(const QString& text);
    // Two lines: "MEGAsyncX <version>" then "<email> (<used>/<max>)".
    static QString trayTooltipHeader();
    static void handleDeadSession(MegaApplication* app, const QString& email);

    // email:password, one per line. Credentials are written and read in clear text.
    static void importFromFile(QWidget* parent);
    static void exportToFile(QWidget* parent);

    // Called on startup after a switch to an account that had no stored session.
    static void resumePendingLogin(MegaApplication* app);
};

#endif
