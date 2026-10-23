import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    objectName: "playPage"
    required property QtObject game
    required property QtObject settings
    required property QtObject displays
    property QtObject covers: null
    property string folderDraft: settings.values.root
    property string observedRoot: ""
    property string observedGame: settings.gameId
    property string folderError: ""
    Component.onCompleted: observedRoot = settings.values.root
    Connections {
        target: page.settings
        function onChanged() {
            if (page.observedRoot !== page.settings.values.root || page.observedGame !== page.settings.gameId) {
                page.observedGame = page.settings.gameId
                page.observedRoot = page.settings.values.root
                page.folderDraft = page.observedRoot
                page.folderError = ""
            }
        }
    }
    function commitFolder(): bool {
        if (!settings.setValue("root", folderDraft)) {
            folderError = qsTr("Could not save the game folder. Use an absolute path and check that settings are writable.")
            return false
        }
        folderError = ""
        return true
    }
    title: qsTr("Play")
    padding: 24
    ColumnLayout {
        spacing: 16
        Controls.Label { text: qsTr("Game") }
        RowLayout {
            Layout.fillWidth: true
            spacing: 20
            Item { Layout.fillWidth: true }
            CoverCard {
                id: tekken6Cover
                objectName: "tekken6Cover"
                Layout.preferredWidth: 180
                gameTitle: "Tekken 6"
                coverSource: page.covers ? page.covers.tekken6 : ""
                selected: page.settings.gameId === "tekken6"
                enabled: !page.game.running
                onClicked: page.settings.selectGame("tekken6")
                Keys.onRightPressed: {
                    tagCover.forceActiveFocus()
                    page.settings.selectGame("ttt2")
                }
            }
            CoverCard {
                id: tagCover
                objectName: "ttt2Cover"
                Layout.preferredWidth: 180
                gameTitle: "Tekken Tag Tournament 2"
                coverSource: page.covers ? page.covers.ttt2 : ""
                selected: page.settings.gameId === "ttt2"
                enabled: !page.game.running
                onClicked: page.settings.selectGame("ttt2")
                Keys.onLeftPressed: {
                    tekken6Cover.forceActiveFocus()
                    page.settings.selectGame("tekken6")
                }
            }
            Item { Layout.fillWidth: true }
        }
        Controls.Label { text: qsTr("Location of %1").arg(page.settings.gameTitle) }
        RowLayout {
            Layout.fillWidth: true
            Controls.TextField {
                objectName: "gameFolder"
                text: page.folderDraft
                Layout.fillWidth: true
                selectByMouse: true
                Accessible.name: qsTr("Game folder")
                onTextEdited: { page.folderDraft = text; page.folderError = "" }
                onEditingFinished: page.commitFolder()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: text
            }
            Controls.Button { text: qsTr("Browse…"); onClicked: folderDialog.open() }
        }
        SettingsPanel {
            objectName: "launchSettingsPanel"
            settings: page.settings
            displays: page.displays
            Layout.fillWidth: true
        }
        Kirigami.InlineMessage {
            visible: page.folderError.length > 0 || page.game.error.length > 0
            text: page.folderError || page.game.error
            type: Kirigami.MessageType.Error
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.fillWidth: true
            Controls.Label {
                visible: page.game.status === "Starting" || page.game.status === "Stopping"
                text: page.game.status
            }
            Item { Layout.fillWidth: true }
            Controls.Button {
                objectName: "playButton"
                text: page.game.running ? qsTr("Stop game") : qsTr("Play")
                icon.name: page.game.running ? "media-playback-stop" : "media-playback-start"
                highlighted: !page.game.running
                Accessible.defaultButton: page.visible && !page.game.running
                implicitHeight: 44
                Layout.preferredWidth: 220
                onClicked: {
                    if (page.game.running) stopDialog.open()
                    else if (page.commitFolder()) { page.displays.refresh(); page.game.start() }
                }
            }
        }
    }
    FolderDialog {
        id: folderDialog
        title: qsTr("Choose the folder containing the native %1 executable").arg(page.settings.executableName)
        onAccepted: page.settings.setValue("root", selectedFolder)
    }
    Controls.Dialog {
        id: stopDialog
        title: qsTr("Stop the game?")
        modal: true
        anchors.centerIn: parent
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No
        onAccepted: page.game.stop()
    }
}
