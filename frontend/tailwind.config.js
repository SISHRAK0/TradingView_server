/** @type {import('tailwindcss').Config} */
module.exports = {
    content: ['index.html', 'src/**/*.{js,jsx,ts,tsx}'],
    darkMode: 'class',
    theme: {
        extend: {
            colors: {
                primary: '#3b82f6',    // custom blue
                secondary: '#6366f1',  // indigo
                accent: '#f472b6',     // pink
                bg: '#f9fafb',         // light gray background
                card: '#ffffff',       // card backgrounds
            },
            fontFamily: {
                sans: ['Inter', 'ui-sans-serif', 'system-ui'],
            },
            boxShadow: {
                card: '0 4px 6px rgba(0, 0, 0, 0.1)',
            },
            borderRadius: {
                xl: '1rem',
            },
        },
    },
    plugins: [require('@tailwindcss/typography')],
};