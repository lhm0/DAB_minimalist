  
let selectedStationId;
let selectedStationName;

// *******************************************************************************************************
//                                    download radio settings from server
// *******************************************************************************************************

// Event listener for initial page setup 
document.addEventListener("DOMContentLoaded", function(event) { 
    getPageData();
});

function getPageData() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var data = JSON.parse(this.responseText);
            // Set volume slider and display
            document.getElementById("volumeSlider").value = data.volume;
            document.getElementById("volumeValue").innerHTML = data.volume + "%";
            
            // Set the selected radio button based on the preset
            document.querySelector('#option' + (parseInt(data.preset)+1).toString()).checked = true;
            
            // Update the stations list
            var stationListDiv = document.getElementById("station-list");
            stationListDiv.innerHTML = ""; // Clear the existing content
            
            // Generate HTML for each station and append it
            data.stations.forEach(function(station) {
                var stationItem = document.createElement("div");
                stationItem.className = "station-item";
                stationItem.innerHTML = `<span>${station.id}: ${station.name}</span>`;

                stationItem.dataset.stationId = station.id; // Store the station ID for later use
                stationItem.dataset.stationName = station.name;
                // Add click event listener for selecting the station
                stationItem.addEventListener('click', function() {
                    selectStation(stationItem); // Übergebe stationId und stationName
                });
                stationListDiv.appendChild(stationItem);
            });

            // Update radio button labels based on presets data
            data.presets.forEach(function(preset) {
                var labelId = 'label' + preset.pnr;
                var label = document.getElementById(labelId);
                if (label) {
                    label.childNodes[0].textContent = preset.name;
                } else {
                    if (labelId!="label0") console.warn("Element with ID '" + labelId + "' not found.");
                }
            });
        }
    };
    xhr.open("GET", "/getParam", true);
    xhr.send();
}

// *******************************************************************************************************
//                          manage stationList according to user interaction
// *******************************************************************************************************

// Function to highlight a certain line in the stationList
function highlightStationById(stationId) {
    var allStations = document.querySelectorAll(".station-item");

    allStations.forEach(function(item) {
        item.classList.remove("selected");
    });

    // find and mark the station with correct "stationId"
    allStations.forEach(function(item) {
        if (item.dataset.stationId === stationId) {
            item.classList.add("selected");  // mark element
        }
    });
}

// Function to highlight the selected station and inform the server
function selectStation(selectedItem) {
    // Remove highlighting from all items
    var allItems = document.querySelectorAll(".station-item");
    allItems.forEach(function(item) {
        item.classList.remove("selected"); // Remove the 'selected' class
    });

    // Add highlighting to the clicked item
    selectedItem.classList.add("selected");

    // Get the station ID from the dataset
    var stationId = selectedItem.dataset.stationId;

    // Inform the server about the selected station
    sendSelectedStationToServer(stationId);

    //remember selected station
    selectedStationId = selectedItem.dataset.stationId;
    selectedStationName = selectedItem.dataset.stationName;
    console.log("selectedStation.Id="+selectedStationId+"  selectedStation.Name="+selectedStationName);
}

// Function to send the selected station ID to the server
function sendSelectedStationToServer(stationId) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/selectStation?id=" + stationId, true);
    xhr.send();
}


// *******************************************************************************************************
//                                        update page periodically
// *******************************************************************************************************

// Start the periodic fetching when the document is loaded
document.addEventListener("DOMContentLoaded", function(event) { 
    fetchInfoPeriodically();
});

// function to fetch tuned station, service info, and volume setting from server
function fetchInfoPeriodically() {
    setInterval(function() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
                try {
                    var data = JSON.parse(this.responseText);
                    
                    document.getElementById("serviceInfo").textContent = data.serviceInfo;
                    highlightStationById(data.station.toString());
                    selectRadioButtonByNumber(data.preset.toString());
                    setVolumeSlider(data.volume);
                } catch (e) {
                    console.error("Fehler beim Parsen der JSON-Daten: ", e);
                }
            }
        };
        xhr.open("GET", "/getInfo", true);
        xhr.send();
    }, 2000); // 2000 Millisekunden = 2 Sekunden
}

// Function to set slider to a certain value
function setVolumeSlider(value) {
    var slider = document.getElementById("volumeSlider");

    if (value < slider.min || value > slider.max) {
        console.warn("value not within valid range");
        return;
    }
    slider.value = value;
    document.getElementById("volumeValue").innerHTML = value + "%";
}

// *******************************************************************************************************
//                                        manage Wifi button
// *******************************************************************************************************

