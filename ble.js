var noble = require('noble');
var fs = require('fs');

var stack = "";
var start = "&";
var end = "%";
var started = false;
var filename = "";

function getDateTime() {
    var date = new Date();

    var hour = date.getHours();
    hour = (hour < 10 ? "0" : "") + hour;

    var min  = date.getMinutes();
    min = (min < 10 ? "0" : "") + min;

    var sec  = date.getSeconds();
    sec = (sec < 10 ? "0" : "") + sec;

    return hour + ":" + min + ":" + sec;
}

process.argv.forEach(function (val, index, array) {
    if (index == 2){
        filename = val;
    }
});

noble.on('stateChange', function(state) {
  if (state === 'poweredOn')
    noble.startScanning();
  else
    noble.stopScanning();
});

noble.on('discover', function(peripheral){
    if (peripheral.advertisement.localName == "IUTEST" || peripheral.advertisement.localName=="HMSoft"){
        console.log(peripheral.id);
        console.log(peripheral.address);
        console.log(peripheral.addressType);
        console.log(peripheral.connectable);
        console.log(peripheral.advertisement.localName);
        console.log(peripheral.advertisement.serviceData.data);
        console.log('advertising the following service uuid\'s: ' + peripheral.advertisement.serviceUuids);

        peripheral.connect(function(error){
            console.log("Error? " + error);

        });

        peripheral.discoverServices();

        peripheral.on('connect', function(){
            console.log('connected to peripheral: ' + peripheral.uuid);
            console.log('CONNECTED : ' + getDateTime());

            peripheral.discoverServices(['ffe0'], function(error, services) {
                var deviceInformationService = services[0];
                console.log('discovered device information service');
                deviceInformationService.discoverCharacteristics(['ffe1'], function(error, characteristics) {
                    var testDataCharacteristics = characteristics[0];

                    testDataCharacteristics.on('read', function(data, isNotification) {
                        // Put it into string if not already
                        var stringval = data.toString();
                        console.log("Data reception confirmed");
                        console.log('data: ', stringval);

                        // If batch has already begun
                        if (started){
                            // Find the end mark: %
                            var result = stringval.indexOf(end);

                            // If not found, continue with batch
                            if (result == -1){
                                stack = stack + stringval;
                            }
                            // Found the endmark
                            else{
                                // Concat everything upto %, not including.
                                var substr = stringval.substring(0, result);
                                // Entire string for batch
                                stack = stack + substr;

                                // Split into list
                                var splitted = stack.trim().split(" ");
                                // Final formatted string
                                var outputstring = "";

                                // Line parser
                                var flag = 0;
                                var count = 1;
                                for (var i=1; i<=splitted.length; i++){
                                    // If beginning of line, add count
                                    if (flag == 0){
                                        outputstring = outputstring + count.toString() + " ";
                                    }
                                    outputstring = outputstring + splitted[i-1] + " ";
                                    // Break line and reset line index integer if Z coordinate
                                    if (flag == 2){
                                        outputstring = outputstring + "\n";
                                        flag =0;
                                        count ++;
                                    } else{
                                        flag ++;
                                    }
                                }

                                // Append onto file
                                fs.appendFile(filename, outputstring, encoding='utf8', function (err) {
                                    if (err) throw err;
                                });

                                // For error checking, find if start index exists in current lineread
                                result = stringval.indexOf(start);
                                started = false;

                                // This should be the case
                                if (result == -1){
                                    stack = "";
                                    return;
                                }
                                // This should be never run
                                else{
                                    stack = stringval.substring(result+1);
                                    started = true;
                                }
                            }
                        }
                        // No ongoing batch, start new one if start flag: &
                        else{
                            var result = stringval.indexOf(start);

                            // This also should not happen, but can happen. If batch corrupted, skip batch
                            if (result == -1){
                                stack = "";
                                return;
                            }
                            // Start of batch recognized. Start concatenation and set start flag on
                            else{
                                console.log("Found start of batch");
                                var substr = stringval.substring(result+1);
                                stack = stack + substr;
                                started = true;
                            }
                        }
                    });

                    // true to enable notify
                    testDataCharacteristics.notify(true, function(error) {
                      console.log('data change notification on');
                    });
                });

            });
        });

        peripheral.on('disconnect', function(){
          console.log('DISCONNECTED : ' + getDateTime());
        });
    }
});
