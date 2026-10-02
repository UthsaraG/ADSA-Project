#ifndef ADSA_BUS_NETWORK_H
#define ADSA_BUS_NETWORK_H

#include "graph.h"

#include <ostream>
#include <string>
#include <vector>

struct BusRoute {
    std::string id;
    std::string name;
    std::vector<int> stops;
    int firstDeparture; // Minutes after midnight at each terminus.
    int lastDeparture;
    int headwayMinutes;
};

struct BusLeg {
    int from;
    int to;
    int departure;
    int arrival;
};

struct BusTrip {
    bool available = false;
    std::string routeId;
    bool reverse = false;
    int requestedAt = 0;
    int departure = 0;
    int arrival = 0;
    std::vector<BusLeg> legs;
};

struct BusPosition {
    std::string vehicleId;
    std::string routeId;
    bool reverse;
    int terminusDeparture;
    int from;
    int to;
    int minutesOnLeg;
    int legMinutes;
};

class BusNetwork {
public:
    BusNetwork(Graph graph, std::vector<BusRoute> routes);
    static BusNetwork referenceCity();
    const Graph& graph() const;
    const std::vector<BusRoute>& routes() const;
    const BusRoute& route(const std::string& id) const;
    int routeMinutes(const std::string& id) const;
    std::vector<BusLeg> timetable(const std::string& id, bool reverse,
                                  int terminusDeparture) const;
    BusTrip planTrip(const std::string& id, int from, int to, int requestedAt) const;
    std::vector<BusPosition> vehiclesAt(int minute) const;
    void printStops(std::ostream& out) const;
    void printNetwork(std::ostream& out) const;
    void printRoutes(std::ostream& out) const;
    void printTrip(const BusTrip& trip, std::ostream& out) const;
    void printSnapshot(int minute, std::ostream& out) const;
    void exportCity(std::ostream& out) const;

private:
    Graph graph_;
    std::vector<BusRoute> routes_;
    void validateRoutes() const;
};

int parseTime(const std::string& value); // Strict HH:MM, 00:00..23:59.
std::string formatTime(int minute);     // Also supports next-day arrivals.

#endif
