/**
 * VideoHotspotApp.qml — Root component for the Video Hotspot miniapp.
 *
 * Provides tab navigation between:
 *   - UploadScreen   — file/folder picker, upload queue, dedup status
 *   - MapScreen      — interactive map with timeline slider
 *   - CacheScreen    — storage management, user-owned vs cached videos
 *   - SettingsScreen — storage limits, folder monitor, network settings
 *
 * Dark mode is the default (field use, nighttime events).
 *
 * Root is a Rectangle (not ApplicationWindow) so this component can be
 * embedded in a QQuickWidget inside Basecamp's tab layout.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#1a1a1a"

    // ── Upload queue data model ────────────────────────────────────────────
    ListModel {
        id: uploadQueue
    }

    // ── Upload start queue ─────────────────────────────────────────────────
    // Staggered timer: starts progress animation for each new item in turn
    Timer {
        id: progressStarter
        interval: 300
        repeat: false
        running: false
        onTriggered: startNextPendingUpload()
    }

    function startNextPendingUpload() {
        for (var i = 0; i < uploadQueue.count; i++) {
            var item = uploadQueue.get(i)
            if (!item.isDuplicate && item.status === "uploading" && !item.animating) {
                uploadQueue.setProperty(i, "animating", true)
                // Start this item's progress via the active timer
                activeProgressTimer.targetIndex = i
                activeProgressTimer.start()
                return
            }
        }
    }

    // Drives one item's progress at a time, chains to next when done
    Timer {
        id: activeProgressTimer
        interval: 50
        repeat: true
        running: false
        property int targetIndex: -1

        onTriggered: {
            if (targetIndex < 0 || targetIndex >= uploadQueue.count) {
                stop()
                return
            }
            var item = uploadQueue.get(targetIndex)
            var newProgress = item.progress + 4
            if (newProgress >= 100) {
                newProgress = 100
                uploadQueue.setProperty(targetIndex, "progress", newProgress)
                uploadQueue.setProperty(targetIndex, "status", "done")
                stop()
                // Chain to next pending item
                progressStarter.start()
            } else {
                uploadQueue.setProperty(targetIndex, "progress", newProgress)
            }
        }
    }

    // ── Mock file names pool ───────────────────────────────────────────────
    property var mockFiles: [
        "event-footage-001.mp4",
        "protest-march-clip2.mp4",
        "rally-coverage.mp4",
        "night-session.mp4",
        "field-report-hd.mp4"
    ]
    property int mockFileIndex: 0

    property var mockFolderFiles: [
        "folder-clip-001.mp4",
        "folder-clip-002.mp4",
        "folder-clip-003.mp4"
    ]
    property int mockFolderIndex: 0

    // ── Helper: add item to queue, detect duplicates ───────────────────────
    function addToQueue(filename) {
        // Check for duplicate
        for (var i = 0; i < uploadQueue.count; i++) {
            if (uploadQueue.get(i).filename === filename) {
                // Mark as dedup
                uploadQueue.setProperty(i, "isDuplicate", true)
                return
            }
        }
        uploadQueue.append({
            "filename": filename,
            "progress": 0,
            "status": "uploading",
            "isDuplicate": false,
            "animating": false
        })
        // Kick off animation if nothing is currently running
        if (!activeProgressTimer.running) {
            progressStarter.start()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Title bar ─────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: "#111111"

            Text {
                anchors.centerIn: parent
                text: "Video Hotspot"
                color: "#f0f0f0"
                font.pixelSize: 16
                font.bold: true
            }
        }

        // ── Tab bar ───────────────────────────────────────────────────────
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            background: Rectangle { color: "#222222" }

            TabButton {
                text: "Upload"
                contentItem: Text {
                    text: parent.text
                    color: tabBar.currentIndex === 0 ? "#4a90d9" : "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: tabBar.currentIndex === 0 ? "#2a2a2a" : "transparent"
                }
            }
            TabButton {
                text: "Map"
                contentItem: Text {
                    text: parent.text
                    color: tabBar.currentIndex === 1 ? "#4a90d9" : "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: tabBar.currentIndex === 1 ? "#2a2a2a" : "transparent"
                }
            }
            TabButton {
                text: "Downloads"
                contentItem: Text {
                    text: parent.text
                    color: tabBar.currentIndex === 2 ? "#4a90d9" : "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: tabBar.currentIndex === 2 ? "#2a2a2a" : "transparent"
                }
            }
            TabButton {
                text: "Settings"
                contentItem: Text {
                    text: parent.text
                    color: tabBar.currentIndex === 3 ? "#4a90d9" : "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: tabBar.currentIndex === 3 ? "#2a2a2a" : "transparent"
                }
            }
        }

        // ── Screen stack ──────────────────────────────────────────────────
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // ── Upload Screen ──────────────────────────────────────────────
            Rectangle {
                color: "#1a1a1a"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Text {
                        text: "Upload Videos"
                        color: "#f0f0f0"
                        font.pixelSize: 20
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 12

                        Button {
                            id: uploadFileBtn
                            text: "📂  Upload File"
                            background: Rectangle {
                                color: uploadFileBtn.pressed ? "#4a4a4a" : (uploadFileBtn.hovered ? "#454545" : "#3a3a3a")
                                radius: 4
                                Behavior on color { ColorAnimation { duration: 80 } }
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#f0f0f0"
                                horizontalAlignment: Text.AlignHCenter
                            }
                            onClicked: {
                                // Mock: cycle through sample files
                                var filename = mockFiles[mockFileIndex % mockFiles.length]
                                mockFileIndex++
                                addToQueue(filename)
                            }
                        }

                        Button {
                            id: uploadFolderBtn
                            text: "📁  Upload Folder"
                            background: Rectangle {
                                color: uploadFolderBtn.pressed ? "#4a4a4a" : (uploadFolderBtn.hovered ? "#454545" : "#3a3a3a")
                                radius: 4
                                Behavior on color { ColorAnimation { duration: 80 } }
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#f0f0f0"
                                horizontalAlignment: Text.AlignHCenter
                            }
                            onClicked: {
                                // Mock: add 3 folder clips
                                addToQueue("folder-clip-001.mp4")
                                addToQueue("folder-clip-002.mp4")
                                addToQueue("folder-clip-003.mp4")
                            }
                        }

                        Button {
                            id: monitorBtn
                            text: "👁  Monitor Folder"
                            background: Rectangle {
                                color: monitorBtn.pressed ? "#3a6a3a" : (monitorBtn.hovered ? "#306030" : "#2a5a2a")
                                radius: 4
                                Behavior on color { ColorAnimation { duration: 80 } }
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#aaffaa"
                                horizontalAlignment: Text.AlignHCenter
                            }
                            onClicked: {
                                // Mock: add a folder-monitored file
                                addToQueue("~/Videos/Hotspot/new-footage-" + Math.floor(Math.random() * 900 + 100) + ".mp4")
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#333333"
                    }

                    RowLayout {
                        Text {
                            text: "Upload Queue"
                            color: "#aaaaaa"
                            font.pixelSize: 13
                            font.bold: true
                        }
                        Rectangle {
                            visible: uploadQueue.count > 0
                            width: queueCountLabel.implicitWidth + 12
                            height: 18
                            radius: 9
                            color: "#4a90d9"
                            Text {
                                id: queueCountLabel
                                anchors.centerIn: parent
                                text: uploadQueue.count
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                    }

                    // Empty state
                    Rectangle {
                        visible: uploadQueue.count === 0
                        Layout.fillWidth: true
                        height: 80
                        color: "#222222"
                        radius: 6
                        Text {
                            anchors.centerIn: parent
                            text: "No files queued — click Upload File or Upload Folder to start"
                            color: "#666666"
                            font.pixelSize: 12
                        }
                    }

                    // Queue list
                    ListView {
                        id: queueList
                        visible: uploadQueue.count > 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: uploadQueue
                        spacing: 8
                        clip: true

                        delegate: Rectangle {
                            width: queueList.width
                            height: 60
                            color: "#2a2a2a"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                // File icon
                                Text {
                                    text: model.isDuplicate ? "⚠️" : (model.status === "done" ? "✅" : "⏳")
                                    font.pixelSize: 16
                                }

                                // Filename
                                Text {
                                    text: model.filename
                                    color: "#f0f0f0"
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }

                                // Dedup badge
                                Rectangle {
                                    visible: model.isDuplicate
                                    width: dupLabel.implicitWidth + 16
                                    height: 22
                                    radius: 11
                                    color: "#aa6600"
                                    Text {
                                        id: dupLabel
                                        anchors.centerIn: parent
                                        text: "DUPLICATE"
                                        color: "#ffcc88"
                                        font.pixelSize: 10
                                        font.bold: true
                                    }
                                }

                                // Done label
                                Text {
                                    visible: !model.isDuplicate && model.status === "done"
                                    text: "✓ Uploaded"
                                    color: "#44cc44"
                                    font.pixelSize: 12
                                }

                                // Progress bar + percent
                                RowLayout {
                                    visible: !model.isDuplicate && model.status === "uploading"
                                    spacing: 6

                                    Rectangle {
                                        width: 120
                                        height: 20
                                        radius: 3
                                        color: "#333333"

                                        Rectangle {
                                            width: parent.width * (model.progress / 100)
                                            height: parent.height
                                            radius: 3
                                            color: "#4a90d9"
                                            Behavior on width {
                                                NumberAnimation { duration: 60 }
                                            }
                                        }
                                    }

                                    Text {
                                        text: model.progress + "%"
                                        color: "#4a90d9"
                                        font.pixelSize: 12
                                        Layout.preferredWidth: 36
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        visible: uploadQueue.count === 0
                    }
                }
            }

            // ── Map Screen ─────────────────────────────────────────────────
            Rectangle {
                color: "#1a1a1a"
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1c2b1c"

                        // Placeholder map grid
                        Grid {
                            anchors.fill: parent
                            columns: 8
                            Repeater {
                                model: 48
                                Rectangle {
                                    width: parent.width / 8
                                    height: parent.height / 6
                                    color: index % 3 === 0 ? "#1e2e1e" : (index % 5 === 0 ? "#243024" : "#1c281c")
                                    border.color: "#162416"
                                    border.width: 1
                                }
                            }
                        }

                        // Video pins on the map
                        Rectangle { x: 180; y: 140; width: 14; height: 14; radius: 7; color: "#ff4444" }
                        Rectangle { x: 320; y: 200; width: 14; height: 14; radius: 7; color: "#ff4444" }
                        Rectangle { x: 500; y: 120; width: 14; height: 14; radius: 7; color: "#ffaa22" }
                        Rectangle { x: 650; y: 280; width: 14; height: 14; radius: 7; color: "#ff4444" }
                        Rectangle { x: 820; y: 180; width: 14; height: 14; radius: 7; color: "#aaffaa" }

                        Text {
                            anchors.centerIn: parent
                            text: "Map View — 5 videos indexed"
                            color: "#44aa44"
                            font.pixelSize: 14
                            opacity: 0.7
                        }
                    }

                    // Timeline slider
                    Rectangle {
                        Layout.fillWidth: true
                        height: 60
                        color: "#111111"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12
                            Text { text: "Timeline:"; color: "#aaaaaa"; font.pixelSize: 12 }
                            Slider {
                                id: timelineSlider
                                Layout.fillWidth: true
                                from: 0; to: 1; value: 0.6
                                background: Rectangle {
                                    height: 4; radius: 2
                                    color: "#333333"
                                    Rectangle {
                                        width: parent.width * timelineSlider.value
                                        height: parent.height
                                        radius: 2
                                        color: "#4a90d9"
                                    }
                                }
                            }
                            Text { text: "2026-03-14 14:00"; color: "#4a90d9"; font.pixelSize: 12 }
                        }
                    }
                }
            }

            // ── Downloads/Cache Screen ─────────────────────────────────────
            Rectangle {
                color: "#1a1a1a"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16
                    Text { text: "Downloads & Cache"; color: "#f0f0f0"; font.pixelSize: 20; font.bold: true }
                    Text { text: "Storage: 1.2 GB of 10 GB used"; color: "#aaaaaa"; font.pixelSize: 13 }
                    Rectangle {
                        Layout.fillWidth: true; height: 12; radius: 6; color: "#333333"
                        Rectangle { width: parent.width * 0.12; height: parent.height; radius: 6; color: "#4a90d9" }
                    }
                    Text { text: "Your Videos (3)"; color: "#aaaaaa"; font.pixelSize: 13; font.bold: true }
                    Repeater {
                        model: ["sample-event.mp4 — 2.3 MB  ✓ Owned", "march-footage.mp4 — 8.1 MB  ✓ Owned", "clip-b.mp4 — 4.5 MB  ✓ Owned"]
                        Rectangle {
                            Layout.fillWidth: true; height: 44; color: "#2a2a2a"; radius: 6
                            Text { anchors.centerIn: parent; text: modelData; color: "#f0f0f0"; font.pixelSize: 12 }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ── Settings Screen ────────────────────────────────────────────
            Rectangle {
                color: "#1a1a1a"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 20
                    Text { text: "Settings"; color: "#f0f0f0"; font.pixelSize: 20; font.bold: true }
                    RowLayout {
                        Text { text: "Storage limit:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Slider {
                            id: storageSlider
                            from: 1; to: 50; value: 10; Layout.fillWidth: true
                        }
                        Text { text: Math.round(storageSlider.value) + " GB"; color: "#4a90d9"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        Text { text: "Monitor folder:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Text { text: "~/Videos/Hotspot"; color: "#f0f0f0"; font.pixelSize: 13 }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Change"
                            background: Rectangle { color: "#3a3a3a"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#f0f0f0"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: { /* mock: would open folder dialog */ }
                        }
                    }
                    RowLayout {
                        Text { text: "Auto-monitoring:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Switch { checked: true }
                    }
                    RowLayout {
                        Text { text: "Logos node:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Text { text: "● connected (mock)"; color: "#44cc44"; font.pixelSize: 13 }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
