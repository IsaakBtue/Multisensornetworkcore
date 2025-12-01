/// Data arrays to store the history for the graphs
const temperatureData = [];
const co2Data = [];
const humidityData = [];
const labels = [];

// Define the threshold for the CO₂ alert (in ppm)
const co2AlertThreshold = 800;

// Get the canvas elements from the HTML
const tempCtx = document.getElementById('temperature-chart').getContext('2d');
const co2Ctx = document.getElementById('co2-chart').getContext('2d');
const humidityCanvas = document.getElementById('humidity-chart');
const humidityCtx = humidityCanvas ? humidityCanvas.getContext('2d') : null;
const batteryAlert = document.getElementById('low-battery-alert');

// NEW: Get modal and room control elements
const addDeviceModal = document.getElementById('add-device-modal');
const addDeviceBtn = document.getElementById('add-device-btn');
const closeModalBtn = document.querySelector('.close-button');
const saveDeviceBtn = document.getElementById('save-device-btn');
const deviceCodeInput = document.getElementById('device-code-input');
const roomNameInput = document.getElementById('room-name-input');
const modalMessage = document.getElementById('modal-message');
const buildingGrid = document.getElementById('buildingGrid');
const statusSummary = document.getElementById('statusSummary');
const relativeDistances = document.getElementById('relativeDistances');
const humidityValueEl = document.getElementById('humidity-value');
const co2Card = document.getElementById('co2-card');
let currentSensors = [];
let selectedSensorName = null;
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

const humidityChart = humidityCtx ? new Chart(humidityCtx, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [{
            label: 'Humidity (%)',
            data: humidityData,
            borderColor: 'rgb(134, 222, 183)',
            tension: 0.2
        }]
    },
    options: {
        scales: {
            y: {
                beginAtZero: false,
                title: {
                    display: true,
                    text: 'Humidity (%)'
                },
                suggestedMin: 30,
                suggestedMax: 70
            }
        }
    }
}) : null;

// A new function to simulate a low battery status
function isBatteryLow() {
    // This will return true ~10% of the time for demonstration
    return Math.random() < 0.1; 
}

// Function to fetch live sensor data from our own backend (ESP32 gateway -> Vercel)
async function getSensorData() {
    const apiUrl = '/api/ingest'; // Same origin as the deployed site
    try {
        const response = await fetch(apiUrl);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();

        if (!data.ok || !data.reading) {
            // Return zero values when no data is available
            return {
                temperature: '0.0 °C',
                humidity: '0.0 %',
                co2: '0 ppm',
                rawTemp: 0,
                rawCo2: 0,
                rawHumidity: 0
            };
        }

        const { temperature, co2, humidity } = data.reading;

        return {
            temperature: `${temperature.toFixed(1)} °C`,
            humidity: `${humidity.toFixed(1)} %`,
            co2: `${co2.toFixed(0)} ppm`,
            rawTemp: temperature,
            rawCo2: co2,
            rawHumidity: humidity
        };
    } catch (error) {
        console.error("Could not fetch data:", error);
        return {
            temperature: '0.0 °C',
            humidity: '0.0 %',
            co2: '0 ppm',
            rawTemp: 0,
            rawCo2: 0,
            rawHumidity: 0
        };
    }
}

