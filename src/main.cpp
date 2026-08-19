#include "graph.hpp"
#include "planner.hpp"
#include "json_exporter.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <limits>

// Helper functions for zero-dependency JSON parsing
std::string get_json_string(const std::string& obj, const std::string& key) {
    size_t pos = obj.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos) return "";
    size_t quote1 = obj.find('"', colon);
    if (quote1 == std::string::npos) return "";
    size_t quote2 = obj.find('"', quote1 + 1);
    if (quote2 == std::string::npos) return "";
    return obj.substr(quote1 + 1, quote2 - quote1 - 1);
}

double get_json_double(const std::string& obj, const std::string& key) {
    size_t pos = obj.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0;
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos) return 0.0;
    size_t end = obj.find_first_of(",}\n\r", colon);
    try { return std::stod(obj.substr(colon + 1, end - colon - 1)); } catch (...) { return 0.0; }
}

int get_json_int(const std::string& obj, const std::string& key) {
    size_t pos = obj.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0;
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos) return 0;
    size_t end = obj.find_first_of(",}\n\r", colon);
    try { return std::stoi(obj.substr(colon + 1, end - colon - 1)); } catch (...) { return 0; }
}

bool get_json_bool(const std::string& obj, const std::string& key) {
    size_t pos = obj.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos) return false;
    size_t end = obj.find_first_of(",}\n\r", colon);
    return (obj.substr(colon + 1, end - colon - 1).find("true") != std::string::npos);
}

bool load_network(LaneNetwork& net, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filepath << "\n";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t nodes_pos = content.find("\"nodes\"");
    size_t edges_pos = content.find("\"edges\"");

    if (nodes_pos == std::string::npos || edges_pos == std::string::npos) return false;

    std::string nodes_sec = content.substr(nodes_pos, edges_pos - nodes_pos);
    std::string edges_sec = content.substr(edges_pos);

    size_t pos = 0;
    while ((pos = nodes_sec.find('{', pos)) != std::string::npos) {
        size_t end = nodes_sec.find('}', pos);
        if (end == std::string::npos) break;
        std::string block = nodes_sec.substr(pos, end - pos + 1);
        Node n;
        n.id = get_json_int(block, "id");
        n.name = get_json_string(block, "name");
        n.x = get_json_double(block, "x");
        n.y = get_json_double(block, "y");
        n.heading = get_json_double(block, "heading");
        n.is_door_or_lift = get_json_bool(block, "is_door_or_lift");
        net.add_node(n);
        pos = end + 1;
    }

    pos = 0;
    while ((pos = edges_sec.find('{', pos)) != std::string::npos) {
        size_t end = edges_sec.find('}', pos);
        if (end == std::string::npos) break;
        std::string block = edges_sec.substr(pos, end - pos + 1);
        LaneEdge e;
        e.from_node = get_json_int(block, "from_node");
        e.to_node = get_json_int(block, "to_node");
        e.type = (get_json_string(block, "type") == "STRAIGHT") ? GeometryType::STRAIGHT : GeometryType::ARC;
        e.length = get_json_double(block, "length");
        e.arc_radius = get_json_double(block, "radius");
        e.speed_limit = get_json_double(block, "speed_limit");
        e.lane_width = get_json_double(block, "lane_width");
        e.is_one_way = get_json_bool(block, "is_one_way");
        e.traversal_penalty = get_json_double(block, "traversal_penalty");
        net.add_edge(e);
        pos = end + 1;
    }
    return true;
}


int main() {
    LaneNetwork network;
    // Hardcoded absolute path for bulletproof bash script execution
    if (!load_network(network, "/workspace/data/floor_plan.json")) {
        return 1;
    }

    LanePlanner planner(network);
    
    // Keep name-based querying for readability
    std::vector<std::pair<std::string, std::string>> queries = {
        {"Lobby", "Pharmacy"},
        {"Ward_1", "Charging_Bay"},
        {"Lift_A", "Stores"},
        {"Pharmacy", "Operating_Theatre"},
        {"Charging_Bay", "Lobby"},
        {"Waste_Disposal", "Cafeteria"}
    };

    double turning_radius = 1.0;

    for (size_t i = 0; i < queries.size(); ++i) {
        // Resolve names to IDs deterministically
        int start_id = network.get_deterministic_node_id(queries[i].first);
        int goal_id = network.get_deterministic_node_id(queries[i].second);
        
        // Pass the deterministic IDs to the planner
        PlanResult result;
        if (start_id == -1 || goal_id == -1) {
            result.status = PlannerStatus::UNKNOWN_LOCATION;
            result.message = "Error: Start or Goal location name not found in lane network.";
        } else {
            result = planner.plan_route(start_id, goal_id);
        }
        
        // Only verify drivability if we actually found a path
        if (result.status == PlannerStatus::SUCCESS) {
            planner.verify_drivability(result, turning_radius);
        }
        
        // Output using the new JSON Exporter
        JsonExporter::write_json(network, result, "route_" + std::to_string(i+1) + ".json");
    }
    return 0;
}