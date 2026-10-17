<template>
  <div class="chart-wrapper">
    <div class="chart-header">
      <div class="chart-title">{{ title }}</div>
      <div class="chart-controls">
        <button @click="resetZoom" class="btn-icon" title="Reset Zoom">
          <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
            <path d="M8 3.5a.5.5 0 0 0-1 0V9a.5.5 0 0 0 .252.434l3.5 2a.5.5 0 0 0 .496-.868L8 8.71V3.5z"/>
            <path d="M8 16A8 8 0 1 0 8 0a8 8 0 0 0 0 16zm7-8A7 7 0 1 1 1 8a7 7 0 0 1 14 0z"/>
          </svg>
        </button>
      </div>
    </div>
    <div class="chart-container">
      <canvas ref="chartRef"></canvas>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch, onBeforeUnmount } from 'vue'
import { Chart, registerables } from 'chart.js'
import zoomPlugin from 'chartjs-plugin-zoom'
import 'chartjs-adapter-date-fns'

Chart.register(...registerables, zoomPlugin)

const props = defineProps({
  data: {
    type: Object,
    required: true,
  },
  title: {
    type: String,
    default: '',
  },
  yAxisLabel: {
    type: String,
    default: '',
  },
  unit: {
    type: String,
    default: '',
  },
})

const chartRef = ref(null)
let chartInstance = null

const createChart = () => {
  if (chartInstance) {
    chartInstance.destroy()
  }

  if (!chartRef.value || !props.data) return

  const datasets = props.data.datasets.map(dataset => ({
    ...dataset,
    borderWidth: 1,
    borderColor: 'rgb(37, 99, 235)',
    backgroundColor: 'rgba(37, 99, 235, 0.6)',
  }))

  chartInstance = new Chart(chartRef.value, {
    type: 'bar',
    data: {
      ...props.data,
      datasets,
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: {
        duration: 750,
      },
      plugins: {
        legend: {
          display: false,
        },
        tooltip: {
          mode: 'index',
          intersect: false,
          backgroundColor: 'rgba(0, 0, 0, 0.8)',
          titleColor: '#fff',
          bodyColor: '#fff',
          borderColor: 'rgba(255, 255, 255, 0.2)',
          borderWidth: 1,
          padding: 12,
          displayColors: true,
          callbacks: {
            label: function(context) {
              let label = context.dataset.label || ''
              if (label) {
                label += ': '
              }
              if (context.parsed.y !== null) {
                label += context.parsed.y.toFixed(2)
                if (props.unit) {
                  label += ' ' + props.unit
                }
              }
              return label
            }
          }
        },
        zoom: {
          pan: {
            enabled: true,
            mode: 'x',
            modifierKey: 'ctrl',
          },
          zoom: {
            wheel: {
              enabled: true,
            },
            pinch: {
              enabled: true,
            },
            mode: 'x',
          },
          limits: {
            x: { min: 'original', max: 'original' },
          },
        },
      },
      scales: {
        x: {
          type: 'time',
          time: {
            displayFormats: {
              hour: 'HH:mm',
              minute: 'HH:mm',
              day: 'MMM d',
            },
          },
          grid: {
            display: true,
            color: 'rgba(0, 0, 0, 0.05)',
          },
          ticks: {
            maxRotation: 0,
            autoSkipPadding: 20,
          },
        },
        y: {
          beginAtZero: true,
          title: {
            display: !!props.yAxisLabel,
            text: props.yAxisLabel,
          },
          grid: {
            display: true,
            color: 'rgba(0, 0, 0, 0.05)',
          },
        },
      },
      interaction: {
        intersect: false,
        mode: 'index',
      },
    },
  })
}

const resetZoom = () => {
  if (chartInstance) {
    chartInstance.resetZoom()
  }
}

onMounted(() => {
  createChart()
})

watch(() => props.data, (newData) => {
  if (chartInstance && newData) {
    // Update chart data without recreating (smoother updates)
    chartInstance.data = {
      ...newData,
      datasets: newData.datasets.map(dataset => ({
        ...dataset,
        borderWidth: 1,
        borderColor: 'rgb(37, 99, 235)',
        backgroundColor: 'rgba(37, 99, 235, 0.6)',
      })),
    }
    chartInstance.update('none') // Update without animation for live data
  } else {
    createChart()
  }
}, { deep: true })

onBeforeUnmount(() => {
  if (chartInstance) {
    chartInstance.destroy()
  }
})
</script>

<style scoped>
.chart-wrapper {
  width: 100%;
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.chart-title {
  font-size: 14px;
  font-weight: 600;
  color: #374151;
}

.chart-controls {
  display: flex;
  gap: 8px;
}

.btn-icon {
  background: #f3f4f6;
  border: 1px solid #e5e7eb;
  border-radius: 6px;
  padding: 6px 8px;
  cursor: pointer;
  color: #6b7280;
  transition: all 0.2s;
  display: flex;
  align-items: center;
  justify-content: center;
}

.btn-icon:hover {
  background: #e5e7eb;
  color: #374151;
}

.chart-container {
  position: relative;
  height: 350px;
  width: 100%;
}
</style>
