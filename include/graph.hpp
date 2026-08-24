#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <optional>

// Geometry Types, two types, line and an arc.
// Using this to prevent implicit conversion to integers, which could've cause bugs
// Type safety is maintained. Use static_cast<int> to typecast FYI
enum class GeometryType {
    STRAIGHT,
    ARC
};

// Node Representation
struct Node {
    int id;           // Primary key, better than a string
    std::string name; // Empty if unnamed waypoint
    double x;         // meters
    double y;         // meters
    double heading;   // radians [-pi, pi], Useful is there is a specific docking angle

    bool is_door_or_lift{false}; // Extra penalty, because they are not like other nodes
};

// Directed Lane Segment Representation
struct LaneEdge {
    int id;           // Not needed, but still might be useful
    int from_node;
    int to_node;
    GeometryType type{GeometryType::STRAIGHT};
    
    // Geometry specifics
    double length;          // meters, our base cost for the traversal algorithm that we have

    // Arc radius, determines the traversability, because the robo has some turning radius constraints
    double arc_radius{0.0}; // meters (only used if type == ARC)
    double arc_center_x{0.0};
    double arc_center_y{0.0};

    // Attributes for Cost Model & Vehicle Limits
    double speed_limit;    // m/s, can make use in time-based routing thing as well
    double lane_width;     // meters
    bool is_one_way{true};

    double traversal_penalty{0.0}; // Delay penalty for doors/lifts, how undesireable the route is 
};

// Lane Network Graph
class LaneNetwork {
public:
    LaneNetwork() = default; // Make the default constructor


    // The builder functions
    void add_node(const Node& node);
    void add_edge(const LaneEdge& edge);
    void register_named_location(const std::string& name, int node_id);


    // The getter functions, for nodes
    const Node* get_node(int id) const;
    const Node* get_node_by_name(const std::string& name) const;

    // The getter functions, for the edges/lanes
    const std::vector<LaneEdge>& get_outgoing_edges(int node_id) const;

    // The getter functions, for the graph 
    const std::map<int, Node>& get_all_nodes() const { return nodes_; }
    const std::map<int, std::vector<LaneEdge>>& get_adjacency_list() const { return adj_list_; }

    int get_deterministic_node_id(const std::string& target_name) const;

private:
    // Using std::map (instead of std::unordered_map) ensures strict deterministic order during iteration
    // std::map used Red-Black trees, deterministic memory iteration, numerically sorted list is given by it
    std::map<int, Node> nodes_;
    std::map<std::string, int> named_locations_;

    // Who is connected to whom
    std::map<int, std::vector<LaneEdge>> adj_list_;
};