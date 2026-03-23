/**
 * VideoHotspotApp.qml — Root component for the Video Hotspot miniapp.
 *
 * Tab order: Map → Upload → Downloads → Settings
 * Map is the default landing tab.
 *
 * Map is rendered as a vector SVG (map.svg) via Qt's Image item + Qt6 SVG
 * image plugin.  SVG scales losslessly at any resolution — no pixelation.
 * Timeline slider filters pin visibility by time value.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Helper to format bytes as human-readable string
function formatBytes(bytes) {
    if (bytes < 0) bytes = 0
    if (bytes < 1024) return bytes + " B"
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB"
    if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + " MB"
    return (bytes / (1024 * 1024 * 1024)).toFixed(2) + " GB"
}

Rectangle {
    id: root
    color: "#1a1a1a"

    // ── Upload queue data model ────────────────────────────────────────────
    ListModel {
        id: uploadQueue
    }

    // ── Upload animation chain ─────────────────────────────────────────────
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
                activeProgressTimer.targetIndex = i
                activeProgressTimer.start()
                return
            }
        }
    }

    Timer {
        id: activeProgressTimer
        interval: 50
        repeat: true
        running: false
        property int targetIndex: -1

        onTriggered: {
            if (targetIndex < 0 || targetIndex >= uploadQueue.count) { stop(); return }
            var item = uploadQueue.get(targetIndex)
            var newProgress = item.progress + 4
            if (newProgress >= 100) {
                newProgress = 100
                uploadQueue.setProperty(targetIndex, "progress", newProgress)
                uploadQueue.setProperty(targetIndex, "status", "done")
                stop()
                progressStarter.start()
            } else {
                uploadQueue.setProperty(targetIndex, "progress", newProgress)
            }
        }
    }

    // ── Mock file pool ─────────────────────────────────────────────────────
    property var mockFiles: [
        "event-footage-001.mp4",
        "protest-march-clip2.mp4",
        "rally-coverage.mp4",
        "night-session.mp4",
        "field-report-hd.mp4"
    ]
    property int mockFileIndex: 0

    function addToQueue(filename) {
        for (var i = 0; i < uploadQueue.count; i++) {
            if (uploadQueue.get(i).filename === filename) {
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
        if (!activeProgressTimer.running) progressStarter.start()
    }

    // ── Map pins data ──────────────────────────────────────────────────────
    // fx/fy are fractional positions (0‒1) within the map area.
    // timeValue: 0‒1, represents the pin's point in the demo time range.
    // Slider value ≥ timeValue → pin fully visible; below → faded.
    ListModel {
        id: mapPinsModel
        ListElement { fx: 0.22; fy: 0.30; timeValue: 0.20; pinColor: "#4a90d9"; label: "clip-a.mp4" }
        ListElement { fx: 0.38; fy: 0.44; timeValue: 0.38; pinColor: "#ff9800"; label: "clip-b.mp4" }
        ListElement { fx: 0.55; fy: 0.26; timeValue: 0.55; pinColor: "#4a90d9"; label: "sample1.mp4" }
        ListElement { fx: 0.70; fy: 0.52; timeValue: 0.72; pinColor: "#4caf50"; label: "archive.mp4" }
        ListElement { fx: 0.82; fy: 0.38; timeValue: 0.88; pinColor: "#4a90d9"; label: "event-2.mp4" }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Title bar ──────────────────────────────────────────────────────
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

        // ── Tab bar — Map first ────────────────────────────────────────────
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            background: Rectangle { color: "#222222" }

            TabButton {
                text: "Map"
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
                text: "Upload"
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

        // ── Screen stack ───────────────────────────────────────────────────
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // ══ 0 — Map Screen ═════════════════════════════════════════════
            Rectangle {
                color: "#1a1a1a"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // ── Map area: SVG background + pin overlay ─────────────
                    Item {
                        id: mapArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // Vector map rendered from SVG — scales losslessly at
                        // any resolution.  Requires qt6-svg-plugins at runtime.
                        Image {
                            id: mapImage
                            anchors.fill: parent
                            source: "qrc:/qml/qml/map.svg"
                            fillMode: Image.Stretch
                            smooth: true
                            antialiasing: true
                        }

                        // ── Video pin overlay ──────────────────────────────
                        // Pins are positioned fractionally over the map area.
                        // Opacity is driven by the timeline slider value:
                        //   pin.timeValue ≤ slider → fully visible (1.0)
                        //   pin.timeValue  > slider → faded  (0.15)
                        Repeater {
                            model: mapPinsModel
                            delegate: Item {
                                id: pinDelegate
                                x: model.fx * mapArea.width  - 14
                                y: model.fy * mapArea.height - 14
                                width: 28
                                height: 28

                                opacity: model.timeValue <= timelineSlider.value ? 1.0 : 0.15
                                Behavior on opacity {
                                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                                }

                                // Glow ring
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 28; height: 28; radius: 14
                                    color: "transparent"
                                    border.color: model.pinColor
                                    border.width: 1.5
                                    opacity: 0.5
                                }
                                // Pin body
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 22; height: 22; radius: 11
                                    color: model.pinColor
                                    opacity: 0.88
                                }
                                // Play icon
                                Text {
                                    anchors.centerIn: parent
                                    text: "▶"
                                    color: "#ffffff"
                                    font.pixelSize: 9
                                    leftPadding: 1
                                }
                            }
                        }

                        // ── "Pins visible" counter ─────────────────────────
                        Rectangle {
                            anchors { left: parent.left; top: parent.top; margins: 12 }
                            width: pinCountText.implicitWidth + 16
                            height: 24
                            radius: 4
                            color: "#1a1a1aCC"
                            border.color: "#444444"
                            Text {
                                id: pinCountText
                                anchors.centerIn: parent
                                // Count how many pins are within the slider range
                                text: {
                                    var n = 0
                                    for (var i = 0; i < mapPinsModel.count; i++) {
                                        if (mapPinsModel.get(i).timeValue <= timelineSlider.value) n++
                                    }
                                    return n + " of " + mapPinsModel.count + " videos"
                                }
                                color: "#4a90d9"
                                font.pixelSize: 11
                                font.family: "monospace"
                            }
                        }
                    }

                    // ── Timeline slider ────────────────────────────────────
                    Rectangle {
                        Layout.fillWidth: true
                        height: 60
                        color: "#111111"
                        border.color: "#333333"
                        border.width: 0
                        Rectangle { width: parent.width; height: 1; color: "#333333" }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            Text {
                                text: "Timeline:"
                                color: "#aaaaaa"
                                font.pixelSize: 12
                            }

                            Slider {
                                id: timelineSlider
                                Layout.fillWidth: true
                                from: 0; to: 1; value: 0.6

                                background: Rectangle {
                                    x: timelineSlider.leftPadding
                                    y: timelineSlider.topPadding + timelineSlider.availableHeight / 2 - height / 2
                                    width: timelineSlider.availableWidth
                                    height: 6
                                    radius: 3
                                    color: "#333333"

                                    Rectangle {
                                        width: timelineSlider.visualPosition * parent.width
                                        height: parent.height
                                        radius: 3
                                        color: "#4a90d9"
                                    }
                                }

                                handle: Rectangle {
                                    x: timelineSlider.leftPadding + timelineSlider.visualPosition
                                       * (timelineSlider.availableWidth - width)
                                    y: timelineSlider.topPadding + timelineSlider.availableHeight / 2 - height / 2
                                    width: 18; height: 18; radius: 9
                                    color: timelineSlider.pressed ? "#6aA0f9" : "#4a90d9"
                                    border.color: "#ffffff"
                                    border.width: 1
                                    Behavior on color { ColorAnimation { duration: 80 } }
                                }
                            }

                            Text {
                                // Show a date label that changes with the slider
                                text: {
                                    var base = new Date(2026, 2, 14)  // 2026-03-14
                                    var offDays = Math.round((1.0 - timelineSlider.value) * 30)
                                    base.setDate(base.getDate() - offDays)
                                    var y = base.getFullYear()
                                    var m = ("0" + (base.getMonth()+1)).slice(-2)
                                    var d = ("0" + base.getDate()).slice(-2)
                                    var h = Math.round(timelineSlider.value * 23)
                                    return y + "-" + m + "-" + d + " " + ("0"+h).slice(-2) + ":00"
                                }
                                color: "#4a90d9"
                                font.pixelSize: 12
                                font.family: "monospace"
                            }
                        }
                    }
                }
            }

            // ══ 1 — Upload Screen ══════════════════════════════════════════
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
                                addToQueue("~/Videos/Hotspot/new-footage-"
                                    + Math.floor(Math.random() * 900 + 100) + ".mp4")
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

                                Text {
                                    text: model.isDuplicate ? "⚠️" : (model.status === "done" ? "✅" : "⏳")
                                    font.pixelSize: 16
                                }

                                Text {
                                    text: model.filename
                                    color: "#f0f0f0"
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }

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

                                Text {
                                    visible: !model.isDuplicate && model.status === "done"
                                    text: "✓ Uploaded"
                                    color: "#44cc44"
                                    font.pixelSize: 12
                                }

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

            // ══ 2 — Downloads/Cache Screen ═════════════════════════════════
            Rectangle {
                color: "#1a1a1a"

                // ListModel populated from StorageClient::cachedEntries() via QML
                ListModel { id: cachedVideosModel }

                // Load cached entries from core on component ready
                Component.onCompleted: refreshCachedVideos()

                function refreshCachedVideos() {
                    cachedVideosModel.clear()
                    if (typeof storageClient !== "undefined" && storageClient) {
                        var entries = storageClient.cachedEntriesVariantList()
                        for (var i = 0; i < entries.length; i++) {
                            var e = entries[i]
                            var filename = e.localPath ? e.localPath.split("/").pop() : e.cid
                            cachedVideosModel.append({
                                "filename": filename,
                                "cid": e.cid,
                                "sizeBytes": e.sizeBytes,
                                "sizeLabel": formatBytes(e.sizeBytes),
                                "userOwned": e.userOwned
                            })
                        }
                    }
                }

                // Listen for storage changes to refresh the list
                Connections {
                    target: storageClient
                    function onStorageStatsChanged() { refreshCachedVideos() }
                    function onDownloadComplete(cid, localPath) { refreshCachedVideos() }
                    function onUploadComplete(filePath, cid) { refreshCachedVideos() }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    Text { text: "Downloads & Cache"; color: "#f0f0f0"; font.pixelSize: 20; font.bold: true }

                    // Real storage stats from StorageClient
                    property real storageUsed: (typeof storageClient !== "undefined" && storageClient)
                        ? storageClient.statsTotalUsedBytes() : 0
                    property real storageAllocated: (typeof storageClient !== "undefined" && storageClient)
                        ? (storageClient.statsAllocatedBytes() > 0 ? storageClient.statsAllocatedBytes() : 10 * 1024 * 1024 * 1024)
                        : 10 * 1024 * 1024 * 1024
                    property real storagePct: storageAllocated > 0 ? (storageUsed / storageAllocated) : 0

                    Text {
                        text: "Storage: " + formatBytes(storageUsed) + " of " + formatBytes(storageAllocated) + " used"
                        color: "#aaaaaa"
                        font.pixelSize: 13
                    }

                    Rectangle {
                        id: storageBar
                        Layout.fillWidth: true; height: 12; radius: 6; color: "#333333"
                        Rectangle {
                            width: storageBar.parent ? storageBar.parent.storagePct * storageBar.width : 0
                            height: parent.height; radius: 6; color: "#4a90d9"
                            Behavior on width { NumberAnimation { duration: 200 } }
                        }
                    }

                    Text {
                        text: "Your Videos (" + cachedVideosModel.count + ")"
                        color: "#aaaaaa"; font.pixelSize: 13; font.bold: true
                    }

                    Rectangle {
                        visible: cachedVideosModel.count === 0
                        Layout.fillWidth: true; height: 60; color: "#222222"; radius: 6
                        Text {
                            anchors.centerIn: parent
                            text: "No videos cached — upload videos to see them here"
                            color: "#666666"; font.pixelSize: 12
                        }
                    }

                    ListView {
                        visible: cachedVideosModel.count > 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: cachedVideosModel
                        spacing: 6
                        clip: true

                        delegate: Rectangle {
                            width: parent ? parent.width : 0
                            height: 48
                            color: "#2a2a2a"; radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: model.userOwned ? "✓" : "⬇"
                                    color: model.userOwned ? "#44cc44" : "#4a90d9"
                                    font.pixelSize: 14
                                }

                                Text {
                                    text: model.filename
                                    color: "#f0f0f0"; font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: model.sizeLabel + (model.userOwned ? "  ✓ Owned" : "  ⎋ Cached")
                                    color: "#aaaaaa"; font.pixelSize: 11
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // ══ 3 — Settings Screen ════════════════════════════════════════
            Rectangle {
                color: "#1a1a1a"

                // Refresh settings display when storage stats change
                Connections {
                    target: storageClient
                    function onStorageStatsChanged() { settingsLoader.update() }
                }

                Component {
                    id: settingsLoader
                    function update() {
                        // Force binding recomputation by touching slider
                        storageSlider.valueChanged(storageSlider.value)
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 20
                    Text { text: "Settings"; color: "#f0f0f0"; font.pixelSize: 20; font.bold: true }

                    // Real storage stats from StorageClient
                    property real storedAllocated: (typeof storageClient !== "undefined" && storageClient)
                        ? storageClient.statsAllocatedBytes() : 0
                    property real storedUsed: (typeof storageClient !== "undefined" && storageClient)
                        ? storageClient.statsTotalUsedBytes() : 0

                    RowLayout {
                        Text { text: "Storage limit:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Slider {
                            id: storageSlider
                            from: 1; to: 50
                            // Initialise from core if a limit has been set, else default 10 GB
                            value: parent.parent.storedAllocated > 0
                                   ? (parent.parent.storedAllocated / (1024 * 1024 * 1024))
                                   : 10
                            Layout.fillWidth: true
                            onMoved: {
                                var limitBytes = Math.round(value) * 1024 * 1024 * 1024
                                if (typeof storageClient !== "undefined" && storageClient)
                                    storageClient.setStorageLimit(limitBytes)
                            }
                        }
                        Text {
                            text: {
                                var currentGiB = parent.parent.storedAllocated > 0
                                      ? (parent.parent.storedAllocated / (1024 * 1024 * 1024)).toFixed(0)
                                      : Math.round(storageSlider.value)
                                return currentGiB + " GB"
                            }
                            color: "#4a90d9"; font.pixelSize: 13
                        }
                    }

                    // Real storage usage breakdown
                    RowLayout {
                        visible: parent.parent.storedUsed > 0
                        Text {
                            text: "Used: " + formatBytes(parent.parent.storedUsed)
                                  + "  (owned: " + formatBytes((typeof storageClient !== "undefined" && storageClient) ? storageClient.statsUserOwnedBytes() : 0)
                                  + ", cached: " + formatBytes((typeof storageClient !== "undefined" && storageClient) ? storageClient.statsCachedBytes() : 0) + ")"
                            color: "#aaaaaa"; font.pixelSize: 11
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }

                    RowLayout {
                        Text { text: "Monitor folder:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Text { text: "~/Videos/Hotspot"; color: "#f0f0f0"; font.pixelSize: 13 }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Change"
                            background: Rectangle { color: "#3a3a3a"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#f0f0f0"; horizontalAlignment: Text.AlignHCenter }
                            onClicked: { /* TODO: real folder picker */ }
                        }
                    }
                    RowLayout {
                        Text { text: "Auto-monitoring:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Switch { checked: true }
                    }
                    RowLayout {
                        Text { text: "Logos node:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Text {
                            text: (typeof logosConnected !== "undefined" && logosConnected)
                                  ? "● connected (Logos)"
                                  : "● connected (mock)"
                            color: (typeof logosConnected !== "undefined" && logosConnected) ? "#44cc44" : "#44cc44"
                            font.pixelSize: 13
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
