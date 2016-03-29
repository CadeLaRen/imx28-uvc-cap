import QtQuick 1.0
Item {
    width:  rootloader.viewWidth
    height: rootloader.viewHeight
    //          界面标题内容
    //          将Text放置在resources属性中，是为了祖父辈控件调用方便
    resources : [
        Text { text:"qml menu" }
    ]
    children : [
        ProButton  {
                    gridX : 0;  gridY : 0
                    imgPath: "../images/button2.png"; 
                    imgText : "progressbar"; 
                    execText : "../demos/qml/progressbar/start-progressbar"
        },
        ProButton  {
                    gridX : 1;  gridY : 0
                    imgPath: "../images/button3.png"; 
                    imgText : "dialcontrol"; 
                    execText : "../demos/qml/dialcontrol/start-dialcontrol"
        },
        ProButton  {
                    gridX : 2;  gridY : 0
                    imgPath: "../images/button4.png"; 
                    imgText : "tabwidget"; 
                    execText : "../demos/qml/tabwidget/start-tabwidget"
        },
        ProButton  {
                    gridX : 3;  gridY : 0
                    imgPath: "../images/button5.png"; 
                    imgText : "spinner"; 
                    execText : "../demos/qml/spinner/start-spinner"
        },
        MenuButton {
                    gridX : 3;  gridY : 1
                    imgPath: "../images/button6.png";
                    imgText : "MainMenu    "; 
                    menuName : "./MainMenu.qml"
        } 
    ]
}

