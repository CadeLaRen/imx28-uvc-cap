var inited = false;
var oldValue;
function initValue()
{
    oldValue = lightControl.lightLevel();
    lightSlider.setValue(oldValue);
    inited = true;
}

function changeValueAfterInit()
{
    if(inited) {
        lightControl.changeLevel(lightSlider.value);
    }
}

function cancelChange()
{
    //console.log(oldValue);
    lightControl.changeLevel(oldValue);
    drawerLoader.source = drawerLoader.systemQml;
}
