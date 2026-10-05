#include "../src/train_network.h"
#include "../src/bus_network.h"

#include <algorithm>

#include <iostream>
#include <set>
#include <stdexcept>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) { ++failures; std::cout << "FAIL: " << what << '\n'; }
}
template <class F> bool throws(F f) { try { f(); } catch (const std::exception&) { return true; } return false; }

Graph cityStops() {
    Graph g;
    for (int id = 1; id <= 20; ++id) g.addStop({id, "Stop " + std::to_string(id), 0, 0, "Zone", false});
    return g;
}
} // namespace

int main() {
    const auto net = TrainNetwork::referenceCity(cityStops());
    const Graph& g = net.graph();

    check(g.edgeCount(TransportMode::Train) == 13, "13 train links");
    check(g.edgeCount(TransportMode::Bus) == 0, "no bus edges in this test graph");
    check(g.travelMinutes(4, 5, TransportMode::Train) == 5, "4->5 is 5 min");
    check(g.travelMinutes(5, 4, TransportMode::Train) == 5, "5->4 is 5 min (undirected)");
    check(g.travelMinutes(15, 17, TransportMode::Train) == 20, "15-17 is 20 min");
    check(g.travelMinutes(12, 19, TransportMode::Train) == 7, "12-19 is 7 min");
    check(throws([&] { g.travelMinutes(1, 2, TransportMode::Train); }), "no train link 1-2");

    // Lines cover each link exactly once.
    std::set<std::pair<int, int>> covered;
    int segments = 0;
    check(net.lines().size() == 5, "five train lines T1-T5");
    for (const auto& line : net.lines()) {
        check(line.headwayMinutes == 20, "every train line has a 20-minute headway");
        check(net.timetable(line.id, false, 350).front().departure == 350, "second forward departure is 05:50");
        check(net.timetable(line.id, true, 350).front().departure == 350, "second reverse departure is 05:50");
        check(throws([&] { net.timetable(line.id, false, 340); }), "reject old 05:40 departure");
        for (std::size_t i = 1; i < line.stops.size(); ++i) {
            int a = line.stops[i - 1], b = line.stops[i];
            covered.insert({std::min(a, b), std::max(a, b)});
            ++segments;
        }
    }
    check(segments == 13 && covered.size() == 13, "lines cover all 13 links exactly once");

    check(net.lineMinutes("T1") == 60, "T1 is 60 min");
    check(net.lineMinutes("T2") == 40, "T2 is 40 min");
    check(net.lineMinutes("T5") == 7, "T5 is 7 min");

    // Timetable and trips. T1 forward from 4 at 07:10 (430), on the 20-minute grid.
    const auto legs = net.timetable("T1", false, 430);
    check(legs.size() == 5 && legs.back().arrival == 430 + 60, "T1 timetable ends 08:10");
    const auto trip = net.planTrip("T1", 5, 17, 7 * 60 + 2); // upstream offset 5 min
    check(trip.available && !trip.reverse, "trip 5->17 forward");
    check(trip.departure == 7 * 60 + 15 && trip.arrival == 7 * 60 + 50, "board 07:15 arrive 07:50");
    const auto missed = net.planTrip("T1", 5, 17, 7 * 60 + 16);
    check(missed.departure == 7 * 60 + 35, "missed train waits for next at 07:35");
    const auto rev = net.planTrip("T1", 19, 15, 8 * 60);
    check(rev.available && rev.reverse && rev.legs.size() == 2, "reverse trip 19->15");
    check(!net.planTrip("T5", 12, 19, 23 * 60 + 45).available, "no train after last departure");
    check(net.planTrip("T5", 12, 19, 23 * 60 + 30).available, "last departure still boards");
    check(throws([&] { net.planTrip("T5", 12, 4, 480); }), "stop not on line");
    check(throws([&] { net.planTrip("T9", 12, 19, 480); }), "unknown line");
    check(throws([&] { net.planTrip("T1", 5, 5, 480); }), "same stop");
    check(throws([&] { net.timetable("T1", false, 421); }), "off-timetable departure");

    check(throws([&] { net.timetable("T1", false, 420); }), "07:00 is off the new 20-minute grid");
    check(net.planTrip("T1", 5, 17, 435).departure == 435, "board exactly at downstream 07:15 departure");
    check(net.planTrip("T1", 5, 17, 1411).departure == 1415, "last train reaches hub A after 23:30");
    check(!net.planTrip("T1", 5, 17, 1416).available, "no train after last downstream departure");
    const auto at0540 = net.vehiclesAt(340);
    check(std::none_of(at0540.begin(), at0540.end(), [](const TrainPosition& p) {
        return p.terminusDeparture == 340;
    }), "no old 10-minute departure in snapshots");

    const auto bus = BusNetwork::referenceCity();
    const auto combined = TrainNetwork::referenceCity(bus.graph());
    check(combined.graph().transferStops() == std::vector<int>{5, 10, 17}, "only hubs A/B and multimodal center permit transfers");
    for (int id : {5, 10, 17}) {
        check(combined.graph().canTransfer(id, TransportMode::Bus, TransportMode::Train), "bus to train at designated hub");
        check(combined.graph().canTransfer(id, TransportMode::Train, TransportMode::Bus), "train to bus at designated hub");
    }
    for (int id : {3, 6, 7, 11, 12, 15, 19, 20}) {
        check(!combined.graph().canTransfer(id, TransportMode::Bus, TransportMode::Train), "shared unmarked stop cannot transfer");
    }
    check(!combined.graph().canTransfer(5, TransportMode::Bus, TransportMode::Bus), "same mode is not a bus/train transfer");
    check(bus.graph().transferStops().empty(), "bus graph alone has no operational bus/train transfers");
    check(throws([&] { combined.graph().canTransfer(99, TransportMode::Bus, TransportMode::Train); }), "reject nonexistent transfer stop");

    // Moving trains: first forward T1 at 05:30, 5:37 -> on 5->10 leg, 2 of 10 min.
    bool found = false;
    for (const auto& p : net.vehiclesAt(5 * 60 + 37)) {
        if (p.vehicleId == "T1-F-330") found = (p.from == 5 && p.to == 10 && p.minutesOnLeg == 2 && p.legMinutes == 10);
    }
    check(found, "T1-F-330 position at 05:37");
    check(net.vehiclesAt(3 * 60).empty(), "no trains at 03:00");

    // Mixed-mode: a graph that already has a bus edge keeps it.
    Graph mixed = cityStops();
    mixed.addEdge(4, 5, 9, TransportMode::Bus);
    const auto both = TrainNetwork::referenceCity(mixed);
    check(both.graph().travelMinutes(4, 5, TransportMode::Bus) == 9 &&
          both.graph().travelMinutes(4, 5, TransportMode::Train) == 5, "bus and train edges coexist");

    // Bad configuration is rejected.
    check(throws([&] { TrainNetwork(cityStops(), {{"X", "Bad", {4, 5}, 330, 1410, 10}}); }), "line over missing link rejected");

    net.printNetwork(std::cout);
    net.printLines(std::cout);
    net.printTrip(trip, std::cout);
    net.printSnapshot(5 * 60 + 37, std::cout);

    if (failures == 0) std::cout << "\nAll train tests passed.\n";
    return failures == 0 ? 0 : 1;
}
