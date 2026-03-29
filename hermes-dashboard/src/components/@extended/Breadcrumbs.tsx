import React, { useEffect, useState } from 'react';
import type { ComponentType, ReactNode } from 'react';
import { Link, useLocation } from 'react-router-dom';

import { useTheme } from '@mui/material/styles';
import type { SxProps, Theme } from '@mui/material/styles';
import Divider from '@mui/material/Divider';
import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';
import MuiBreadcrumbs from '@mui/material/Breadcrumbs';

import MainCard from 'components/MainCard';
import navigation from 'menu-items';

import ApartmentOutlined from '@ant-design/icons/ApartmentOutlined';
import HomeOutlined from '@ant-design/icons/HomeOutlined';
import HomeFilled from '@ant-design/icons/HomeFilled';

interface MenuItem {
  id: string;
  type?: string;
  url?: string;
  title?: string;
  icon?: ComponentType;
  breadcrumbs?: boolean;
  children?: MenuItem[];
  link?: string;
}

interface BreadcrumbLink {
  title: string;
  to?: string;
  icon?: ComponentType;
}

interface BreadcrumbsProps {
  card?: boolean;
  custom?: boolean;
  divider?: boolean;
  heading?: string;
  icon?: boolean;
  icons?: boolean;
  links?: BreadcrumbLink[];
  maxItems?: number;
  rightAlign?: boolean;
  separator?: ComponentType<{ style?: React.CSSProperties }>;
  title?: boolean;
  titleBottom?: boolean;
  sx?: SxProps<Theme>;
  [key: string]: unknown;
}

export default function Breadcrumbs({
  card = false,
  custom = false,
  divider = false,
  heading,
  icon,
  icons,
  links,
  maxItems,
  rightAlign,
  separator,
  title = true,
  titleBottom = true,
  sx,
  ...others
}: BreadcrumbsProps) {
  const theme = useTheme();
  const location = useLocation();

  const [main, setMain] = useState<MenuItem | undefined>();
  const [item, setItem] = useState<MenuItem | undefined>();

  const iconSX = {
    marginRight: theme.spacing(0.75),
    marginLeft: 0,
    width: '1rem',
    height: '1rem',
    color: theme.palette.secondary.main
  };

  const customLocation = location.pathname;

  useEffect(() => {
    (navigation as { items: MenuItem[] }).items?.forEach((menu) => {
      if (menu.type === 'group') {
        if (menu.url && menu.url === customLocation) { setMain(menu); setItem(menu); }
        else getCollapse(menu);
      }
    });
  });

  const getCollapse = (menu: MenuItem) => {
    if (!custom && menu.children) {
      menu.children.forEach((collapse) => {
        if (collapse.type === 'collapse') {
          getCollapse(collapse);
          if (collapse.url === customLocation) { setMain(collapse); setItem(collapse); }
        } else if (collapse.type === 'item' && customLocation === collapse.url) {
          setMain(menu);
          setItem(collapse);
        }
      });
    }
  };

  const SeparatorIcon = separator;
  const separatorIcon = SeparatorIcon
    ? React.createElement(SeparatorIcon, { style: { fontSize: '0.75rem', marginTop: 2 } })
    : '/';

  let mainContent: ReactNode;
  let itemContent: ReactNode;
  let breadcrumbContent: ReactNode = <Typography />;
  let itemTitle = '';

  if (main && main.type === 'collapse' && main.breadcrumbs !== false) {
    const CollapseIcon = main.icon ?? ApartmentOutlined;
    mainContent = (
      <Typography
        {...(main.url && { component: Link, to: main.url })}
        variant={window.location.pathname === main.url ? 'subtitle1' : 'h6'}
        sx={{ textDecoration: 'none' }}
        color={window.location.pathname === main.url ? 'text.primary' : 'text.secondary'}
      >
        {icons && <CollapseIcon style={iconSX} />}
        {main.title}
      </Typography>
    );
  }

  if ((item && item.type === 'item') || (item?.type === 'group' && item.url) || custom) {
    itemTitle = item?.title ?? '';
    const ItemIcon = item?.icon ?? ApartmentOutlined;
    itemContent = (
      <Typography variant="subtitle1" color="text.primary">
        {icons && <ItemIcon style={iconSX} />}
        {itemTitle}
      </Typography>
    );

    let tempContent = (
      <MuiBreadcrumbs aria-label="breadcrumb" maxItems={maxItems ?? 8} separator={separatorIcon}>
        <Typography component={Link} to="/" color="text.secondary" variant="h6" sx={{ textDecoration: 'none' }}>
          {icons && <HomeOutlined style={iconSX} />}
          {icon && !icons && <HomeFilled style={{ ...iconSX, marginRight: 0 }} />}
          {(!icon || icons) && 'Home'}
        </Typography>
        {mainContent}
        {itemContent}
      </MuiBreadcrumbs>
    );

    if (custom && links && links.length > 0) {
      tempContent = (
        <MuiBreadcrumbs aria-label="breadcrumb" maxItems={maxItems ?? 8} separator={separatorIcon}>
          {links.map((link, index) => {
            const CollapseIcon = link.icon ?? ApartmentOutlined;
            return (
              <Typography
                key={index}
                {...(link.to && { component: Link, to: link.to })}
                variant={!link.to ? 'subtitle1' : 'h6'}
                sx={{ textDecoration: 'none' }}
                color={!link.to ? 'text.primary' : 'text.secondary'}
              >
                {link.icon && <CollapseIcon style={iconSX} />}
                {link.title}
              </Typography>
            );
          })}
        </MuiBreadcrumbs>
      );
    }

    if (item?.breadcrumbs !== false || custom) {
      breadcrumbContent = (
        <MainCard
          border={card}
          sx={card === false ? { mb: 3, bgcolor: 'inherit', backgroundImage: 'none', ...sx } : { mb: 3, ...sx }}
          {...others}
          content={card}
          shadow="none"
        >
          <Grid
            container
            direction={rightAlign ? 'row' : 'column'}
            spacing={1}
            sx={{ justifyContent: rightAlign ? 'space-between' : 'flex-start', alignItems: rightAlign ? 'center' : 'flex-start' }}
          >
            {title && !titleBottom && (
              <Grid>
                <Typography variant="h2">{custom ? heading : item?.title}</Typography>
              </Grid>
            )}
            <Grid>{tempContent}</Grid>
            {title && titleBottom && (
              <Grid sx={{ mt: card === false ? 0.25 : 1 }}>
                <Typography variant="h2">{custom ? heading : item?.title}</Typography>
              </Grid>
            )}
          </Grid>
          {card === false && divider !== false && <Divider sx={{ mt: 2 }} />}
        </MainCard>
      );
    }
  }

  return <>{breadcrumbContent}</>;
}
