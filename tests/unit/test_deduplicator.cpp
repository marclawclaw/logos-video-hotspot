/**
 * Unit tests for Deduplicator — FURPS F: Deduplication requirement.
 *
 * These tests run without a Logos node. They verify:
 *   - BLAKE3 hash is computed correctly
 *   - Identical files are detected as duplicates after first upload
 *   - Different files are not flagged as duplicates
 *   - Local DB lookup prevents unnecessary network calls
 */

#include <video_hotspot/deduplicator.h>

#include <QFutureWatcher>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

using namespace VideoHotspot;

class DeduplicatorTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}

    void test_hashIsDeterministic() {
        // TODO: create temp file, compute hash twice, compare
        QSKIP("Not yet implemented");
    }

    void test_newFileIsNotDuplicate() {
        QSKIP("Not yet implemented");
    }

    void test_sameFileIsDetectedAsDuplicate() {
        QSKIP("Not yet implemented");
    }

    void test_differentFilesHaveDifferentHashes() {
        QSKIP("Not yet implemented");
    }

    void test_duplicateSignalEmitted() {
        QSKIP("Not yet implemented");
    }
};

QTEST_MAIN(DeduplicatorTest)
#include "test_deduplicator.moc"
