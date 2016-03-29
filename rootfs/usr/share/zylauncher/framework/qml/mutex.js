
function mutexChoose()
{
    currentIndex = index;
    //  offTimeSet.children.length - 1 是避免操作到CLButton
    for(var i = 0; i < offTimeSet.children.length - 1; i++) {
        offTimeSet.children[i].checked = false;
    }
    offTimeSet.children[currentIndex].checked = true;
}

function isThisCheckBox()
{
    if(seconds * 1000 == agent.offTime()) {
        checked = true;  
        currentIndex = index;
    } else {
        checked = false; 
    }
}

function confirmChoose()
{
    //console.log(currentIndex);
    agent.setAutoOffTime(offTimeSet.children[currentIndex].seconds * 1000);
    drawerLoader.source = drawerLoader.systemQml;
}
