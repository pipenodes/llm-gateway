import type { SxProps, Theme } from '@mui/material/styles';
import { Link } from 'react-router-dom';
import ButtonBase from '@mui/material/ButtonBase';

import Logo from './LogoMain';
import LogoIcon from './LogoIcon';
import { APP_DEFAULT_PATH } from 'config';

interface LogoSectionProps {
  isIcon?: boolean;
  sx?: SxProps<Theme>;
  to?: string;
}

export default function LogoSection({ isIcon, sx, to }: LogoSectionProps) {
  return (
    <ButtonBase disableRipple component={Link} to={to ?? APP_DEFAULT_PATH} sx={sx} aria-label="Logo">
      {isIcon ? <LogoIcon /> : <Logo />}
    </ButtonBase>
  );
}
