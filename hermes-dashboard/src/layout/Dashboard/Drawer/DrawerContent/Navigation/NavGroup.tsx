import List from '@mui/material/List';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';

import NavItem from './NavItem';
import type { NavItemConfig } from './NavItem';
import { useGetMenuMaster } from 'api/menu';

interface NavGroupConfig {
  id: string;
  type: string;
  title?: string;
  children?: NavItemConfig[];
}

export default function NavGroup({ item }: { item: NavGroupConfig }) {
  const { menuMaster } = useGetMenuMaster();
  const drawerOpen = menuMaster.isDashboardDrawerOpened;

  const navCollapse = item.children?.map((menuItem) => {
    if (menuItem.type === 'item') {
      return <NavItem key={menuItem.id} item={menuItem} level={1} />;
    }
    return (
      <Typography key={menuItem.id} variant="caption" color="error" sx={{ p: 2.5 }}>
        {menuItem.type}
      </Typography>
    );
  });

  return (
    <List
      subheader={
        item.title && drawerOpen ? (
          <Box sx={{ pl: 3, mb: 1.5 }}>
            <Typography variant="subtitle2" color="textSecondary">
              {item.title}
            </Typography>
          </Box>
        ) : undefined
      }
      sx={{ mb: drawerOpen ? 1.5 : 0, py: 0, zIndex: 0 }}
    >
      {navCollapse}
    </List>
  );
}
