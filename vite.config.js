import { defineConfig } from 'vite'
import legacy from '@vitejs/plugin-legacy'

export default defineConfig({
  plugins: [
    legacy({
      targets: ['defaults', 'not IE 11']
    })
  ],
  build: {
    target: 'es2020',
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: true,
        drop_debugger: true
      }
    },
    rollupOptions: {
      output: {
        manualChunks: {
          three: ['three'],
          physics: ['cannon-es'],
          animation: ['gsap'],
          postprocessing: ['postprocessing']
        }
      }
    }
  },
  server: {
    host: '0.0.0.0',
    port: 3000
  },
  optimizeDeps: {
    include: ['three', 'cannon-es', 'gsap', 'postprocessing']
  }
})

