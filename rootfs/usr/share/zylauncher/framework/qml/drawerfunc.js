function drawerPressed()
{
    operationTime.shortOperation = 1; 
    operationTime.oldY = title.y;

    rootloader.running = true;
    flick.interactive = false;

    operationTime.start();
}

function slideDrawer()
{
    if(operationTime.shortOperation == 1) {
        //console.log("short operation"); 
        if (title.y > operationTime.oldY) {
            drawerDragButtomAnimation.start();
        } else {
            drawerDragTopAnimation.start();
        }

    } else {
        //console.log("long operation"); 
        if (drawer.height > rootloader.height/2) {
            drawerDragButtomAnimation.start();
        } else {
            drawerDragTopAnimation.start();
        }
    }
}

function drawerRestore()
{
    rootloader.running = false;
    flick.interactive = true;
}

function drawerDragEnable()
{
    if(drawerLoader.source != drawerLoader.systemQml) {
        titleMouseArea.enabled= false; 
    } else {
        titleMouseArea.enabled= true; 
    }
}
