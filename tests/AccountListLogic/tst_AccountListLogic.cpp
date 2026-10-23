#include "AccountListLogic.h"

#include <QtTest>

namespace
{
AccountStorage storage(long long used, long long total, qint64 updatedSecs = 0)
{
    AccountStorage account;
    account.used = used;
    account.total = total;
    account.updatedSecs = updatedSecs;
    return account;
}
}

class TestAccountListLogic: public QObject
{
    Q_OBJECT

private slots:
    void indexOfIgnoresCase()
    {
        const QStringList emails{QStringLiteral("a@x.com"), QStringLiteral("B@x.com")};
        QCOMPARE(AccountListLogic::indexOf(emails, QStringLiteral("b@X.com")), 1);
        QCOMPARE(AccountListLogic::indexOf(emails, QStringLiteral("c@x.com")), -1);
        QCOMPARE(AccountListLogic::indexOf(emails, QString()), -1);
    }

    void insertAfterEarlierFileNeighbor()
    {
        QStringList stored{QStringLiteral("a"), QStringLiteral("z")};
        const QStringList file{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
        AccountListLogic::insertByFileNeighbors(stored, file, QStringLiteral("b"));
        QCOMPARE(stored, (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("z")}));
    }

    void insertBeforeLaterFileNeighbor()
    {
        QStringList stored{QStringLiteral("x"), QStringLiteral("c")};
        const QStringList file{QStringLiteral("b"), QStringLiteral("c")};
        AccountListLogic::insertByFileNeighbors(stored, file, QStringLiteral("b"));
        QCOMPARE(stored, (QStringList{QStringLiteral("x"), QStringLiteral("b"), QStringLiteral("c")}));
    }

    void insertWithoutNeighborAppends()
    {
        QStringList stored{QStringLiteral("x")};
        const QStringList file{QStringLiteral("b")};
        AccountListLogic::insertByFileNeighbors(stored, file, QStringLiteral("b"));
        QCOMPARE(stored, (QStringList{QStringLiteral("x"), QStringLiteral("b")}));
    }

    void menuWindow_data()
    {
        QTest::addColumn<int>("total");
        QTest::addColumn<int>("active");
        QTest::addColumn<int>("start");
        QTest::addColumn<int>("count");

        QTest::newRow("short list") << 4 << 2 << 0 << 4;
        QTest::newRow("no active") << 30 << -1 << 0 << 10;
        QTest::newRow("active first") << 30 << 0 << 0 << 10;
        QTest::newRow("active middle") << 30 << 15 << 10 << 10;
        QTest::newRow("active last") << 30 << 29 << 20 << 10;
        QTest::newRow("active near end") << 30 << 27 << 20 << 10;
        QTest::newRow("exactly limit") << 10 << 9 << 0 << 10;
        QTest::newRow("empty") << 0 << -1 << 0 << 0;
    }

    void menuWindow()
    {
        QFETCH(int, total);
        QFETCH(int, active);
        QFETCH(int, start);
        QFETCH(int, count);

        const auto window = AccountListLogic::menuWindow(total, active, 10);
        QCOMPARE(window.start, start);
        QCOMPARE(window.count, count);
        if (active >= 0)
        {
            QVERIFY(active >= window.start && active < window.start + window.count);
        }
    }

    void sortByFreeSpaceKeepsUnknownLast()
    {
        const QStringList emails{QStringLiteral("unknown1"),
                                 QStringLiteral("small"),
                                 QStringLiteral("unknown2"),
                                 QStringLiteral("big")};
        AccountStorageMap map;
        map.insert(QStringLiteral("small"), storage(19, 20));
        map.insert(QStringLiteral("big"), storage(5, 20));
        QCOMPARE(AccountListLogic::sortedByFreeSpace(emails, map),
                 (QStringList{QStringLiteral("big"),
                              QStringLiteral("small"),
                              QStringLiteral("unknown1"),
                              QStringLiteral("unknown2")}));
    }

    void nextWithFreeSpaceWrapsAndSkipsCurrent()
    {
        const QStringList emails{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
        AccountStorageMap map;
        map.insert(QStringLiteral("a"), storage(0, 100));
        map.insert(QStringLiteral("b"), storage(0, 100));
        map.insert(QStringLiteral("c"), storage(95, 100));

        QCOMPARE(AccountListLogic::nextWithFreeSpace(emails, QStringLiteral("b"), map, 10),
                 QStringLiteral("a"));
        QCOMPARE(AccountListLogic::nextWithFreeSpace(emails, QStringLiteral("a"), map, 10),
                 QStringLiteral("b"));
        QCOMPARE(AccountListLogic::nextWithFreeSpace(emails, QString(), map, 10),
                 QStringLiteral("a"));
    }

    void nextWithFreeSpaceFindsNothing()
    {
        const QStringList emails{QStringLiteral("a"), QStringLiteral("b")};
        AccountStorageMap map;
        map.insert(QStringLiteral("a"), storage(0, 100));
        map.insert(QStringLiteral("b"), storage(99, 100));
        QVERIFY(AccountListLogic::nextWithFreeSpace(emails, QStringLiteral("a"), map, 10).isEmpty());
        QVERIFY(AccountListLogic::nextWithFreeSpace(QStringList(), QString(), map, 10).isEmpty());
    }

    void freeSpaceNeverNegative()
    {
        QCOMPARE(storage(30, 20).freeSpace(), 0LL);
    }

    void snapshotAge()
    {
        const qint64 now = 1000000;
        QVERIFY(AccountListLogic::snapshotAge(0, now).isEmpty());
        QVERIFY(AccountListLogic::snapshotAge(now - 59 * 60, now).isEmpty());
        QCOMPARE(AccountListLogic::snapshotAge(now - 5 * 3600, now), QStringLiteral("5 h ago"));
        QCOMPARE(AccountListLogic::snapshotAge(now - 3 * 86400, now), QStringLiteral("3 d ago"));
    }
};

QTEST_GUILESS_MAIN(TestAccountListLogic)
#include "tst_AccountListLogic.moc"
