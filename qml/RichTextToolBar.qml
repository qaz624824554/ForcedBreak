import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

// 富文本编辑工具栏：加粗、斜体、下划线、字号、颜色。
// 由使用方注入一个 RichTextFormatter 实例。
RowLayout {
    id: root

    required property var formatter

    spacing: 6

    component FormatButton: Button {
        Layout.preferredWidth: 36
        checkable: true
        font.pixelSize: 15
    }

    FormatButton {
        text: qsTr("B")
        font.bold: true
        checked: root.formatter.bold
        onClicked: root.formatter.toggleBold()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("加粗")
    }

    FormatButton {
        text: qsTr("I")
        font.italic: true
        checked: root.formatter.italic
        onClicked: root.formatter.toggleItalic()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("斜体")
    }

    FormatButton {
        text: qsTr("U")
        font.underline: true
        checked: root.formatter.underline
        onClicked: root.formatter.toggleUnderline()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("下划线")
    }

    Label { text: qsTr("字号") }

    ComboBox {
        id: sizeBox
        Layout.preferredWidth: 84
        editable: false
        model: [10, 12, 14, 16, 18, 24, 28, 32, 36, 48, 60, 72]
        // 跟随光标处的实际字号；-1 表示当前字号不在预设列表中
        currentIndex: model.indexOf(root.formatter.fontSize)
        onActivated: root.formatter.setFontSize(model[currentIndex])
    }

    Label { text: qsTr("颜色") }

    Button {
        Layout.preferredWidth: 42
        onClicked: colorDialog.open()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("文字颜色")

        background: Rectangle {
            radius: 3
            border.color: "#808080"
            border.width: 1
            color: root.formatter.textColor
        }
    }

    ColorDialog {
        id: colorDialog
        selectedColor: root.formatter.textColor
        onAccepted: root.formatter.setColor(selectedColor)
    }

    Item { Layout.fillWidth: true }
}
