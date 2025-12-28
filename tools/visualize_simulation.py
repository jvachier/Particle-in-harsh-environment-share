"""
Visualization Tool for Particle Simulation Output

Automatically creates normalized 1D line plots for concentration and density profiles.

Output:
- 6 separate HTML files: c_x.html, c_y.html, c_z.html, p_x.html, p_y.html, p_z.html
- Default output directory: ../plots/
- Each plot shows normalized PDF (2*values/sum(values))

Usage:
    cd tools
    uv run python visualize_simulation.py              # Uses ../data/metal/*.bin → ../plots/
    uv run python visualize_simulation.py -o output/   # Save to custom directory
    uv run python visualize_simulation.py ../data/pragma/*.bin  # Custom input files
"""

import argparse
import numpy as np
import struct
import os
import sys
import re
from pathlib import Path
from typing import List, Dict, Optional

try:
    import plotly.graph_objects as go
    import plotly.express as px
except ImportError:
    print("Error: Plotly is required")
    print("Install with: uv add plotly")
    sys.exit(1)


def parse_config_file(config_path: str = "../config/simulation_parameters.h") -> Dict:
    """Parse C++ config header"""
    config = {
        "nx": 50,
        "ny": 50,
        "nz": 1600,
        "x_length": 10.0,
        "y_length": 10.0,
        "z_length": 80.0,
    }

    script_dir = Path(__file__).parent
    config_file = script_dir / config_path

    if not config_file.exists():
        for alt_path in [
            Path("../config/simulation_parameters.h"),
            Path("config/simulation_parameters.h"),
        ]:
            if alt_path.exists():
                config_file = alt_path
                break

    if not config_file.exists():
        return config

    try:
        with open(config_file, "r") as f:
            content = f.read()

        patterns = {
            "nx": r"int\s+nx\s*=\s*(\d+)",
            "ny": r"int\s+ny\s*=\s*(\d+)",
            "nz": r"int\s+nz\s*=\s*(\d+)",
            "x_length": r"double\s+x_length\s*=\s*([\d.]+)",
            "y_length": r"double\s+y_length\s*=\s*([\d.]+)",
            "z_length": r"double\s+z_length\s*=\s*([\d.]+)",
        }

        for key, pattern in patterns.items():
            match = re.search(pattern, content)
            if match:
                value = match.group(1)
                config[key] = int(value) if key in ["nx", "ny", "nz"] else float(value)

        print(
            f"✓ Config: {config['nx']}×{config['ny']}×{config['nz']}, domain {config['z_length']:.1f}m"
        )
    except Exception as e:
        print(f"Warning: Could not parse config: {e}")

    return config


class SimulationData:
    """Container for simulation data"""

    def __init__(self, filename: str):
        self.filename = filename
        self.positions = None
        self.values = None
        self.field_type = None  # 'c' or 'p'
        self.axis = None  # 'x', 'y', or 'z'
        self.timestep = None

    def load(self) -> bool:
        """Load binary data: alternating [position, value] pairs"""
        try:
            basename = os.path.basename(self.filename)

            # Determine field type
            if basename.startswith("c") or "_c" in basename:
                self.field_type = "c"
            elif basename.startswith("p") or "_p" in basename:
                self.field_type = "p"

            # Determine axis
            if "cx" in basename or "px" in basename or "_x_" in basename:
                self.axis = "x"
            elif "cy" in basename or "py" in basename or "_y_" in basename:
                self.axis = "y"
            elif "cz" in basename or "pz" in basename or "_z_" in basename:
                self.axis = "z"

            # Extract timestep
            match = re.search(r"_(\d+)\.bin", basename)
            if match:
                self.timestep = int(match.group(1))
            elif "initial" in basename.lower():
                self.timestep = 0
            else:
                self.timestep = 0

            # Read binary data
            with open(self.filename, "rb") as f:
                data_bytes = f.read()

            num_values = len(data_bytes) // 8
            data = struct.unpack(f"{num_values}d", data_bytes)

            # Split into positions and values (alternating pairs)
            self.positions = np.array(data[0::2])
            self.values = np.array(data[1::2])

            return True

        except Exception as e:
            print(f"Error loading {self.filename}: {e}")
            return False


