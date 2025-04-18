import {defineConfig} from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
    plugins: [react()],
    server: {
        proxy: {
            "/symbols": "http://localhost:8080",
            "/price": "http://localhost:8080",
            "/klines": "http://localhost:8080",
            "/config": "http://localhost:8080",
            "/signals": "http://localhost:8080",
            "/settings": "http://localhost:8080"
        }
    }
});
