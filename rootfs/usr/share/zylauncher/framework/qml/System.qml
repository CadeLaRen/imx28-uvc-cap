import QtQuick 1.0
import "./colibri"

Item {
    property int basicSpin : loaderWidth/8
    CLButton {
        x : basicSpin/2; y : (loaderButtom - basicSpin * 3) 
        width : basicSpin; height : basicSpin
        text : "level"
        onClicked : {drawerLoader.source = "BacklightSlider.qml"}
    }
    CLButton {
        x : basicSpin/2 + 3 * basicSpin; y : (loaderButtom - basicSpin * 3) 
        width : basicSpin; height : basicSpin
        text : "off"
        onClicked : { agent.turnOffLight(1) }
    }
    CLButton {
        x : basicSpin/2 + 6 * basicSpin; y : (loaderButtom - basicSpin * 3) 
        width : basicSpin; height : basicSpin
        text : "time"
        onClicked : {drawerLoader.source = "OffTimeSet.qml"}
    }
}
