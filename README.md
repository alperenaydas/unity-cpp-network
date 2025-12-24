PurposeEngine: Authoritative Network Synchronization
===========
A custom-build, tick-based networking architecture designed for real-time games. This project implements a high-performance **Authorative Server** in C++ and a **Predictive Client** in Unity, focused on achieving the "Gold Standard" of network synchronization.

## Key Technical Features
- **Authorative Server (C++17)**: A high-throughput, headless dedicated server built with <b>ENet (UDP)</b>. It manages the "Source of Truth" through fixed-tick physics simulation (50hz)
- **Client-Side Prediction**: Immediate local response for player inputs, eliminating feeling of network latency.
- **Deterministic Server Reconciliation**: A robust history-buffer system that detects state divergence and re-simulates local physics to match the server's authoratitive state.
- **Native Interop (C++/C#)**: Low-level C++ DLL integration with Unity via P/Invoke, utilizing thread-safe Ring Buffers for zero-allocation state updates.
- **Headless Bot Testing**: A standalone C++ bot client for stress-testing server logic and input handling without the overhead of a game engine.

## Tech Stack
- **Language**: C++17 (Server/Plugin), C# (Unity Client)
- **Networking**: ENet (UDP reliable/unreliable channels)
- **Engine**: Unity (Client-side rendering & prediction)
- **Arcihtecture**: Decoupled Transport/Logic layers, Thread-safe shared buffers.

## Architecture Overview
  The system is split into three distinct layers to ensure scalability and maintability.
  1. **Transport Layer**: Handles raw UDP packet serialization and ENet peer programming.
  2. **Logic Layer**: A tick-based simulation where inputs are processed and physics are integrated.
  3. **Synchronization Layer**: Manages the history of local predictions and performs the "Rewind & Replay" logic when a server connection arrives.

## Project Roadmap
- [x] Authoritative Movement Logic
- [x] Client-side Prediction & Reconciliation
- [x] Standalone Stress-Test Bot
- [ ] Entity Interpolation: Sliding remote players smoothly between network snapshots.
- [ ] Rotation/Yaw Synchronization: Optimizing packet structure for orientation.
- [ ] Lag Compensation: Implementing the "Rewind" logic for hit registration.

## Getting Started
  Prerequisites:
  - CMake 3.10+
  - Unity 6000.3.2f1
  - ENet Library

## Build
```
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

