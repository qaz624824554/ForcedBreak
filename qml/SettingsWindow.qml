import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ForcedBreak

// 设置窗口：休息时间 / 显示文案 / 解锁密码三个 Tab。
// 三个托盘菜单项打开的都是本窗口，只是切换 currentTab。
ApplicationWindow {
    id: root

    // 由 SettingsWindowManager 设置，对应 TabBar 的序号
    property alias currentTab: tabBar.currentIndex

    width: 640
    height: 520
    minimumWidth: 560
    minimumHeight: 440
    title: qsTr("ForcedBreak 设置")
    visible: false

    onClosing: statusText.text = ""

    header: TabBar {
        id: tabBar
        TabButton { text: qsTr("休息时间") }
        TabButton { text: qsTr("显示文案") }
        TabButton { text: qsTr("解锁密码") }
    }

    footer: Label {
        id: statusText
        padding: 10
        color: "#2d7dd2"
        text: ""
    }

    // 统一的操作反馈，3 秒后自动清除
    function notify(message) {
        statusText.text = message
        statusTimer.restart()
    }

    Timer {
        id: statusTimer
        interval: 3000
        onTriggered: statusText.text = ""
    }

    StackLayout {
        anchors.fill: parent
        anchors.margins: 16
        currentIndex: tabBar.currentIndex

        // ---------- 休息时间 ----------
        ColumnLayout {
            spacing: 16

            GridLayout {
                columns: 3
                columnSpacing: 12
                rowSpacing: 12

                Label { text: qsTr("工作时长") }
                SpinBox {
                    id: workSpin
                    from: 1
                    to: 480
                    value: AppSettings.workMinutes
                    editable: true
                    onValueModified: AppSettings.workMinutes = value
                }
                Label { text: qsTr("分钟（1–480）") }

                Label { text: qsTr("休息时长") }
                SpinBox {
                    id: breakSpin
                    from: 5
                    to: 3600
                    value: AppSettings.breakSeconds
                    editable: true
                    onValueModified: AppSettings.breakSeconds = value
                }
                Label { text: qsTr("秒（5–3600）") }

                Label { text: qsTr("提前提醒") }
                SpinBox {
                    id: preNotifySpin
                    from: 0
                    to: 3600
                    value: AppSettings.preNotifySeconds
                    editable: true
                    onValueModified: AppSettings.preNotifySeconds = value
                }
                Label { text: qsTr("秒（0 表示不提醒）") }
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: "#777777"
                // 两句分别 qsTr：qsTr 的参数须为字符串字面量，拼接要放在外面
                text: qsTr("修改工作时长会立即按新时长重新计算当前周期的剩余时间。") + "\n"
                      + qsTr("提前提醒在距离休息还剩指定秒数时弹出一次托盘通知；提前量不小于工作时长时不提醒。")
            }

            Item { Layout.fillHeight: true }
        }

        // ---------- 显示文案 ----------
        ColumnLayout {
            spacing: 12

            RichTextFormatter {
                id: msgFormatter
                document: messageArea.textDocument
                cursorPosition: messageArea.cursorPosition
                selectionStart: messageArea.selectionStart
                selectionEnd: messageArea.selectionEnd
            }

            RichTextToolBar {
                Layout.fillWidth: true
                formatter: msgFormatter
            }

            // 编辑区用黑底白字，与遮罩上的实际呈现保持一致
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "black"
                border.color: "#3a3a3a"
                border.width: 1

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true

                    TextArea {
                        id: messageArea
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        color: "white"
                        selectByMouse: true
                        text: AppSettings.messageHtml
                        background: null
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("恢复默认文案")
                    onClicked: {
                        messageArea.text = AppSettings.defaultMessage()
                        root.notify(qsTr("已载入默认文案，点击保存后生效。"))
                    }
                }
                Button {
                    text: qsTr("保存")
                    onClicked: {
                        // RichText 模式下 TextArea.text 即为带格式的 HTML
                        AppSettings.messageHtml = messageArea.text
                        root.notify(qsTr("文案已保存。"))
                    }
                }
            }
        }

        // ---------- 解锁密码 ----------
        ColumnLayout {
            spacing: 16

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 12

                Label { text: qsTr("旧密码") }
                TextField {
                    id: oldPassword
                    Layout.preferredWidth: 240
                    echoMode: TextInput.Password
                }

                Label { text: qsTr("新密码") }
                TextField {
                    id: newPassword
                    Layout.preferredWidth: 240
                    echoMode: TextInput.Password
                }

                Label { text: qsTr("确认新密码") }
                TextField {
                    id: confirmPassword
                    Layout.preferredWidth: 240
                    echoMode: TextInput.Password
                    onAccepted: savePasswordButton.clicked()
                }
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: "#777777"
                text: qsTr("解锁密码仅用于提前结束休息；退出程序和打开本设置页都不需要密码。")
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }

                Button {
                    id: savePasswordButton
                    text: qsTr("保存新密码")
                    onClicked: {
                        if (newPassword.text.length === 0) {
                            root.notify(qsTr("新密码不能为空。"))
                        } else if (newPassword.text !== confirmPassword.text) {
                            root.notify(qsTr("两次输入的新密码不一致。"))
                        } else if (!AppSettings.changePassword(oldPassword.text, newPassword.text)) {
                            root.notify(qsTr("旧密码错误，密码未修改。"))
                        } else {
                            oldPassword.text = ""
                            newPassword.text = ""
                            confirmPassword.text = ""
                            root.notify(qsTr("密码已更新。"))
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
