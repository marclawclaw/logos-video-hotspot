/**
 * Unit tests for Deduplicator — FURPS F: Deduplication requirement.
 *
 * These tests run without a Logos node. They verify:
 *   - SHA-256 (mock BLAKE3) hash is computed correctly
 *   - Identical files are detected as duplicates after first upload
 *   - Different files are not flagged as duplicates
 *   - Local DB lookup prevents unnecessary network calls
 */

#include <video_hotspot/deduplicator.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

using namespace VideoHotspot;

class DeduplicatorTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmpDir;

    static void writeFile(const QString& path, const QByteArray& data)
    {
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(data);
    }

    template<typename T>
    static T waitFuture(QFuture<T> future)
    {
        QEventLoop loop;
        QFutureWatcher<T> watcher;
        QObject::connect(&watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);
        watcher.setFuture(future);
        loop.exec();
        return watcher.result();
    }

private slots:
    void initTestCase()
    {
        // Use temp dir for DB to avoid polluting real data
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_tmpDir.isValid());
    }

    void test_hashIsDeterministic()
    {
        const QString path = m_tmpDir.path() + "/file1.mp4";
        writeFile(path, "hello, video hotspot");

        Deduplicator ded;
        const QString h1 = waitFuture(ded.computeHash(path));
        const QString h2 = waitFuture(ded.computeHash(path));

        QVERIFY(!h1.isEmpty());
        QCOMPARE(h1, h2);
    }

    void test_newFileIsNotDuplicate()
    {
        const QString path = m_tmpDir.path() + "/new_file.mp4";
        writeFile(path, QByteArray::fromHex("deadbeef") + QDateTime::currentDateTime().toString().toUtf8());

        Deduplicator ded;
        const DedupResult result = waitFuture(ded.checkFile(path));

        QVERIFY(!result.isDuplicate);
        QVERIFY(!result.blake3Hash.isEmpty());
    }

    void test_sameFileIsDetectedAsDuplicate()
    {
        const QString path = m_tmpDir.path() + "/same_file.mp4";
        const QByteArray content = "unique content for dedup test " +
                                   QUuid::createUuid().toByteArray();
        writeFile(path, content);

        Deduplicator ded;

        // First check: not a duplicate
        const DedupResult first = waitFuture(ded.checkFile(path));
        QVERIFY(!first.isDuplicate);

        // Record as uploaded
        const QString fakeCid = "cid_" + first.blake3Hash;
        ded.recordUpload(path, first.blake3Hash, fakeCid);

        // Second check: should be a duplicate
        const DedupResult second = waitFuture(ded.checkFile(path));
        QVERIFY(second.isDuplicate);
        QCOMPARE(second.existingCid, fakeCid);
    }

    void test_differentFilesHaveDifferentHashes()
    {
        const QString path1 = m_tmpDir.path() + "/diff1.mp4";
        const QString path2 = m_tmpDir.path() + "/diff2.mp4";
        writeFile(path1, "content A");
        writeFile(path2, "content B");

        Deduplicator ded;
        const QString h1 = waitFuture(ded.computeHash(path1));
        const QString h2 = waitFuture(ded.computeHash(path2));

        QVERIFY(!h1.isEmpty());
        QVERIFY(!h2.isEmpty());
        QVERIFY(h1 != h2);
    }

    void test_emptyFileHasHash()
    {
        const QString path = m_tmpDir.path() + "/empty.mp4";
        writeFile(path, "");

        Deduplicator ded;
        const QString h = waitFuture(ded.computeHash(path));
        // SHA-256 of empty string is well-known
        QCOMPARE(h, QString("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    }

    void test_missingFileReturnsEmpty()
    {
        Deduplicator ded;
        const QString h = waitFuture(ded.computeHash("/nonexistent/path/file.mp4"));
        QVERIFY(h.isEmpty());
    }
};

QTEST_MAIN(DeduplicatorTest)
#include "test_deduplicator.moc"
