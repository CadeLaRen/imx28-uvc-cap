/*
 *  author     : liujianpei
 *  Description:
 */

import ProLauncher 1.0
import QtQuick 1.0
import "demofunc.js" as Logic

Item {
    id : button

    property alias imgPath: img.source
    property alias imgText: txt.text
    property string execText

    property int    gridX
    property int    gridY
    property int    order

    signal buttonPressed()
    onButtonPressed: Logic.proStart(execText)

    x : Logic.indexToX(gridX)
    y : Logic.indexToY(gridY)

    width: rootloader.iconWidth + 10; height: rootloader.iconHeight + 10
    Launcher {
        id: launcher
        onProEnd: Logic.operationFinished()
    }

    Rectangle {
        id: iconBackGround 
        radius: 5
        anchors.fill : parent
        opacity : 0
        SequentialAnimation {
            id : proStartAnimation 
            PropertyAnimation {target : iconBackGround; property : "scale"; to : 1.2; duration : 80}
            PropertyAnimation {target : iconBackGround; property : "scale"; to : 1;   duration : 80}
        }
    }

    Image {
        id: img
        anchors.centerIn : parent
        width:rootloader.iconWidth; height:rootloader.iconHeight
        sourceSize.width:rootloader.iconWidth; sourceSize.height:rootloader.iconHeight
        MouseArea {
            id:mouseArea
            anchors.fill: parent  
            onPressed: Logic.proStart(execText)
        }
        states: [
            State {
                name: "focusButton"
                when: focus == true
                PropertyChanges {target: iconBackGround; opacity : 0.25}
            }

        ]
    }
    ReadOnlyTextEdit{
        id: txt
        color: "#FFFFFF"
        font.pointSize : rootloader.iconFontSize
        anchors.top:  parent.bottom
        anchors.left: parent.left
        anchors.right:parent.right
        anchors.topMargin : 5
    } 

}
