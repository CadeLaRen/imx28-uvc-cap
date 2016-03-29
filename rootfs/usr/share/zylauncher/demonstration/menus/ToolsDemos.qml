import QtQuick 1.0
Item {
    width:  rootloader.viewWidth 
    height: rootloader.viewHeight
    //          界面标题内容
    //          将Text放置在resources属性中，是为了祖父辈控件调用方便
    resources : [
        Text { text:"tools demos" }
    ]
    children: [
        ProButton  {
                    gridX : 0; gridY : 0
                    imgPath: "../images/filebrower.png"; 
                    imgText : "  file browser"; 
                    execText : "../demos/tools/filebrower/filebrower"
        },
        ProButton  {
                    gridX : 1; gridY : 0
                    imgPath: "../images/netinfo.png"; 
                    imgText : "  netinfo"; 
                    execText : "../demos/tools/netinfo/netinfo"
        },
        ProButton  {
                    gridX : 2; gridY : 0
                    imgPath: "../images/rtc.png"; 
                    imgText : "      rtc"; 
                    execText : "../demos/tools/rtc/rtc"
        },
        ProButton  {
                    gridX : 3; gridY : 0
                    imgPath: "../images/serialport.png"; 
                    imgText : "serialport"; 
                    execText : "../demos/tools/serialport/serialport"
        },

        MenuButton {
                    gridX : 3; gridY : 1
                    imgPath: "../images/back.png";
                    imgText : "  MainMenu    ";
                    menuName : "./MainMenu.qml"
        }
    ]
}

