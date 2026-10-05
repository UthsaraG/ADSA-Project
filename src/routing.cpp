#include "routing.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace {
// Arrival state contains a stop and its last transport mode (-1 at origin).
using State = std::pair<int, int>;
int modeId(TransportMode mode) { return mode == TransportMode::Bus ? 0 : 1; }
TransportMode modeValue(int mode) { return mode == 0 ? TransportMode::Bus : TransportMode::Train; }
bool allows(const Graph& graph, const State& state, TransportMode next) {
    return state.second == -1 || state.second == modeId(next) ||
           graph.canTransfer(state.first, modeValue(state.second), next);
}
struct Parent {
    State previous;
    int minutes;
    TransportMode mode;
};

PathResults graphSearch(const Graph& graph, int from, int to, bool weighted) {
    graph.stop(from);
    graph.stop(to);
    const State initial{from, -1};
    std::map<State, int> distance{{initial, 0}};
    std::map<State, Parent> parent;
    using Entry = std::pair<int, State>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap;
    std::queue<State> pending;
    if (weighted) heap.push({0, initial});
    else pending.push(initial);
    while (weighted ? !heap.empty() : !pending.empty()) {
        State current;
        int cost;
        if (weighted) {
            cost = heap.top().first;
            current = heap.top().second;
            heap.pop();
            if (cost != distance.at(current)) continue;
        } else {
            current = pending.front();
            pending.pop();
            cost = distance.at(current);
        }
        if (current.first == to) {
            PathResults result;
            result.found = true;
            State step = current;
            result.stopIds.push_back(step.first);
            while (step != initial) {
                const auto& previous = parent.at(step);
                result.totalMinutes += previous.minutes;
                result.modes.push_back(previous.mode);
                step = previous.previous;
                result.stopIds.push_back(step.first);
            }
            std::reverse(result.stopIds.begin(), result.stopIds.end());
            std::reverse(result.modes.begin(), result.modes.end());
            return result;
        }
        for (const auto& edge : graph.neighbors(current.first)) {
            if (!allows(graph, current, edge.mode)) continue;
            const State next{edge.to, modeId(edge.mode)};
            const int candidate = cost + (weighted ? edge.minutes : 1);
            const auto existing = distance.find(next);
            if (existing != distance.end() && candidate >= existing->second) continue;
            distance[next] = candidate;
            parent.insert_or_assign(next, Parent{current, edge.minutes, edge.mode});
            if (weighted) heap.push({candidate, next});
            else pending.push(next);
        }
    }
    return {};
}
} // namespace

PathResults bfs(const Graph& graph, int from, int to) { return graphSearch(graph, from, to, false); }
PathResults dijkstra(const Graph& graph, int from, int to) { return graphSearch(graph, from, to, true); }

JourneyResults planJourney(const BusNetwork& bus, const TrainNetwork& train,
                           int from, int to, int requestedAt) {
    const Graph& city = train.graph();
    city.stop(from);
    city.stop(to);
    if (requestedAt < 0 || requestedAt >= 1440) {
        throw std::invalid_argument("Journey time must be in the current day.");
    }
    const State initial{from, -1};
    std::map<State, int> arrival{{initial, requestedAt}};
    std::map<State, std::pair<State, JourneyLeg>> parent;
    using Entry = std::pair<int, State>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
    pending.push({requestedAt, initial});
    while (!pending.empty()) {
        const int clock = pending.top().first;
        const State current = pending.top().second;
        pending.pop();
        if (clock != arrival.at(current)) continue;
        if (current.first == to) {
            JourneyResults result;
            result.found = true;
            result.arrival = clock;
            result.totalMinutes = clock - requestedAt;
            State step = current;
            while (step != initial) {
                const auto& previous = parent.at(step);
                result.legs.push_back(previous.second);
                result.waitingMinutes += previous.second.departure - previous.second.readyAt;
                result.ridingMinutes += previous.second.arrival - previous.second.departure;
                step = previous.first;
            }
            std::reverse(result.legs.begin(), result.legs.end());
            return result;
        }
        if (clock >= 1440) continue; // Trips may end next day but cannot board next-day services.
        auto consider = [&](const auto& network, const auto& services, TransportMode mode) {
            if (!allows(city, current, mode)) return;
            for (const auto& service : services) {
                if (std::find(service.stops.begin(), service.stops.end(), current.first) == service.stops.end()) continue;
                for (int destination : service.stops) {
                    if (destination == current.first) continue;
                    const auto trip = network.planTrip(service.id, current.first, destination, clock);
                    if (!trip.available) continue;
                    const State next{destination, modeId(mode)};
                    const auto existing = arrival.find(next);
                    if (existing != arrival.end() && trip.arrival >= existing->second) continue;
                    std::vector<int> stops{current.first};
                    for (const auto& leg : trip.legs) stops.push_back(leg.to);
                    arrival[next] = trip.arrival;
                    parent.insert_or_assign(next, std::make_pair(current, JourneyLeg{
                        mode, service.id, current.first, destination, clock,
                        trip.departure, trip.arrival, std::move(stops)}));
                    pending.push({trip.arrival, next});
                }
            }
        };
        consider(bus, bus.routes(), TransportMode::Bus);
        consider(train, train.lines(), TransportMode::Train);
    }
    return {};
}

