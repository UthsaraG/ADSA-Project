#ifndef ADSA_ROUTING_H
#define ADSA_ROUTING_H

#include "graph.h"
#include "simulate.h"
#include "bus_network.h"
#include "train_network.h"
#include <ostream>
#include <vector>

struct PathResults {
    std::vector<int> stopIds;
    int totalMinutes = 0;
    bool found = false;
    std::vector<TransportMode> modes;
};

// Graph-only comparisons: BFS minimizes hops; Dijkstra minimizes riding time.
// Both track arrival mode so bus/train changes require a designated transfer stop.
PathResults bfs(const Graph& graph, int startId, int endId);
PathResults dijkstra(const Graph& graph, int startId, int endId);
void profileNetwork(const Graph& graph, std::vector<Passenger> passengers);

struct JourneyLeg {
    TransportMode mode;
    std::string serviceId;
    int from;
    int to;
    int readyAt;
    int departure;
    int arrival;
    std::vector<int> stopIds;
};

struct JourneyResults {
    bool found = false;
    int totalMinutes = 0;
    int waitingMinutes = 0;
    int ridingMinutes = 0;
    int arrival = 0;
    std::vector<JourneyLeg> legs;
};

// Earliest scheduled arrival. Uses actual bus/train planTrip timetables,
// 15/20-minute headways and transfer restrictions; no next-day departures.
JourneyResults planJourney(const BusNetwork& bus, const TrainNetwork& train,
                           int from, int to, int requestedAt);
void printJourney(const JourneyResults& journey, const Graph& city, std::ostream& out);
void profileNetwork(const BusNetwork& bus, const TrainNetwork& train,
                    const std::vector<Passenger>& passengers);

#endif
