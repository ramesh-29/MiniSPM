import SPMAdmin 1.0
import QtQuick 2.0

//Item {

//}
//import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 1600
    height: 1200
    title: "SPM Admin"

    // Instantiate SPMBenchFullTest
    SPMBenchFullTest {
        id: benchTest
    }

    Button {
        id: powerButton
        text: "Start Test"
        font.bold: true
        font.pixelSize: 20
        width: 150
        height: 75
        //        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 50 // Distance from the top
        anchors.leftMargin: 50
        onClicked: {
            // Call the setupPowerSource function of the C++ class
            benchTest.setupPowerSource();
        }
    }

    GridLayout {
        id: meterGrid // Assign an id to the GridLayout
        columns: 9
        columnSpacing: 17.5
        anchors.centerIn: parent


        // First Row
        Repeater {
            model: 9
            Rectangle {
                width: 125
                radius: 10

                height: 150
                color: benchTest.getMeterColor(index) // Get color from SPMBenchFullTest
                border.width: 2
                border.color: "black"
                Text {
                    anchors.centerIn: parent
                    text: "Meter " + (index + 1)
                    font.bold: true
                }
                MouseArea {
                    id: mouseArea // Assign an id to reference it
                    anchors.fill: parent
                    onEntered: {
                        //                                meterGrid.children[index].children[0].text = "Meter " + (index + 1)
                        errorText.visible = true
                        errorBackground.visible = true
                    }
                    onExited: {
                        //                              meterGrid.children[index].children[0].text = "Meter " + (index + 1);
                        errorBackground.visible = false
                        errorText.visible = false
                    }
                }
                Rectangle {
                    id: errorBackground // Background rectangle for hello text
                    color: "white" // White background color
                    border.color: "black" // Border color
                    radius: 4
                    visible: false // Initially hide the background
                    anchors {
                        top: errorText.top
                        left: errorText.left
                        right: errorText.right
                        bottom: errorText.bottom
                    }
                }
                Text {
                    id: errorText // Assign an id to the Text element
                    text: " "
                    color: "black"
                    anchors.topMargin: 5 // Add 5 units of margin
                    anchors.rightMargin:2 // Add 5 units of margin
                    anchors.leftMargin:2
                    wrapMode: Text.WordWrap // Wrap the text onto the next line if it exceeds the bounds
                    font.pixelSize: 14 // Adjust the font size as needed
                    visible: false // Initially hide the text
                    anchors.top: meterGrid.children[index].children[0].bottom // Position below "Meter X"
                    //                    anchors.left: parent.left
                    //                     anchors.horizontalCenter: meterGrid.children[index].children[0].horizontalCenter
                    anchors.right: parent.right
                }
            }
        }

        Repeater {
            model: 54
            Rectangle {
                width: 125
                radius: 10

                height: 50
                color: "white" //
                border.width: 1
                border.color: "black"
                Text {
                    anchors.centerIn: parent
                    wrapMode: Text.WordWrap
                    width: parent.width - 10 // Adjust based on your requirements
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: {
                        if (index < 9) {
                            return "Functional Test";
                        } else if (index >= 9 && index < 18) {
                            return "Calibration Test";
                        } else if (index >= 18 && index < 27) {
                            return "Error1 Test";
                        } else if (index >= 27 && index < 36) {
                            return "Error2 Test";
                        } else if (index >= 36 && index < 45) {
                            return "Starting Current Test";
                        } else if (index >= 45 && index < 54) {
                            return "NicSync Test";
                        }
                    }
                    font.bold: true
                }
            }

        }
    }

    Connections {
        target: benchTest
        onMeterColorChanged: {
            if(index === 7 ){
                for (var i = 0; i < 64; i++) {
                    if(i>=9){
                        if(i===9)continue;
                        meterGrid.children[i].color = "white";
                        continue;
                    }
                    meterGrid.children[i].children[0].text = "Meter" + (i + 1)
                    meterGrid.children[i].children[3].text = " ";
                    meterGrid.children[i].color = benchTest.getMeterColor(i);
                }
                return;
            }
            console.log(index,"*************");
            // Update color of meters
            for ( i = 0; i < 9; i++) {
                if(benchTest.getFailStage(i)!==""){
                    meterGrid.children[i].children[0].text = "Meter " + (i + 1) + "\n\nFailed At: \n" + benchTest.getFailStage(i);
                    meterGrid.children[i].children[3].text = " " + benchTest.getMeterValue(i) + " "; // Assuming error text is the fourth child
                    if(meterGrid.children[i].color !== benchTest.getMeterColor(i)){
                        meterGrid.children[i+1+(9*(index+1))].color = "red";
                    }

                }else{
                    var no = benchTest.getmeterSerialNo(i);
                    if(no!==-1){
                        meterGrid.children[i].children[0].text = "Meter " + (i + 1) + "\n" + benchTest.getmeterSerialNo(i);
                    }else{
                      meterGrid.children[i].children[0].text = "Meter " + (i + 1);
                    }

//                    meterGrid.children[i].children[0].text = "Meter " + (i + 1) + "\n" + benchTest.getmeterSerialNo(i);
                    meterGrid.children[i].children[3].text = " "; // Assuming error text is the fourth child
                    meterGrid.children[i+1+(9*(index+1))].color = "green";
                }
                meterGrid.children[i].color = benchTest.getMeterColor(i);
            }

        }
    }
}
