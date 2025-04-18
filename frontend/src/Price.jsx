import React, {useEffect, useState} from "react";

export default function Price({sym}) {
    const [p, setP] = useState("–");
    useEffect(() => {
        if (!sym) {
            setP("–");
            return;
        }
        const f = () => fetch(`/price?symbol=${sym}`)
            .then(r => r.json()).then(j => setP(j.price ? Number(j.price).toFixed(4) : "Err"))
            .catch(() => setP("Err"));
        f();
        const id = setInterval(f, 3_000);
        return () => clearInterval(id);
    }, [sym]);
    return <span style={{fontFamily: "monospace"}}>{p}</span>;
}
