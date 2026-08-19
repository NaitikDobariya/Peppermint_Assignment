#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <optional>

// Geometry Types
enum class GeometryType {
    STRAIGHT,
    ARC
};

// Node Representation
struct Node {
    int id;
    std::string name; // Empty if unnamed waypoint
    double x;         // meters
    double y;         // meters
    double heading;   // radians [-pi, pi]
    bool is_door_or_lift{false};
};

// Directed Lane Segment Representation
struct LaneEdge {
    int id;
    int from_node;
    int to_node;
    GeometryType type{GeometryType::STRAIGHT};
    
    // Geometry specifics
    double length;          // meters
    double arc_radius{0.0}; // meters (only used if type == ARC)
    double arc_center_x{0.0};
    double arc_center_y{0.0};

    // Attributes for Cost Model & Vehicle Limits
    double speed_limit;    // m/s
    double lane_width;     // meters
    bool is_one_way{true};
    double traversal_penalty{0.0}; // Delay penalty for doors/lifts
};

// Lane Network Graph
class LaneNetwork {
public:
    LaneNetwork() = default;

    void add_node(const Node& node);
    void add_edge(const LaneEdge& edge);
    void register_named_location(const std::string& name, int node_id);

    const Node* get_node(int id) const;
    const Node* get_node_by_name(const std::string& name) const;
    const std::vector<LaneEdge>& get_outgoing_edges(int node_id) const;

    const std::map<int, Node>& get_all_nodes() const { return nodes_; }
    const std::map<int, std::vector<LaneEdge>>& get_adjacency_list() const { return adj_list_; }

    int get_deterministic_node_id(const std::string& target_name) const;

private:
    // Using std::map ensures strict deterministic order during iteration
    std::map<int, Node> nodes_;
    std::map<std::string, int> named_locations_;
    std::map<int, std::vector<LaneEdge>> adj_list_;
};