// Event listener for "reset wifi" button
document.getElementById('reset-wifi').addEventListener('click', () => {
    resetWifi();
});

// function to call Wifi credential page
function resetWifi() {
    window.location.href = "/resetWifi";
}

// *******************************************************************************************************
//                                        manage scan button
// *******************************************************************************************************

// Event listener for "scan" button
document.getElementById("scan").addEventListener('click', () => {
    startScan();
});

function startScan() {
    var userConfirmed = confirm("A scan will delete previous station data and presets. Are you sure?");

    if (userConfirmed) {
        document.getElementById("scanInfo").style.display = "block";
        document.getElementById("scanInfo").innerHTML = "<br><br>scanning..... <br><br>please wait... <br><br>Scan might take up to 1 minute.";

        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/scan", true);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4 && xhr.status == 200) {
                console.log("scan completed");

                window.location.reload();  // reload page
            }
        };
        xhr.send();
    }
}

// *******************************************************************************************************
//                                        manage volume slider
// *******************************************************************************************************

// Event listener for volume slider
document.getElementById('volumeSlider').addEventListener('change', () => {
    updateSliderVolume();
});

// Function to send the slider setting to the server
function updateSliderVolume(element) {
    var sliderValue = document.getElementById("volumeSlider").value;
    document.getElementById("volumeValue").innerHTML = sliderValue + "%";
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?value="+sliderValue, true);
    xhr.send();
}

// *******************************************************************************************************
//                                        manage radio buttons
//                      there is a distinction between short and long clicks
// *******************************************************************************************************

//functions to distinguish short and long clicks

// add event-Listener for elements of class 'radioButton'
document.querySelectorAll('.radioButton').forEach(function(label) {
    label.addEventListener('mouseup', function(event) { handleMouseUp(this);});
    label.addEventListener('mousedown', function(event) { handleMouseDown(this);});
    label.addEventListener('touchstart', function(event) { handleTouchStart(this); });
    label.addEventListener('touchend', function(event) { handleTouchEnd(this); });
    label.addEventListener('touchcancel', function(event) { handleTouchCancel(this); });
});

let clickStartTime;
const longPressThreshold = 500; // time in milliseconds for long click

function handleMouseDown(element) {
    clickStartTime = new Date().getTime(); // save tiem of click start
}

function handleMouseUp(element) {
    const clickDuration = new Date().getTime() - clickStartTime;

    if (clickDuration >= longPressThreshold) {
        handleLongPress(element); 
    } else {
        toggleCheckbox(element); 
    }
}

function handleTouchStart(element) {
    clickStartTime = new Date().getTime(); 
}

function handleTouchEnd(element) {
    const clickDuration = new Date().getTime() - clickStartTime;

    if (clickDuration >= longPressThreshold) {
        handleLongPress(element);  
    } else {
        toggleCheckbox(element);  
    }
}

function handleTouchCancel(element) {
    clickStartTime = 0;
}

// function to handle short click: send update of newly selected radio button to server
function toggleCheckbox(element) {
    var buttonNumber = element.id.replace('label', '');
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/updateradiobutton?button=" + buttonNumber, true);
    // Event-Listener für die Server-Antwort
    xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
            var stationId = xhr.responseText; 
            highlightStationById(stationId); 
        }
    }
    xhr.send();
}

// function to handle long click: update button name, send new setting to server
function handleLongPress(element) {
    var labelId = element.id; 
    var label = document.getElementById(labelId);
    label.childNodes[0].textContent = selectedStationName;       // renew button name
    var buttonNumber = element.id.replace('label', '');

    console.log("/savePreset?button=" + buttonNumber + "&stationId=" + selectedStationId +"&stationName="+selectedStationName); 
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/savePreset?button=" + buttonNumber + "&stationId=" + selectedStationId +"&stationName="+selectedStationName ,true); 
    xhr.send();

    selectStation(selectedStationId);
}

// Function to select a radio button
function selectRadioButtonByNumber(buttonNumber) {
    var radioButtonId = 'option' + buttonNumber;
    var radioButton = document.getElementById(radioButtonId);
    
    if (!radioButton) {  // if button does not exist
        if (radioButtonId!=0) console.warn("Radio-Button mit ID '" + radioButtonId + "' nicht gefunden. Deselektiere alle Radio-Buttons.");
        
        // deselect all radio buttons
        var allRadioButtons = document.querySelectorAll('input[type="radio"]');
        allRadioButtons.forEach(function(button) {
            button.checked = false;
        });
        
        return; 
    }
    radioButton.checked = true;
}

