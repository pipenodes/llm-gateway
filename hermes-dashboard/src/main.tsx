import { createRoot } from 'react-dom/client';

import 'i18n';

import 'assets/style.css';
import 'simplebar-react/dist/simplebar.min.css';

import '@fontsource/public-sans/400.css';
import '@fontsource/public-sans/500.css';
import '@fontsource/public-sans/600.css';
import '@fontsource/public-sans/700.css';

import App from './App';
import { ConfigProvider } from 'contexts/ConfigContext';
import { initWebVitals } from 'lib/vitals';

const container = document.getElementById('root');
if (!container) throw new Error('Root element not found');

createRoot(container).render(
  <ConfigProvider>
    <App />
  </ConfigProvider>
);

initWebVitals();