// Function to update the HTML page and both graphs
async function updateDashboard() {
    
    const data = await getSensorData();

    // Update the HTML text displays
    document.getElementById('temperature-value').textContent = data.temperature;
    document.getElementById('co2-value').textContent = data.co2;
    if (humidityValueEl) {
        humidityValueEl.textContent = data.humidity;
    }

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
    // Only update graphs if we have real data (non-zero values indicate actual readings)
    if (data.rawTemp !== null && data.rawCo2 !== null && data.rawHumidity !== null) {
        // Only add to graph if values are non-zero (real data)
        if (data.rawTemp !== 0 || data.rawCo2 !== 0 || data.rawHumidity !== 0) {
            temperatureData.push(data.rawTemp);
            co2Data.push(data.rawCo2);
            humidityData.push(data.rawHumidity);

            const now = new Date();
            labels.push(`${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`);

            const maxPoints = 20;
            if (labels.length > maxPoints) {
                temperatureData.shift();
                co2Data.shift();
                humidityData.shift();
                labels.shift();
            }

            tempChart.update();
            co2Chart.update();
            if (humidityChart) {
                humidityChart.update();
            }
        }
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

// Facility map data - Single floor with 50 ESPs in rectangular building layout
// Generate 50 sensors with distance-based positioning
const cols = 5;
const rows = 10;
const buildingSensors = [];

// First, generate sensors with their distance measurements
for (let row = 0; row < rows; row++) {
    for (let col = 0; col < cols; col++) {
        const sensorNum = row * cols + col + 1;
        const isActive = Math.random() > 0.25;
        const distance = (Math.random() * 20 + 5).toFixed(1);
        
        buildingSensors.push({
            name: `ESP-${String(sensorNum).padStart(2, '0')}`,
            status: isActive ? 'active' : 'inactive',
            distance: parseFloat(distance), // Store as number for calculations
            distanceDisplay: `${distance} m`,
            x: 0, // Will be calculated
            y: 0, // Will be calculated
        });
    }
}

// Distance-based positioning algorithm
// Use distance measurements to create a grid with natural spacing
function calculatePositions() {
    const margin = 1; // 1% margin for zoomed out effect
    const availableWidth = 100 - (2 * margin);
    const availableHeight = 100 - (2 * margin);
    
    // Calculate average distance to determine spacing scale
    const avgDistance = buildingSensors.reduce((sum, s) => sum + s.distance, 0) / buildingSensors.length;
    const maxDistance = Math.max(...buildingSensors.map(s => s.distance));
    const minDistance = Math.min(...buildingSensors.map(s => s.distance));
    
    // Use distance values to create irregular grid spacing
    // Map distances to positions: larger distances = more spacing
    const positions = [];
    
    for (let i = 0; i < buildingSensors.length; i++) {
        const row = Math.floor(i / cols);
        const col = i % cols;
        
        // Base grid position
        const baseLeft = margin + (col / (cols - 1)) * availableWidth;
        const baseTop = margin + (row / (rows - 1)) * availableHeight;
        
        // Use distance measurement to add variation
        // Normalize distance to 0-1 range
        const distanceNorm = (buildingSensors[i].distance - minDistance) / (maxDistance - minDistance || 1);
        
        // Add offset based on distance (sensors with larger distances spread out more)
        const leftOffset = (Math.random() - 0.5) * 8 + (distanceNorm - 0.5) * 4;
        const topOffset = (Math.random() - 0.5) * 6 + (distanceNorm - 0.5) * 3;
        
        positions.push({
            x: Math.max(margin, Math.min(100 - margin, baseLeft + leftOffset)),
            y: Math.max(margin, Math.min(100 - margin, baseTop + topOffset))
        });
    }
    
    // Normalize to ensure all sensors are visible and spread out
    let minX = Math.min(...positions.map(p => p.x));
    let maxX = Math.max(...positions.map(p => p.x));
    let minY = Math.min(...positions.map(p => p.y));
    let maxY = Math.max(...positions.map(p => p.y));
    
    // Ensure we have a range
    if (maxX === minX) { maxX = minX + 1; }
    if (maxY === minY) { maxY = minY + 1; }
    
    const rangeX = maxX - minX;
    const rangeY = maxY - minY;
    
    // Scale to fit with margin
    for (let i = 0; i < positions.length; i++) {
        const normalizedX = ((positions[i].x - minX) / rangeX) * (100 - 2 * margin) + margin;
        const normalizedY = ((positions[i].y - minY) / rangeY) * (100 - 2 * margin) + margin;
        
        buildingSensors[i].left = normalizedX;
        buildingSensors[i].top = normalizedY;
    }
}

calculatePositions();

// Single floor structure
const buildingLevels = [
    {
        id: 'level-1',
        label: 'Building Floor',
        sensors: buildingSensors
    }
];

// Initialize with the single floor
function initializeFacilityMap() {
    const level = buildingLevels[0];
    if (level) {
        renderSensors(level.sensors);
        renderSummary(level.sensors);
    }
}

function renderSensors(sensors) {
    currentSensors = sensors;
    selectedSensorName = null;
    buildingGrid.innerHTML = '';
    
    // Calculate sensor size based on total count (more sensors = smaller dots)
    // Base size decreases as sensor count increases
    const baseSize = Math.max(8, 24 - (sensors.length * 0.2)); // Scales from 24px to 8px
    document.documentElement.style.setProperty('--esp-size', `${baseSize}px`);
    
    sensors.forEach((sensor) => {
        const point = document.createElement('div');
        point.className = 'esp-point';
        point.style.top = `${sensor.top}%`;
        point.style.left = `${sensor.left}%`;
        point.dataset.status = sensor.status === 'active' ? 'active' : 'inactive';
        point.dataset.sensorName = sensor.name;

        const tooltip = document.createElement('span');
        tooltip.className = 'tooltip';
        tooltip.textContent = `${sensor.name} • ${sensor.status === 'active' ? 'Online' : 'Offline'}`;
        point.appendChild(tooltip);

        const distance = document.createElement('span');
        distance.className = 'distance-label';
        point.appendChild(distance);

        point.addEventListener('click', () => handleSensorClick(sensor.name));

        buildingGrid.appendChild(point);
    });
    updateDistanceLabels();
    updateRelativePanel();
}

function renderSummary(sensors) {
    const activeCount = sensors.filter((sensor) => sensor.status === 'active').length;
    const inactiveCount = sensors.length - activeCount;

    statusSummary.innerHTML = `
        <div class="summary-card">
            <span>Active</span>
            <strong>${activeCount}</strong>
        </div>
        <div class="summary-card">
            <span>Offline</span>
            <strong>${inactiveCount}</strong>
        </div>
        <div class="summary-card">
            <span>Total Distance</span>
            <strong>${sumDistances(sensors)}</strong>
        </div>
    `;
}

function sumDistances(sensors) {
    const total = sensors.reduce((sum, sensor) => {
        return Number.isFinite(sensor.distance) ? sum + sensor.distance : sum;
    }, 0);
    return `${total.toFixed(1)} m`;
}

function handleSensorClick(sensorName) {
    selectedSensorName = sensorName;
    document.querySelectorAll('.esp-point').forEach((point) => {
        point.classList.toggle('selected', point.dataset.sensorName === sensorName);
    });
    updateDistanceLabels();
    updateRelativePanel();
}

function updateDistanceLabels() {
    const selected = currentSensors.find((sensor) => sensor.name === selectedSensorName);
    document.querySelectorAll('.distance-label').forEach((label) => {
        label.classList.remove('visible');
        label.textContent = '';
    });
    if (!selected) return;

    currentSensors.forEach((sensor) => {
        const point = buildingGrid.querySelector(`.esp-point[data-sensor-name="${sensor.name}"]`);
        if (!point) return;
        const label = point.querySelector('.distance-label');
        if (!label) return;

        if (sensor.name === selected.name) {
            label.textContent = '0.0 m';
        } else {
            const relative = calculateRelativeDistance(selected, sensor);
            label.textContent = formatMeters(relative);
        }
        label.classList.add('visible');
    });
}

function calculateRelativeDistance(sensorA, sensorB) {
    // Use the average of both sensors' distance measurements as the relative distance
    // This represents the actual measured distance between sensors
    return (sensorA.distance + sensorB.distance) / 2;
}

function formatMeters(value) {
    return `${value.toFixed(1)} m`;
}

function updateRelativePanel() {
    relativeDistances.innerHTML = '';
    const selected = currentSensors.find((sensor) => sensor.name === selectedSensorName);

    if (!selected) {
        relativeDistances.innerHTML = '<div class="relative-placeholder">Select a sensor on the map to compare.</div>';
        return;
    }

    currentSensors
        .filter((sensor) => sensor.name !== selected.name)
        .map((sensor) => {
            const rawDistance = calculateRelativeDistance(selected, sensor);
            return {
                name: sensor.name,
                value: rawDistance,
                label: formatMeters(rawDistance)
            };
        })
        .sort((a, b) => a.value - b.value)
        .forEach((entry) => {
            const row = document.createElement('div');
            row.className = 'relative-item';
            row.innerHTML = `<span>${entry.name}</span><strong>${entry.label}</strong>`;
            relativeDistances.appendChild(row);
        });
}

if (buildingGrid && statusSummary && relativeDistances) {
    initializeFacilityMap();
}

// Update the dashboard every 3 seconds
setInterval(updateDashboard, 3000);

// Run the function once when the page first loads
updateDashboard();