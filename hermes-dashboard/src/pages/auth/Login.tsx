import { useEffect, useState } from 'react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { useFormik } from 'formik';
import * as Yup from 'yup';
import { useTranslation } from 'react-i18next';

import Alert from '@mui/material/Alert';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import FormControl from '@mui/material/FormControl';
import FormHelperText from '@mui/material/FormHelperText';
import IconButton from '@mui/material/IconButton';
import InputAdornment from '@mui/material/InputAdornment';
import InputLabel from '@mui/material/InputLabel';
import OutlinedInput from '@mui/material/OutlinedInput';
import Paper from '@mui/material/Paper';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

import EyeOutlined from '@ant-design/icons/EyeOutlined';
import EyeInvisibleOutlined from '@ant-design/icons/EyeInvisibleOutlined';
import LockOutlined from '@ant-design/icons/LockOutlined';
import MailOutlined from '@ant-design/icons/MailOutlined';

import { useAuth } from 'contexts/AuthContext';
import { APP_DEFAULT_PATH } from 'config';

const USE_JWT = !!import.meta.env.VITE_AUTH_URL;

// ── Validation schemas ─────────────────────────────────────────────────────────

const keySchema = Yup.object({
  adminKey: Yup.string().min(8, 'auth.login.keyMinLength').required('auth.login.keyRequired')
});

const jwtSchema = Yup.object({
  email: Yup.string().email('auth.login.emailInvalid').required('auth.login.emailRequired'),
  password: Yup.string().min(6, 'auth.login.passwordMinLength').required('auth.login.passwordRequired')
});

// ── Component ──────────────────────────────────────────────────────────────────

