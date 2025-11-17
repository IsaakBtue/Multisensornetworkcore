/// Data arrays to store the history for the graphs
const temperatureData = [];
const co2Data = [];
const labels = [];

// Define the threshold for the CO₂ alert (in ppm)
const co2AlertThreshold = 800;

// Get the canvas elements from the HTML
const tempCtx = document.getElementById('temperature-chart').getContext('2d');
const co2Ctx = document.getElementById('co2-chart').getContext('2d');
const batteryAlert = document.getElementById('low-battery-alert');

// NEW: Get modal and room control elements
const addDeviceModal = document.getElementById('add-device-modal');
const addDeviceBtn = document.getElementById('add-device-btn');
const closeModalBtn = document.querySelector('.close-button');
const saveDeviceBtn = document.getElementById('save-device-btn');
const deviceCodeInput = document.getElementById('device-code-input');
const roomNameInput = document.getElementById('room-name-input');
const roomSelect = document.getElementById('room-select');
const modalMessage = document.getElementById('modal-message');
// END NEW

// Initialize the Temperature graph
const tempChart = new Chart(tempCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Temperature (°C)',
            data: temperatureData,
            borderColor: 'rgb(75, 192, 192)',
            tension: 0.1
        }]
    },
    options: {
        scales: {
            y: {
                beginAtZero: false,
                title: {
                    display: true,
                    text: 'Temperature (°C)'
                }
            },
            x: {
                title: {
                    display: true,
                    text: 'Time'
                }
            }
        }
    }
});

// Initialize the CO₂ graph
const co2Chart = new Chart(co2Ctx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'CO₂ Level (ppm)',
            data: co2Data,
            borderColor: 'rgb(255, 99, 132)',
            tension: 0.1
        }]
    },
    options: {
        scales: {
            y: {
                beginAtZero: false,
                title: {
                    display: true,
                    text: 'CO₂ Level (ppm)'
                }
            }
        }
    }
});

// A new function to simulate a low battery status
function isBatteryLow() {
    // This will return true ~10% of the time for demonstration
    return Math.random() < 0.1; 
}

// Function to fetch data from the Open-Meteo API
async function getSensorData() {
    // In a real application, this API call would use the currently selected device/room ID
    // For this example, we keep the original fixed call.
    const apiUrl = 'https://api.open-meteo.com/v1/forecast?latitude=51.4408&longitude=5.4779&current=temperature_2m,shortwave_radiation';
    try {
        const response = await fetch(apiUrl);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        const temperature = data.current.temperature_2m.toFixed(1);
        const solarRadiation = data.current.shortwave_radiation.toFixed(0);
        
        // This is still a simulated value
        const co2 = (Math.random() * 800 + 400).toFixed(0); 
        
        return {
            temperature: `${temperature} °C`,
            co2: `${co2} ppm`,
            solarRadiation: `${solarRadiation} W/m²`,
            rawTemp: parseFloat(temperature),
            rawCo2: parseInt(co2)
        };
    } catch (error) {
        console.error("Could not fetch data:", error);
        return {
            temperature: 'N/A',
            co2: 'N/A',
            solarRadiation: 'N/A',
            rawTemp: null,
            rawCo2: null
        };
    }
}

// Function to update the HTML page and both graphs
async function updateDashboard() {
    
    const data = await getSensorData();

    // Get the CO₂ card element to add/remove the alert class
    const co2Card = document.querySelector('.sensor-card:nth-child(2)');

    // Update the HTML text displays
    document.getElementById('temperature-value').textContent = data.temperature;
    document.getElementById('co2-value').textContent = data.co2;
    document.getElementById('solar-radiation-value').textContent = data.solarRadiation;

    // Check if the CO₂ level exceeds the threshold and apply the alert class
    if (data.rawCo2 > co2AlertThreshold) {
        co2Card.classList.add('alert');
    } else {
        co2Card.classList.remove('alert');
    }

    // Check for low battery and show the alert if necessary
    if (isBatteryLow()) {
        batteryAlert.classList.remove('hidden');
    } else {
        batteryAlert.classList.add('hidden');
    }

    // --- Graph Update Logic ---
    if (data.rawTemp !== null && data.rawCo2 !== null) {
        temperatureData.push(data.rawTemp);
        co2Data.push(data.rawCo2);

        const now = new Date();
        labels.push(`${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`);

        const maxPoints = 20;
        if (labels.length > maxPoints) {
            temperatureData.shift();
            co2Data.shift();
            labels.shift();
        }

        tempChart.update();
        co2Chart.update();
    }
}

// NEW: Functions for the Add Device Modal
function openModal() {
    addDeviceModal.classList.remove('hidden');
    modalMessage.classList.add('hidden'); // Clear any previous message
    deviceCodeInput.value = ''; // Clear previous input
    roomNameInput.value = '';    // Clear previous input
}

function closeModal() {
    addDeviceModal.classList.add('hidden');
}

function handleSaveDevice() {
    const code = deviceCodeInput.value.trim();
    const roomName = roomNameInput.value.trim();

    if (code && roomName) {
        // --- In a real application, you would send this data to a server ---
        console.log(`Saving new device: Code=${code}, Room=${roomName}`);
        
        // 1. Create a new option for the dropdown
        const newOption = document.createElement('option');
        // A real value might be the device ID from the server
        const optionValue = roomName.replace(/\s/g, '').toLowerCase(); 
        newOption.value = optionValue; 
        newOption.textContent = `${roomName}`;

        // 2. Add the new option to the select dropdown
        roomSelect.appendChild(newOption);
        
        // 3. Set the newly added room as the active room
        roomSelect.value = optionValue;

        // 4. Show success message
        modalMessage.textContent = `✅ Success! Device ${code} added for ${roomName}.`;
        modalMessage.classList.remove('hidden');
        
        // Close the modal after a short delay
        setTimeout(closeModal, 2000); 

    } else {
        modalMessage.textContent = '❌ Please enter both a device code and a room name.';
        modalMessage.classList.remove('hidden');
    }
}

// Event Listeners for the modal
addDeviceBtn.addEventListener('click', openModal);
closeModalBtn.addEventListener('click', closeModal);
saveDeviceBtn.addEventListener('click', handleSaveDevice);

// Close modal if user clicks outside of it
window.addEventListener('click', (event) => {
    if (event.target === addDeviceModal) {
        closeModal();
    }
});
// END NEW

// Update the dashboard every 3 seconds
setInterval(updateDashboard, 3000);

// Run the function once when the page first loads
updateDashboard();