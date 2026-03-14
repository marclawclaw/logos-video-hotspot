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
                            text: "📂  Select File"
                            background: Rectangle { color: "#3a3a3a"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#f0f0f0"; horizontalAlignment: Text.AlignHCenter }
                        }
                        Button {
                            text: "📁  Select Folder"
                            background: Rectangle { color: "#3a3a3a"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#f0f0f0"; horizontalAlignment: Text.AlignHCenter }
                        }
                        Button {
                            text: "👁  Monitor Folder"
                            background: Rectangle { color: "#2a5a2a"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#aaffaa"; horizontalAlignment: Text.AlignHCenter }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#333333"
                    }

                    Text {
                        text: "Upload Queue"
                        color: "#aaaaaa"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 60
                        color: "#2a2a2a"
                        radius: 6
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            Text { text: "sample-event-footage.mp4"; color: "#f0f0f0"; font.pixelSize: 13 }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 120; height: 20; radius: 3
                                color: "#333333"
                                Rectangle { width: parent.width * 0.72; height: parent.height; radius: 3; color: "#4a90d9" }
                            }
                            Text { text: "72%"; color: "#4a90d9"; font.pixelSize: 12 }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 60
                        color: "#2a2a2a"
                        radius: 6
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            Text { text: "protest-march-clip2.mp4"; color: "#f0f0f0"; font.pixelSize: 13 }
                            Item { Layout.fillWidth: true }
                            Text { text: "✓ Already uploaded"; color: "#ffaa44"; font.pixelSize: 12 }
                        }
                    }

                    Item { Layout.fillHeight: true }
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
                                Layout.fillWidth: true
                                from: 0; to: 1; value: 0.6
                                background: Rectangle {
                                    height: 4; radius: 2
                                    color: "#333333"
                                    Rectangle { width: parent.width * 0.6; height: parent.height; radius: 2; color: "#4a90d9" }
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
                        Slider { from: 1; to: 50; value: 10; Layout.fillWidth: true }
                        Text { text: "10 GB"; color: "#4a90d9"; font.pixelSize: 13 }
                    }
                    RowLayout {
                        Text { text: "Monitor folder:"; color: "#aaaaaa"; font.pixelSize: 13; Layout.preferredWidth: 160 }
                        Text { text: "~/Videos/Hotspot"; color: "#f0f0f0"; font.pixelSize: 13 }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Change"
                            background: Rectangle { color: "#3a3a3a"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#f0f0f0"; horizontalAlignment: Text.AlignHCenter }
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
