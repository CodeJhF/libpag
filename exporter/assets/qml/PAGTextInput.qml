import QtQuick
import QtQuick.Controls

Rectangle {
    id: main

    property string displayText: ""

    signal editingFinish(string text)

    implicitWidth: 100
    implicitHeight: 30

    Text {
        id: showText
        text: displayText
        font.pixelSize: 14
        font.family: "PingFang SC"
        color: parent.enabled ? "white" : "gray"
        visible: !isInEditing()
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        clip: true
    }

    TextInput {
        id: textInput
        text: displayText
        font.pixelSize: 14
        font.family: "PingFang SC"
        color: "white"
        visible: isInEditing()
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        renderType: Text.NativeRendering
        selectByMouse: true
        clip: true

        onEditingFinished: {
            main.displayText = textInput.text;
            editingFinish(textInput.text);
            showText.text = main.displayText;
        }

        onFocusChanged: function (focus) {
            if (focus || (textInput.text.length === 0)) {
                textInput.text = showText.text;
                textInput.focus = focus;
                showText.focus = !focus;
            }
        }

        Keys.onReturnPressed: {
            focus = false;
        }

        Keys.onEnterPressed: {
            focus = false;
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        onPressed: function (mouse) {
            textInput.focus = true;
            textInput.forceActiveFocus();
            showText.focus = false;

            textInput.cursorPosition = textInput.positionAt(mouse.x - textInput.anchors.leftMargin, mouse.y - (textInput.height - textInput.contentHeight) / 2);
        }
    }

    function isInEditing() {
        return textInput.activeFocus || main.activeFocus;
    }
}
