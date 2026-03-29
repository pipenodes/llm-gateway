import type { ComponentType, MouseEvent } from 'react';
import { Link, useLocation, matchPath } from 'react-router-dom';

import useMediaQuery from '@mui/material/useMediaQuery';
import Avatar from '@mui/material/Avatar';
import Chip from '@mui/material/Chip';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Tooltip from '@mui/material/Tooltip';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';

import IconButton from 'components/@extended/IconButton';
import { handlerDrawerOpen, useGetMenuMaster } from 'api/menu';

interface ChipConfig {
  color?: 'primary' | 'secondary' | 'error' | 'info' | 'success' | 'warning';
  variant?: 'filled' | 'outlined';
  size?: 'small' | 'medium';
  label: string;
  avatar?: string;
}

interface ActionConfig {
  type: 'function' | 'link';
  icon: ComponentType<{ style?: React.CSSProperties }>;
  function?: () => void;
  url?: string;
  target?: boolean;
}

export interface NavItemConfig {
  id: string;
  title: string;
  type: string;
  url?: string;
  icon?: ComponentType<{ style?: React.CSSProperties }>;
  target?: boolean;
  disabled?: boolean;
  chip?: ChipConfig;
  breadcrumbs?: boolean;
  link?: string;
  actions?: ActionConfig[];
}

interface NavItemProps {
  item: NavItemConfig;
  level: number;
  isParents?: boolean;
  setSelectedID?: (id: string) => void;
}

export default function NavItem({ item, level, isParents = false, setSelectedID }: NavItemProps) {
  const { menuMaster } = useGetMenuMaster();
  const drawerOpen = menuMaster.isDashboardDrawerOpened;
  const downLG = useMediaQuery((theme: import('@mui/material/styles').Theme) => theme.breakpoints.down('lg'));

  const itemTarget = item.target ? '_blank' : '_self';

  const itemHandler = () => {
    if (downLG) handlerDrawerOpen(false);
    if (isParents && setSelectedID) setSelectedID(item.id);
  };

  const Icon = item.icon;
  const itemIcon = Icon ? (
    <Icon style={{ fontSize: drawerOpen ? '1rem' : '1.25rem', ...(isParents && { fontSize: 20 }) }} />
  ) : null;

  const { pathname } = useLocation();
  const isSelected = !!matchPath({ path: item.link ?? item.url ?? '', end: false }, pathname);

  const textColor = 'text.primary';
  const iconSelectedColor = 'primary.main';

  return (
    <Box sx={{ position: 'relative' }}>
      <ListItemButton
        component={Link}
        to={item.url ?? '#'}
        target={itemTarget}
        disabled={item.disabled}
        selected={isSelected}
        sx={(theme) => ({
          zIndex: 1201,
          pl: drawerOpen ? `${level * 28}px` : 1.5,
          py: !drawerOpen && level === 1 ? 1.25 : 1,
          ...(drawerOpen && {
            '&:hover': { bgcolor: 'primary.lighter' },
            '&.Mui-selected': {
              bgcolor: 'primary.lighter',
              borderRight: '2px solid',
              borderColor: 'primary.main',
              color: iconSelectedColor,
              '&:hover': { color: iconSelectedColor, bgcolor: 'primary.lighter' }
            }
          }),
          ...(!drawerOpen && {
            '&:hover': { bgcolor: 'transparent' },
            '&.Mui-selected': { '&:hover': { bgcolor: 'transparent' }, bgcolor: 'transparent' }
          })
        })}
        onClick={itemHandler}
      >
        {itemIcon && (
          // spec: RF-00-UX-DEFAULT-APP-SHELL — ícones colapsados exibem Tooltip com o nome do item
          !drawerOpen && level === 1 ? (
            <Tooltip title={item.title} placement="right" arrow>
              <ListItemIcon
                sx={{
                  minWidth: 28,
                  color: isSelected ? iconSelectedColor : textColor,
                  borderRadius: 1.5,
                  width: 36,
                  height: 36,
                  alignItems: 'center',
                  justifyContent: 'center',
                  '&:hover': { bgcolor: 'secondary.lighter' },
                  ...(isSelected && { bgcolor: 'primary.lighter', '&:hover': { bgcolor: 'primary.lighter' } })
                }}
              >
                {itemIcon}
              </ListItemIcon>
            </Tooltip>
          ) : (
            <ListItemIcon
              sx={{
                minWidth: 28,
                color: isSelected ? iconSelectedColor : textColor,
                ...(!drawerOpen && {
                  borderRadius: 1.5,
                  width: 36,
                  height: 36,
                  alignItems: 'center',
                  justifyContent: 'center',
                  '&:hover': { bgcolor: 'secondary.lighter' }
                }),
                ...(!drawerOpen && isSelected && { bgcolor: 'primary.lighter', '&:hover': { bgcolor: 'primary.lighter' } })
              }}
            >
              {itemIcon}
            </ListItemIcon>
          )
        )}
        {(drawerOpen || (!drawerOpen && level !== 1)) && (
          <ListItemText
            primary={
              <Typography variant="h6" sx={{ color: isSelected ? iconSelectedColor : textColor }}>
                {item.title}
              </Typography>
            }
          />
        )}
        {(drawerOpen || (!drawerOpen && level !== 1)) && item.chip && (
          <Chip
            color={item.chip.color}
            variant={item.chip.variant}
            size={item.chip.size}
            label={item.chip.label}
            avatar={item.chip.avatar ? <Avatar>{item.chip.avatar}</Avatar> : undefined}
          />
        )}
      </ListItemButton>
      {(drawerOpen || (!drawerOpen && level !== 1)) &&
        item.actions?.map((action, index) => {
          const ActionIcon = action.icon;
          return (
            <IconButton
              key={index}
              {...(action.type === 'function' && {
                onClick: (event: MouseEvent) => { event.stopPropagation(); action.function?.(); }
              })}
              {...(action.type === 'link' && {
                component: Link,
                to: action.url ?? '#',
                target: action.target ? '_blank' : '_self'
              })}
              color="secondary"
              variant="outlined"
              sx={{
                position: 'absolute',
                top: 12,
                right: 20,
                zIndex: 1202,
                width: 20,
                height: 20,
                mr: -1,
                ml: 1,
                color: 'secondary.dark',
                borderColor: isSelected ? 'primary.light' : 'secondary.light',
                '&:hover': { borderColor: isSelected ? 'primary.main' : 'secondary.main' }
              }}
            >
              <ActionIcon style={{ fontSize: '0.625rem' }} />
            </IconButton>
          );
        })}
    </Box>
  );
}
