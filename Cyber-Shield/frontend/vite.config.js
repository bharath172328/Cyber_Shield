import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    host: true,
    // In dev, forward /api to the C++ backend so no CORS setup is needed.
    proxy: { '/api': { target: process.env.VITE_PROXY_TARGET || 'http://localhost:8080', changeOrigin: true } },
  },
  preview: { port: 4173, proxy: { '/api': { target: 'http://localhost:8080', changeOrigin: true } } },
  test: {
    environment: 'jsdom',
    environmentOptions: { jsdom: { url: 'http://localhost:5173' } },
    globals: true,
    testTimeout: 30000,
  },
});
