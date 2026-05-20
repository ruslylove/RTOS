<script setup lang="ts">
import { computed } from 'vue'
import { useNav } from '@slidev/client'

const { currentPage, total, currentLayout } = useNav()

// The cover is slide 1 and uses the `cover` layout — keep it clean there.
const hideOnCover = computed(
  () => currentPage.value === 1 || currentLayout?.value === 'cover',
)
</script>

<template>
  <footer v-if="!hideOnCover" class="rtos-page-number">
    {{ currentPage }} / {{ total }}
  </footer>
</template>

<style scoped>
.rtos-page-number {
  position: absolute;
  right: 0;
  bottom: 0;
  margin: 0.55rem 0.9rem;
  font-size: 0.72rem;
  font-variant-numeric: tabular-nums;
  letter-spacing: 0.02em;
  color: #003874;
  opacity: 0.5;
  user-select: none;
  z-index: 10;
}
:global(.dark) .rtos-page-number {
  color: #7ba7d9;
}
</style>