def plot_1d_profiles(data_list: List[SimulationData], output_dir: str = "./plots/"):
    """Create 6 separate figures: c_x, c_y, c_z, p_x, p_y, p_z"""

    # Organize data
    data_dict = {"c": {"x": [], "y": [], "z": []}, "p": {"x": [], "y": [], "z": []}}

    for data in data_list:
        if data.field_type in data_dict and data.axis in data_dict[data.field_type]:
            data_dict[data.field_type][data.axis].append(data)

    # Sort by timestep
    for field_type in data_dict:
        for axis in data_dict[field_type]:
            data_dict[field_type][axis].sort(key=lambda d: d.timestep)

    # Create 6 separate plots
    field_names = {"c": "Concentration", "p": "Density"}
    colorscales = {"c": "Viridis", "p": "Plasma"}

    for field_type in ["c", "p"]:
        for axis in ["x", "y", "z"]:
            datasets = data_dict[field_type][axis]
            if not datasets:
                continue

            fig = go.Figure()

            # Get time range for coloring
            times = [d.timestep for d in datasets]
            if len(times) > 1:
                min_time, max_time = min(times), max(times)
            else:
                min_time, max_time = 0, 1

            # Add each timestep as a line (normalized)
            for data in datasets:
                if len(times) > 1:
                    color_frac = (data.timestep - min_time) / (max_time - min_time)
                else:
                    color_frac = 0.5

                color = px.colors.sample_colorscale(
                    colorscales[field_type], color_frac
                )[0]
                label = (
                    f"t = {data.timestep} years" if data.timestep > 0 else "t = 0 year"
                )

                # Normalize: 2*values/sum(values) as PDF
                normalized_values = 2.0 * data.values / np.sum(data.values)

                fig.add_trace(
                    go.Scatter(
                        x=data.positions,
                        y=normalized_values,
                        mode="lines",
                        name=label,
                        line=dict(width=3, color=color),
                    )
                )

            fig.update_layout(
                title=f"{field_names[field_type]} along {axis}-axis (Normalized PDF)",
                xaxis_title=f"{axis} (m)",
                yaxis_title=f"{field_names[field_type]} (Normalized)",
                template="plotly_white",
                width=900,
                height=600,
                font=dict(size=14),
                showlegend=True,
            )

            output_file = f"{output_dir}/{field_type}_{axis}.html"
            fig.write_html(output_file)
            print(f"✓ Saved {output_file}")


