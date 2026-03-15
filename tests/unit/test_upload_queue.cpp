/**
 * Unit tests for UploadQueue — FURPS F: Upload Queue requirements.
 *
 * These tests verify pipeline behaviour (status transitions, dedup detection,
 * folder enqueue) without a live Logos daemon.
 *
 * When built with LOGOS_CORE_AVAILABLE (SDK present), StorageClient requires
 * initLogos() to be called; uploads without a real daemon will fail with
 * UploadStatus::Failed. Tests that verify pipeline termination accept both
 * Complete and Failed as valid terminal states.
 *
 * The deduplication test relies on the Deduplicator recording the hash of the
 * first upload. Since the Deduplicator records the hash at the start of the
 * upload pipeline (before the SDK call), duplicate detection still works even
 * when the underlying upload fails — see UploadQueue::enqueue() step 1.
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

    // ── Queue API ────────────────────────────────────────────────────────────

    void test_enqueueValidFile()
    {
        const QString path = m_tmpDir.path() + "/video.mp4";
        QFile f(path); f.open(QIODevice::WriteOnly);
        f.write("fake video content for queue test"); f.close();

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

        QVERIFY(queue.enqueue("/nonexistent/video.mp4").isEmpty());
    }

    // ── Pipeline termination ─────────────────────────────────────────────────
    // Verify the queue always reaches a terminal state (Complete, Failed, or
    // Duplicate) — never gets stuck. Does not assert on success/failure outcome
    // since that depends on whether a Logos daemon is running.

    void test_pipelineAlwaysTerminates()
    {
        const QString path = m_tmpDir.path() + "/pipeline_test.mp4";
        const QByteArray content = "unique content " + QUuid::createUuid().toByteArray();
        QFile f(path); f.open(QIODevice::WriteOnly);
        f.write(content); f.close();

        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        QEventLoop loop;
        UploadItem lastItem;
        bool terminated = false;

        connect(&queue, &UploadQueue::itemChanged, &loop,
                [&](const UploadItem& item) {
                    if (item.status == UploadStatus::Complete ||
                        item.status == UploadStatus::Failed  ||
                        item.status == UploadStatus::Duplicate) {
                        lastItem = item;
                        terminated = true;
                        loop.quit();
                    }
                });

        QTimer::singleShot(10000, &loop, &QEventLoop::quit);  // 10 s safety net
        QVERIFY(!queue.enqueue(path).isEmpty());
        loop.exec();

        QVERIFY2(terminated, "Pipeline timed out — never reached a terminal state");
        qDebug() << "Pipeline terminal status:" << static_cast<int>(lastItem.status);
        if (lastItem.status == UploadStatus::Failed) {
            qDebug() << "Upload failed (expected without daemon):" << lastItem.errorMsg;
        }
    }

    // ── Deduplication ────────────────────────────────────────────────────────
    // Deduplicator records the file hash only after a successful upload.
    // This test requires a live Logos daemon; it is skipped automatically
    // when the first upload fails (no daemon reachable).

    void test_duplicateFileIsDetected()
    {
        const QString path = m_tmpDir.path() + "/dup_test.mp4";
        const QByteArray content = "dedup test " + QUuid::createUuid().toByteArray();
        QFile f(path); f.open(QIODevice::WriteOnly);
        f.write(content); f.close();

        StorageClient storage;
        Deduplicator dedup;
        UploadQueue queue(&storage, &dedup);

        // First upload — need it to succeed for dedup to be recorded
        UploadStatus firstStatus = UploadStatus::Pending;
        {
            QEventLoop loop;
            connect(&queue, &UploadQueue::itemChanged, &loop,
                    [&](const UploadItem& item) {
                        if (item.status == UploadStatus::Complete ||
                            item.status == UploadStatus::Failed ||
                            item.status == UploadStatus::Duplicate) {
                            firstStatus = item.status;
                            loop.quit();
                        }
                    });
            QTimer::singleShot(10000, &loop, &QEventLoop::quit);
            QVERIFY(!queue.enqueue(path).isEmpty());
            loop.exec();
        }

        if (firstStatus == UploadStatus::Failed) {
            QSKIP("Logos daemon not reachable — dedup test requires a live daemon");
        }

        // Second upload of the same file — must be Duplicate
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
            QVERIFY(!queue.enqueue(path).isEmpty());
            loop.exec();

            QCOMPARE(result.status, UploadStatus::Duplicate);
            QVERIFY(!result.existingCid.isEmpty());
        }
    }

    // ── Folder enqueue ────────────────────────────────────────────────────────

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
