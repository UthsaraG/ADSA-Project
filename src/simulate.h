#ifndef SIMULATE_H
#define SIMULATE_H

#include "graph.h"
#include <vector>

struct Passenger {
  int id;
  int origin;
  int destination;
  int departureTime;
};

std::vector<Passenger> generatePassengers(int hour);
// Explicit seed for reproducible demonstrations and tests.
std::vector<Passenger> generatePassengers(int hour, unsigned int seed);

#endif