import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window
import ForcedBreak

// 单块屏幕的全屏遮罩：纯黑背景、居中富文本文案、倒计时、右下角"跳过"按钮。
// 窗口的 screen 与 geometry 由 OverlayController 在 C++ 侧设定。
Window {
    id: overlay

    // Qt.Tool 使窗口不出现在任务栏和 Alt+Tab 列表中，
    // 避免多块遮罩之间互相干扰焦点
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "black"
    visible: false

    // 倒计时格式化为 mm:ss
    readonly property string remainText: {
        const total = BreakScheduler.breakRemainingSeconds
        const mm = Math.floor(total / 60)
        const ss = total % 60
        return (mm < 10 ? "0" : "") + mm + ":" + (ss < 10 ? "0" : "") + ss
    }

    Item {
        anchors.fill: parent

        Column {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.8, 1200)
            spacing: 48

            Text {
                width: parent.width
                text: AppSettings.messageHtml
                textFormat: Text.RichText
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: "white"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: overlay.remainText
                color: "#8a8a8a"
                font.pixelSize: 40
                font.family: "Consolas"
            }
        }

        // 右下角低调的跳过入口
        Button {
            id: skipButton
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 24
            visible: !unlockPanel.visible
            text: qsTr("跳过")
            flat: true
            opacity: hovered ? 0.9 : 0.35
            onClicked: {
                unlockPanel.visible = true
                passwordField.text = ""
                passwordField.forceActiveFocus()
            }

            contentItem: Text {
                text: skipButton.text
                color: "white"
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: "transparent"
                border.color: "#666666"
                border.width: 1
                radius: 4
                implicitWidth: 88
                implicitHeight: 34
            }
        }

        // 密码解锁面板
        Rectangle {
            id: unlockPanel
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 24
            visible: false
            width: 320
            height: 120
            radius: 6
            color: "#1a1a1a"
            border.color: errorShake.running ? "#c0392b" : "#3a3a3a"
            border.width: 1

            // 万一窗口仍被抢过一次焦点（如显示器热插拔），激活后把焦点还给输入框
            Connections {
                target: overlay
                function onActiveChanged() {
                    if (overlay.active && unlockPanel.visible)
                        passwordField.forceActiveFocus()
                }
            }

            // 输错密码时的抖动提示
            SequentialAnimation {
                id: errorShake
                loops: 3
                NumberAnimation { target: unlockPanel; property: "anchors.rightMargin"; to: 34; duration: 45 }
                NumberAnimation { target: unlockPanel; property: "anchors.rightMargin"; to: 14; duration: 45 }
                onFinished: unlockPanel.anchors.rightMargin = 24
            }

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: qsTr("输入解锁密码")
                    color: "#cccccc"
                    font.pixelSize: 14
                }

                TextField {
                    id: passwordField
                    width: parent.width
                    echoMode: TextInput.Password
                    color: "white"
                    background: Rectangle {
                        color: "#262626"
                        border.color: "#4a4a4a"
                        radius: 3
                    }
                    onAccepted: unlockPanel.tryUnlock()
                }

                Row {
                    anchors.right: parent.right
                    spacing: 8

                    Button {
                        text: qsTr("取消")
                        onClicked: unlockPanel.visible = false
                    }
                    Button {
                        text: qsTr("解锁")
                        onClicked: unlockPanel.tryUnlock()
                    }
                }
            }

            function tryUnlock() {
                if (AppSettings.verifyPassword(passwordField.text)) {
                    // 解锁即本次休息作废，由 BreakScheduler 重新开始完整工作周期
                    BreakScheduler.unlock()
                } else {
                    passwordField.text = ""
                    errorShake.restart()
                }
            }
        }
    }
}
