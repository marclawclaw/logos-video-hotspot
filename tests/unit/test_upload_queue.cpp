/**
 * Unit tests for UploadQueue — FURPS F: Upload Queue requirements.
 */

#include <video_hotspot/upload_queue.h>
#include <video_hotspot/deduplicator.h>
#include <video_hotspot/storage_client.h>

#include <QEventLoop>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace VideoHotspot;

class UploadQueueTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmpDir;

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_tmpDir.isValid());
    }

    void test_enqueueValidFile()
    {
        const QString path = m_tmpDir.path() + "/video.mp4";
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("fake video content for queue test");
        f.close();

        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        const QString id = queue.enqueue(path);
        QVERIFY(!id.isEmpty());
    }

    void test_enqueueMissingFileReturnsEmpty()
    {
        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        const QString id = queue.enqueue("/nonexistent/video.mp4");
        QVERIFY(id.isEmpty());
    }

    void test_enqueueAndWaitForCompletion()
    {
        const QString path = m_tmpDir.path() + "/complete_test.mp4";
        const QByteArray content = "unique content " + QUuid::createUuid().toByteArray();
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content);
        f.close();

        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        QEventLoop loop;
        UploadItem lastItem;

        connect(&queue, &UploadQueue::itemChanged, &loop,
                [&](const UploadItem& item) {
                    if (item.status == UploadStatus::Complete ||
                        item.status == UploadStatus::Failed ||
                        item.status == UploadStatus::Duplicate) {
                        lastItem = item;
                        loop.quit();
                    }
                });

        QTimer::singleShot(10000, &loop, &QEventLoop::quit);

        const QString id = queue.enqueue(path);
        QVERIFY(!id.isEmpty());

        loop.exec();

        QVERIFY(lastItem.status == UploadStatus::Complete ||
                lastItem.status == UploadStatus::Duplicate);
        QVERIFY(!lastItem.cid.isEmpty());
    }

    void test_duplicateFileIsDetected()
    {
        const QString path = m_tmpDir.path() + "/dup_test.mp4";
        const QByteArray content = "duplicate detection test " + QUuid::createUuid().toByteArray();
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content);
        f.close();

        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        // First upload
        {
            QEventLoop loop;
            connect(&queue, &UploadQueue::itemChanged, &loop,
                    [&](const UploadItem& item) {
                        if (item.status == UploadStatus::Complete) loop.quit();
                    });
            QTimer::singleShot(10000, &loop, &QEventLoop::quit);
            queue.enqueue(path);
            loop.exec();
        }

        // Second upload — should be a duplicate
        {
            QEventLoop loop;
            UploadItem result;
            connect(&queue, &UploadQueue::itemChanged, &loop,
                    [&](const UploadItem& item) {
                        if (item.status == UploadStatus::Complete ||
                            item.status == UploadStatus::Duplicate ||
                            item.status == UploadStatus::Failed) {
                            result = item;
                            loop.quit();
                        }
                    });
            QTimer::singleShot(10000, &loop, &QEventLoop::quit);
            queue.enqueue(path);
            loop.exec();

            QCOMPARE(result.status, UploadStatus::Duplicate);
            QVERIFY(!result.existingCid.isEmpty());
        }
    }

    void test_enqueueFolder()
    {
        QDir dir(m_tmpDir.path() + "/folder_test");
        dir.mkpath(".");

        for (int i = 0; i < 3; ++i) {
            QFile f(dir.path() + QString("/video%1.mp4").arg(i));
            f.open(QIODevice::WriteOnly);
            f.write(QString("folder video %1 content").arg(i).toUtf8());
            f.close();
        }

        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        const QStringList ids = queue.enqueueFolder(dir.path());
        QCOMPARE(ids.size(), 3);
    }
};

QTEST_MAIN(UploadQueueTest)
#include "test_upload_queue.moc"
