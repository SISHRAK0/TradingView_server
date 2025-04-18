import React, {useEffect, useState} from "react";
import {
    LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid
} from "recharts";
import Price from "./Price.jsx";

/* ► инлайн‑CSS – работает без Tailwind */
const styles = {
    container: {padding: 24, fontFamily: "Inter, sans-serif"},
    card: {background: "#fff", boxShadow: "0 1px 3px rgba(0,0,0,.1)", padding: 16, borderRadius: 8},
    btn: {background: "#2563eb", color: "#fff", padding: "0 16px", borderRadius: 4}
};

const EXCHANGES = ["BINANCE", "FINANDY", "BYBIT", "OKX"];

const algoName = (id) => ({
    "-1": "—",
    0: "RSI+MA",
    1: "RSI",
    2: "BB+MA",
    3: "MACD"
})[id] ?? "?";

export default function App() {
    const [symbols, setSymbols] = useState([]);
    const [signals, setSignals] = useState([]);
    const [cfgs, setCfgs] = useState({});
    const [klines, setKlines] = useState([]);
    const [selected, setSelected] = useState(null);

    /* ---- формы ---- */
    const [form, setForm] = useState({symbol: "", ex: "BINANCE", tf: "1m", algo: -1, qty: 0.001});
    const [keys, setKeys] = useState({exchange: "BINANCE", key: "", secret: ""});
    const [finFile, setFinFile] = useState(null);

    /* ---- symbols ---- */
    useEffect(() => {
        const f = () =>
            fetch("/symbols")
                .then(r => r.json())
                .then(d => setSymbols(Array.isArray(d) ? d : []))
                .catch(() => setSymbols([]));
        f();
        const id = setInterval(f, 2_000);
        return () => clearInterval(id);
    }, []);

    /* ---- signals ---- */
    useEffect(() => {
        const f = () =>
            fetch("/signals")
                .then(r => r.json())
                .then(d => setSignals(Array.isArray(d) ? d : []))
                .catch(() => setSignals([]));
        f();
        const id = setInterval(f, 5_000);
        return () => clearInterval(id);
    }, []);

    /* ---- actions ---- */
    const addSymbol = async () => {
        const sym = form.symbol.trim().toUpperCase();
        if (!sym) return;
        const payload = {...form, symbol: sym};
        await fetch("/symbols", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: JSON.stringify(payload)
        });
        await fetch("/config", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: JSON.stringify(payload)
        });
        setCfgs(p => ({...p, [sym]: payload}));
        setForm(f => ({...f, symbol: ""}));
    };

    const fetchChart = async (sym) => {
        try {
            const tf = cfgs[sym]?.tf || "1m";
            const r = await fetch(`/klines?symbol=${sym}&tf=${tf}&limit=200`);
            const d = await r.json();
            if (Array.isArray(d)) {
                setKlines(d.map(c => ({
                    t: new Date(c[0]).toLocaleTimeString(),
                    close: c[1],
                    ma: c[2],
                    rsi: c[3],
                    bbU: c[4],
                    bbL: c[5],
                    macd: c[6],
                    macdSig: c[7]
                })));
                setSelected(sym);
            } else {
                setKlines([]);
                setSelected(null);
            }
        } catch {
            setKlines([]);
            setSelected(null);
        }
    };

    const saveKeys = async () => {
        await fetch("/settings/keys", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: JSON.stringify(keys)
        });
        alert("Keys saved");
    };

    const uploadFinandy = async (e) => {
        const file = e.target.files[0];
        if (!file) return;
        setFinFile(file);
        const txt = await file.text();
        await fetch("/settings/finandy/json", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: txt
        });
        alert("Finandy JSON uploaded");
    };

    const last = klines.length > 0 ? klines[klines.length - 1] : null;

    return (
        <div style={styles.container}>
            {/* watch form */}
            <div style={{display: "flex", gap: 8}}>
                <input
                    style={{flex: 1, padding: 8, border: "1px solid #ccc"}}
                    placeholder="BTCUSDT"
                    value={form.symbol}
                    onChange={e => setForm(f => ({...f, symbol: e.target.value}))}
                />
                <select
                    style={{padding: 8}}
                    value={form.ex}
                    onChange={e => setForm(f => ({...f, ex: e.target.value}))}
                >
                    {EXCHANGES.map(x => <option key={x}>{x}</option>)}
                </select>
                <select
                    style={{padding: 8}}
                    value={form.tf}
                    onChange={e => setForm(f => ({...f, tf: e.target.value}))}
                >
                    {["1m", "5m", "15m", "1h"].map(x => <option key={x}>{x}</option>)}
                </select>
                <select
                    style={{padding: 8}}
                    value={form.algo}
                    onChange={e => setForm(f => ({...f, algo: +e.target.value}))}
                >
                    <option value={-1}>— none —</option>
                    <option value={0}>RSI+MA</option>
                    <option value={1}>RSI only</option>
                    <option value={2}>BB+MA</option>
                    <option value={3}>MACD cross</option>
                </select>
                <input
                    type="number"
                    step="0.0001"
                    value={form.qty}
                    onChange={e => setForm(f => ({...f, qty: +e.target.value}))}
                    style={{width: 80, padding: 8, border: "1px solid #ccc"}}
                />
                <button style={styles.btn} onClick={addSymbol}>Add</button>
            </div>

            {/* keys form */}
            <div style={{...styles.card, marginTop: 24}}>
                <h3 style={{marginBottom: 8, fontWeight: 600}}>API keys</h3>
                <div style={{display: "flex", gap: 8, alignItems: "center"}}>
                    <select
                        value={keys.exchange}
                        onChange={e => setKeys(k => ({...k, exchange: e.target.value}))}
                        style={{padding: 8}}
                    >
                        {EXCHANGES.map(x => <option key={x}>{x}</option>)}
                    </select>
                    <input
                        placeholder="key"
                        value={keys.key}
                        onChange={e => setKeys(k => ({...k, key: e.target.value}))}
                        style={{flex: 1, padding: 8, border: "1px solid #ccc"}}
                    />
                    <input
                        placeholder="secret"
                        value={keys.secret}
                        onChange={e => setKeys(k => ({...k, secret: e.target.value}))}
                        style={{flex: 1, padding: 8, border: "1px solid #ccc"}}
                    />
                    <button style={styles.btn} onClick={saveKeys}>Save</button>
                </div>
            </div>

            {/* finandy json */}
            <div style={{...styles.card, marginTop: 16}}>
                <h3 style={{marginBottom: 8, fontWeight: 600}}>Finandy JSON template</h3>
                <input type="file" accept="application/json" onChange={uploadFinandy}/>
                {finFile && <span style={{marginLeft: 8}}>{finFile.name}</span>}
            </div>

            {/* watchlist & signals */}
            <div style={{display: "grid", gridTemplateColumns: "1fr 1fr", gap: 24, marginTop: 24}}>
                {/* watchlist */}
                <div style={styles.card}>
                    <h3 style={{marginBottom: 8, fontWeight: 600}}>Watchlist</h3>
                    <table style={{width: "100%", fontSize: 14}}>
                        <thead>
                        <tr>
                            <th>Sym</th>
                            <th>Algo</th>
                            <th>Price</th>
                            <th></th>
                        </tr>
                        </thead>
                        <tbody>
                        {symbols?.map(s => (
                            <tr key={s}>
                                <td style={{padding: 4, fontFamily: "monospace"}}>{s}</td>
                                <td>{algoName(cfgs[s]?.algo ?? -1)}</td>
                                <td><Price sym={s}/></td>
                                <td>
                                    <button onClick={() => fetchChart(s)}>📈</button>
                                </td>
                            </tr>
                        ))}
                        </tbody>
                    </table>
                </div>

                {/* signals */}
                <div style={styles.card}>
                    <h3 style={{marginBottom: 8, fontWeight: 600}}>Signals</h3>
                    <ul style={{height: 200, overflowY: "auto", fontSize: 14, margin: 0, paddingLeft: 16}}>
                        {signals?.map((s, i) => <li key={i}>{s}</li>)}
                    </ul>
                </div>
            </div>

            {/* chart */}
            {selected && klines.length > 0 && (
                <div style={{...styles.card, marginTop: 24}}>
                    <h3 style={{marginBottom: 8, fontWeight: 600}}>
                        {selected} ({cfgs[selected]?.tf || "1m"})
                    </h3>
                    <div style={{height: 300}}>
                        <ResponsiveContainer width="100%" height="100%">
                            <LineChart data={klines}>
                                <CartesianGrid strokeDasharray="3 3"/>
                                <XAxis dataKey="t" hide/>
                                <YAxis yAxisId="left" domain={["auto", "auto"]}/>
                                <Tooltip/>
                                <Line yAxisId="left" type="monotone" dataKey="close" dot={false} stroke="#2563eb"/>
                                <Line yAxisId="left" type="monotone" dataKey="ma" dot={false} stroke="#f97316"
                                      strokeDasharray="4 4"/>
                                <Line yAxisId="left" type="monotone" dataKey="bbU" dot={false} stroke="#64748b"
                                      strokeDasharray="2 2"/>
                                <Line yAxisId="left" type="monotone" dataKey="bbL" dot={false} stroke="#64748b"
                                      strokeDasharray="2 2"/>
                            </LineChart>
                        </ResponsiveContainer>
                    </div>
                    {last && (
                        <div style={{marginTop: 8, fontSize: 14}}>
                            <span style={{marginRight: 16}}>RSI: {last.rsi.toFixed(2)}</span>
                            <span style={{marginRight: 16}}>MA: {last.ma.toFixed(2)}</span>
                            <span>MACD: {last.macd.toFixed(4)}</span>
                        </div>
                    )}
                </div>
            )}
        </div>
    );
}
