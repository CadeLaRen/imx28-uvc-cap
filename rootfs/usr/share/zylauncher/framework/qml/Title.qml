/*
 *  author     : liujianpei
 *  Description: 演示界面的标题栏部分
 */
import QtQuick 1.0
import "./colibri"
import "drawerfunc.js"   as DrawerFunc
//  标题区域
Rectangle {
    id : title
    property alias titleDrageEnable : titleMouseArea.enabled
    property alias closeButtonVisible: closeButton.visible
    width : rootloader.viewWidth; height : rootloader.iconStartY - 3
    color : "black"
    z : 1


    CLButton {
        id : "closeButton"
        width : rootloader.iconStartY - 3 ; height : rootloader.iconStartY - 3
        x : parent.width - width
        text : "X"
        onClicked : { agent.closeCurrentPro() }
        visible : false
        z : 1
    }


    CLButton {
        id : "keyboardButton"
        signal openKeyboard()
        width  : height
        height : rootloader.iconStartY - 3
        z : 1
        text : ""
        backgroundImage : Image {
                                id : "bg"
                                width : keyboardButton.width 
                                height : keyboardButton.height
                                source: "../../demonstration/images/keyboard.png" 
                            }

        onClicked : { agent.openKeyboard() }
    }


    Rectangle {
        id : drawer
        width : parent.width; height : parent.y 
        y: 0 - parent.y
        z: 1
        color : "grey"



        Loader {
            id : drawerLoader
            property int loaderButtom : title.y
            property int loaderWidth  : rootloader.viewWidth
            property int loaderHeight : rootloader.viewHeight
            property url systemQml : "System.qml"
            z: 1
            source: systemQml
            onSourceChanged : { DrawerFunc.drawerDragEnable() }
        }

        Timer {
            id : operationTime 
            property bool shortOperation
            property int  oldY;
            interval : 1000 
            onTriggered: { operationTime.shortOperation = 0 }
        }
        MouseArea {
            anchors.fill : parent    
        }

        SequentialAnimation { 
            id : drawerDragButtomAnimation
            PropertyAnimation { target : title; property : "y";  to : rootloader.height - title.height; easing.type : Easing.OutCirc; duration : 100}
        }
        SequentialAnimation { 
            id : drawerDragTopAnimation
            PropertyAnimation { target : title; property : "y";  to : 0; easing.type : Easing.OutCirc; duration : 100}
            ScriptAction { script: DrawerFunc.drawerRestore() }
        }

    }

    ReadOnlyTextEdit{
        width : title.width
        height: title.height
        horizontalAlignment: TextEdit.AlignHCenter
        verticalAlignment: TextEdit.AlignVCenter
        text: rootloader.titleName 
        font.pointSize:rootloader.titleFontSize 
        color:"white"
    }

    MouseArea {
        id: titleMouseArea
        anchors.fill : parent
        drag.target: title
        drag.axis: Drag.YAxis
        drag.minimumY : 0
        drag.maximumY : mainBackground.height - title.height
        onPressed : { DrawerFunc.drawerPressed() }
        onReleased :{ DrawerFunc.slideDrawer() }
    }
}