export default function LoginPage() {
  const navigate = useNavigate();
  const [params] = useSearchParams();
  const { login, isAuthenticated } = useAuth();
  const { t } = useTranslation();
  const [showSecret, setShowSecret] = useState(false);
  const [loginError, setLoginError] = useState<string | null>(null);

  useEffect(() => { document.title = 'Login | HERMES Gateway'; }, []);

  useEffect(() => {
    if (isAuthenticated) {
      navigate(decodeURIComponent(params.get('redirectTo') ?? APP_DEFAULT_PATH), { replace: true });
    }
  }, [isAuthenticated, navigate, params]);

  // ── Admin key mode ───────────────────────────────────────────────────────────
  const keyFormik = useFormik({
    initialValues: { adminKey: '' },
    validationSchema: keySchema,
    onSubmit: async ({ adminKey }, { setSubmitting }) => {
      setLoginError(null);
      try {
        await login(adminKey);
        navigate(decodeURIComponent(params.get('redirectTo') ?? APP_DEFAULT_PATH), { replace: true });
      } catch (err) {
        setLoginError(err instanceof Error ? err.message : t('common.error'));
      } finally {
        setSubmitting(false);
      }
    }
  });

  // ── JWT mode ─────────────────────────────────────────────────────────────────
  const jwtFormik = useFormik({
    initialValues: { email: '', password: '' },
    validationSchema: jwtSchema,
    onSubmit: async ({ email, password }, { setSubmitting }) => {
      setLoginError(null);
      try {
        await login(email, password);
        navigate(decodeURIComponent(params.get('redirectTo') ?? APP_DEFAULT_PATH), { replace: true });
      } catch (err) {
        setLoginError(err instanceof Error ? err.message : t('common.error'));
      } finally {
        setSubmitting(false);
      }
    }
  });

  return (
    <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'center', minHeight: '100vh', bgcolor: 'background.default', px: 2, py: 4 }}>
      <Stack spacing={3} sx={{ width: '100%', maxWidth: 440 }}>
        <Stack alignItems="center" spacing={1}>
          <Typography variant="h3" component="h1" fontWeight={700} color="primary">
            {t('auth.login.title')}
          </Typography>
          <Typography variant="body2" color="text.secondary">
            {USE_JWT ? t('auth.login.subtitleJwt') : t('auth.login.subtitle')}
          </Typography>
        </Stack>

        <Paper elevation={0} sx={{ p: { xs: 3, sm: 4 }, border: '1px solid', borderColor: 'divider', borderRadius: 2 }}>
          <Stack spacing={3}>
            {loginError && (
              <Alert severity="error" onClose={() => setLoginError(null)}>{loginError}</Alert>
            )}

            {USE_JWT ? (
              // ── JWT login form ───────────────────────────────────────────────
              <form onSubmit={jwtFormik.handleSubmit} noValidate>
                <Stack spacing={2.5}>
                  <FormControl fullWidth error={jwtFormik.touched.email && Boolean(jwtFormik.errors.email)}>
                    <InputLabel htmlFor="email">{t('auth.login.emailLabel')}</InputLabel>
                    <OutlinedInput id="email" type="email" label={t('auth.login.emailLabel')} autoFocus autoComplete="email"
                      startAdornment={<InputAdornment position="start"><MailOutlined style={{ color: 'inherit' }} /></InputAdornment>}
                      {...jwtFormik.getFieldProps('email')}
                    />
                    {jwtFormik.touched.email && jwtFormik.errors.email && (
                      <FormHelperText>{t(jwtFormik.errors.email)}</FormHelperText>
                    )}
                  </FormControl>

                  <FormControl fullWidth error={jwtFormik.touched.password && Boolean(jwtFormik.errors.password)}>
                    <InputLabel htmlFor="password">{t('auth.login.passwordLabel')}</InputLabel>
                    <OutlinedInput id="password" type={showSecret ? 'text' : 'password'} label={t('auth.login.passwordLabel')} autoComplete="current-password"
                      startAdornment={<InputAdornment position="start"><LockOutlined style={{ color: 'inherit' }} /></InputAdornment>}
                      endAdornment={
                        <InputAdornment position="end">
                          <IconButton onClick={() => setShowSecret((v) => !v)} edge="end" size="small">
                            {showSecret ? <EyeInvisibleOutlined /> : <EyeOutlined />}
                          </IconButton>
                        </InputAdornment>
                      }
                      {...jwtFormik.getFieldProps('password')}
                    />
                    {jwtFormik.touched.password && jwtFormik.errors.password && (
                      <FormHelperText>{t(jwtFormik.errors.password)}</FormHelperText>
                    )}
                  </FormControl>

                  <Button type="submit" variant="contained" size="large" fullWidth disabled={jwtFormik.isSubmitting} sx={{ mt: 1, py: 1.5 }}>
                    {jwtFormik.isSubmitting ? t('auth.login.submitting') : t('auth.login.submit')}
                  </Button>
                </Stack>
              </form>
            ) : (
              // ── Admin key form ───────────────────────────────────────────────
              <form onSubmit={keyFormik.handleSubmit} noValidate>
                <Stack spacing={2.5}>
                  <FormControl fullWidth error={keyFormik.touched.adminKey && Boolean(keyFormik.errors.adminKey)}>
                    <InputLabel htmlFor="adminKey">{t('auth.login.adminKeyLabel')}</InputLabel>
                    <OutlinedInput id="adminKey" type={showSecret ? 'text' : 'password'} label={t('auth.login.adminKeyLabel')} autoFocus autoComplete="off"
                      startAdornment={<InputAdornment position="start"><LockOutlined style={{ color: 'inherit' }} /></InputAdornment>}
                      endAdornment={
                        <InputAdornment position="end">
                          <IconButton onClick={() => setShowSecret((v) => !v)} edge="end" size="small">
                            {showSecret ? <EyeInvisibleOutlined /> : <EyeOutlined />}
                          </IconButton>
                        </InputAdornment>
                      }
                      {...keyFormik.getFieldProps('adminKey')}
                    />
                    {keyFormik.touched.adminKey && keyFormik.errors.adminKey && (
                      <FormHelperText>{t(keyFormik.errors.adminKey)}</FormHelperText>
                    )}
                  </FormControl>

                  <Button type="submit" variant="contained" size="large" fullWidth disabled={keyFormik.isSubmitting} sx={{ mt: 1, py: 1.5 }}>
                    {keyFormik.isSubmitting ? t('auth.login.submitting') : t('auth.login.submit')}
                  </Button>
                </Stack>
              </form>
            )}

            <Typography variant="caption" color="text.secondary" align="center">
              {USE_JWT ? t('auth.login.hintJwt') : t('auth.login.hint')}
            </Typography>
          </Stack>
        </Paper>
      </Stack>
    </Box>
  );
}
