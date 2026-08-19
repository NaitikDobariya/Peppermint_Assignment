import json
import glob
import os
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

def render_map_and_routes(map_path, route_files):
    if not os.path.exists(map_path):
        print(f"Error: Could not find map at {map_path}")
        return

    with open(map_path, 'r') as f:
        map_data = json.load(f)

    nodes = {n['id']: n for n in map_data['nodes']}
    edges = map_data['edges']

    if not route_files:
        print("No route JSON files found to plot.")
        return

    for route_file in route_files:
        with open(route_file, 'r') as f:
            route_data = json.load(f)

        fig, ax = plt.subplots(figsize=(12, 8))
        ax.set_facecolor('#f8f9fa')

        # 1. Draw Network Edges (One-Way vs Two-Way)
        for edge in edges:
            n1, n2 = nodes.get(edge['from_node']), nodes.get(edge['to_node'])
            if n1 and n2:
                is_one_way = edge.get('is_one_way', False)
                width = edge.get('lane_width', 1.5) * 2

                if is_one_way:
                    ax.annotate('', xy=(n2['x'], n2['y']), xytext=(n1['x'], n1['y']),
                                arrowprops=dict(arrowstyle='->,head_width=0.4,head_length=0.6',
                                                color='#495057', lw=width, shrinkA=6, shrinkB=6),
                                zorder=1)
                else:
                    ax.plot([n1['x'], n2['x']], [n1['y'], n2['y']], color='#ced4da', linewidth=width, zorder=1)

        # 2. Draw Nodes & Labels
        for n_id, n in nodes.items():
            color = '#ffc107' if n.get('is_door_or_lift') else '#6c757d'
            ax.scatter(n['x'], n['y'], c=color, s=30, zorder=2)
            if n.get('name'):
                ax.text(n['x'], n['y'] + 1.2, n['name'], fontsize=8, ha='center', fontweight='bold', color='#212529', zorder=4)

        # 3. Overlay Planned Route or Display Failure
        path_ids = route_data.get('path_node_ids', [])
        status = route_data.get('status', 'UNKNOWN')
        drivable = route_data.get('is_drivable', False)

        if status == "SUCCESS" and path_ids:
            rx = [nodes[nid]['x'] for nid in path_ids if nid in nodes]
            ry = [nodes[nid]['y'] for nid in path_ids if nid in nodes]
            
            ax.plot(rx, ry, color='#0d6efd', linewidth=4, alpha=0.8, zorder=3, label='Planned Path')
            ax.scatter(rx[0], ry[0], c='#198754', s=120, zorder=5, label='Origin')
            ax.scatter(rx[-1], ry[-1], c='#dc3545', s=120, zorder=5, label='Destination')
        else:
            ax.text(0.5, 0.5, f"NO PATH FOUND / {status}", transform=ax.transAxes,
                    fontsize=16, color='#dc3545', fontweight='bold',
                    ha='center', va='center', bbox=dict(boxstyle="round,pad=0.5", fc="white", ec="#dc3545", lw=2))

        # Title configuration
        drivable_str = "Drivable: YES" if drivable else "Drivable: NO"
        # Extract just the "route_X" part for a cleaner title
        base_name = os.path.basename(route_file).replace('O0_run1_', '')
        title_str = (f"Route Plot: {base_name} | Status: {status} | "
                     f"Distance: {route_data.get('total_distance', 0):.2f}m | {drivable_str}")
        
        ax.set_title(title_str, fontweight='bold', fontsize=11)
        ax.set_aspect('equal')
        ax.grid(True, linestyle='--', alpha=0.4)

        # 4. Custom Legend
        custom_legend = [
            Line2D([0], [0], color='#ced4da', lw=3, label='Two-Way Lane'),
            Line2D([0], [0], color='#495057', lw=3, label='One-Way Lane (→)'),
            Line2D([0], [0], color='#0d6efd', lw=4, label='Planned Path'),
            Line2D([0], [0], marker='o', color='w', markerfacecolor='#198754', markersize=10, label='Origin'),
            Line2D([0], [0], marker='o', color='w', markerfacecolor='#dc3545', markersize=10, label='Destination')
        ]
        ax.legend(handles=custom_legend, loc='upper right')

        out_png = route_file.replace('.json', '.png')
        plt.tight_layout()
        plt.savefig(out_png, dpi=200)
        plt.close()
        print(f"Generated plot: {out_png}")

if __name__ == "__main__":
    map_file = "/workspace/data/floor_plan.json"
    if not os.path.exists(map_file):
        map_file = "data/floor_plan.json"

    # Only grab the O0_run1 files from the results folder to avoid generating 150 duplicate images
    routes = glob.glob("results/O0_run1_route_*.json")
    render_map_and_routes(map_file, routes)