#include "json_exporter.hpp"
#include <fstream>
#include <iostream>

void JsonExporter::write_json(const LaneNetwork& network, const PlanResult& plan, const std::string& filename) {
    // Ensure the output file has a .json extension
    std::string out_name = filename;
    if (out_name.find(".json") == std::string::npos) {
        out_name += ".json";
    }

    std::ofstream out(out_name);
    if (!out.is_open()) {
        std::cerr << "Failed to open export file: " << out_name << "\n";
        return;
    }

    // Helper lambda to convert enum to string
    auto geom_type_to_string = [](GeometryType type) {
        return type == GeometryType::STRAIGHT ? "STRAIGHT" : "ARC";
    };

    out << "{\n";
    out << "  \"status\": \"" << (plan.status == PlannerStatus::SUCCESS ? "SUCCESS" : "FAILED") << "\",\n";
    out << "  \"message\": \"" << plan.message << "\",\n";
    out << "  \"total_cost\": " << plan.total_cost << ",\n";
    out << "  \"total_distance\": " << plan.total_distance << ",\n";
    out << "  \"is_drivable\": " << (plan.is_drivable ? "true" : "false") << ",\n";
    
    // Output Node IDs
    out << "  \"path_node_ids\": [";
    for (size_t i = 0; i < plan.path_node_ids.size(); ++i) {
        out << plan.path_node_ids[i] << (i + 1 < plan.path_node_ids.size() ? ", " : "");
    }
    out << "],\n";

    // Output Explicit Geometry Array
    out << "  \"route_geometry\": [\n";
    for (size_t i = 0; i < plan.route_geometry.size(); ++i) {
        const auto& seg = plan.route_geometry[i];
        out << "    {\n";
        out << "      \"from_node\": " << seg.from_node << ",\n";
        out << "      \"to_node\": " << seg.to_node << ",\n";
        out << "      \"type\": \"" << geom_type_to_string(seg.type) << "\",\n";
        out << "      \"length\": " << seg.length << ",\n";
        out << "      \"arc_radius\": " << seg.arc_radius << ",\n";
        out << "      \"speed_limit\": " << seg.speed_limit << "\n";
        out << "    }" << (i + 1 < plan.route_geometry.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    
    out.close();
}