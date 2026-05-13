const API_BASE_URL = "http://localhost:8000";

const temperatureValue = document.getElementById("temperatureValue");
const humidityValue = document.getElementById("humidityValue");
const lastUpdatedValue = document.getElementById("lastUpdatedValue");
const statusText = document.getElementById("statusText");
const statusDot = document.getElementById("statusDot");

const chartCanvas = document.getElementById("climateChart");

let climateChart = null;

function formatTime(timestamp) {
  const date = new Date(timestamp);

  return date.toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
  });
}

function formatDateTime(timestamp) {
  const date = new Date(timestamp);

  return date.toLocaleString([], {
    month: "short",
    day: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}

function setOnlineStatus(isOnline) {
  if (isOnline) {
    statusText.textContent = "Online";
    statusDot.classList.remove("offline");
    statusDot.classList.add("online");
  } else {
    statusText.textContent = "Offline";
    statusDot.classList.remove("online");
    statusDot.classList.add("offline");
  }
}

async function fetchLatestReading() {
  try {
    const response = await fetch(`${API_BASE_URL}/readings/latest`);

    if (!response.ok) {
      throw new Error("Could not fetch latest reading");
    }

    const reading = await response.json();

    temperatureValue.textContent = reading.temperature.toFixed(1);
    humidityValue.textContent = reading.humidity.toFixed(1);
    lastUpdatedValue.textContent = formatDateTime(reading.timestamp);

    setOnlineStatus(true);
  } catch (error) {
    console.error(error);
    setOnlineStatus(false);
  }
}

async function fetchLast24Hours() {
  try {
    const response = await fetch(`${API_BASE_URL}/readings/last-24h`);

    if (!response.ok) {
      throw new Error("Could not fetch 24-hour readings");
    }

    const readings = await response.json();

    updateChart(readings);
    setOnlineStatus(true);
  } catch (error) {
    console.error(error);
    setOnlineStatus(false);
  }
}

function updateChart(readings) {
  const labels = readings.map((reading) => formatTime(reading.timestamp));
  const temperatureData = readings.map((reading) => reading.temperature);
  const humidityData = readings.map((reading) => reading.humidity);

  if (climateChart === null) {
    climateChart = new Chart(chartCanvas, {
      type: "line",
      data: {
        labels: labels,
        datasets: [
          {
            label: "Temperature (°C)",
            data: temperatureData,
            tension: 0.3,
          },
          {
            label: "Humidity (%)",
            data: humidityData,
            tension: 0.3,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: true,
        scales: {
          y: {
            beginAtZero: false,
          },
        },
      },
    });

    return;
  }

  climateChart.data.labels = labels;
  climateChart.data.datasets[0].data = temperatureData;
  climateChart.data.datasets[1].data = humidityData;
  climateChart.update();
}

async function refreshDashboard() {
  await fetchLatestReading();
  await fetchLast24Hours();
}

refreshDashboard();

setInterval(refreshDashboard, 5000);
