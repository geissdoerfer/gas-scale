<template>
  <div class="chart-container">
    <canvas ref="chartRef"></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { Chart, registerables } from 'chart.js'
import 'chartjs-adapter-date-fns'

Chart.register(...registerables)

const props = defineProps({
  data: {
    type: Object,
    required: true,
  },
  title: {
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

  if (!chartRef.value) return

  chartInstance = new Chart(chartRef.value, {
    type: 'line',
    data: props.data,
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: true,
          position: 'top',
        },
        title: {
          display: !!props.title,
          text: props.title,
        },
      },
      scales: {
        x: {
          type: 'time',
          time: {
            unit: 'hour',
          },
        },
        y: {
          beginAtZero: false,
        },
      },
      interaction: {
        intersect: false,
        mode: 'index',
      },
    },
  })
}

onMounted(() => {
  createChart()
})

watch(() => props.data, () => {
  createChart()
}, { deep: true })
</script>

<style scoped>
.chart-container {
  position: relative;
  height: 300px;
  width: 100%;
}
</style>