def plot_3d_animated(data_list: List[SimulationData], output_dir: str = ".", max_years: int = 300):
    """Create animated 3D visualizations using 1D profile data (0→max_years)

    Creates 3D scatter/line plots showing spatial distribution over time.
    """
    # Organize data by field type and axis
    data_dict = {"c": {"x": [], "y": [], "z": []}, "p": {"x": [], "y": [], "z": []}}

    for data in data_list:
        # Filter: only include timesteps up to max_years
        if data.timestep > max_years:
            continue

        if data.field_type in data_dict and data.axis in data_dict[data.field_type]:
            data_dict[data.field_type][data.axis].append(data)

    # Sort by timestep
    for field_type in data_dict:
        for axis in data_dict[field_type]:
            data_dict[field_type][axis].sort(key=lambda d: d.timestep)

    field_names = {"c": "Concentration", "p": "Density"}

    # Create animated 3D surface plots for each field type
    for field_type in ["c", "p"]:
        z_data = data_dict[field_type]["z"]
        x_data = data_dict[field_type]["x"]
        y_data = data_dict[field_type]["y"]

        if not z_data:
            print(f"No z-data for {field_type}")
            continue

        timesteps = sorted(set(d.timestep for d in z_data))
        if not timesteps:
            continue

        # Create frames for animation
        frames = []

        for timestep in timesteps:
            # Find data for this timestep
            z_dataset = next((d for d in z_data if d.timestep == timestep), None)
            x_dataset = next((d for d in x_data if d.timestep == timestep), None)
            y_dataset = next((d for d in y_data if d.timestep == timestep), None)

            if not z_dataset:
                continue

            # Normalize data
            z_normalized = 2.0 * z_dataset.values / np.sum(z_dataset.values)

            # Create surface from 1D profiles
            if x_dataset and y_dataset:
                x_norm = 2.0 * x_dataset.values / np.sum(x_dataset.values)
                y_norm = 2.0 * y_dataset.values / np.sum(y_dataset.values)

                # Create 2D surface: outer product of x and y profiles
                X, Y = np.meshgrid(x_dataset.positions, y_dataset.positions)

                # Different strategies for concentration vs density
                if field_type == "p":
                    # For density (highly anisotropic): sum of 1D profiles
                    # This creates a surface where the height represents combined probability
                    xy_sum = np.zeros((len(y_norm), len(x_norm)))
                    for i in range(len(y_norm)):
                        for j in range(len(x_norm)):
                            # Add contributions from x and y profiles
                            xy_sum[i, j] = x_norm[j] + y_norm[i]

                    # Normalize and scale to make visible
                    if xy_sum.max() > 0:
                        Z_surface = (xy_sum / xy_sum.max()) * 0.1  # Scale to 0.1 max height
                    else:
                        Z_surface = xy_sum
                else:
                    # For concentration: use outer product (works well for c)
                    xy_product = np.outer(y_norm, x_norm)
                    if xy_product.max() > 0:
                        xy_product_normalized = xy_product / xy_product.max()
                    else:
                        xy_product_normalized = xy_product
                    Z_surface = xy_product_normalized * z_normalized.max()

            else:
                # Fallback: use z-profile value across xy plane
                n_points = 50
                x_range = np.linspace(0, 10, n_points)
                y_range = np.linspace(0, 10, n_points)
                X, Y = np.meshgrid(x_range, y_range)
                Z_surface = np.ones_like(X) * z_normalized[len(z_dataset.positions) // 2]

            # Create 3D surface plot
            frame_data = go.Surface(
                x=X,
                y=Y,
                z=Z_surface,
                colorscale='Viridis' if field_type == 'c' else 'Plasma',
                showscale=True,
                colorbar=dict(title=f"{field_names[field_type]}<br>(Normalized)"),
                name=f"t={timestep}y",
                hovertemplate='x: %{x:.2f}m<br>y: %{y:.2f}m<br>value: %{z:.4f}<extra></extra>'
            )

            frames.append(go.Frame(
                data=[frame_data],
                name=str(timestep),
                layout=go.Layout(title_text=f"{field_names[field_type]} Surface (t={timestep} years)")
            ))

        if not frames:
            print(f"No frames created for {field_type}")
            continue

        # Create initial figure with first frame
        fig = go.Figure(
            data=[frames[0].data[0]],
            frames=frames
        )

        # Add animation controls
        fig.update_layout(
            title=f"{field_names[field_type]} Surface Evolution (0→{max_years} years)",
            scene=dict(
                xaxis_title="x (m)",
                yaxis_title="y (m)",
                zaxis_title=f"{field_names[field_type]} (Normalized)",
                camera=dict(eye=dict(x=1.5, y=1.5, z=1.3)),
                aspectmode='cube'
            ),
            updatemenus=[{
                "buttons": [
                    {
                        "args": [None, {"frame": {"duration": 500, "redraw": True},
                                       "fromcurrent": True,
                                       "transition": {"duration": 300}}],
                        "label": "▶ Play",
                        "method": "animate"
                    },
                    {
                        "args": [[None], {"frame": {"duration": 0, "redraw": True},
                                         "mode": "immediate",
                                         "transition": {"duration": 0}}],
                        "label": "⏸ Pause",
                        "method": "animate"
                    }
                ],
                "direction": "left",
                "pad": {"r": 10, "t": 87},
                "showactive": False,
                "type": "buttons",
                "x": 0.1,
                "xanchor": "right",
                "y": 0,
                "yanchor": "top"
            }],
            sliders=[{
                "active": 0,
                "yanchor": "top",
                "y": 0.05,
                "xanchor": "left",
                "currentvalue": {
                    "prefix": "Year: ",
                    "visible": True,
                    "xanchor": "right"
                },
                "pad": {"b": 10, "t": 50},
                "len": 0.9,
                "x": 0.1,
                "steps": [
                    {
                        "args": [[f.name], {
                            "frame": {"duration": 300, "redraw": True},
                            "mode": "immediate",
                            "transition": {"duration": 300}
                        }],
                        "label": str(timesteps[i]),
                        "method": "animate"
                    }
                    for i, f in enumerate(frames)
                ]
            }],
            width=1200,
            height=900
        )

        output_file = f"{output_dir}/{field_type}_3d_surface_animation.html"
        fig.write_html(output_file)
        print(f"✓ Saved {output_file}")



def main():
    parser = argparse.ArgumentParser(
        description="Visualization of particle simulation data"
    )
    parser.add_argument(
        "files", nargs="*", help="Binary data files (default: ../data/metal/*.bin)"
    )
    parser.add_argument("--output-dir", "-o", default=None, help="Output directory (default: ../plots/)")

    args = parser.parse_args()

    # Use default path if no files specified
    if not args.files:
        import glob

        script_dir = Path(__file__).parent
        default_path = script_dir / "../data/metal/*.bin"
        args.files = glob.glob(str(default_path))

        if not args.files:
            print("Error: No binary files found in ../data/metal/")
            print("Usage: uv run python visualize_simulation.py [files...]")
            return 1

    # Set default output directory relative to script location
    if args.output_dir is None:
        script_dir = Path(__file__).parent
        args.output_dir = str(script_dir / "../plots")

    # Load config
    config = parse_config_file()

    # Load all data files
    data_list = []
    for filepath in args.files:
        data = SimulationData(filepath)
        if data.load():
            data_list.append(data)

    if not data_list:
        print("Error: No valid data")
        return 1

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Always generate all 1D plots
    plot_1d_profiles(data_list, args.output_dir)

    # Generate 3D animated plots (0→300 years)
    plot_3d_animated(data_list, args.output_dir, max_years=300)

    return 0


if __name__ == "__main__":
    main()
