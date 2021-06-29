import QtQuick 2.15
import QtQuick.Controls 2.15
import untitled 1.0

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

    Text {
        text: qsTr("Hello BinderJet")
        anchors.verticalCenterOffset: -318
        anchors.horizontalCenterOffset: -497
        anchors.centerIn: parent
        font.family: Constants.font.family
    }

    RoundButton {
        id: positiveXButton
        x: 226
        y: 185
        text: "^"
    }
}
