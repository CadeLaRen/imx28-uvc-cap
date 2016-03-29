/*
 *  author     : liujianpei
 *  Description: 多分辨率支持
 */

function createConfig(titleFontSize,iconWidth,iconHeight,iconFontSize,iconStartX,iconStartY,iconSpinX)
{
    rootloader.titleFontSize = titleFontSize;
    rootloader.iconWidth = iconWidth;
    rootloader.iconHeight = iconHeight;
    rootloader.iconFontSize = iconFontSize;
    rootloader.iconStartX = iconStartX;
    rootloader.iconStartY = iconStartY;
    rootloader.iconSpinX = iconSpinX;
    rootloader.iconSpinY = (rootloader.viewHeight - iconStartY)/3;
}

function resolutionChange()
{
    if (rootloader.viewWidth != 0 && rootloader.viewHeight != 0) {
        setConfig(); 
    }
}

//  在此设置不同分辨率下界面的相关参数                        
function setConfig()
{
   if(rootloader.viewWidth == 320 && rootloader.viewHeight == 240) {            //  320 * 240
        createConfig(10,40,40,10,15,20,80);
   } else if (rootloader.viewWidth == 800 && rootloader.viewHeight == 480) {    //  800 * 480 
        createConfig(25,85,85,20,40,50,190);
   } else if (rootloader.viewWidth == 600 && rootloader.viewHeight == 480) {    //  600 * 480 
        createConfig(25,60,60,15,40,50,140);
   } else if (rootloader.viewWidth == 640 && rootloader.viewHeight == 480) {    //  640 * 480
        createConfig(25,60,60,15,40,50,150);
   }else if (rootloader.viewWidth == 720 && rootloader.viewHeight == 574) {    //  720 * 574
        createConfig(30,80,80,20,50,60,175);
   } else if (rootloader.viewWidth == 800 && rootloader.viewHeight == 600) {    //  800 * 600 
        createConfig(40,100,100,20,40,80,200);
   } else if (rootloader.viewWidth == 1024 && rootloader.viewHeight == 768) {   //  1024 * 768 
        createConfig(40,100,100,23,50,100,240);
   } else if (rootloader.viewWidth == 480 && rootloader.viewHeight == 272) {    //  480 * 272 
        createConfig(10,60,60,10,15,20,122);
   } else {                                                                     //  当前不支持的分辨率，使用320*240的配置 
        console.log("unsupport resolution: ",rootloader.viewWidth,rootloader.viewHeight);
        console.log("select default 320*240 resolution"); 
        createConfig(10,40,40,10,15,20,80);
   }
   // 在界面大小改变后，重新载入qml文件，以获取新的maxYIndex，用于设置flick.contentHeight
   var tmpstr;
   tmpstr = rootloader.source;
   rootloader.source = "";
   rootloader.source = tmpstr;
   rootloader.titleName = rootloader.item.resources[0].text;
   //console.log(rootloader.viewWidth,rootloader.viewHeight);
}
