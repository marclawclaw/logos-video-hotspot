/**
 * VideoHotspotApp.qml — Root component for the Video Hotspot miniapp.
 *
 * Tab order: Map → Upload → Downloads → Settings
 * Map is the default landing tab.
 *
 * Map uses a Canvas for crisp, resolution-independent rendering (no pixel
 * scaling artefacts). Timeline slider filters pin visibility by time value.
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

                    // ── Map area: Canvas background + pin overlay ──────────
                    Item {
                        id: mapArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // Vector map drawn on Canvas — renders at native DPI,
                        // no pixel scaling artefacts.
                        Canvas {
                            id: mapCanvas
                            anchors.fill: parent

                            // Repaint whenever the container is resized so
                            // the map always fills at full resolution.
                            onWidthChanged:  requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                var ctx = getContext("2d")
                                var w = width
                                var h = height

                                // ── Base terrain ──────────────────────────
                                ctx.fillStyle = "#1c2b1c"
                                ctx.fillRect(0, 0, w, h)

                                // ── Tile grid (subtle, thin) ──────────────
                                ctx.strokeStyle = "#243224"
                                ctx.lineWidth = 0.5
                                var tile = 80
                                ctx.beginPath()
                                for (var gx = 0; gx < w; gx += tile) {
                                    ctx.moveTo(gx, 0); ctx.lineTo(gx, h)
                                }
                                for (var gy = 0; gy < h; gy += tile) {
                                    ctx.moveTo(0, gy); ctx.lineTo(w, gy)
                                }
                                ctx.stroke()

                                // ── Block fills (simulated city blocks) ───
                                ctx.fillStyle = "#1e2e1e"
                                var blocks = [
                                    [0.10, 0.10, 0.18, 0.22],
                                    [0.42, 0.12, 0.20, 0.18],
                                    [0.68, 0.08, 0.14, 0.16],
                                    [0.08, 0.50, 0.16, 0.20],
                                    [0.44, 0.48, 0.18, 0.22],
                                    [0.70, 0.58, 0.16, 0.18],
                                    [0.20, 0.70, 0.22, 0.16],
                                    [0.56, 0.74, 0.20, 0.14],
                                ]
                                for (var b = 0; b < blocks.length; b++) {
                                    ctx.fillRect(blocks[b][0]*w, blocks[b][1]*h,
                                                 blocks[b][2]*w, blocks[b][3]*h)
                                }

                                // ── Main roads ────────────────────────────
                                ctx.strokeStyle = "#2e3e2e"
                                ctx.lineWidth = 14
                                ctx.lineCap = "round"
                                // Vertical arterial
                                ctx.beginPath()
                                ctx.moveTo(w * 0.36, 0); ctx.lineTo(w * 0.36, h)
                                ctx.stroke()
                                // Horizontal arterial
                                ctx.lineWidth = 10
                                ctx.beginPath()
                                ctx.moveTo(0, h * 0.36); ctx.lineTo(w, h * 0.36)
                                ctx.stroke()

                                // ── Secondary roads ───────────────────────
                                ctx.strokeStyle = "#283828"
                                ctx.lineWidth = 6
                                ctx.beginPath()
                                ctx.moveTo(w * 0.65, 0); ctx.lineTo(w * 0.65, h)
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.moveTo(0, h * 0.65); ctx.lineTo(w, h * 0.65)
                                ctx.stroke()

                                // ── Minor streets ─────────────────────────
                                ctx.strokeStyle = "#243024"
                                ctx.lineWidth = 2
                                var minorX = [0.12, 0.28, 0.50, 0.76, 0.90]
                                var minorY = [0.20, 0.50, 0.80]
                                ctx.beginPath()
                                for (var mx = 0; mx < minorX.length; mx++) {
                                    ctx.moveTo(minorX[mx]*w, 0)
                                    ctx.lineTo(minorX[mx]*w, h)
                                }
                                for (var my = 0; my < minorY.length; my++) {
                                    ctx.moveTo(0, minorY[my]*h)
                                    ctx.lineTo(w, minorY[my]*h)
                                }
                                ctx.stroke()

                                // ── Road centre-lines (dashed yellow) ─────
                                ctx.strokeStyle = "#3a4a1a"
                                ctx.lineWidth = 1
                                ctx.setLineDash([8, 12])
                                ctx.beginPath()
                                ctx.moveTo(w * 0.36, 0); ctx.lineTo(w * 0.36, h)
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.moveTo(0, h * 0.36); ctx.lineTo(w, h * 0.36)
                                ctx.stroke()
                                ctx.setLineDash([])

                                // ── Zoom controls (drawn on canvas corner) ─
                                ctx.fillStyle = "#2a2a2a"
                                ctx.strokeStyle = "#444444"
                                ctx.lineWidth = 1
                                roundRect(ctx, w-44, 16, 28, 28, 4)
                                ctx.fill(); ctx.stroke()
                                roundRect(ctx, w-44, 52, 28, 28, 4)
                                ctx.fill(); ctx.stroke()
                                ctx.fillStyle = "#f0f0f0"
                                ctx.font = "bold 18px sans-serif"
                                ctx.textAlign = "center"
                                ctx.textBaseline = "middle"
                                ctx.fillText("+", w-30, 30)
                                ctx.fillText("−", w-30, 66)
                            }

                            // Helper: rounded rectangle path
                            function roundRect(ctx, x, y, w, h, r) {
                                ctx.beginPath()
                                ctx.moveTo(x+r, y)
                                ctx.lineTo(x+w-r, y)
                                ctx.arcTo(x+w, y,   x+w, y+r,   r)
                                ctx.lineTo(x+w, y+h-r)
                                ctx.arcTo(x+w, y+h, x+w-r, y+h, r)
                                ctx.lineTo(x+r, y+h)
                                ctx.arcTo(x, y+h,   x, y+h-r,   r)
                                ctx.lineTo(x, y+r)
                                ctx.arcTo(x, y,     x+r, y,      r)
                                ctx.closePath()
                            }
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
                        model: ["sample-event.mp4 — 2.3 MB  ✓ Owned",
                                "march-footage.mp4 — 8.1 MB  ✓ Owned",
                                "clip-b.mp4 — 4.5 MB  ✓ Owned"]
                        Rectangle {
                            Layout.fillWidth: true; height: 44; color: "#2a2a2a"; radius: 6
                            Text { anchors.centerIn: parent; text: modelData; color: "#f0f0f0"; font.pixelSize: 12 }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ══ 3 — Settings Screen ════════════════════════════════════════
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
                            onClicked: { /* mock */ }
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
