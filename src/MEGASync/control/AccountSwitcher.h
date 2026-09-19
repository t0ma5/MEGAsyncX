#ifndef ACCOUNT_SWITCHER_H
#define ACCOUNT_SWITCHER_H

#include <QString>

class MegaApplication;
class QMenu;

class AccountSwitcher
{
public:
    static constexpr int kMaxAccounts = 5;

    static void persistCurrentSession(MegaApplication* app);
    static void switchTo(MegaApplication* app, const QString& email);
    static void addAccount(MegaApplication* app);
    static void forget(MegaApplication* app, const QString& email);
    static QString trayStatusLine();
    static void handleDeadSession(MegaApplication* app, const QString& email);
};

#endif
