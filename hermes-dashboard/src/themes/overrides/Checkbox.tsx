import getColors from 'utils/getColors';
import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';
import BorderOutlined from '@ant-design/icons/BorderOutlined';
import CheckSquareFilled from '@ant-design/icons/CheckSquareFilled';
import MinusSquareFilled from '@ant-design/icons/MinusSquareFilled';

function getColorStyle({ color, theme }: { color: string; theme: Theme }) {
  const colors = getColors(theme, color);
  const { lighter, main, dark } = colors;
  return {
    '&:hover': { backgroundColor: lighter ?? 'transparent', '& .icon': { borderColor: main } },
    '&.Mui-focusVisible': { outline: `2px solid ${dark}`, outlineOffset: -4 }
  };
}

function checkboxStyle(size: 'small' | 'medium' | 'large') {
  const fontSize = size === 'small' ? 1.15 : size === 'large' ? 1.6 : 1.35;
  return { '& .icon': { fontSize: `${fontSize}rem` } };
}

export default function Checkbox(theme: Theme) {
  const p = tv(theme);
  const secondary = p.secondary as unknown as Record<string, string>;
  return {
    MuiCheckbox: {
      defaultProps: {
        className: 'size-small',
        icon: <BorderOutlined className="icon" />,
        checkedIcon: <CheckSquareFilled className="icon" />,
        indeterminateIcon: <MinusSquareFilled className="icon" />
      },
      styleOverrides: {
        root: {
          borderRadius: 0,
          color: secondary[300],
          '&.size-small': checkboxStyle('small'),
          '&.size-medium': checkboxStyle('medium'),
          '&.size-large': checkboxStyle('large')
        },
        colorPrimary: getColorStyle({ color: 'primary', theme }),
        colorSecondary: getColorStyle({ color: 'secondary', theme }),
        colorSuccess: getColorStyle({ color: 'success', theme }),
        colorWarning: getColorStyle({ color: 'warning', theme }),
        colorInfo: getColorStyle({ color: 'info', theme }),
        colorError: getColorStyle({ color: 'error', theme })
      }
    }
  };
}
