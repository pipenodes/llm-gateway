import { RouterProvider } from 'react-router-dom';

import ThemeCustomization from 'themes';
import ScrollTop from 'components/ScrollTop';
import { AnnouncerProvider } from 'contexts/AnnouncerContext';
import { SnackbarProvider } from 'contexts/SnackbarContext';
import { SWRConfigProvider } from 'contexts/SWRConfigProvider';
import { AuthProvider } from 'contexts/AuthContext';
import router from 'routes';

export default function App() {
  return (
    <ThemeCustomization>
      <AuthProvider>
        <AnnouncerProvider>
          <SnackbarProvider>
            <SWRConfigProvider>
              <ScrollTop>
                <RouterProvider router={router} />
              </ScrollTop>
            </SWRConfigProvider>
          </SnackbarProvider>
        </AnnouncerProvider>
      </AuthProvider>
    </ThemeCustomization>
  );
}
