/*
 *  author     : liujianpei
 *  Description: 处理键盘逻辑
 *               处理使用键盘切换焦点图标时的动画效果
 */
function init()
{
    flick.currentRowY = 0;
    initFocus();
}

/*
 *  初始化键盘焦点
 */
function initFocus()
{
    rootloader.totalNum = rootloader.item.children.length - 1; 
    for (var i = 0; i < rootloader.totalNum; i++) {
        rootloader.item.children[i].order = i;
    }
    rootloader.item.children[0].focus = true;
    rootloader.currentIcon = 0;
}


function focusButtonPressed()
{
   rootloader.item.children[rootloader.currentIcon].buttonPressed(); 
}

function focusLeft()
{
    if(rootloader.running == true) {
        //console.log("one process still running,could not start another process"); 
        return  
    }
    
    if (rootloader.currentIcon <= 0) {
        rootloader.currentIcon = rootloader.totalNum ; 
    } else {
        rootloader.currentIcon = rootloader.currentIcon - 1; 
    }
    focusChangeAnimation();
}

function focusRight()
{
    if(rootloader.running == true) {
        //console.log("one process still running,could not start another process"); 
        return  
    }

    if (rootloader.currentIcon + 1 > rootloader.totalNum) {
        rootloader.currentIcon = 0; 
    } else {
        rootloader.currentIcon = rootloader.currentIcon + 1; 
    }
    focusChangeAnimation();
}


function focusUp()
{
    if(rootloader.running == true) {
        //console.log("one process still running,could not start another process"); 
        return  
    }

    for (var i = rootloader.currentIcon - 1; 1 ; i--) {
        if (i < 0 ) {
            i = rootloader.totalNum; 
        }
        if (rootloader.item.children[i].gridX == rootloader.item.children[rootloader.currentIcon].gridX) {
            rootloader.currentIcon = i; 
            break;
        }
    }
    focusChangeAnimation();
}

function focusDown()
{
    if(rootloader.running == true) {
        //console.log("one process still running,could not start another process"); 
        return  
    }

    for (var i = rootloader.currentIcon + 1; 1 ; i++) {
        if (i > rootloader.totalNum ) {
            i = 0; 
        }
        if (rootloader.item.children[i].gridX == rootloader.item.children[rootloader.currentIcon].gridX) {
            rootloader.currentIcon = i; 
            break;
        }
    }
    focusChangeAnimation();
}

//处理使用键盘切换焦点时，界面的切换动画
function focusChangeAnimation()
{
    rootloader.item.children[rootloader.currentIcon].focus = true;

    var focusIconY = rootloader.item.children[rootloader.currentIcon].y;
    var tripleSpinY = rootloader.iconSpinY * 3;
    var focusIconButtomY = Math.floor(focusIconY/tripleSpinY) * tripleSpinY;

    if (focusIconButtomY != flick.currentRowY) {
        flick.toRowYDuration = Math.abs(flick.currentRowY - focusIconButtomY); 
        flick.currentRowY = focusIconButtomY;
        flick.interactive = false;
        rootloader.running = true;
        keyMoveAnimation.start();
    }     
}

function focusChangeScript()
{
    flick.interactive = true;
    rootloader.running = false;
}

