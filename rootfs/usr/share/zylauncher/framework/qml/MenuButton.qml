/*
 *  author     : liujianpei
 *  Description:
 */
import QtQuick 1.0
import "demofunc.js" as Logic

Item {
    id : button

    property alias imgPath: img.source
    property alias imgText: txt.text
    property string menuName

    property int    gridX
    property int    gridY
    property int    order

    signal buttonPressed()
    onButtonPressed: Logic.menuSwitch(menuName)

    x : Logic.indexToX(gridX)
    y : Logic.indexToY(gridY)

    width: rootloader.iconWidth + 10; height: rootloader.iconHeight + 10

    Rectangle {
        id: iconBackGround 
        radius: 5
        anchors.fill : parent
        opacity : 0
    }

    Image {
        id: img
        anchors.centerIn : parent
        width:rootloader.iconWidth; height:rootloader.iconHeight
        sourceSize.width:rootloader.iconWidth; sourceSize.height:rootloader.iconHeight
        MouseArea {
            id:mouseArea
            anchors.fill: parent  
            onPressed: Logic.menuSwitch(menuName)
        }
        states: [
            State {
                name : "focusButton" 
                when : focus == true
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
