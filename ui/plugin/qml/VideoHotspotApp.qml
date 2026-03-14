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
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    title: "Video Hotspot"

    // Dark palette
    palette.window:      "#1a1a1a"
    palette.windowText:  "#f0f0f0"
    palette.base:        "#2a2a2a"
    palette.text:        "#f0f0f0"
    palette.button:      "#3a3a3a"
    palette.buttonText:  "#f0f0f0"
    palette.highlight:   "#4a90d9"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton { text: "Upload" }
            TabButton { text: "Map" }
            TabButton { text: "Downloads" }
            TabButton { text: "Settings" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Placeholder screens — replace with proper components
            Item { Label { anchors.centerIn: parent; text: "Upload Screen — TODO" } }
            Item { Label { anchors.centerIn: parent; text: "Map Screen — TODO" } }
            Item { Label { anchors.centerIn: parent; text: "Downloads/Cache Screen — TODO" } }
            Item { Label { anchors.centerIn: parent; text: "Settings Screen — TODO" } }
        }
    }
}
