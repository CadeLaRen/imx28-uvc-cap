import QtQuick 1.0
import "./colibri"
import "mutex.js" as ChooseLogic
Item {
    id : offTimeSet
    property int currentIndex
    children : [ 
        MutexCLCheckBox {
            index : 0
            seconds: 15
            labelText : "15 second" 
        },
        MutexCLCheckBox {
            index : 1
            seconds: 60
            labelText : "1  minitus" 
        },
        MutexCLCheckBox {
            index : 2
            seconds: 300
            labelText : "5  minitus" 
        },
        MutexCLCheckBox {
            index : 3
            seconds: 900
            labelText : "15 minitus" 
        },
        MutexCLCheckBox {
            index :  4
            seconds: 0
            labelText : "never off" 
        }
    ]
    CLButton {
        text : " OK "  
        x : loaderWidth * 6 / 8; y : loaderButtom - loaderButtom * 1 / 5
        onClicked : {ChooseLogic.confirmChoose()}
    }
}
