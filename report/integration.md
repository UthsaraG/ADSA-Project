# Full group integration - Parts III and IV

## Team code incorporated

The combined application includes Member 1's bus graph and simulator, Member 2's
train network, Member 3's hourly demand generator from `member3-simulation`, and
Member 4's BFS, Dijkstra and profiling contribution from
`member4-routing-profiling`. The Member 4 branch already contains the latest
Member 3 commits, so merging that branch incorporates both contributions and
preserves their Git history. The latest main/bus/train work is preserved.

## Agreed rules

Bus/train changes are permitted only at Transport Hub A (5), Multimodal
Transport Center (10), and Transport Hub B (17). A stop served by both modes
is not automatically a transfer point. T1-T5 depart every 20 minutes from both
termini, from 05:30 to 23:30 inclusive; bus service remains every 15 minutes from
06:00 to 22:00. The established edge travel times and city data remain unchanged.

## Passenger demand

Member 3's ranges are retained: 80-150 passengers per hour in morning hours 7,
8 and 9 and evening hours 17, 18 and 19; 10-40 in other hours. Generated origin
and destination IDs are distinct and lie in 1-20; each departure belongs to the
selected hour. A seed argument permits reproducible data for the demonstration
and tests. Demand hour inputs are validated in the range 0-23.

## Routing and scheduling

Member 4's BFS and Dijkstra operate on `(stop ID, last transport mode)` states.
Tracking only stop IDs would wrongly allow changing mode at any shared stop.
A transition to a different mode calls `Graph::canTransfer`. BFS minimizes links;
Dijkstra minimizes graph riding time. Path reconstruction retains the selected
mode and edge time, including when bus and train edges share endpoints. Invalid
stop IDs are rejected before searching.

The added scheduled planner uses an earliest-arrival Dijkstra search over the
same mode states. At each stop it considers lines serving that stop and asks
`BusNetwork::planTrip` or `TrainNetwork::planTrip` for the next departure and
arrival at each downstream stop. Thus bus and train frequencies, intermediate
boarding offsets and the last service of the day affect the result. Positive
travel times and waiting for the next service give FIFO connections, allowing
one earliest-arrival label per mode state. Changes between lines of the same
mode are allowed at shared stops. There is no added dwell or walking time.

A passenger ready at Industrial Zone A at 07:00, travelling to Hospital, waits
10 minutes for T1, arrives at Transport Hub A at 07:15, waits 5 minutes for B4,
and reaches Hospital at 07:35. Total elapsed time is 35 minutes: 15 waiting and
20 riding. The train-to-bus change occurs at stop 5.

## Profiling

Each generated passenger is routed using their departure time. Statistics show
passenger count, completed/unavailable journeys, average elapsed/waiting/riding
time, bus/train mode changes, and the busiest stop. A stop's usage is counted
once per completed passenger journey, including intermediate stops. Graph-only
profiling remains available separately and explicitly excludes timetable waits.
The recorded full demonstration uses seed 2202 to compare peak hour 7 and
non-peak hour 12. These are model results under fixed service assumptions, not
real transport measurements or capacity/congestion simulations.

## Interface and verification

Menu options 16-19 and commands `--demand`, `--route`, `--journey`, and
`--full-demo` expose the integrated modules. VS Code includes full-demo and
integration-test tasks. Existing bus/train menus and commands still work.

Validation includes the original 100 bus checks, the expanded train suite, and
2,960 integration checks. The integration suite covers all 400 morning city
pairs, service-valid reconstructed itineraries, forbidden mode changes, selected
parallel-edge modes, before/after service behavior, invalid stops, seeded demand
and peak/off-peak ranges. All suites build with strict C++17 warnings and pass
under Address Sanitizer and UndefinedBehaviorSanitizer. The main application,
command-line interface, interactive menu and full demo are also checked.

This is a contribution to the group's final report. Combine it with the other
sections within the assignment's report limit and add presentation screenshots.
