#!/usr/bin/env python3
"""
Visualization Tool for Particle Simulation Output

This script provides comprehensive visualization and analysis of the particle
simulation output data (concentration field and density field).

Features:
- Load binary output files (.bin)
- 3D spatial visualization (heatmaps, slices, volumes)
- Time evolution plots
- Statistical analysis
- Comparison between multiple runs
- Export figures and animations

Usage:
    python visualize_simulation.py data/*.bin --plot-type 3d
    python visualize_simulation.py data/*.bin --stats
    python visualize_simulation.py data/*.bin --animate
    python visualize_simulation.py data/concentration_*.bin data/density_*.bin --compare
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import animation
from mpl_toolkits.mplot3d import Axes3D
import struct
import os
import sys
from pathlib import Path
from typing import List, Tuple, Optional


class SimulationData:
    """Container for simulation output data"""

    def __init__(self, filename: str):
        self.filename = filename
        self.data = None
        self.shape = None
        self.field_type = None  # 'concentration' or 'density'
        self.timestep = None

    def load(self) -> bool:
        """Load binary data from file"""
        try:
            # Infer grid dimensions from filename or use defaults
            # Expected format: concentration_betan_para.bin or density_betan_para.bin
            if 'concentration' in self.filename.lower():
                self.field_type = 'concentration'
            elif 'density' in self.filename.lower():
                self.field_type = 'density'
            else:
                self.field_type = 'unknown'

            # Default grid from simulation_config.h
            nx, ny, nz = 50, 50, 1600

            # Read binary data as doubles
            with open(self.filename, 'rb') as f:
                data_bytes = f.read()

            # Each double is 8 bytes
            num_values = len(data_bytes) // 8
            expected_size = nx * ny * nz

            if num_values == expected_size:
                # 3D field data
                data_flat = struct.unpack(f'{num_values}d', data_bytes)
                self.data = np.array(data_flat).reshape(nx, ny, nz)
                self.shape = (nx, ny, nz)
            else:
                print(f"Warning: Unexpected data size in {self.filename}")
                print(f"  Expected: {expected_size} values, Got: {num_values} values")
                # Try to load anyway
                data_flat = struct.unpack(f'{num_values}d', data_bytes)
                self.data = np.array(data_flat)
                self.shape = self.data.shape

            return True

        except Exception as e:
            print(f"Error loading {self.filename}: {e}")
            return False

    def get_stats(self) -> dict:
        """Compute statistics of the field"""
        if self.data is None:
            return {}

        return {
            'min': np.min(self.data),
            'max': np.max(self.data),
            'mean': np.mean(self.data),
            'std': np.std(self.data),
            'median': np.median(self.data),
            'total': np.sum(self.data),
            'shape': self.shape
        }


def plot_2d_slice(data: np.ndarray, slice_axis: str = 'z', slice_idx: Optional[int] = None,
                  title: str = "Field Slice", cmap: str = 'viridis', save_path: Optional[str] = None):
    """
    Plot a 2D slice through the 3D data

    Args:
        data: 3D numpy array (nx, ny, nz)
        slice_axis: 'x', 'y', or 'z'
        slice_idx: Index to slice at (default: middle)
        title: Plot title
        cmap: Colormap name
        save_path: Path to save figure (optional)
    """
    fig, ax = plt.subplots(figsize=(10, 8))

    if slice_axis.lower() == 'x':
        idx = slice_idx if slice_idx is not None else data.shape[0] // 2
        slice_data = data[idx, :, :]
        ax.set_xlabel('Y index')
        ax.set_ylabel('Z index')
        title += f' (X={idx})'
    elif slice_axis.lower() == 'y':
        idx = slice_idx if slice_idx is not None else data.shape[1] // 2
        slice_data = data[:, idx, :]
        ax.set_xlabel('X index')
        ax.set_ylabel('Z index')
        title += f' (Y={idx})'
    else:  # z
        idx = slice_idx if slice_idx is not None else data.shape[2] // 2
        slice_data = data[:, :, idx]
        ax.set_xlabel('X index')
        ax.set_ylabel('Y index')
        title += f' (Z={idx})'

    im = ax.imshow(slice_data.T, origin='lower', cmap=cmap, aspect='auto')
    ax.set_title(title)
    plt.colorbar(im, ax=ax, label='Field value')

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved figure to {save_path}")

    plt.show()


def plot_multiple_slices(data: np.ndarray, axis: str = 'z', num_slices: int = 4,
                         title: str = "Field Slices", cmap: str = 'viridis',
                         save_path: Optional[str] = None):
    """Plot multiple slices along an axis"""

    if axis.lower() == 'z':
        n = data.shape[2]
    elif axis.lower() == 'y':
        n = data.shape[1]
    else:
        n = data.shape[0]

    indices = np.linspace(0, n-1, num_slices, dtype=int)

    fig, axes = plt.subplots(1, num_slices, figsize=(5*num_slices, 4))

    for i, idx in enumerate(indices):
        if axis.lower() == 'x':
            slice_data = data[idx, :, :]
            axes[i].set_xlabel('Y')
            axes[i].set_ylabel('Z')
        elif axis.lower() == 'y':
            slice_data = data[:, idx, :]
            axes[i].set_xlabel('X')
            axes[i].set_ylabel('Z')
        else:  # z
            slice_data = data[:, :, idx]
            axes[i].set_xlabel('X')
            axes[i].set_ylabel('Y')

        im = axes[i].imshow(slice_data.T, origin='lower', cmap=cmap, aspect='auto')
        axes[i].set_title(f'{axis.upper()}={idx}')
        plt.colorbar(im, ax=axes[i])

    fig.suptitle(title, fontsize=16)
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved figure to {save_path}")

    plt.show()


def plot_profile(data: np.ndarray, axis: str = 'z', position: Optional[Tuple[int, int]] = None,
                 title: str = "Field Profile", save_path: Optional[str] = None):
    """
    Plot 1D profile along an axis at a specific position

    Args:
        data: 3D numpy array
        axis: 'x', 'y', or 'z'
        position: (i, j) position for other two axes (default: center)
        title: Plot title
        save_path: Path to save figure
    """
    fig, ax = plt.subplots(figsize=(10, 6))

    if position is None:
        i, j = data.shape[0]//2, data.shape[1]//2
    else:
        i, j = position

    if axis.lower() == 'x':
        profile = data[:, j, i]
        ax.set_xlabel('X index')
        title += f' at Y={j}, Z={i}'
    elif axis.lower() == 'y':
        profile = data[i, :, j]
        ax.set_xlabel('Y index')
        title += f' at X={i}, Z={j}'
    else:  # z
        profile = data[i, j, :]
        ax.set_xlabel('Z index')
        title += f' at X={i}, Y={j}'

    ax.plot(profile, linewidth=2)
    ax.set_ylabel('Field value')
    ax.set_title(title)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved figure to {save_path}")

    plt.show()


def plot_3d_isosurface(data: np.ndarray, threshold: Optional[float] = None,
                       title: str = "3D Isosurface", save_path: Optional[str] = None):
    """
    Plot 3D isosurface (requires mayavi or plotly for true 3D)
    This version shows a simple 3D scatter plot for high-value regions
    """
    if threshold is None:
        threshold = np.mean(data) + np.std(data)

    # Find points above threshold
    indices = np.where(data > threshold)

    fig = plt.figure(figsize=(12, 10))
    ax = fig.add_subplot(111, projection='3d')

    # Subsample if too many points
    step = max(1, len(indices[0]) // 10000)

    scatter = ax.scatter(indices[0][::step], indices[1][::step], indices[2][::step],
                        c=data[indices][::step], cmap='viridis', alpha=0.6, s=1)

    ax.set_xlabel('X index')
    ax.set_ylabel('Y index')
    ax.set_zlabel('Z index')
    ax.set_title(title + f' (threshold={threshold:.2e})')

    plt.colorbar(scatter, ax=ax, label='Field value')

    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved figure to {save_path}")

    plt.show()


def plot_statistics(data_list: List[SimulationData], save_path: Optional[str] = None):
    """Plot statistical comparison of multiple datasets"""

    if len(data_list) == 0:
        print("No data to plot")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    # Extract statistics
    stats = [d.get_stats() for d in data_list]
    labels = [os.path.basename(d.filename) for d in data_list]

    # Min/Max
    mins = [s['min'] for s in stats]
    maxs = [s['max'] for s in stats]
    x = np.arange(len(data_list))

    axes[0, 0].bar(x - 0.2, mins, 0.4, label='Min', alpha=0.7)
    axes[0, 0].bar(x + 0.2, maxs, 0.4, label='Max', alpha=0.7)
    axes[0, 0].set_ylabel('Value')
    axes[0, 0].set_title('Min/Max Values')
    axes[0, 0].set_xticks(x)
    axes[0, 0].set_xticklabels(labels, rotation=45, ha='right')
    axes[0, 0].legend()
    axes[0, 0].grid(True, alpha=0.3)

    # Mean/Std
    means = [s['mean'] for s in stats]
    stds = [s['std'] for s in stats]

    axes[0, 1].errorbar(x, means, yerr=stds, fmt='o-', capsize=5, markersize=8)
    axes[0, 1].set_ylabel('Value')
    axes[0, 1].set_title('Mean ± Std Dev')
    axes[0, 1].set_xticks(x)
    axes[0, 1].set_xticklabels(labels, rotation=45, ha='right')
    axes[0, 1].grid(True, alpha=0.3)

    # Total (integral)
    totals = [s['total'] for s in stats]

    axes[1, 0].bar(x, totals, alpha=0.7, color='green')
    axes[1, 0].set_ylabel('Total (Sum)')
    axes[1, 0].set_title('Total Field Integral')
    axes[1, 0].set_xticks(x)
    axes[1, 0].set_xticklabels(labels, rotation=45, ha='right')
    axes[1, 0].grid(True, alpha=0.3)

    # Histogram comparison
    for i, data in enumerate(data_list):
        axes[1, 1].hist(data.data.flatten(), bins=50, alpha=0.5,
                       label=labels[i], density=True)

    axes[1, 1].set_xlabel('Field value')
    axes[1, 1].set_ylabel('Probability density')
    axes[1, 1].set_title('Value Distribution')
    axes[1, 1].legend()
    axes[1, 1].grid(True, alpha=0.3)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved figure to {save_path}")

    plt.show()


def create_animation(data_list: List[SimulationData], axis: str = 'z',
                     slice_idx: Optional[int] = None, interval: int = 200,
                     save_path: Optional[str] = None):
    """
    Create animation cycling through datasets (time evolution)

    Args:
        data_list: List of SimulationData objects (ordered in time)
        axis: Which axis to slice
        slice_idx: Index to slice at
        interval: Milliseconds between frames
        save_path: Path to save animation (e.g., 'animation.mp4')
    """
    if len(data_list) == 0:
        print("No data for animation")
        return

    fig, ax = plt.subplots(figsize=(10, 8))

    # Get slice index
    if slice_idx is None:
        slice_idx = data_list[0].shape[2] // 2 if axis == 'z' else data_list[0].shape[0] // 2

    # Get first frame
    if axis.lower() == 'z':
        first_slice = data_list[0].data[:, :, slice_idx]
        ax.set_xlabel('X index')
        ax.set_ylabel('Y index')
    elif axis.lower() == 'y':
        first_slice = data_list[0].data[:, slice_idx, :]
        ax.set_xlabel('X index')
        ax.set_ylabel('Z index')
    else:
        first_slice = data_list[0].data[slice_idx, :, :]
        ax.set_xlabel('Y index')
        ax.set_ylabel('Z index')

    # Find global min/max for consistent colorbar
    vmin = min(d.data.min() for d in data_list)
    vmax = max(d.data.max() for d in data_list)

    im = ax.imshow(first_slice.T, origin='lower', cmap='viridis',
                   vmin=vmin, vmax=vmax, aspect='auto')
    title = ax.text(0.5, 1.05, '', transform=ax.transAxes, ha='center', fontsize=14)
    plt.colorbar(im, ax=ax, label='Field value')

    def update(frame):
        if axis.lower() == 'z':
            slice_data = data_list[frame].data[:, :, slice_idx]
        elif axis.lower() == 'y':
            slice_data = data_list[frame].data[:, slice_idx, :]
        else:
            slice_data = data_list[frame].data[slice_idx, :, :]

        im.set_array(slice_data.T)
        title.set_text(f'{data_list[frame].field_type.capitalize()} - Frame {frame+1}/{len(data_list)}')
        return [im, title]

    anim = animation.FuncAnimation(fig, update, frames=len(data_list),
                                  interval=interval, blit=True)

    if save_path:
        anim.save(save_path, writer='pillow' if save_path.endswith('.gif') else 'ffmpeg',
                 fps=1000//interval, dpi=150)
        print(f"Saved animation to {save_path}")

    plt.show()


def print_statistics(data_list: List[SimulationData]):
    """Print detailed statistics to console"""

    print("\n" + "="*70)
    print("SIMULATION DATA STATISTICS")
    print("="*70)

    for data in data_list:
        stats = data.get_stats()
        print(f"\nFile: {os.path.basename(data.filename)}")
        print(f"  Type: {data.field_type}")
        print(f"  Shape: {stats.get('shape', 'N/A')}")
        print(f"  Min:    {stats['min']:.6e}")
        print(f"  Max:    {stats['max']:.6e}")
        print(f"  Mean:   {stats['mean']:.6e}")
        print(f"  Median: {stats['median']:.6e}")
        print(f"  Std:    {stats['std']:.6e}")
        print(f"  Total:  {stats['total']:.6e}")

    print("\n" + "="*70 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description='Visualize particle simulation output data',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    parser.add_argument('files', nargs='+', help='Binary data files to visualize')
    parser.add_argument('--stats', action='store_true', help='Print statistics')
    parser.add_argument('--plot-slice', choices=['x', 'y', 'z'], help='Plot 2D slice')
    parser.add_argument('--slice-index', type=int, help='Slice index (default: middle)')
    parser.add_argument('--plot-multi-slice', choices=['x', 'y', 'z'], help='Plot multiple slices')
    parser.add_argument('--num-slices', type=int, default=4, help='Number of slices to plot')
    parser.add_argument('--plot-profile', choices=['x', 'y', 'z'], help='Plot 1D profile')
    parser.add_argument('--plot-3d', action='store_true', help='Plot 3D isosurface')
    parser.add_argument('--threshold', type=float, help='Threshold for 3D plot')
    parser.add_argument('--compare', action='store_true', help='Compare multiple files statistically')
    parser.add_argument('--animate', action='store_true', help='Create animation from files')
    parser.add_argument('--output', '-o', help='Output file for saving plots/animations')
    parser.add_argument('--colormap', '-c', default='viridis',
                       help='Matplotlib colormap (default: viridis)')

    args = parser.parse_args()

    # Load all data files
    print(f"Loading {len(args.files)} data file(s)...")
    data_list = []
    for filepath in args.files:
        data = SimulationData(filepath)
        if data.load():
            data_list.append(data)
            print(f"  ✓ Loaded {os.path.basename(filepath)}")
        else:
            print(f"  ✗ Failed to load {filepath}")

    if len(data_list) == 0:
        print("Error: No valid data files loaded")
        return 1

    # Execute requested visualizations
    if args.stats:
        print_statistics(data_list)

    if args.compare:
        plot_statistics(data_list, save_path=args.output)

    if args.plot_slice:
        for i, data in enumerate(data_list):
            output = f"{args.output}_{i}" if args.output and len(data_list) > 1 else args.output
            plot_2d_slice(data.data, slice_axis=args.plot_slice,
                         slice_idx=args.slice_index,
                         title=f"{data.field_type.capitalize()} Field",
                         cmap=args.colormap, save_path=output)

    if args.plot_multi_slice:
        for data in data_list:
            plot_multiple_slices(data.data, axis=args.plot_multi_slice,
                                num_slices=args.num_slices,
                                title=f"{data.field_type.capitalize()} Field Slices",
                                cmap=args.colormap, save_path=args.output)

    if args.plot_profile:
        for data in data_list:
            plot_profile(data.data, axis=args.plot_profile,
                        title=f"{data.field_type.capitalize()} Profile",
                        save_path=args.output)

    if args.plot_3d:
        for data in data_list:
            plot_3d_isosurface(data.data, threshold=args.threshold,
                              title=f"{data.field_type.capitalize()} 3D",
                              save_path=args.output)

    if args.animate:
        create_animation(data_list, axis='z', interval=200, save_path=args.output)

    # If no specific plot requested, show default visualization
    if not any([args.stats, args.compare, args.plot_slice, args.plot_multi_slice,
                args.plot_profile, args.plot_3d, args.animate]):
        print("\nNo visualization specified. Showing default: statistics + z-slice")
        print_statistics(data_list)
        for data in data_list:
            plot_2d_slice(data.data, slice_axis='z',
                         title=f"{data.field_type.capitalize()} Field (Z-slice)",
                         cmap=args.colormap)

    return 0


if __name__ == '__main__':
    sys.exit(main())
