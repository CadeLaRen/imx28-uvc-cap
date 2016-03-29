/*
 *  author     : liujianpei
 *  Description: zylauncher演示框架
 */


import QtQuick 1.0
import "demofunc.js" as Logic
import "keyfunc.js"  as Key
import "config.js"   as Config

Image {
    id: mainBackground
    property alias argWidth: rootloader.viewWidth
    property alias argHeight: rootloader.viewHeight
    property alias titleHeight: title.height
    width : rootloader.viewWidth; height : rootloader.viewHeight;

    source : "../../demonstration/images/background.jpg"
    fillMode: Image.Tile


    //  标题区域
    Title {
        id : title 
        titleDrageEnable : true
    }
    //  flick区域
    Flickable {
        
        /*  下面语句可以用于重现文字消失的bug
            bug重现方式: 首先点击界面上某图标，启动任一Qt程序,再关闭此程序，回到宫格界面。
            静置界面一段时间，再拖动界面，下面的文字有很大的几率会消失。
            Qt库版本：4.7.3

        Text {text:"reproduce the text bug"; font.pointSize:32; color: "red";z: 1}

        */ 

        id : flick

        property int currentRowY
        property int toRowYDuration
        property int oldContentY
        property int shortFlick     // 1 ，此次flick耗时短于xxx秒

        y : rootloader.iconStartY
        width : rootloader.viewWidth; height : rootloader.viewHeight - rootloader.iconStartY
        maximumFlickVelocity : height
        flickDeceleration : 30000
        //  contentHeight 要根据界面中图标位置决定,具体值在config.js的indexToY中设置
        contentWidth : rootloader.viewWidth; contentHeight : 1 
        pressDelay : 80
        onMovementEnded : { Logic.rowSwitch() }
        onMovementStarted:{ Logic.rowSwitchStart() }

        Timer {
            id : timer 
            interval : 650
            onTriggered: {flick.shortFlick = 0}
        }

        focus : true
        Keys.onPressed :{
            if(event.key == Qt.Key_Left) {
                Key.focusLeft(); 
                event.accepted = true;
            } else if(event.key == Qt.Key_Right || event.key == Qt.Key_Tab) {
                Key.focusRight(); 
                event.accepted = true;
            } else if(event.key == Qt.Key_Up) {
                Key.focusUp();
                event.accepted = true;
            } else if (event.key == Qt.Key_Down) {
                Key.focusDown();
                event.accepted = true;
            } else if(event.key == Qt.Key_Return) { 
                Key.focusButtonPressed();
                event.accepted = true;
            } else {
                console.log("unsupport key"); 
            }
        }


        Loader {
            id : rootloader 
            property bool running
            //property bool titleDragenable
            //titleDragenable : true
            running : false
            property url  tmpSource

            
            //  保存演示界面的各类配置
            property int    viewWidth
            property int    viewHeight

            property int    titleFontSize
            titleFontSize : 8 
            property int    iconWidth
            property int    iconHeight
            property int    iconFontSize 
            iconFontSize  : 8
            property int    iconStartX 
            property int    iconStartY

            property int    iconSpinX
            property int    iconSpinY
            property string  titleName
            property int    totalNum
            property int    currentIcon

            onViewWidthChanged:   Config.resolutionChange()
            onViewHeightChanged:  Config.resolutionChange()

            source : "../../demonstration/menus/MainMenu.qml"
            onLoaded: { Key.init() }

            //onRunningChanged : {console.log("running change ",running)}

        }
    }
    


    //  添加滚动条效果
    ScrollBar {
        id: verticalScrollBar
        width: 12; height: flick.height-12
        anchors.right: flick.right
        anchors.top:   flick.top
        opacity: 0
        orientation: Qt.Vertical
        position: flick.visibleArea.yPosition
        pageSize: flick.visibleArea.heightRatio
    }
    states: State {
        name: "ShowBars"
        when: flick.movingVertically || flick.movingHorizontally
        PropertyChanges { target: verticalScrollBar; opacity: 1 }
        PropertyChanges { target: rootloader; running: true }
    }
    transitions: Transition {
        NumberAnimation { properties: "opacity"; duration: 200 }
    }
   
    //  切换Menu时的动画效果
    SequentialAnimation {
        id : switchMenuAnimaion
        PropertyAnimation {target : rootloader; property : "x"; from : 0; to : rootloader.viewWidth; easing.type : Easing.InBack; duration : 1100 }
        ScriptAction {  script:Logic.switchMenuScript();    }
        PropertyAnimation {target : rootloader; property : "x"; from : rootloader.viewWidth; to : 0; easing.type : Easing.OutBack; duration : 1100 }
        ScriptAction {  script:Logic.operationFinished();   }
    }

    //  用户flick界面后，界面自动对齐的动画效果
    SequentialAnimation { 
        id : switchRowAnimation
        PropertyAnimation {  target : flick; property : "contentY";  to : flick.currentRowY; easing.type : Easing.OutCirc; duration : flick.toRowYDuration}
        ScriptAction {  script: Logic.switchRowScript() }
    }
    // 用户使用键盘切换焦点，需要切换界面时的动画效果
    SequentialAnimation { 
        id : keyMoveAnimation
        PropertyAnimation {  target : flick; property : "contentY";  to : flick.currentRowY; easing.type : Easing.OutCirc; duration : flick.toRowYDuration}
        ScriptAction {  script: Key.focusChangeScript() }
    }
}
