<script setup lang="ts">
import { computed } from 'vue'
import { useNav, useSlideContext } from '@slidev/client'

// Rendered on the TOP layer so it stays visible above each slide's content.
const { total, go } = useNav()
// `$page` is THIS slide's own number (per-slide; correct in print/export too,
// unlike useNav().currentPage which is the single global "current" page).
const { $page } = useSlideContext()

// Course Contents is slide 2 of the master deck (cover = 1, contents = 2).
const TOC_PAGE = 2
const showPageNumber = computed(() => $page.value !== 1)
const showTocButton = computed(
  () => $page.value !== 1 && $page.value !== TOC_PAGE,
)
</script>

<template>
  <footer v-if="showPageNumber" class="rtos-page-number">
    {{ $page }} / {{ total }}
  </footer>

  <!-- Jump-to-Course-Contents control, sits just above the navbar -->
  <button
    v-if="showTocButton"
    class="rtos-toc-button"
    title="Course Contents"
    aria-label="Go to Course Contents"
    @click="go(TOC_PAGE)"
  >
    <svg
      viewBox="0 0 24 24" width="14" height="14" fill="none"
      stroke="currentColor" stroke-width="2.4"
      stroke-linecap="round" stroke-linejoin="round"
    >
      <line x1="9" y1="6" x2="20" y2="6" />
      <line x1="9" y1="12" x2="20" y2="12" />
      <line x1="9" y1="18" x2="20" y2="18" />
      <circle cx="4.3" cy="6" r="1.4" fill="currentColor" stroke="none" />
      <circle cx="4.3" cy="12" r="1.4" fill="currentColor" stroke="none" />
      <circle cx="4.3" cy="18" r="1.4" fill="currentColor" stroke="none" />
    </svg>
  </button>
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
  pointer-events: none;
  z-index: 20;
}
:global(.dark) .rtos-page-number {
  color: #7ba7d9;
}

.rtos-toc-button {
  position: absolute;
  left: 0.7rem;
  bottom: 2.55rem;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 1.6rem;
  height: 1.6rem;
  padding: 0;
  border: 1px solid rgba(0, 56, 116, 0.3);
  border-radius: 9999px;
  background: rgba(255, 255, 255, 0.72);
  color: #003874;
  opacity: 0.45;
  cursor: pointer;
  pointer-events: auto;
  transition: opacity 0.18s ease, background 0.18s ease, transform 0.18s ease;
  z-index: 20;
}
.rtos-toc-button:hover {
  opacity: 1;
  background: #ffffff;
  transform: translateY(-1px);
}
:global(.dark) .rtos-toc-button {
  border-color: rgba(123, 167, 217, 0.4);
  background: rgba(22, 28, 40, 0.72);
  color: #7ba7d9;
}
:global(.dark) .rtos-toc-button:hover {
  background: rgba(32, 40, 56, 0.95);
}
</style>
