import os
import re
import xml.etree.ElementTree as ET

RESULT_DIR = "results/xml"

results = []

for filename in os.listdir(RESULT_DIR):

    if not filename.endswith(".xml"):
        continue

    filepath = os.path.join(RESULT_DIR, filename)

    # Parse metadata dari nama file
    pattern = r"results-(\w+)-(\d+)nodes-(\d+)bytes-(\d+)sources\.xml"

    match = re.match(pattern, filename)

    if not match:
        print(f"Skip invalid filename: {filename}")
        continue

    routing = match.group(1).upper()
    nodes = int(match.group(2))
    packet_size = int(match.group(3))
    sources = int(match.group(4))

    # Parse XML
    tree = ET.parse(filepath)
    root = tree.getroot()

    flow_stats = root.find("FlowStats")

    total_tx = 0
    total_rx = 0

    total_delay_ns = 0
    total_jitter_ns = 0

    flow_count = 0

    for flow in flow_stats:

        tx_packets = int(flow.attrib["txPackets"])
        rx_packets = int(flow.attrib["rxPackets"])

        delay_ns = float(
            flow.attrib["delaySum"]
            .replace("+", "")
            .replace("ns", "")
        )

        jitter_ns = float(
            flow.attrib["jitterSum"]
            .replace("+", "")
            .replace("ns", "")
        )

        total_tx += tx_packets
        total_rx += rx_packets

        total_delay_ns += delay_ns
        total_jitter_ns += jitter_ns

        flow_count += 1

    # QoS calculation
    avg_delay_ms = (
        (total_delay_ns / total_rx) / 1e6
        if total_rx > 0 else 0
    )

    avg_jitter_ms = (
        (total_jitter_ns / total_rx) / 1e6
        if total_rx > 0 else 0
    )

    packet_loss_percent = (
        ((total_tx - total_rx) / total_tx) * 100
        if total_tx > 0 else 0
    )

    results.append({
        "routing": routing,
        "nodes": nodes,
        "packet_size": packet_size,
        "sources": sources,
        "delay_ms": avg_delay_ms,
        "jitter_ms": avg_jitter_ms,
        "packet_loss": packet_loss_percent
    })

# Sorting hasil
results = sorted(
    results,
    key=lambda x: (
        x["routing"],
        x["nodes"],
        x["packet_size"],
        x["sources"]
    )
)

# Print table
print("\n=== QoS COMPARISON RESULTS ===\n")

for r in results:

    print(
        f"{r['routing']:5} | "
        f"{r['nodes']:2} Nodes | "
        f"{r['packet_size']:4} Bytes | "
        f"{r['sources']} Src | "
        f"Delay: {r['delay_ms']:.3f} ms | "
        f"Jitter: {r['jitter_ms']:.3f} ms | "
        f"Loss: {r['packet_loss']:.2f}%"
    )
