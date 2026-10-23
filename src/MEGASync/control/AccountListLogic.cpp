#include "AccountListLogic.h"

#include <QCoreApplication>

#include <algorithm>

namespace AccountListLogic
{
int indexOf(const QStringList& emails, const QString& email)
{
    for (int i = 0; i < emails.size(); ++i)
    {
        if (QString::compare(emails.at(i), email, Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }
    return -1;
}

void insertByFileNeighbors(QStringList& stored, const QStringList& fileOrder, const QString& email)
{
    const int fileIndex = indexOf(fileOrder, email);
    if (fileIndex < 0)
    {
        stored.append(email);
        return;
    }

    for (int i = fileIndex - 1; i >= 0; --i)
    {
        const int at = indexOf(stored, fileOrder.at(i));
        if (at >= 0)
        {
            stored.insert(at + 1, email);
            return;
        }
    }
    for (int i = fileIndex + 1; i < fileOrder.size(); ++i)
    {
        const int at = indexOf(stored, fileOrder.at(i));
        if (at >= 0)
        {
            stored.insert(at, email);
            return;
        }
    }
    stored.append(email);
}

MenuWindow menuWindow(int total, int activeIndex, int limit)
{
    MenuWindow window;
    window.count = std::min(total, limit);
    if (activeIndex >= 0 && activeIndex < total && total > limit)
    {
        const int side = limit / 2;
        int before = std::min(side, activeIndex);
        const int after = std::min(limit - 1 - before, total - activeIndex - 1);
        if (before + after + 1 < limit)
        {
            before = std::min(activeIndex, limit - 1 - after);
        }
        window.start = activeIndex - before;
        window.count = before + 1 + after;
    }
    return window;
}

QStringList sortedByFreeSpace(const QStringList& emails, const AccountStorageMap& storage)
{
    QStringList sorted = emails;
    std::stable_sort(sorted.begin(),
                     sorted.end(),
                     [&storage](const QString& left, const QString& right)
                     {
                         const auto leftFound = storage.constFind(left);
                         const auto rightFound = storage.constFind(right);
                         const bool leftKnown = leftFound != storage.constEnd();
                         const bool rightKnown = rightFound != storage.constEnd();
                         if (leftKnown != rightKnown)
                         {
                             return leftKnown;
                         }
                         if (!leftKnown)
                         {
                             return false;
                         }
                         return leftFound->freeSpace() > rightFound->freeSpace();
                     });
    return sorted;
}

QString nextWithFreeSpace(const QStringList& emails,
                          const QString& current,
                          const AccountStorageMap& storage,
                          long long minimumFree)
{
    const int total = emails.size();
    const int start = indexOf(emails, current);
    // With no current account, start from the top of the list. Otherwise skip current itself.
    const int steps = start >= 0 ? total - 1 : total;
    for (int step = 1; step <= steps; ++step)
    {
        const QString& email = emails.at((start + step) % total);
        const auto found = storage.constFind(email);
        if (found != storage.constEnd() && found->freeSpace() >= minimumFree)
        {
            return email;
        }
    }
    return QString();
}

QString snapshotAge(qint64 updatedSecs, qint64 nowSecs)
{
    constexpr qint64 hour = 60 * 60;
    constexpr qint64 day = 24 * hour;
    const qint64 age = nowSecs - updatedSecs;
    if (updatedSecs <= 0 || age < hour)
    {
        return QString();
    }
    if (age < day)
    {
        return QCoreApplication::translate("AccountSwitcher", "%1 h ago").arg(age / hour);
    }
    return QCoreApplication::translate("AccountSwitcher", "%1 d ago").arg(age / day);
}
}
