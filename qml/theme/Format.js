.pragma library

// Display helpers only. Exact integer byte totals remain available as strings
// in tooltips; Javascript numbers cannot represent every 64-bit integer.
function size(bytes) {
    if (bytes === undefined || bytes === null || !isFinite(Number(bytes)))
        return "—";
    var value = Math.max(0, Number(bytes));
    var units = ["B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"];
    var i = 0;
    while (value >= 1024 && i < units.length - 1) {
        value /= 1024;
        i++;
    }
    return value.toLocaleString(Qt.locale(), 'f', i === 0 ? 0 : 1) + " " + units[i];
}

function exactSize(decimal) {
    if (!decimal || decimal === "0")
        return "0 B";
    var value = Number(decimal);
    return (value > Number.MAX_SAFE_INTEGER ? "≈" : "") + size(value);
}

function speed(value, measured) {
    return measured ? size(value) + "/s" : "Waiting for sample";
}

function protocol(value) {
    return value === 6 ? "TCP" : value === 17 ? "UDP" : String(value);
}

function timestamp(seconds) {
    if (!seconds && seconds !== 0)
        return "—";
    return new Date(Number(seconds) * 1000).toLocaleString(Qt.locale(), Locale.ShortFormat);
}

function relative(seconds) {
    if (!seconds && seconds !== 0)
        return "—";
    var elapsed = Math.floor(Date.now() / 1000 - Number(seconds));
    if (elapsed < 0) return "Future timestamp";
    if (elapsed < 60) return elapsed + "s ago";
    if (elapsed < 3600) return Math.floor(elapsed / 60) + "m ago";
    if (elapsed < 86400) return Math.floor(elapsed / 3600) + "h ago";
    return Math.floor(elapsed / 86400) + "d ago";
}

function endpoint(name, ip, port, protocolNumber) {
    // Observed DNS name is an association, not a verified owner.
    var address = ip.indexOf(":") >= 0 ? "[" + ip + "]" : ip;
    var host = name ? name + " → " + address : address;
    if (port === undefined || port === null) return host;
    return host + ":" + port + " · " + protocol(protocolNumber);
}