void printJourney(const JourneyResults& journey, const Graph& city, std::ostream& out) {
    if (!journey.found) { out << "No scheduled journey available today.\n"; return; }
    out << "SCHEDULED JOURNEY | arrive " << formatTime(journey.arrival) << "; total "
        << journey.totalMinutes << " min; waiting " << journey.waitingMinutes
        << " min; riding " << journey.ridingMinutes << " min\n";
    for (const auto& leg : journey.legs) {
        out << "  " << modeName(leg.mode) << ' ' << leg.serviceId << " | "
            << city.stop(leg.from).name << " -> " << city.stop(leg.to).name
            << " | wait " << leg.departure - leg.readyAt << " min; board "
            << formatTime(leg.departure) << "; arrive " << formatTime(leg.arrival) << '\n';
    }
}

// Preserve Member 4's graph-only profiling API for comparisons.
void profileNetwork(const Graph& graph, std::vector<Passenger> passengers) {
    long long total = 0;
    int found = 0;
    for (const auto& passenger : passengers) {
        const auto path = dijkstra(graph, passenger.origin, passenger.destination);
        if (path.found) { ++found; total += path.totalMinutes; }
    }
    std::cout << "GRAPH-ONLY STATISTICS (riding time, excludes timetable waits)\n"
              << "Total passengers: " << passengers.size() << "\nTrips found: " << found
              << "\nTrips not found: " << passengers.size() - found << '\n';
    if (found) std::cout << "Average riding time: " << static_cast<double>(total) / found << " min\n";
}

void profileNetwork(const BusNetwork& bus, const TrainNetwork& train,
                    const std::vector<Passenger>& passengers) {
    long long total = 0, waiting = 0, riding = 0, modeChanges = 0;
    int found = 0;
    std::map<int, int> stopUsage;
    for (const auto& passenger : passengers) {
        const auto journey = planJourney(bus, train, passenger.origin, passenger.destination, passenger.departureTime);
        if (!journey.found) continue;
        ++found;
        total += journey.totalMinutes;
        waiting += journey.waitingMinutes;
        riding += journey.ridingMinutes;
        std::map<int, bool> used;
        used[passenger.origin] = true;
        for (std::size_t i = 0; i < journey.legs.size(); ++i) {
            if (i && journey.legs[i - 1].mode != journey.legs[i].mode) ++modeChanges;
            for (int id : journey.legs[i].stopIds) used[id] = true;
        }
        for (const auto& entry : used) ++stopUsage[entry.first];
    }
    std::cout << "SCHEDULED NETWORK STATISTICS (includes service waits)\n"
              << "Total passengers: " << passengers.size() << "\nTrips found: " << found
              << "\nTrips not found: " << passengers.size() - found << '\n';
    if (found) {
        std::cout << std::fixed << std::setprecision(2)
                  << "Average total travel time: " << static_cast<double>(total) / found << " min\n"
                  << "Average waiting time: " << static_cast<double>(waiting) / found << " min\n"
                  << "Average riding time: " << static_cast<double>(riding) / found << " min\n"
                  << "Bus/train mode changes: " << modeChanges << '\n';
    }
    int busiest = -1, count = 0;
    for (const auto& entry : stopUsage) {
        if (entry.second > count) { busiest = entry.first; count = entry.second; }
    }
    if (busiest != -1) std::cout << "Busiest stop: " << train.graph().stop(busiest).name
                                << " (used by " << count << " completed journeys)\n";
}
