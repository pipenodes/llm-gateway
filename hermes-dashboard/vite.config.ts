import { defineConfig, loadEnv } from 'vite';
import react from '@vitejs/plugin-react';
import tsconfigPaths from 'vite-tsconfig-paths';
import { visualizer } from 'rollup-plugin-visualizer';
import { VitePWA } from 'vite-plugin-pwa';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '');
  const BASE_URL = env.VITE_APP_BASE_NAME || '/';
  const PORT = 3000;

  return {
    base: BASE_URL,
    server: {
      open: true,
      port: PORT,
      host: true,
      proxy: {
        '/health': 'http://localhost:8080',
        '/metrics': 'http://localhost:8080',
        '/admin': 'http://localhost:8080',
        '/v1': 'http://localhost:8080'
      }
    },
    preview: {
      open: true,
      host: true
    },
    define: {
      global: 'window'
    },
    plugins: [
      react(),
      tsconfigPaths(),

      // PWA — manifest + service worker (cache-first para assets estáticos)
      VitePWA({
        registerType: 'autoUpdate',
        includeAssets: ['favicon.svg', 'robots.txt'],
        manifest: {
          name: 'Produto Enterprise',
          short_name: 'Enterprise',
          description: 'Aplicação Enterprise Class com React + MUI',
          theme_color: '#1677ff',
          background_color: '#f5f5f5',
          display: 'standalone',
          orientation: 'portrait-primary',
          start_url: '/',
          scope: '/',
          icons: [
            { src: '/pwa-192x192.svg', sizes: '192x192', type: 'image/svg+xml' },
            { src: '/pwa-512x512.svg', sizes: '512x512', type: 'image/svg+xml' },
            { src: '/pwa-512x512.svg', sizes: '512x512', type: 'image/svg+xml', purpose: 'any maskable' }
          ]
        },
        workbox: {
          // Cache de assets estáticos com hash (imutáveis)
          globPatterns: ['**/*.{js,css,html,ico,png,svg,woff2}'],
          runtimeCaching: [
            {
              // Fontes Google (se usadas)
              urlPattern: /^https:\/\/fonts\.googleapis\.com\/.*/i,
              handler: 'CacheFirst',
              options: { cacheName: 'google-fonts-cache', expiration: { maxEntries: 10, maxAgeSeconds: 60 * 60 * 24 * 365 } }
            },
            {
              // API — network-first com fallback
              urlPattern: ({ url }) => url.pathname.startsWith('/api/'),
              handler: 'NetworkFirst',
              options: { cacheName: 'api-cache', expiration: { maxEntries: 50, maxAgeSeconds: 300 } }
            }
          ]
        }
      }),

      // Bundle analyzer — ativo apenas com: npm run build:analyze
      mode === 'analyze' && visualizer({
        filename: 'dist/stats.html',
        open: true,
        gzipSize: true,
        brotliSize: true,
        template: 'treemap'
      })
    ].filter(Boolean),

    build: {
      chunkSizeWarningLimit: 1000,
      sourcemap: true,
      cssCodeSplit: true,
      rollupOptions: {
        output: {
          // Chunks manuais para separar vendor libs pesadas
          manualChunks: (id) => {
            // React core
            if (id.includes('node_modules/react/') || id.includes('node_modules/react-dom/') || id.includes('node_modules/react-router')) return 'vendor-react';
            // Emotion (base de MUI — deve vir antes de @mui)
            if (id.includes('node_modules/@emotion/')) return 'vendor-emotion';
            // MUI X (charts e data-grid — separado de @mui/material para evitar circular)
            if (id.includes('node_modules/@mui/x-')) return 'vendor-mui-x';
            // MUI core
            if (id.includes('node_modules/@mui/')) return 'vendor-mui';
            // i18n
            if (id.includes('node_modules/i18next') || id.includes('node_modules/react-i18next')) return 'vendor-i18n';
            // Icons Ant Design
            if (id.includes('node_modules/@ant-design/')) return 'vendor-icons';
            // Formulários
            if (id.includes('node_modules/formik') || id.includes('node_modules/yup')) return 'vendor-form';
            // SWR
            if (id.includes('node_modules/swr')) return 'vendor-swr';
          },
          chunkFileNames: 'js/[name]-[hash].js',
          entryFileNames: 'js/[name]-[hash].js',
          assetFileNames: (assetInfo) => {
            const name = assetInfo.name ?? '';
            const ext = name.split('.').pop() ?? 'bin';
            if (/\.css$/.test(name)) return `css/[name]-[hash].${ext}`;
            if (/\.(png|jpe?g|gif|svg|webp|ico)$/.test(name)) return `images/[name]-[hash].${ext}`;
            if (/\.(woff2?|eot|ttf|otf)$/.test(name)) return `fonts/[name]-[hash].${ext}`;
            return `assets/[name]-[hash].${ext}`;
          }
        }
      },
      ...(mode === 'production' && {
        esbuild: {
          drop: ['console', 'debugger'],
          pure: ['console.log', 'console.info', 'console.debug', 'console.warn']
        }
      })
    },
    optimizeDeps: {
      include: ['@mui/material/Tooltip', 'react', 'react-dom', 'react-router-dom']
    }
  };
});
