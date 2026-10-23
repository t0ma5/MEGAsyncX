#ifndef ACCOUNT_SWITCHER_H
#define ACCOUNT_SWITCHER_H

#include "AccountListLogic.h"
#include "Preferences.h"

#include <QCoreApplication>
#include <QString>
#include <QStringList>

class MegaApplication;
class QWidget;

class AccountSwitcher
{
    Q_DECLARE_TR_FUNCTIONS(AccountSwitcher)

public:
    static constexpr int kMaxAccounts = Preferences::MAX_SAVED_ACCOUNTS;
    static constexpr int kMenuAccountLimit = 10;
    // "Next account with free space" skips accounts with less than this left.
    static constexpr long long kMinimumFreeForNext = 1024LL * 1024 * 1024;

    static void persistCurrentSession(MegaApplication* app);
    static void switchTo(MegaApplication* app, const QString& email);
    static void addAccount(MegaApplication* app);
    static void switchToNextWithFreeSpace(MegaApplication* app);
    static void forget(MegaApplication* app, const QString& email);
    // Removes every listed account except the active one and returns how many went.
    static int forgetMany(MegaApplication* app, const QStringList& emails);
    static bool confirm(const QString& text);
    // Two lines: "MEGAsyncX <version>" then "<email> (<used>/<max>)".
    static QString trayTooltipHeader();
    static void handleDeadSession(MegaApplication* app, const QString& email);
    // "<email> (<used>/<total>)", with ", 3 d ago" once the snapshot is an hour old, or
    // "<email> (? GB)" before the account has reported storage.
    static QString accountLabel(const QString& email, const AccountStorageMap& storage);
    // Rebuilds the tray menus and tooltip after the saved list changes.
    static void refreshTray();

    // email:password, one per line. Credentials are written and read in clear text.
    static void importFromFile(QWidget* parent);
    static void exportToFile(QWidget* parent);

    // Called on startup after a switch to an account that had no stored session.
    static void resumePendingLogin(MegaApplication* app);
};

#endif
