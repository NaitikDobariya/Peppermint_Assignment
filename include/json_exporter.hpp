#pragma once

#include "graph.hpp"
#include "planner.hpp"
#include <string>

class JsonExporter {
public:
    // Generates a JSON file containing the network routing results and explicit geometry
    static void write_json(const LaneNetwork& network, 
                           const PlanResult& plan, 
                           const std::string& filename);
};