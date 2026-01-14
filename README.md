# Purpose Networking Engine
> A high-performance, competitive multiplayer networking architecture built from scratch in C++ and Unity.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Unity](https://img.shields.io/badge/Unity-6000.3.2f1-black.svg)
![Networking](https://img.shields.io/badge/Networking-ENet%20%7C%20UDP-red.svg)

## Overview
**Purpose** is a custom game networking stack designed for **competitive, high-frequency (50Hz+) shooters**. Unlike generic solutions, it implements a custom bit-packed protocol to achieve minimal bandwidth usage and sub-tick precision.

The core objective was to engineer a system capable of handling **1,000+ concurrent entities** on a single thread with **Server-Side Rewind (Lag Compensation)** and **Spatial Grid Interest Management**, proving that high-performance networking doesn't require massive cloud infrastructure—just efficient code.

## Key Features

### 1. Spatial Grid Interest Management (SGIM)
- **O(N) Visibility:** Replaced O(N²) broadcasts with a spatial hashing grid (5m buckets).
- **Dynamic Culling:** Clients only receive updates for entities within their immediate 9-cell neighborhood.
- **Massive Scale:** Supports **1,000+ active bots** in the world while keeping individual player bandwidth under **1 Mbps**.

### 2. Competitive Architecture
- **Authoritative Server:** C++ Dedicated Server running a fixed-step 50Hz physics loop.
- **Client-Side Prediction:** Unity client predicts movement immediately; server reconciles only on divergence.
- **Lag Compensation:** Server rewinds hitboxes to the exact millisecond the client fired, ensuring "What you see is what you hit."

### 3. Custom Protocol (Hybrid Bit-Packing)
- **Variable-Resolution Streams:** Critical packets (WorldState, Input) use a custom `BitWriter` to pack data tight (e.g., Booleans take 1 bit, Rotations take 8 bits).
- **Quantized Positions:** World coordinates are transmitted as 1cm-precision integers instead of raw floats, reducing payload size by >50%.
- **Delta Checks:** The server compares the current state against the client's last ACKed tick to skip sending redundant data.

### 4. Specialized Client Modes
- **Player Mode:** Subject to grid culling, physics, and prediction logic.
- **Spectator Mode:** "God View" network priority. Spectators bypass the spatial grid to receive the full 1,000-entity stream for debugging and recording.

---

## Tech Stack

| Component | Technology | Description |
| :--- | :--- | :--- |
| **Server** | C++ | Core logic, ENet wrapper, Spatial Hashing, Physics, Lag Comp history buffer. |
| **Client Native** | C++ (DLL) | Network service layer, metric tracking, packet unpacking. |
| **Client Game** | Unity (C#) | Gameplay, interpolation, prediction, rendering. |
| **Transport** | ENet (UDP) | Reliable/Unreliable channels, fragmentation handling. |

---

### The "Spatial Grid" Architecture
To scale beyond 100 players, *Purpose* abandons the concept of a "Global Broadcast."
1.  **Grid Hashing:** The world is divided into buckets. Entities function as keys in a `std::unordered_map`.
2.  **Neighbor Lookup:** When building a packet for Client A, the server queries only the 9 surrounding buckets.
3.  **Result:** A server with 1,000 bots scattered across the map sends only ~50 entities to any given client, keeping packet size tiny while maintaining a massive persistent world.

## Engineering Deep Dive

### The "BitStream" Protocol
Standard C structs align to bytes (8 bits). In *Purpose*, we treat the packet as a continuous stream of bits. This allows us to use variable-length encoding for game state.
- **Boolean Flags:** Take exactly 1 bit.
- **Quantized Floats:** World positions are converted to integers, ensuring determinism across different CPU architectures.
- **Delta Check:** Before writing a position, the server compares it to the client's last acknowledged state. If the value hasn't changed, a single `0` bit is written, skipping the 32-bit payload entirely.

### Server-Side Rewind (Lag Compensation)
When a player fires, the packet includes the `RenderTick` they were looking at.
1. Server receives `Fire` command.
2. Server looks up `HistoryBuffer` for that exact tick.
3. Server performs a **Sub-Tick Lerp** between snapshots to reconstruct the world state at `T - Ping`.
4. Raycast is performed against these "ghost" positions.

---

## Installation & Build

### Prerequisites
- [CMake](https://cmake.org/) (3.10 or later).
- C++17 compliant compiler (MSVC, GCC, or Clang).
- Unity 6000.3.2f1.
- [ENet](http://enet.bespin.org/) (Included in source).

### 1. Build the Native Stack
Run the following commands in the root directory to build the Server executable and the Client
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
Artifacts location:
- Windows: build/Release/
- Linux/Mac: build/

### 2. Setup Unity
1. Copy `PurposeClient.dll` into `UnityProject/Assets/Plugins/`.
2. Open the Unity Project.
3. Open `Scenes/GameScene`.

### 3. Run the Stress Test
1. Press **Play** in Unity to join as a spectator/player.
2. Start `PurposeServer.exe`.
3. Run `StressTest.bat` (launches 100 headless bot instances).

---

## Performance Metrics
*Measurements taken with 100 concurrent clients on local server.*

| Metric | Value | Note |
| :--- | :--- | :--- |
| **Tick Rate** | 50Hz | Fixed Step |
| **Bandwidth (Active)** | ~850 KB/s | Per Client (1000 Players moving) |
| **Bandwidth (Idle)** | ~40-100 KB/s | Dependent on Grid Density |
| **Packet Loss** | < 0.1% | Local Loopback (Corrected for Fragmentation) |

---

## License
This project is open-source under the MIT License.
