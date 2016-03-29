import QtQuick 1.0
Item {
    width:  rootloader.viewWidth 
    height: rootloader.viewHeight
    //          界面标题内容
    //          将Text放置在resources属性中，是为了祖父辈控件调用方便
    resources : [
        Text { text:"small demos" }
    ]
    children: [
        ProButton  {
                    gridX : 0; gridY : 0
                    imgPath: "../images/cookie_1.png"; 
                    imgText : "hello app "; 
                    execText : "../demos/small_demos/hello/start-hello"
        },
        ProButton  {
                    gridX : 1; gridY : 0
                    imgPath: "../images/cookie_2.png"; 
                    imgText : "decoration"; 
                    execText : "../demos/small_demos/simpledecoration/simpledecoration"
        },
        ProButton  {
                    gridX : 2; gridY : 0
                    imgPath: "../images/cookie_3.png"; 
                    imgText : "digital clock"; 
                    execText : "../demos/small_demos/digitalclock/digitalclock"
        },
        ProButton  {
                    gridX : 3; gridY : 0
                    imgPath: "../images/cookie_4.png"; 
                    imgText : "sliders"; 
                    execText : "../demos/small_demos/sliders/sliders"
        },
        MenuButton {
                    gridX : 3; gridY : 1
                    imgPath: "../images/cookie_cry.png";
                    imgText : "  MainMenu    ";
                    menuName : "./MainMenu.qml"
        }
    ]
}

