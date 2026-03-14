/**
 * Unit tests for UploadQueue.
 *
 * Verified requirements (FURPS F - Upload):
 *   - Enqueuing a file adds it to the queue with Pending status
 *   - AwaitingGeo status set when file has no EXIF location
 *   - provideGeolocation() unblocks an AwaitingGeo item
 *   - Duplicate files transition to Duplicate status
 *   - Failed uploads can be retried
 *   - Queue persists across restart (SQLite-backed)
 */

#include <video_hotspot/upload_queue.h>

#include <QSignalSpy>
#include <QtTest>

using namespace VideoHotspot;

class UploadQueueTest : public QObject {
    Q_OBJECT

private slots:
    void test_enqueueAddsItemAsPending() {
        QSKIP("Not yet implemented");
    }

    void test_duplicateTransitionsToDuplicateStatus() {
        QSKIP("Not yet implemented");
    }

    void test_noExifLocationTransitionsToAwaitingGeo() {
        QSKIP("Not yet implemented");
    }

    void test_provideGeolocationUnblocksUpload() {
        QSKIP("Not yet implemented");
    }

    void test_failedUploadCanBeRetried() {
        QSKIP("Not yet implemented");
    }
};

QTEST_MAIN(UploadQueueTest)
#include "test_upload_queue.moc"
