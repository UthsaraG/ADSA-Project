#include "../src/routing.h"

#include <iostream>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool value, const char* description) {
    ++checks;
    if (!value) throw std::runtime_error(description);
}
template<class F> void rejects(F action, const char* description) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, description);
}
Graph smallCity() {
    Graph graph;
    for (int id = 1; id <= 4; ++id) graph.addStop({id, "Stop " + std::to_string(id), 0, 0, "Test", id == 4});
    return graph;
}
}

int main() {
    try {
        auto graph = smallCity();
        graph.addEdge(1, 2, 1, TransportMode::Bus);
        graph.addEdge(1, 4, 5, TransportMode::Bus);
        auto mixed = graph;
        mixed.addEdge(2, 3, 1, TransportMode::Train);
        mixed.addEdge(4, 3, 5, TransportMode::Train);
        const BusNetwork buses(graph, {{"Bshort", "Unmarked", {1, 2}, 0, 120, 15},
                                       {"Bvalid", "Hub", {1, 4}, 0, 120, 15}});
        const TrainNetwork trains(mixed, {{"Tshort", "Unmarked", {2, 3}, 0, 120, 20},
                                          {"Tvalid", "Hub", {4, 3}, 0, 120, 20}});
        for (bool weighted : {false, true}) {
            const auto result = weighted ? dijkstra(mixed, 1, 3) : bfs(mixed, 1, 3);
            check(result.found && result.stopIds == std::vector<int>{1, 4, 3} && result.totalMinutes == 10,
                  "Algorithms must use the marked hub, rejecting the faster unmarked change.");
            check(result.modes == std::vector<TransportMode>{TransportMode::Bus, TransportMode::Train},
                  "Preserve exact modes when reconstructing path.");
        }
        const auto scheduled = planJourney(buses, trains, 1, 3, 0);
        check(scheduled.found && scheduled.arrival == 25 && scheduled.waitingMinutes == 15 && scheduled.ridingMinutes == 10,
              "Marked transfer: bus arrives at 00:05, train departs at 00:20, arrives at 00:25.");
        check(scheduled.legs.size() == 2 && scheduled.legs[0].to == 4 && scheduled.legs[1].from == 4,
              "Scheduled journey changes only at marked hub.");
        check(!planJourney(buses, trains, 1, 3, 121).found, "No journey after last departure.");

        auto disconnected = smallCity();
        disconnected.addEdge(1, 2, 1, TransportMode::Bus);
        disconnected.addEdge(2, 3, 1, TransportMode::Train);
        check(!bfs(disconnected, 1, 3).found && !dijkstra(disconnected, 1, 3).found,
              "Physical connectivity via an unmarked change is not an allowed path.");
        const BusNetwork noTransferBus(disconnected, {{"B", "B", {1, 2}, 0, 120, 15}});
        const TrainNetwork noTransferTrain(disconnected, {{"T", "T", {2, 3}, 0, 120, 20}});
        check(!planJourney(noTransferBus, noTransferTrain, 1, 3, 0).found, "No scheduled route via unmarked transfer.");
        rejects([&] { dijkstra(mixed, 1, 99); }, "Reject nonexistent destination (no reconstruction loop).");
        rejects([&] { bfs(mixed, 99, 99); }, "Reject nonexistent equal endpoints.");

        auto parallel = smallCity();
        parallel.addEdge(1, 2, 10, TransportMode::Bus);
        parallel.addEdge(1, 2, 2, TransportMode::Train);
        const auto shortest = dijkstra(parallel, 1, 2);
        check(shortest.totalMinutes == 2 && shortest.modes == std::vector<TransportMode>{TransportMode::Train},
              "Parallel modes must report the selected edge's actual time.");

        const auto bus = BusNetwork::referenceCity();
        const auto train = TrainNetwork::referenceCity(bus.graph());
        check(train.graph().transferStops() == std::vector<int>{5, 10, 17}, "Exact designated transfer set.");
        for (const auto& line : train.lines()) check(line.headwayMinutes == 20, "Every train line has 20-minute service.");
        const auto sample = planJourney(bus, train, 4, 13, 420);
        check(sample.found && sample.arrival == 455 && sample.totalMinutes == 35 && sample.waitingMinutes == 15,
              "07:00 industrial-to-hospital journey arrives 07:35 including waits.");
        check(sample.legs.size() == 2 && sample.legs[0].mode == TransportMode::Train && sample.legs[0].to == 5 &&
              sample.legs[1].mode == TransportMode::Bus, "Reference train-then-bus itinerary.");
        check(!planJourney(bus, train, 4, 13, 1439).found, "Train-only origin after final service has no journey.");
        rejects([&] { planJourney(bus, train, 4, 13, 1440); }, "Reject next-day request.");
        rejects([&] { planJourney(bus, train, 0, 13, 420); }, "Reject invalid origin.");

        // Check every morning origin/destination pair for a physically and temporally valid itinerary.
        for (int from = 1; from <= 20; ++from) {
            for (int to = 1; to <= 20; ++to) {
                const auto journey = planJourney(bus, train, from, to, 420);
                check(journey.found, "Every city pair is served in morning (with permitted transfers).");
                check(journey.totalMinutes == journey.waitingMinutes + journey.ridingMinutes &&
                      journey.arrival == 420 + journey.totalMinutes, "Elapsed time equals waits plus rides.");
                int at = from, ready = 420;
                for (std::size_t i = 0; i < journey.legs.size(); ++i) {
                    const auto& leg = journey.legs[i];
                    check(leg.from == at && leg.readyAt == ready && leg.departure >= ready && leg.arrival > leg.departure,
                          "Each leg continues from the preceding arrival.");
                    if (i && leg.mode != journey.legs[i - 1].mode) {
                        check(train.graph().canTransfer(leg.from, journey.legs[i - 1].mode, leg.mode),
                              "Every reconstructed mode change uses a permitted stop.");
                    }
                    if (leg.mode == TransportMode::Bus) {
                        const auto actual = bus.planTrip(leg.serviceId, leg.from, leg.to, leg.readyAt);
                        check(actual.available && actual.departure == leg.departure && actual.arrival == leg.arrival,
                              "Bus itinerary matches service timetable.");
                    } else {
                        const auto actual = train.planTrip(leg.serviceId, leg.from, leg.to, leg.readyAt);
                        check(actual.available && actual.departure == leg.departure && actual.arrival == leg.arrival,
                              "Train itinerary matches the 20-minute timetable.");
                    }
                    at = leg.to;
                    ready = leg.arrival;
                }
                check(at == to, "Journey ends at requested destination.");
            }
        }
        const auto peak = generatePassengers(7, 2202);
        const auto repeated = generatePassengers(7, 2202);
        const auto offPeak = generatePassengers(12, 2202);
        check(peak.size() >= 80 && peak.size() <= 150 && offPeak.size() >= 10 && offPeak.size() <= 40,
              "Preserve Member 3 peak/off-peak demand ranges.");
        check(peak.size() == repeated.size(), "Seeded demand has reproducible size.");
        for (std::size_t i = 0; i < peak.size(); ++i) {
            check(peak[i].origin == repeated[i].origin && peak[i].destination == repeated[i].destination &&
                  peak[i].departureTime == repeated[i].departureTime, "Seeded passenger data is reproducible.");
            check(peak[i].origin != peak[i].destination && peak[i].departureTime >= 420 && peak[i].departureTime < 480,
                  "Demand uses distinct city stops and times in selected hour.");
        }
        rejects([] { generatePassengers(24, 2202); }, "Reject invalid demand hour.");
        std::cout << "PASS: " << checks << " integration checks (all-pair journeys, permitted changes, schedules, demand, BFS/Dijkstra).\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
