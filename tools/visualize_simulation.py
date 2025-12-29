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


def main():
    parser = argparse.ArgumentParser(
        description="Visualization of particle simulation data"
    )
    parser.add_argument(
        "files", nargs="*", help="Binary data files (default: ../data/metal/*.bin)"
    )
    parser.add_argument(
        "--output-dir", "-o", default=None, help="Output directory (default: ../plots/)"
    )

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

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Always generate all 1D plots
    plot_1d_profiles(data_list, args.output_dir)


if __name__ == "__main__":
    main()
