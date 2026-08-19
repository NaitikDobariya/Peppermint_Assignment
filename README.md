# Peppermint Robotics: Lane Navigation Planner

This project implements a standalone, C++17 based lane navigation planner for material handling robots operating in structured environments like hospitals or hotels. Instead of relying on unpredictable free-space planning, we constrain the robot to a predefined, directed graph of lanes. This ensures that the robot behaves predictably around people and takes the exact same route everyday.

<img width="2400" height="1600" alt="image" src="https://github.com/user-attachments/assets/c7adbdbd-7a8e-4245-818d-4b4dbfbbba59" />


The system features a simulated floor plan with over 42 nodes, including topologies like loops, one-way corridors, and lifts. The whole thing is built to be lightweight, with minimal/zero external dependencies for the core planner itself.

## Repository Structure

The codebase is organized as follows:

```text
.
├── CMakeLists.txt
├── README.md
├── data/
│   └── floor_plan.json               # The simulated lane network graph
├── images/                           # Generated route plots are saved here
├── include/
│   ├── graph.hpp
│   ├── json_exporter.hpp
│   └── planner.hpp
├── results/                          # Determinism test JSON outputs
├── scripts/
│   ├── plot_map.py                   # Python visualizer for the network
│   └── test_determinism.sh           # Determinism checker   
└── src/
    ├── graph.cpp
    ├── json_exporter.cpp
    ├── main.cpp
    └── planner.cpp

```

## Prerequisites

You do not necessarily need Docker to run this! A standard Linux machine will work perfectly fine. You just need:

* **C++17** and **CMake** (for building the core planner)


* **Python 3** with `matplotlib` (for generating the plots)



If you prefer using a containerized workflow (or run into dependency issues on your host machine), the repository includes a `.devcontainer` configuration. You can open the folder in VSCode or Cursor and select **"Reopen in Container"** to get a fully configured environment instantly.

## Installation & Execution

### 1. Build the Planner

Open your terminal, navigate to the root of the workspace, and run the standard CMake build steps:

```bash
mkdir build
cd build
cmake ..
make -j4

```

### 2. Run the Simulation

Execute the built binary. It will load the graph, process the hardcoded queries, evaluate drivability based on the vehicle's turning radius, and dump the results as JSON files:

```bash
./run_simulation

```

### 3. Visualize the Routes

Once the JSON routes are generated, use the Python script to plot them. The output images will be saved in the `/images` directory:

```bash
cd ..
python3 scripts/plot_map.py

```

## Algorithm & Geometry

The core engine behind the routing relies on a graph search algorithm algorithm similar to Dijkstra/A*. However, the cost model does not just blindly chase the shortest physical distance. It incorporates traversal penalties for executing tight turns, navigating through heavy-traffic intersections, or passing through doors. This makes the robot prefer smoother, safer main corridors over convoluted shortcuts.

When a route is found, the planner doesn't just spit out a list of node names. Our `json_exporter.cpp` serializes the path into explicit geometry. The downstream kinematics controller receives an array detailing whether a segment is a `STRAIGHT` line or an `ARC`, along with exact lane widths, curve radii, and speed limits, so it actually knows how to physically drive the path. We also do a drivability check against a minimum turning radius to ensure the forklift or robot can actually handle the planned corners.

## The Determinism Guarantee

This was the trickiest part of the build. In a hospital, a robot that arbitrarily picks a different corridor for no visible reason is a robot the staff will stop trusting. We need the exact same route, every single run, on every machine, under every optimization level.

To prove this holds, you can run our bash script:

```bash
./scripts/test_determinism.sh

```

This script builds the C++ code across six different compiler optimization levels (from `-O0` all the way up to `-Ofast`), runs them multiple times, and verifies the MD5 checksums of all the resulting JSON files.

**Where non-determinism could have crept in, and how we fixed it:**

* **Unordered Memory Iteration:** Using standard hash maps (`std::unordered_map`) results in unpredictable iteration orders based on memory seeds. We eliminated this by strictly using `std::map<int, Node>`, which uses Red-Black trees to guarantee deterministic, numerically sorted iteration across all compilers.
* **Tie-Breaking & Floating-Point Drift:** If two routes have the exact same cost, a standard priority queue might pop them in an arbitrary order. Furthermore, aggressive optimizations like `-Ofast` disregard strict IEEE math standards, which can slightly alter floating-point accumulated costs. We fixed this by embedding the unique integer `Node IDs` directly into the search states. If path costs are identical, the algorithm falls back to evaluating the lowest Node ID rather than relying on floating-point micro-comparisons.
* **Name Ambiguity:** If a user requests a route to "Charging_Bay", and there are three charging bays on the floor, a standard string map overwrite can cause unpredictable routing. We wrote a `get_deterministic_node_id` method that evaluates string requests and reliably resolves collisions by always routing to the target with the lowest internal integer ID.
