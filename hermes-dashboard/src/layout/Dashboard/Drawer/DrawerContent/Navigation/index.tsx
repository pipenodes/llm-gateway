import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';

import NavGroup from './NavGroup';
import menuItem from 'menu-items';

export default function Navigation() {
  const navGroups = (menuItem as { items: Array<{ id: string; type: string; title?: string; children?: unknown[] }> }).items.map((item) => {
    if (item.type === 'group') {
      return <NavGroup key={item.id} item={item as never} />;
    }
    return (
      <Typography key={item.id} variant="h6" color="error" align="center">
        Fix - Navigation Group
      </Typography>
    );
  });

  return <Box sx={{ pt: 2 }}>{navGroups}</Box>;
}
