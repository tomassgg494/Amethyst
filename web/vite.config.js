import vue from '@vitejs/plugin-vue'
import { defineConfig } from 'vite'

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue()],
  // BASE_PATH=/Amethyst/ when deploying to GitHub Pages project site
  base: process.env.BASE_PATH || '/',
})
