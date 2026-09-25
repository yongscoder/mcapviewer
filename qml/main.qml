import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.platform 1.1

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 820
    title: "MCAP Viewer"
    color: "#1e1e1e"

    property bool isPlaying: false

    FileDialog {
        id: fileDialog
        title: "Open MCAP file"
        nameFilters: ["MCAP files (*.mcap)", "All files (*)"]
        onAccepted: {
            root.isPlaying = false
            controller.openFile(currentFile.toString())
        }
    }

    Timer {
        id: playTimer
        interval: Math.max(16, controller.frameIntervalMs)
        repeat: true
        onTriggered: {
            if (controller.frameCount === 0) {
                root.isPlaying = false
                return
            }
            var next = controller.currentFrame + 1
            if (next >= controller.frameCount) next = 0
            controller.currentFrame = next
        }
    }

    // Keep the timer in sync with the play/pause state.
    onIsPlayingChanged: isPlaying ? playTimer.start() : playTimer.stop()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // ── Toolbar ─────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            height: 36
            spacing: 6

            Button {
                text: "Open…"
                onClicked: fileDialog.open()
                implicitHeight: 32
            }

            Label {
                text: "Topic:"
                color: "#ccc"
                verticalAlignment: Text.AlignVCenter
            }

            ComboBox {
                id: topicCombo
                Layout.fillWidth: true
                model: controller.topics
                implicitHeight: 32
                enabled: !controller.loading
                onCurrentTextChanged: {
                    if (currentText.length > 0)
                        controller.selectedTopic = currentText
                }
                Connections {
                    target: controller
                    onSelectedTopicChanged: {
                        var idx = controller.topics.indexOf(controller.selectedTopic)
                        if (idx >= 0) topicCombo.currentIndex = idx
                    }
                }
            }

            Label {
                text: controller.frameCount + " frames"
                color: "#aaa"
                verticalAlignment: Text.AlignVCenter
            }
        }

        // ── Image area ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#111"
            radius: 2

            Image {
                id: imgView
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: controller.frameSource
                cache: false
                smooth: true
                asynchronous: false
            }

            Label {
                anchors.centerIn: parent
                text: "Open an MCAP file to view images"
                color: "#555"
                font.pixelSize: 18
                visible: controller.frameSource.length === 0 && !controller.loading
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: controller.loading
                visible: controller.loading
            }
        }

        // ── Frame navigation ────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            height: 42
            spacing: 6
            enabled: controller.frameCount > 0 && !controller.loading

            // Previous frame
            Button {
                text: "◀"
                implicitWidth: 40
                implicitHeight: 34
                onClicked: {
                    root.isPlaying = false
                    controller.currentFrame = Math.max(0, controller.currentFrame - 1)
                }
            }

            // Play / Pause
            Button {
                implicitWidth: 52
                implicitHeight: 34
                text: root.isPlaying ? "⏸ Pause" : "▶ Play"
                onClicked: root.isPlaying = !root.isPlaying
            }

            // Next frame
            Button {
                text: "▶"
                implicitWidth: 40
                implicitHeight: 34
                onClicked: {
                    root.isPlaying = false
                    controller.currentFrame = Math.min(controller.frameCount - 1,
                                                       controller.currentFrame + 1)
                }
            }

            Slider {
                id: frameSlider
                Layout.fillWidth: true
                from: 0
                to: Math.max(0, controller.frameCount - 1)
                stepSize: 1
                value: controller.currentFrame
                snapMode: Slider.SnapAlways

                onMoved: {
                    root.isPlaying = false
                    controller.currentFrame = Math.round(value)
                }

                Connections {
                    target: controller
                    onCurrentFrameChanged: {
                        if (Math.round(frameSlider.value) !== controller.currentFrame)
                            frameSlider.value = controller.currentFrame
                    }
                }
            }

            Label {
                text: (controller.currentFrame + 1) + " / " + controller.frameCount
                color: "#ccc"
                horizontalAlignment: Text.AlignRight
                Layout.minimumWidth: 80
                verticalAlignment: Text.AlignVCenter
            }
        }

        // ── Status bar ──────────────────────────────────────────
        Label {
            Layout.fillWidth: true
            text: controller.statusText
            color: "#888"
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }

    // Keyboard shortcuts
    Shortcut { sequence: "Space";   onActivated: root.isPlaying = !root.isPlaying }
    Shortcut { sequence: "Left";    onActivated: { root.isPlaying = false; controller.currentFrame = Math.max(0, controller.currentFrame - 1) } }
    Shortcut { sequence: "Right";   onActivated: { root.isPlaying = false; controller.currentFrame = Math.min(controller.frameCount - 1, controller.currentFrame + 1) } }
    Shortcut { sequence: "Ctrl+O";  onActivated: fileDialog.open() }
}
