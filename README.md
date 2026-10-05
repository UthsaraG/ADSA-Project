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

The train module (Part II) is integrated. The demand and routing source files
remain placeholders on this branch for their owners.
Part I plans a direct trip on a **chosen bus line**. Automatic shortest paths,
bus changes and passenger demand belong to the remaining parts.
Part II similarly plans a direct trip on a chosen train line.

## Run in Visual Studio Code

1. Open this repository folder in **Visual Studio Code** (File > Open Folder).
2. Have a C++17 compiler available: Apple Clang on macOS; GCC/G++ on Linux or
   Windows (for example, an existing MinGW-w64 installation on Windows).
3. Press **Ctrl+Shift+B** (**Cmd+Shift+B** on macOS) to build.
4. Select **Terminal > Run Task > Run transport** to use the interactive menu.
5. Select **Run Part I demo** for a reproducible demonstration, or **Run bus tests**
   for the automated checks. Select **Run train tests** or **Run Part II demo**
   for the train module.

The checked-in tasks use Clang on macOS and `g++` elsewhere. All use C++17 and
compile the group source files. The Microsoft C/C++ extension is recommended for
editing and F5 debugging; it is not required to build or run. F5 also requires
LLDB on macOS or GDB on Windows/Linux, available to the extension.

If you use the installed **Code Runner** extension's **Run Code** button on
macOS, the workspace settings also build the complete project and launch the
interactive menu in the terminal. Open the repository folder, then run any
project `.cpp` file. Compiling `bus_network.cpp` alone cannot produce an
executable: it needs the graph implementation and `main.cpp`. The build/run
tasks above remain available on all supported platforms.

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

The JSON includes stops, bus edges, proposed lines and assumptions. This command continues to export the bus snapshot; it does not export train data.
The combined runtime graph is available through `TrainNetwork::graph()`. The shared `Graph` already permits
separate `TransportMode::Bus` and `TransportMode::Train` edges with the same endpoints.

```cpp
const BusNetwork bus = BusNetwork::referenceCity();
const TrainNetwork train = TrainNetwork::referenceCity(bus.graph());
const Graph& city = train.graph(); // 20 stops, 23 bus links, 13 train links
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
to integrate Parts III-IV and prepare the final group report/submission.

## Part II: train usage and integration

The interactive menu now includes train links, lines, timetables, direct trips,
snapshots and a Part II demonstration. Existing bus commands retain their behavior.

```sh
./transport --city
./transport --transfers
./transport --train-network
./transport --train-lines
./transport --train-timetable T1 F 07:10
./transport --train-trip T1 5 17 07:02
./transport --train-snapshot 05:37
./transport --train-demo
```

The combined graph has 20 stops, 23 bus links and 13 train links. The T1 trip
above boards at 07:15 and arrives at 07:50: 13 minutes waiting and 35 riding.
T1-T5 are proposed lines with departures from both termini every 20 minutes
from 05:30 through 23:30 inclusive, with zero dwell time. The 20-minute interval is the agreed service frequency. The final train can arrive after midnight;
requests and snapshots are restricted to the current day.

| Line | Forward stop IDs (reverse also operates) | One-way minutes |
| --- | --- | --- |
| T1 | 4 - 5 - 10 - 15 - 17 - 19 | 60 |
| T2 | 6 - 3 - 7 - 11 | 40 |
| T3 | 3 - 10 | 20 |
| T4 | 7 - 12 - 20 - 19 | 20 |
| T5 | 12 - 19 | 7 |

Run **Terminal > Run Task > Run train tests**, or:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic src/graph.cpp src/bus_network.cpp src/train_network.cpp tests/train_network_tests.cpp -o train_tests
./train_tests
```

On Windows use `.\train_tests.exe` and `.\transport.exe`; on macOS `clang++`
may be used instead of `g++`. See [Part II report](report/part2.md) and its
[demonstration output](report/part2-demo.txt).

### Agreed settings for Members 3 and 4

1. T1-T5 depart both termini every 20 minutes from 05:30 through 23:30.
   Departures include 05:30, 05:50 and 06:10; 07:10 is valid and 07:00 is not.
2. Mode changes are restricted to Transport Hub A (5), Multimodal Transport
   Center (10), and Transport Hub B (17). Other stops shared by both modes do
   not permit bus/train transfers. Direct-trip commands stay on one line; the
   integrated scheduled journey planner checks this rule at every mode change.
3. Industrial Zone A (4) is train-only; journeys to or from it require a train.

After these changes are merged into `main`, Members 3 and 4 should update their
branches without discarding their work:

```sh
git fetch origin
git merge origin/main
```

Member 3 can use `planTrip` and `vehiclesAt` on either network. Member 4 should
use the combined `train.graph()` for multimodal routing and respect the agreed
transfer policy using `Graph::canTransfer(id, fromMode, toMode)`.
`Graph::transferStops()` returns the designated stops that have both bus and
train edges, and `--transfers` (menu option 15) lists their serving lines. Include `train_network.h` alongside `bus_network.h` when using
these APIs.

## Complete group application (Parts I-IV)

The latest contributions from `member3-simulation` and
`member4-routing-profiling` are merged into the bus/train application. Member 3's
peak demand model generates 80-150 passengers per hour at 07:00-09:59 and
17:00-19:59, and 10-40 in other hours. An optional seed makes the runs reproducible.
Member 4's BFS and Dijkstra algorithms now track transport mode, so they cannot
switch from bus to train at an unmarked shared stop.

```sh
./transport --journey 4 13 07:00
./transport --route 4 13
./transport --demand 7 2202
./transport --demand 12 2202
./transport --full-demo
```

The journey from Industrial Zone A (4) to Hospital (13), ready at 07:00, boards
T1 at 07:10, reaches Transport Hub A (5) at 07:15, then boards B4 at 07:20 and
arrives at 07:35. It includes 15 minutes waiting and 20 minutes riding. The mode
change occurs at a designated transfer stop. All T1-T5 service intervals are
20 minutes; bus intervals remain 15 minutes.

Menu options **16-19** expose passenger demand/profiling, BFS/Dijkstra comparisons,
scheduled journeys and the full demonstration. In VS Code, select **Run full
group demo** or **Run integration tests** from Terminal > Run Task.

BFS minimizes graph links and Dijkstra minimizes graph riding time. Their
`--route` output excludes waits and labels that fact. The scheduled planner
instead minimizes actual arrival time using each line's timetable. Demand
profiling calls that planner for each passenger and reports completed/unavailable
journeys, mean total/waiting/riding times, mode changes and the busiest stop.
Same-mode line changes are permitted at shared stops; bus/train changes are
restricted to 5, 10 and 17. There is zero dwell/walking time. Requests belong to
one day; last trains may finish after midnight, but next-day departures are not
planned. Passenger demand does not alter service capacity or travel times.

The integration suite checks all 400 morning origin/destination pairs, every
itinerary's continuity and service times, forbidden transfers, parallel-mode
edges, invalid stops, service boundaries, peak/off-peak demand and reproducibility.
See [integration notes](report/integration.md) and
[full demonstration output](report/full-group-demo.txt).
