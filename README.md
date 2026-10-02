# ADSA Problem C - Smart City Transport System

## Part I: bus routes and bus network (Hemsara)

A portable C++17 console simulation of the bus network in the supplied city map.
It represents all 20 city locations and the 23 solid blue bus links in a weighted,
undirected adjacency-list graph. Seven proposed bus service lines cover every
bus link. Timetables, direct trips with waiting times, and time-based bus movement
are available through the menu and command line.

| Member | Work |
| --- | --- |
| 1 - Hemsara | Shared graph, city data, bus network, Part I entry point |
| 2 | Train network (Part II) |
| 3 | Variable passenger demand (Part III) |
| 4 | Shortest paths and efficiency profiling (Part IV) |

The train, demand and routing source files remain placeholders for their owners.
Part I plans a direct trip on a **chosen bus line**. Automatic shortest paths,
bus changes, train operation and passenger demand belong to the remaining parts.

## Run in Visual Studio Code

1. Open this repository folder in **Visual Studio Code** (File > Open Folder).
2. Have a C++17 compiler available: Apple Clang on macOS; GCC/G++ on Linux or
   Windows (for example, an existing MinGW-w64 installation on Windows).
3. Press **Ctrl+Shift+B** (**Cmd+Shift+B** on macOS) to build.
4. Select **Terminal > Run Task > Run transport** to use the interactive menu.
5. Select **Run Part I demo** for a reproducible demonstration, or **Run bus tests**
   for the automated checks.

The checked-in tasks use Clang on macOS and `g++` elsewhere. All use C++17 and
compile the group source files. The Microsoft C/C++ extension is recommended for
editing and F5 debugging; it is not required to build or run. F5 also requires
LLDB on macOS or GDB on Windows/Linux, available to the extension.

## Build and run from the VS Code terminal

macOS:

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic -g src/*.cpp -o transport
./transport
```

Linux:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -g src/*.cpp -o transport
./transport
```

Windows PowerShell:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -g src/*.cpp -o transport.exe
.\transport.exe
```

No external C++ libraries, package downloads, or JSON parser are needed.

## Demonstrations

Use `./transport` on macOS/Linux or `.\transport.exe` on Windows before these arguments:

```sh
./transport --demo
./transport --stops
./transport --network
./transport --routes
./transport --timetable B3 F 07:00
./transport --trip B3 6 12 07:02
./transport --trip B6 20 13 07:01
./transport --snapshot 06:07
./transport --simulate 06:00 06:15 5
./transport --help
```

`--trip B3 6 12 07:02` means a passenger ready at Public Park A (6) at 07:02,
travelling to Lake (12) on B3. The bus leaves Airport at 07:00 and reaches the
boarding stop at 07:05: **3 minutes waiting, 20 minutes riding, arrival 07:25**.
The reverse B6 example travels from Port to Hospital; waiting starts at 07:01,
boarding is 07:15, and arrival is 07:35.

`--simulate` advances the model clock and prints vehicle positions at the chosen
interval; it does not wait in real time. A snapshot shows each active scheduled
trip's current link and minutes completed on that link. Trip IDs identify service
instances rather than a physical fleet. Menus recover from invalid input; command
errors return a nonzero exit status. Times use 24-hour `HH:MM`.

## Reference facts and explicit assumptions

- Stop IDs, coordinates and zones follow the supplied 20-node table. Coordinates
  describe map positions, not kilometres. The table's `Public Park` is named
  `Public Park A` to match the diagram.
- Only blue solid links become bus edges. Red dashed links are train references.
  **Industrial Zone A (4) has no bus connection.** The other 19 stops form one
  connected bus component; inventing a bus link to stop 4 would change the map.
- Transfer locations are Transport Hub A (5), Multimodal Transport Center (10),
  and Transport Hub B (17), matching the double circles in the diagram.
- All bus links are assumed bidirectional because the map has no arrowheads.
- The blue Airport--Economic Zone link has no visible time label. **15 minutes**
  is the agreed assumption. Other edge times follow the diagram's labels.
- B1-B7 are proposed service lines, not route labels supplied by the assignment.
  Each departs both termini every 15 minutes from 06:00 through 22:00 inclusive.
  A last bus may reach downstream stops after 22:00.
- Link travel times are constant. There is no dwell time, congestion, capacity,
  fleet reuse or variable passenger demand in Part I.

| Line | Forward stop IDs (reverse service also operates) | One-way minutes |
| --- | --- | --- |
| B1 | 2 - 1 - 3 | 20 |
| B2 | 2 - 5 - 1 - 7 | 25 |
| B3 | 1 - 6 - 10 - 11 - 12 | 25 |
| B4 | 6 - 5 - 9 - 8 - 13 - 18 - 17 | 35 |
| B5 | 8 - 9 - 10 | 10 |
| B6 | 13 - 14 - 15 - 16 - 20 | 20 |
| B7 | 14 - 17 - 16 - 19 | 30 |

## City data and teammate integration

`BusNetwork::referenceCity()` in `src/bus_network.cpp` is the runtime source of
truth. `data/city.json` is its **exported snapshot** for teammates and report work;
editing that JSON does not change the simulation. After changing the C++ data,
rebuild and regenerate the snapshot:

```sh
./transport --export-city data/city.json
```

The JSON includes stops, bus edges, proposed lines and assumptions. Train data
can be added by Member 2 during integration. The shared `Graph` already permits
separate `TransportMode::Bus` and `TransportMode::Train` edges with the same endpoints.

```cpp
const BusNetwork bus = BusNetwork::referenceCity();
Graph city = bus.graph(); // Copy the bus graph for multimodal integration.
// Member 2 may add validated train edges to city with TransportMode::Train.
const auto trip = bus.planTrip("B3", 6, 12, parseTime("07:02"));
const auto positions = bus.vehiclesAt(parseTime("06:07"));
```

`graph.h` exposes stop IDs, coordinates, transfer markers, neighbors and edge
times for Member 4's algorithms. `bus_network.h` exposes schedules, trip results
and positions for Member 3. Public result times are integer minutes after
midnight. The algorithms have no dependency on `main.cpp` or console input.

## Checks

Use **Terminal > Run Task > Run bus tests**, or compile the test separately:

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic src/graph.cpp src/bus_network.cpp tests/bus_network_tests.cpp -o bus_tests
./bus_tests
```

Replace `clang++` with `g++` on Linux/Windows and use `.\bus_tests.exe` on Windows.
The tests cover topology, route coverage, both directions, intermediate boarding,
exact/missed departures, before/after service, moving buses, invalid input and
mixed-mode graph edges. See `report/part1.md` for the Part I report contribution
and `report/part1-demo.txt` for reproducible output. The group leader still needs
to integrate Parts II-IV and prepare the final group report/submission.
