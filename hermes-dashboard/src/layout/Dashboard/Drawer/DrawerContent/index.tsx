import Navigation from './Navigation';
import SimpleBar from 'components/third-party/SimpleBar';
import { useGetMenuMaster } from 'api/menu';

// NavCard foi removido (propaganda do template original)

export default function DrawerContent() {
  const { menuMaster } = useGetMenuMaster();
  const _drawerOpen = menuMaster.isDashboardDrawerOpened;

  return (
    <SimpleBar sx={{ '& .simplebar-content': { display: 'flex', flexDirection: 'column' } }}>
      <Navigation />
    </SimpleBar>
  );
}
