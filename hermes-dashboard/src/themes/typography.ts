export default function Typography(fontFamily: string) {
  return {
    htmlFontSize: 16,
    fontFamily,
    fontWeightLight: 300,
    fontWeightRegular: 400,
    fontWeightMedium: 500,
    fontWeightBold: 600,
    h1: { fontWeight: 600, fontSize: '2.375rem', lineHeight: 1.21 },
    h2: { fontWeight: 600, fontSize: '1.875rem', lineHeight: 1.27 },
    h3: { fontWeight: 600, fontSize: '1.5rem', lineHeight: 1.33 },
    h4: { fontWeight: 600, fontSize: '1.25rem', lineHeight: 1.4 },
    h5: { fontWeight: 600, fontSize: '1rem', lineHeight: 1.5 },
    // spec: h6 weight 600 (era 400)
    h6: { fontWeight: 600, fontSize: '0.875rem', lineHeight: 1.57 },
    caption: { fontWeight: 400, fontSize: '0.75rem', lineHeight: 1.66 },
    body1: { fontSize: '0.875rem', lineHeight: 1.57 },
    body2: { fontSize: '0.75rem', lineHeight: 1.66 },
    // spec: subtitle1 16px (era 14px), weight 500
    subtitle1: { fontSize: '1rem', fontWeight: 500, lineHeight: 1.57 },
    // spec: subtitle2 14px (era 12px), weight 500
    subtitle2: { fontSize: '0.875rem', fontWeight: 500, lineHeight: 1.57 },
    overline: { lineHeight: 1.66 },
    // spec: button 14px weight 500 line-height 1.75 — RF-00-UX-DEFAULT-TOKENS §4
    button: { textTransform: 'capitalize' as const, fontWeight: 500, lineHeight: 1.75 }
  };
}
