import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Controls.Pane {
    id: panel
    required property QtObject settings
    required property QtObject displays
    readonly property int selectedRow: displays.rowForMonitor(settings.values.monitor)
    function syncMonitor(): void {
        if (selectedRow >= 0) settings.setValue("monitor", displays.rows[selectedRow].monitor)
    }
    Component.onCompleted: syncMonitor()
    Connections { target: panel.displays; function onChanged() { panel.syncMonitor() } }
    padding: 20
    background: Rectangle {
        color: Kirigami.Theme.alternateBackgroundColor
        radius: 8
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
    }
    contentItem: GridLayout {
        objectName: "configurationGrid"
        columns: panel.width >= 700 ? 2 : 1
        columnSpacing: 32
        rowSpacing: 20
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 400
            spacing: 12
            Controls.Label { text: qsTr("Monitor") }
            Controls.ComboBox {
                objectName: "monitorSelector"
                Layout.fillWidth: true
                model: panel.displays.rows
                textRole: "label"
                currentIndex: panel.selectedRow
                enabled: count > 0
                Accessible.name: qsTr("Game monitor")
                onActivated: (index) => panel.settings.setValue("monitor", panel.displays.rows[index].monitor)
            }
            Controls.Label {
                objectName: "desktopModeInfo"
                text: panel.selectedRow >= 0 ? panel.displays.rows[panel.selectedRow].mode : qsTr("No connected displays detected")
                opacity: 0.7
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Controls.Label { text: qsTr("Fullscreen"); Layout.fillWidth: true }
                Controls.Switch {
                    objectName: "fullscreenSwitch"
                    checked: panel.settings.values.fullscreen
                    Accessible.name: qsTr("Start fullscreen")
                    onToggled: panel.settings.setValue("fullscreen", checked)
                }
            }
            Controls.Label {
                visible: panel.displays.error.length > 0
                text: panel.displays.error
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                color: Kirigami.Theme.negativeTextColor
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 400
            spacing: 12
            Controls.Label { text: qsTr("Render scale") }
            Controls.ComboBox {
                objectName: "renderScaleSelector"
                Layout.fillWidth: true
                model: ["1×", "2×", "3×", "4×"]
                currentIndex: panel.settings.values.scale - 1
                Accessible.name: qsTr("Internal render scale")
                onActivated: (index) => panel.settings.setValue("scale", index + 1)
            }
        }
    }
}
