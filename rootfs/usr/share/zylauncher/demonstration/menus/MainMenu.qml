import QtQuick 1.0
/*
 *  简单使用说明：在界面中，有三种按钮.
 *  程序按钮：ProButton .   作用:启动一个程序
 *  菜单按钮：MenuButton.   作用:启动一个菜单
 *  退出按钮: ExitButton.   作用:退出zylauncher
 *  
 *  qml文件中使用的 "相对路径" 基准为 "qml文件所在路径"
 *
 */

Item {
    width:  rootloader.viewWidth
    height: rootloader.viewHeight
    //          界面标题内容
    //          将Text放置在resources属性中，是为了祖父辈控件调用方便
    resources : [
        Text { text:"main menu" }
    ]
    children : [
        //          第一行图标
        ProButton  {
                    gridX : 0; gridY : 0
                    imgPath: "../images/filebrower.png"; 
                    imgText : "  file browser"; 
                    execText : "../demos/tools/filebrowser/filebrowser"
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
        //          第二行图标
        ProButton {
                    gridX : 0; gridY : 1
                    imgPath: "../images/sysMonitor.png"; 
                    imgText : "sysMonitor";   
                    execText : "../demos/sysMonitor/sysMonitor"
        },
        ProButton  {
                    gridX : 1; gridY : 1
                    imgPath: "../images/cookie_1.png"; 
                    imgText : "hello app "; 
                    execText : "../demos/small_demos/hello/start-hello"
        },
        ProButton  {
                    gridX : 2; gridY : 1
                    imgPath: "../images/cookie_2.png"; 
                    imgText : " socketcan"; 
                    execText : "../demos/tools/socketcan/socketcan"
        },

        ProButton  {
                    gridX : 3; gridY : 1
                    imgPath: "../images/cookie_4.png"; 
                    imgText : "sliders"; 
                    execText : "../demos/small_demos/sliders/sliders"
        },
        //          第三行图标
        ProButton  {
                    gridX : 0;  gridY : 2
                    imgPath: "../images/button4.png"; 
                    imgText : "tabwidget"; 
                    execText : "../demos/qml/tabwidget/start-tabwidget"
        },
        ProButton  {
                    gridX : 1;  gridY : 2
                    imgPath: "../images/button5.png"; 
                    imgText : "spinner"; 
                    execText : "../demos/qml/spinner/start-spinner"
        },
        ProButton {
                    gridX : 2; gridY : 2
                    imgPath: "../images/qt.png";        
                    imgText : "pixelator";  
                    execText : "../demos/pixelator/pixelator"
        },
        ProButton {
                    gridX : 3; gridY : 2
                    imgPath: "../images/behavior.png";   
                    imgText : "behaviors";    
                    execText : "../demos/behaviors/start-behaviors"
        },
        //          第四行图标
        ProButton {
                    gridX : 0; gridY : 3
                    imgPath: "../images/stylesheet.png"; 
                    imgText : "style sheet";  
                    execText : "../demos/stylesheet/stylesheet"
        },
        ProButton  {
                    gridX : 1;  gridY : 3
                    imgPath: "../images/button3.png"; 
                    imgText : "dialcontrol"; 
                    execText : "../demos/qml/dialcontrol/start-dialcontrol"
        },
        ProButton {
                    gridX : 2; gridY : 3
                    imgPath: "../images/note.png";       
                    imgText : "dockWidgets";  
                    execText : "../demos/dockwidgets/dockwidgets"
        },
        ProButton {
                    gridX : 3; gridY : 3
                    imgPath: "../images/p5.png";       
                    imgText : "easing";  
                    execText : "../demos/easing/start-easing"
        },

        //          第五行图标
        ProButton {
                    gridX : 0; gridY : 4
                    imgPath: "../images/p4.png";       
                    imgText : "states";  
                    execText : "../demos/states/start-states"
        },
        ProButton {
                    gridX : 1; gridY : 4
                    imgPath: "../images/p3.png";       
                    imgText : "positioners";  
                    execText : "../demos/positioners/start-positioners"
        },
        ProButton {
                    gridX : 2; gridY : 4
                    imgPath: "../images/p2.png";       
                    imgText : "appchooser";  
                    execText : "../demos/appchooser/appchooser"
        },
        ProButton {
                    gridX : 3; gridY : 4
                    imgPath: "../images/p1.png";       
                    imgText : "tooltips";  
                    execText : "../demos/tooltips/tooltips"
        },
        //          第六行图标
        ProButton  {
                    gridX : 0;  gridY : 5
                    imgPath: "../images/button2.png"; 
                    imgText : "progressbar"; 
                    execText : "../demos/qml/progressbar/start-progressbar"
        }
    ]
}
