import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Controls.AbstractButton {
    id: card
    required property string gameTitle
    required property url coverSource
    required property bool selected
    readonly property int coverStatus: cover.status
    activeFocusOnTab: true
    padding: 8
    implicitWidth: 180
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    Accessible.role: Accessible.RadioButton
    Accessible.name: gameTitle
    Accessible.checkable: true
    Accessible.checked: selected
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    background: Rectangle {
        radius: 8
        color: card.selected || card.hovered
            ? Kirigami.Theme.alternateBackgroundColor : "transparent"
        border.width: card.selected || card.visualFocus ? 2 : 1
        border.color: card.selected || card.visualFocus
            ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.18)
    }
    contentItem: ColumnLayout {
        spacing: 8
        opacity: card.enabled ? 1 : 0.55
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 508 / 360
            Image {
                id: cover
                anchors.fill: parent
                source: card.coverSource
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 360
                smooth: true
            }
            Controls.Label {
                objectName: "coverFallback"
                anchors.fill: parent
                visible: cover.status !== Image.Ready
                text: card.gameTitle
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                font.bold: true
                font.pixelSize: 24
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Controls.Label {
                text: card.gameTitle
                Layout.fillWidth: true
                Layout.minimumHeight: font.pixelSize * 2.5
                wrapMode: Text.WordWrap
                font.bold: card.selected
                verticalAlignment: Text.AlignVCenter
            }
            Kirigami.Icon {
                source: "emblem-ok-symbolic"
                visible: card.selected
                implicitWidth: 20
                implicitHeight: 20
            }
        }
    }
}
