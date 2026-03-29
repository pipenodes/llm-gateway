import { useState } from 'react';
import FormControl from '@mui/material/FormControl';
import InputAdornment from '@mui/material/InputAdornment';
import OutlinedInput from '@mui/material/OutlinedInput';
import Box from '@mui/material/Box';
import Chip from '@mui/material/Chip';

import SearchOutlined from '@ant-design/icons/SearchOutlined';
import CommandPalette from 'components/CommandPalette';

export default function Search() {
  const [paletteOpen, setPaletteOpen] = useState(false);

  return (
    <Box sx={{ width: '100%', ml: { xs: 0, md: 1 } }}>
      <FormControl sx={{ width: { xs: '100%', md: 224 } }}>
        <OutlinedInput
          size="small"
          readOnly
          onClick={() => setPaletteOpen(true)}
          startAdornment={
            <InputAdornment position="start" sx={{ mr: -0.5 }}>
              <SearchOutlined />
            </InputAdornment>
          }
          endAdornment={
            <InputAdornment position="end">
              <Chip label="Ctrl+K" size="small" variant="outlined" sx={{ height: 20, fontSize: '0.65rem', cursor: 'pointer' }} />
            </InputAdornment>
          }
          aria-describedby="header-search-text"
          slotProps={{ input: { 'aria-label': 'Abrir busca rápida' } }}
          placeholder="Buscar..."
          sx={{ cursor: 'pointer', '& input': { cursor: 'pointer' } }}
        />
      </FormControl>

      <CommandPalette open={paletteOpen} onClose={() => setPaletteOpen(false)} />
    </Box>
  );
}
