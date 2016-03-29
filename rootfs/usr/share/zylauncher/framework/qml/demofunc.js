/*
 *  author     : liujianpei
 *  Description:
 */


/*
 *  启动例程
 */
function proStart(execText)
{
    //console.log(rootloader.running);
    if (rootloader.running == true) {
        //console.log("some operation running,could not start process"); 
        return;
    } else {
        launcher.launchPro(execText)
        //console.log("demo started"); 
        
        // 设置title名为进程名
        rootloader.titleName = imgText;

        flick.interactive = false;
        button.focus = true;
        rootloader.currentIcon = button.order;
        rootloader.running = true;
        title.titleDrageEnable = false;
        title.closeButtonVisible = true;
        proStartAnimation.start();
    }
}


/*
 *  例程结束 或者 menu切换动画完成后，调用此函数。
 */
function operationFinished()
{
    rootloader.running = false;
    title.titleDrageEnable = true;
    flick.interactive = true;
    title.closeButtonVisible = false;
    //  恢复title名为menus名
    rootloader.titleName = rootloader.item.resources[0].text;
    //console.log("operation finished\n");
}


/*
 *  计算icon位置信息
 *  计算flick.contentHeight
 */
var maxYIndex = 0;
function indexToY(index)
{
    //获取maxYIndex
    if (maxYIndex < index) {
        maxYIndex = index; 
        var  indexTriple = (Math.floor( maxYIndex / 3) + 1 ) * 3;   //保证contentHeight为iconSpinY 3 的倍数
        flick.contentHeight = indexTriple * rootloader.iconSpinY;
    }
    return (index * iconSpinY)
}

function indexToX(index)
{
    return (index * iconSpinX + iconStartX)
}

/*
 *  menu切换时的动画效果
 */
function menuSwitch(menuName)
{
    if (rootloader.running == true ) {
        //console.log("some operation still running,could not switch"); 
        return;
    } else {
        //启动动画。动画运行期间，禁止启动演示程序;禁止界面flick
        flick.interactive = false;
        rootloader.running = true;
        title.titleDrageEnable = false;
        switchMenuAnimaion.start();

        //封装路径的差异性，统一在界面描述qml文件中 "相对路径" 的基准目录
        var isAbsolutePath;
        var switchPath;
        isAbsolutePath = menuName.indexOf("/");
        if (isAbsolutePath == 0) {  // menuName为绝对路径
            switchPath = menuName; 
        } else {                    // menuName为相对路径
            switchPath = "../../demonstration/menus/" + menuName;
        }
        //console.log(switchPath,isAbsolutePath);
        rootloader.tmpSource = switchPath;
    }
}

function switchMenuScript()
{
    rootloader.source = rootloader.tmpSource; 
    flick.contentY = 0;
}


/*
 *  界面对齐时的动画效果
 */
function rowSwitchStart()
{
    timer.stop();
    timer.start();
    flick.oldContentY = flick.contentY;
    flick.shortFlick = 1;
    //console.log("rowSwitchStart",flick.contentY);
}

function rowSwitch()
{
    if(flick.contentY <= 0 )
    {
        return; 
    }
    //console.log("before",flick.shortFlick,Math.abs(flick.contentY - flick.oldContentY));
    // 滑动时间较长，或者滑动距离过短，则以longFlickCalRow方式切换界面
    if(flick.shortFlick == 0 || Math.abs(flick.contentY - flick.oldContentY) < flick.height/10) {
        flick.currentRowY = longFlickCalRowTo();
        flick.toRowYDuration = Math.abs(flick.currentRowY - flick.contentY)/0.9;
        //console.log("long",flick.currentRowY);
    } else {
        flick.currentRowY = shortFlickCalRowTo();
        flick.toRowYDuration = Math.abs(flick.currentRowY - flick.contentY)/0.9;
        //console.log("short",flick.currentRowY,flick.toRowYDuration);
    }
    
    //在动画执行过程中，禁止界面拖动，防止界面抖动;禁止启动程序，禁止切换Menu
    flick.interactive = false;
    rootloader.running = true;
    title.titleDrageEnable = false;
    switchRowAnimation.start();
}



function shortFlickCalRowTo()
{
    //var buttomY = Math.floor(flick.contentY /tripleSpinY) * tripleSpinY;
    
    var buttomY = flick.currentRowY;
    var tripleSpinY = rootloader.iconSpinY * 3;
    var upButtomY = buttomY - tripleSpinY;
    var downButtomY = buttomY + tripleSpinY;


    if (flick.contentY - flick.oldContentY > 0) {
        return ( downButtomY + tripleSpinY <= flick.contentHeight ? downButtomY : buttomY);
    } else {
        return ( upButtomY >= 0 ? upButtomY : buttomY);
    }
}

function longFlickCalRowTo()
{
    var tripleSpinY = rootloader.iconSpinY * 3;
    var buttomY = Math.floor(flick.contentY /tripleSpinY) * tripleSpinY;
    var topY = buttomY + tripleSpinY;
    if ( (topY - flick.contentY) >= tripleSpinY  / 2 ) {  //当滑动后显示了一屏界面1/2或以上，则显示此屏
        return buttomY;
    } else {
        return topY; 
    }
}

function switchRowScript()
{
    flick.interactive = true;
    rootloader.running = false;
    title.titleDrageEnable = true;
    timer.stop();
    //console.log(flick.contentY);
    flick.shortFlick = 1;
}
