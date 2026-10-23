import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    required property QtObject settings
    required property QtObject game
    required property QtObject displays
    property QtObject covers: null
    property string settingsError: ""
    Connections {
        target: root.settings
        function onError(message) { root.settingsError = message }
        function onChanged() { root.settingsError = "" }
    }
    width: 960
    height: 840
    minimumWidth: 640
    minimumHeight: 480
    title: qsTr("Tekken — Native Linux")
    header: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 24
            Kirigami.Heading { text: qsTr("TEKKEN"); level: 1; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 2; font.letterSpacing: 2 }
            Item { Layout.fillWidth: true }
            Controls.Label { text: qsTr("Native Linux"); opacity: 0.6 }
        }
        Kirigami.Separator { Layout.fillWidth: true }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.settingsError.length > 0
            text: root.settingsError
            type: Kirigami.MessageType.Error
        }
    }
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.initialPage: PlayPage {
        game: root.game
        settings: root.settings
        displays: root.displays
        covers: root.covers
    }
    onClosing: function(close) {
        if (root.game.running) {
            close.accepted = false
            quitDialog.open()
        }
    }
    Controls.Dialog {
        id: quitDialog
        title: qsTr("Stop the running game and quit?")
        anchors.centerIn: parent
        modal: true
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No
        onAccepted: { root.game.stop(); Qt.quit() }
    }
}
