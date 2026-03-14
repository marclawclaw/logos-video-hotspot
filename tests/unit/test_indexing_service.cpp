/**
 * Unit tests for IndexingService — FURPS F: Indexing requirement.
 */

#include <video_hotspot/indexing_service.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace VideoHotspot;

class IndexingServiceTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    /// Clear the shared SQLite "index_db" connection before each test method
    /// so tests don't see data from previous test methods or prior test runs.
    void init()
    {
        // Ensure DB is open (IndexingService opens it lazily)
        IndexingService tmp;   // opens "index_db"
        auto db = QSqlDatabase::database("index_db");
        if (db.isValid() && db.isOpen()) {
            QSqlQuery(db).exec("DELETE FROM videos");
        }
    }

    void test_emptyIndexReturnsZero()
    {
        IndexingService svc;
        QCOMPARE(svc.count(), 0);
    }

    void test_uploadCompleteInsertsRecord()
    {
        IndexingService svc;
        svc.start();

        svc.onUploadComplete(
            "test_cid_001",
            -33.8688, 151.2093,  // Sydney
            QDateTime::currentDateTimeUtc(),
            120, 10 * 1024 * 1024, "video/mp4"
        );

        QVERIFY(svc.count() >= 1);

        const VideoRecord r = svc.findByCid("test_cid_001");
        QCOMPARE(r.cid, QString("test_cid_001"));
        QCOMPARE(r.mimeType, QString("video/mp4"));
        QVERIFY(r.userOwned);
    }

    void test_queryByBounds()
    {
        IndexingService svc;
        svc.start();

        svc.onUploadComplete(
            "test_cid_bounds",
            -33.8688, 151.2093,  // Sydney (in bounds)
            QDateTime::currentDateTimeUtc(),
            30, 5 * 1024 * 1024, "video/mp4"
        );

        VideoFilter filter;
        filter.latMin = -35.0; filter.latMax = -32.0;
        filter.lonMin = 150.0; filter.lonMax = 152.0;

        const auto records = svc.query(filter);
        bool found = false;
        for (const auto& r : records) {
            if (r.cid == "test_cid_bounds") { found = true; break; }
        }
        QVERIFY(found);
    }

    void test_findByCidNotFound()
    {
        IndexingService svc;
        const VideoRecord r = svc.findByCid("nonexistent_cid");
        QVERIFY(r.cid.isEmpty());
    }
};

QTEST_MAIN(IndexingServiceTest)
#include "test_indexing_service.moc"
