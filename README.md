# Purpose Networking Engine
> A high-performance, competitive multiplayer networking architecture built from scratch in C++ and Unity. Optimized for high-density "Deathball" scenarios.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Unity](https://img.shields.io/badge/Unity-6000.3.2f1-black.svg)
![Networking](https://img.shields.io/badge/Networking-ENet%20%7C%20UDP-red.svg)

## Overview
**Purpose** is a custom game networking stack designed for **competitive, high-frequency (50Hz+) shooters**. Unlike generic solutions, it implements a custom bit-packed protocol and a **Hybrid Spatial-Priority Architecture** to achieve minimal bandwidth usage and sub-tick precision even in extreme congestion.

The core objective was to engineer a system capable of handling **1,000+ concurrent entities** on a single thread with **Server-Side Rewind (Lag Compensation)**, **Priority Sorting**, and **Safety-Aware Delta Compression**, proving that high-performance networking doesn't require massive cloud infrastructure—just efficient code.

## Key Features

### 1. Hybrid Spatial-Priority Management
- **O(N) Visibility:** Replaced O(N²) broadcasts with a spatial hashing grid (25m buckets).
- **Priority Sorting:** Instead of blindly sending all nearby entities (which overflows MTU), the server sorts neighbors by distance and sends the **closest 80 entities**.
- **Dynamic Culling:** Clients only receive updates for high-priority entities, preventing the "Disappearing Player" bug in crowded areas.

### 2. Competitive Architecture
- **Authoritative Server:** C++ Dedicated Server running a fixed-step 50Hz physics loop.
- **Client-Side Prediction:** Unity client predicts movement immediately; server reconciles only on divergence (>10cm).
- **Lag Compensation:** Server rewinds hitboxes to the exact millisecond the client fired, ensuring "What you see is what you hit."

### 3. Safety-Aware Protocol (Hybrid Bit-Packing)
- **Variable-Resolution Streams:** Critical packets use a custom `BitWriter` to pack data tight (e.g., Booleans take 1 bit, Rotations take 8 bits).
- **The "Safety Zone":** Implements a 25m "Guaranteed Update" radius. Entities further away force a full position update, preventing the "Zero Spawn" bug when entities emerge from culling.
- **Delta Checks:** The server compares the current state against the client's last ACKed tick to skip sending redundant data (saving ~90% bandwidth).

### 4. Specialized Client Modes
- **Visual/Logic Separation:** Local player logic moves at 50Hz (FixedUpdate) while visuals interpolate smoothly at Render Rate (144Hz+) using an Offset-Aware Smoother.
- **Continuous Interpolation:** Uses a continuous clock (`_clientRenderTime += Time.deltaTime`) instead of discrete snapshots, eliminating jitter for remote entities.
- **Spectator Mode:** "God View" network priority. Spectators bypass the spatial grid to receive the full entity stream for debugging.

---

## Tech Stack

| Component | Technology | Description |
| :--- | :--- | :--- |
| **Server** | C++17 | ENet Host, Priority Sorting, Spatial Hashing, Physics, Lag Comp. |
| **Client Native** | C++ (DLL) | Thread-safe SPSC Queues, metric tracking, packet unpacking. |
| **Client Game** | Unity (C#) | Gameplay, Continuous Interpolation, Visual Smoothing, Prediction. |
| **Transport** | ENet (UDP) | Reliable/Unreliable channels, fragmentation handling. |

---

### The "Deathball" Solution
To scale beyond 100 players in a small area, *Purpose* abandons the concept of a simple "Radius Broadcast."
1.  **Gather:** The Spatial Grid collects potentially 200+ candidates.
2.  **Filter:** The server calculates `DistanceSq` for all candidates.
3.  **Sort & Cap:** `std::partial_sort` selects the **top 80 closest** entities.
4.  **Result:** Bandwidth never exceeds `MTU_SIZE` (1400 bytes). Nearby enemies (high threat) update perfectly; distant enemies (low threat) update less frequently or are culled.

## Engineering Deep Dive

### The "BitStream" Protocol
Standard C structs align to bytes (8 bits). In *Purpose*, we treat the packet as a continuous stream of bits. This allows us to use variable-length encoding for game state.
- **Boolean Flags:** Take exactly 1 bit.
- **Quantized Floats:** World positions are converted to integers (1cm precision), ensuring determinism across different CPU architectures.
- **Delta Check:** Before writing a position, the server compares it to the client's last acknowledged state. If the value hasn't changed, a single `0` bit is written, skipping the 32-bit payload entirely.

### Thread Safety (Lock-Free)
To prevent Unity's Garbage Collector from crashing the network thread:
1. **Native Plugin:** Runs on its own thread managed by ENet.
2. **SPSC Queue:** A Single-Producer Single-Consumer queue marshals packet data safely to the Unity Main Thread.
3. **Zero Allocation:** Packet memory is pooled (`MAX_PACKET_POOL_SIZE`) to avoid runtime allocations during gameplay.

---

## Performance Metrics
*Measurements validated with 100 concurrent clients in a high-density "Deathball" scenario.*

| Metric | Measured Value | Explanation |
| :--- | :--- | :--- |
| **Tick Rate** | **50 Hz** | Fixed physics/network step. |
| **Bandwidth (Per Client)** | **~40 - 70 KB/s** | Hard-capped by MTU (1400B) to prevent fragmentation. |
| **Total Server Upstream** | **~6.8 MB/s** | With 100 active clients receiving full updates. |
| **Packet Loss** | **< 0.1%** | Local loopback; SPSC queue prevents buffer overflow drops. |
| **Scalability** | **1,000+ Entities** | Spatial Grid ensures O(1) lookups; only local neighbors (~80) are serialized. |

---

## License
This project is open-source under the MIT License.
