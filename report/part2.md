# Part II - Train routes and train network

## Scope and source interpretation

Problem C Part II asks for a graph-based simulation of train routes and the
train network. This contribution adds the train module in C++17 on top of the
shared graph from Part I, so that bus and train services live in one city graph
and can be used together by the demand (Part III) and routing/profiling (Part IV)
modules. Only `train_network.h/.cpp` and the test and demo files listed below
belong to this part; the shared graph and bus module are unchanged.

The red dashed links of the reference diagram supply 13 train links and their
travel times. They are added as `TransportMode::Train` edges between the same 20
stops used by the bus network. Twelve of the 20 stops are served by train and
they form one connected train component; the other eight stops (1, 2, 8, 9, 13,
14, 16, 18) have no train link in the map. Industrial Zone A (4) is reached only
by train, because the diagram shows no bus link to it.

## Model and assumptions

Train edges are stored in both directions in the shared adjacency list, since
the reference has no direction arrows. Bus and train edges may join the same two
stops without conflict, because the graph rejects duplicates only within one
mode. Edge times follow the diagram's labels and are constant; coordinates are
not used to infer times.

Five proposed train lines, T1-T5, traverse existing train links and together
cover each of the 13 links exactly once. The diagram shows links, not lines, so
this grouping is a design assumption:

| Line | Stop IDs (reverse service also operates) | One-way minutes |
| --- | --- | --- |
| T1 West-South spine | 4 - 5 - 10 - 15 - 17 - 19 | 60 |
| T2 North-East connector | 6 - 3 - 7 - 11 | 40 |
| T3 Tech Park express | 3 - 10 | 20 |
| T4 East coast | 7 - 12 - 20 - 19 | 20 |
| T5 Lake-Industrial B shuttle | 12 - 19 | 7 |

Each line operates in both directions, with terminus departures from 05:30
through 23:30 inclusive every 20 minutes, as agreed by the group.
Bus service runs 06:00-22:00 every 15 minutes. Stops have zero dwell time, and a
final train may reach downstream stops after 23:30. These service times are
assumptions. The model does not simulate fleet reuse, passenger loads,
congestion or demand.

## Implementation

- `train_network.h/.cpp`: `addTrainEdges` adds the 13 links to any graph that
  already holds the 20 stops. `TrainNetwork` validates the lines against the
  graph, and provides timetables, next direct train trips, clock-based positions
  and printing. Its interface mirrors `BusNetwork` (`timetable`, `planTrip`,
  `vehiclesAt`, `print*`), so other modules can treat both modes alike.
- `tests/train_network_tests.cpp`: 26 automated checks.
- `tests/train_demo.cpp`: reproducible demonstration (output in `part2-demo.txt`).

Line definitions hold only stop IDs. Travel times are read from the train edges
in the graph, so there is a single source of truth for each time. A line is
rejected at construction if it has an unknown or repeated stop, an invalid
schedule, or two consecutive stops with no train link.

The combined city is built as follows:

```cpp
const BusNetwork bus = BusNetwork::referenceCity();
const TrainNetwork train = TrainNetwork::referenceCity(bus.graph());
const Graph& city = train.graph(); // 20 stops, 23 bus links, 13 train links
```

Graph storage stays O(V + E). Edge lookup scans a stop's adjacency list. A
timetable or direct-trip request traverses the chosen line's ordered stops.
A vehicle snapshot enumerates the day's scheduled departures and locates their
current legs; here there are 5 lines, 109 departures per direction and at most
5 legs per line. This is straightforward for the assignment-sized network and is
not a large-city performance benchmark.

## Scheduling and movement

Scheduling follows the bus module. A timetable sums edge weights from a terminus
departure. For a direct trip, the system picks the direction from the order of
the origin and destination on the chosen line, computes the boarding stop's time
offset from that direction's terminus, and rounds up to the next scheduled
terminus departure. A passenger exactly at a departure time can board; one
minute later waits for the next train. No trip is returned once the day's last
train has passed the boarding stop.

A snapshot tests `leg.departure <= clock < leg.arrival`. At an intermediate
arrival the train begins its next leg immediately, and at the final arrival the
trip ends. Each vehicle ID (for example `T1-F-330`) is one scheduled trip, not a
persistent fleet vehicle.

## Demonstration and verification

Build and run the demonstration from the repository root:

```sh
g++ -std=c++17 src/graph.cpp src/bus_network.cpp src/train_network.cpp tests/train_demo.cpp -o train_demo
./train_demo        # .\train_demo.exe on Windows
```

| Case | Expected behavior |
| --- | --- |
| T1 forward from Industrial Zone A at 07:10 | Industrial Zone B arrival 08:10 |
| T1 Transport Hub A to Transport Hub B, ready 07:02 | Board 07:15, wait 13 min, ride 35 min, arrive 07:50 |
| Same passenger ready 07:16 | Board 07:35, arrive 08:10 |
| Reverse T4 Industrial Zone B to Economic Zone, ready 07:01 | Board 07:10, arrive 07:30 |
| T5 Lake to Industrial B, ready 23:30 | Board last train 23:30, arrive 23:37 |
| T5 Lake to Industrial B, ready 23:45 | No remaining departure today |
| First forward T1 at 05:37 | Transport Hub A to Multimodal Transport Center, 2 of 10 minutes elapsed |
| Industrial Zone A | Served by train only |

The automated checks validate the 13 links, both directions of an edge, that the
lines cover every link exactly once, line durations, exact and missed
departures, reverse trips, the last-train boundary, moving trains, invalid
input, coexistence of bus and train edges on the same stops, and rejection of a
line over a missing link. The tests build with strict warnings and also pass
under Address Sanitizer and UndefinedBehaviorSanitizer. These checks establish
the Part II behavior under the stated assumptions; the integrated system still
has to be tested by the group.

## Integration and agreed settings

`TrainNetwork::referenceCity` takes a graph that already has the 20 stops, so
the group leader integrates it with one call and does not modify the shared
graph. Member 3 can call `planTrip` and `vehiclesAt` as for buses, and Member 4
can read train edges with `neighbors(id)` filtered by `TransportMode::Train`.

Agreed settings for the integrated system:

1. T1-T5 operate from 05:30 through 23:30 every 20 minutes.
2. Bus/train transfers are permitted only at Transport Hub A (5), Multimodal
   Transport Center (10), and Transport Hub B (17). Stops 3, 6, 7, 11, 12, 15,
   19 and 20 share services but do not permit a mode change.
   `Graph::canTransfer` checks the marker and the presence of both modes;
   `Graph::transferStops` lists operational transfer points. The `--transfers`
   command and menu option 15 show their serving bus and train lines.
3. Journeys that start or end at Industrial Zone A (4) must use a train.

This document is a Part II contribution for the group's maximum 10-page report,
not the full submission. Include screenshots of the build, the test run and the
demonstration when preparing the final Word/PDF report.
