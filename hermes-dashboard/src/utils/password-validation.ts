export const isNumber = (value: string) => /^(?=.*[0-9]).+$/.test(value);
export const isLowercaseChar = (value: string) => /^(?=.*[a-z]).+$/.test(value);
export const isUppercaseChar = (value: string) => /^(?=.*[A-Z]).+$/.test(value);
export const isSpecialChar = (value: string) => /^(?=.*[-+_!@#$%^&*.,?]).+$/.test(value);
export const minLength = (value: string) => value.length > 7;
