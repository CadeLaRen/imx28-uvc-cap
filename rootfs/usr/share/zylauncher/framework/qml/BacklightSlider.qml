import QtQuick 1.0
import "./colibri"
import "backlight.js" as BacklightLogic

Item {
    property int basicSpin : loaderWidth/8
    CLSlider {
        id : lightSlider
        x : basicSpin / 2; y : (loaderButtom - basicSpin * 4) 
        width : basicSpin * 7; height : basicSpin / 2
        maximum : 255;  minimum : 1
        Component.onCompleted: { BacklightLogic.initValue() }
        onValueChanged: { BacklightLogic.changeValueAfterInit() }
    }

    CLButton {
        x : basicSpin ; y : (loaderButtom - basicSpin * 2)
        width : basicSpin; height : basicSpin
        text : "OK"
        onClicked: {drawerLoader.source = drawerLoader.systemQml}
    }

    CLButton {
        x : basicSpin * 5; y : (loaderButtom - basicSpin * 2)
        width : basicSpin; height : basicSpin
        text : "cancel"
        onClicked: { BacklightLogic.cancelChange() }
    }

}
