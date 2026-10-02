# Part I - Bus routes and bus network

## Scope and source interpretation

Problem C Part I asks for a graph-based simulation of bus routes and the bus
network. This contribution implements the shared graph and the bus module in
C++17, with a console interface that can be built and run in Visual Studio Code.
The assignment brief defines the work; the group guide informs file organisation.
The requested branch is `Hemsara`, overriding the guide's example branch name.
Train routes, variable passenger demand and shortest-path/efficiency profiling
remain the responsibilities of Parts II-IV.

The location table supplies 20 IDs, names, coordinates and zones. The diagram
supplies 23 solid blue bus links and their travel times. Red dashed train links
are excluded from the bus graph. Stop 6 is named Public Park A to reconcile the
table's Public Park with the diagram. Stops 5, 10 and 17 are marked as transfer
locations. Industrial Zone A (4) is retained as an isolated bus vertex because
its only depicted connection is by train. The remaining 19 bus vertices form
one connected component.

## Model and assumptions

Vertices are `Stop` records; edges contain the destination, travel minutes and
transport mode. Both directions of each bus edge are stored in an adjacency
list, since the reference has no direction arrows. The blue Airport--Economic
Zone edge is unlabelled: 15 minutes is the explicitly agreed assumption. Map
coordinates are not interpreted as distances or used to infer travel times.

Seven proposed service lines B1-B7 traverse existing edges and collectively
cover all 23 bus links. Each operates in both directions with a 15-minute
headway, with terminus departures from 06:00 through 22:00 inclusive. Travel
times are constant, stops have zero dwell time, and final services finish their
trips after 22:00 when necessary. These service patterns and schedules are
design assumptions; they are not provided route IDs in the diagram. The model
does not claim to simulate fleet reuse, passenger loads, congestion or demand.

## Implementation

- `graph.h/.cpp`: validated graph insertion, stable stop IDs, per-mode neighbor
  access and travel times, undirected edge counts and connectivity using BFS.
- `bus_network.h/.cpp`: reference city, route validation, schedules, next direct
  bus trips, clock-based positions and JSON export.
- `main.cpp`: interactive menu, reproducible demo, inspection commands, timetable,
  direct-trip and time-stepped simulation commands, with input validation.
- `.vscode/`: build, run, demonstration and test tasks; optional debugger setup.
- `tests/bus_network_tests.cpp`: automated behavioral and reference-data checks.

Graph storage is O(V + E). `std::map` supports stop lookup in O(log V), while
edge lookup scans a stop's adjacency list. BFS connectivity is O((V + E) log V)
with the chosen ordered map/set containers. Timetable and direct-trip work
traverse the selected route's ordered stops. Vehicle snapshots enumerate the
day's scheduled departures and locate their current legs; for this small city
there are 7 lines, 65 departures per direction and at most 6 legs per line.
This is straightforward for the assignment-sized network; it is not a
large-city performance benchmark.

## Scheduling and movement

A timetable sums edge weights from a terminus departure. For a direct trip, the
system first selects the direction from the ordering of the origin/destination
on the line. It computes the boarding stop's time offset from that direction's
terminus, then rounds up to the next scheduled terminus departure. A passenger
exactly at a stop's departure time can board. A passenger arriving one minute
later waits for the next service. No trip is returned once that day's last bus
has passed the boarding stop; service still in transit after 22:00 is allowed.

A snapshot tests `leg.departure <= clock < leg.arrival`. At an intermediate
arrival the bus begins its next leg immediately. At the final arrival the trip
ends. Each displayed vehicle ID represents one scheduled trip instance, not a
persistently simulated fleet vehicle. The simulation can advance the clock in
chosen minute increments to display how bus positions change.

## Demonstration and verification

Build and run via the README instructions or VS Code's tasks. Run
`./transport --demo` on macOS/Linux (`.\transport.exe --demo` on Windows) for
the transcript saved in `part1-demo.txt`.

| Case | Expected behavior |
| --- | --- |
| B3 from Airport at 07:00 | Lake arrival 07:25 |
| B3 Public Park A to Lake, ready 07:02 | Board 07:05, wait 3 min, ride 20 min, arrive 07:25 |
| Same passenger ready 07:06 | Board 07:20, arrive 07:40 |
| Reverse B6 Port to Hospital, ready 07:01 | Board 07:15, arrive 07:35 |
| B3 Airport to Lake, ready 22:01 | No remaining departure today |
| B3 Public Park A to Lake, ready 22:03 | Board last bus at 22:05, arrive 22:25 |
| First forward B3 at 06:07 | Public Park A to Multimodal Transport Center, 2 of 5 minutes elapsed |
| Industrial Zone A | Retained with no bus connection |

The automated checks validate bidirectional edges, all-edge service coverage,
transfer markers, disconnected stop handling, exact/missed departures, reverse
services, intermediate boarding, service boundaries, movement and bad inputs.
The project is compiled with strict warnings. Tests are also run with Address
Sanitizer and UndefinedBehaviorSanitizer on macOS. These checks establish the
Part I implementation's behavior under the stated assumptions; Parts II-IV
must still be integrated and tested by the group.

## Integration and report use

The bus module has no dependency on the console menu. Member 2 can copy its
graph and add train edges using `TransportMode::Train`; Member 3 can call the
schedule/trip APIs; Member 4 can query neighbors, weights and transfer markers.
The JSON file is an exported snapshot, not a runtime input file. Preserve IDs
1-20 across modules and regenerate JSON after changing the reference data.

This document is a Part I contribution for the group's maximum 10-page report,
not the full submission. Include screenshots of the VS Code build/demo and
clock-based positions when preparing the final Word/PDF report. The assignment
brief gives the group deadline as 6 October 2026 at 11:55 PM.
