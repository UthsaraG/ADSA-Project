#ifndef ROUTING_H
#define ROUTING_H

#include "graph.h"
#include <vector>
#include "simulate.h"
using namespace std;

struct PathResults{
    vector<int> stopIds;
    int totalMinutes;
    bool found;
};

PathResults bfs(const Graph& g, int startId, int endId);
PathResults dijkstra(const Graph&g, int startId, int endId);
void profileNetwork(const Graph&g, vector<Passenger> passengers);

#endif
