const hasNumber = (value: string) => /[0-9]/.test(value);
const hasMixed = (value: string) => /[a-z]/.test(value) && /[A-Z]/.test(value);
const hasSpecial = (value: string) => /[!#@$%^&*)(+=._-]/.test(value);

export const strengthColor = (count: number): { label: string; color: string } => {
  if (count < 2) return { label: 'Poor', color: 'error.main' };
  if (count < 3) return { label: 'Weak', color: 'warning.main' };
  if (count < 4) return { label: 'Normal', color: 'warning.dark' };
  if (count < 5) return { label: 'Good', color: 'success.main' };
  if (count < 6) return { label: 'Strong', color: 'success.dark' };
  return { label: 'Poor', color: 'error.main' };
};

export const strengthIndicator = (value: string): number => {
  let strengths = 0;
  if (value.length > 5) strengths += 1;
  if (value.length > 7) strengths += 1;
  if (hasNumber(value)) strengths += 1;
  if (hasSpecial(value)) strengths += 1;
  if (hasMixed(value)) strengths += 1;
  return strengths;
};
