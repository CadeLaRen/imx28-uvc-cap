import QtQuick 1.0
import "./colibri"
import "mutex.js" as ChooseLogic
Item {
    property int    index
    property int    seconds
    property int basicSpin :  loaderWidth/8
    property alias labelText: label.text 
    property alias checked:   checkBox.checked 
    CLCheckBox {
        id : checkBox
        x : basicSpin / 2; y : (loaderButtom - basicSpin/1.2 * (5 - index))
        onClicked : ChooseLogic.mutexChoose();
    }
    ReadOnlyTextEdit {
        id : label
        x : basicSpin * 2; y : (loaderButtom - basicSpin/1.2 * (5 - index))
    }
    Component.onCompleted: {ChooseLogic.isThisCheckBox()}
}
