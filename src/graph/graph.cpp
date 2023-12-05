//trafficGraph.cpp

#include "trafficGraph.h"
#include <random>
#include <limits>
#include <omp.h>

std::vector<Route> TrafficGraph::findAllPaths(Point* source, Point* destination);
std::vector<Route> TrafficGraph::findAlternativePaths(Point* source, Point* destination);
