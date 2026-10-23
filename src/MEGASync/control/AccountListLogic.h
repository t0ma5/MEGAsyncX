#ifndef ACCOUNT_LIST_LOGIC_H
#define ACCOUNT_LIST_LOGIC_H

#include <QHash>
#include <QString>
#include <QStringList>

// Last known storage of one saved account, in bytes. updatedSecs is 0 when the time is unknown.
struct AccountStorage
{
    long long used = 0;
    long long total = 0;
    qint64 updatedSecs = 0;

    long long freeSpace() const
    {
        return total > used ? total - used : 0;
    }
};

using AccountStorageMap = QHash<QString, AccountStorage>;

// Pure list logic for the account switcher. QtCore only, so the unit tests can build it alone.
namespace AccountListLogic
{
// Case-insensitive position of email in emails, or -1.
int indexOf(const QStringList& emails, const QString& email);

// Place a new email beside the closest account from the same file that is already stored.
// An earlier file neighbor wins. Otherwise the account goes just before the next stored
// neighbor. With no neighbor, it goes at the end.
void insertByFileNeighbors(QStringList& stored, const QStringList& fileOrder, const QString& email);

struct MenuWindow
{
    int start = 0;
    int count = 0;
};

// Slice of at most limit accounts with the active one in the middle. Each side starts at up to
// limit / 2. A side that hits the start or end gives its unused slots to the other side.
MenuWindow menuWindow(int total, int activeIndex, int limit);

// Most free space first. Accounts without a snapshot go last, in their original order.
QStringList sortedByFreeSpace(const QStringList& emails, const AccountStorageMap& storage);

// The first account after current, wrapping round the list, with at least minimumFree bytes free
// in its last snapshot. Empty when there is none.
QString nextWithFreeSpace(const QStringList& emails,
                          const QString& current,
                          const AccountStorageMap& storage,
                          long long minimumFree);

// "5 h ago" or "3 d ago". Empty when the time is unknown or under an hour old.
QString snapshotAge(qint64 updatedSecs, qint64 nowSecs);
}

#endif
