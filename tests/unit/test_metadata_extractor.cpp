/**
 * Unit tests for MetadataExtractor — FURPS F: Timestamp & Geolocation Tagging.
 */

#include <video_hotspot/metadata_extractor.h>

#include <QEventLoop>
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace VideoHotspot;

class MetadataExtractorTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmpDir;

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
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_tmpDir.isValid());
    }

    void test_extractReturnsSizeForExistingFile()
    {
        const QString path = m_tmpDir.path() + "/test.mp4";
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(QByteArray(1024, '\0'));
        f.close();

        MetadataExtractor extractor;
        const VideoMetadata meta = waitFuture(extractor.extract(path));

        QCOMPARE(meta.sizeBytes, 1024);
        QVERIFY(!meta.mimeType.isEmpty());
    }

    void test_extractNoLocationForMockFile()
    {
        const QString path = m_tmpDir.path() + "/no_exif.mp4";
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("fake video data");
        f.close();

        MetadataExtractor extractor;
        const VideoMetadata meta = waitFuture(extractor.extract(path));

        // Mock extractor returns no geolocation
        QVERIFY(!meta.location.has_value());
    }

    void test_extractMissingFileReturnsEmpty()
    {
        MetadataExtractor extractor;
        const VideoMetadata meta = waitFuture(extractor.extract("/nonexistent/file.mp4"));
        QCOMPARE(meta.sizeBytes, 0);
    }

    void test_thumbnailGeneratesFile()
    {
        const QString path = m_tmpDir.path() + "/thumb_source.mp4";
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("fake video data for thumbnail");
        f.close();

        MetadataExtractor extractor;
        const QString thumbPath = waitFuture(extractor.thumbnail(path));

        QVERIFY(!thumbPath.isEmpty());
        QVERIFY(QFile::exists(thumbPath));
    }
};

QTEST_MAIN(MetadataExtractorTest)
#include "test_metadata_extractor.moc"
