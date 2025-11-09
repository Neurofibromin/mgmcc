import pandas as pd
import plotly.graph_objects as go
import matplotlib.pyplot as plt
import os
import sys
from io import StringIO

ALLOCATOR_FILES = {
    'results_glibc.csv': 'glibc (std malloc)',
    'results_jemalloc.csv': 'jemalloc',
    'results_tcmalloc.csv': 'tcmalloc'
}
OUTPUT_DIR = "performance_diagrams"
OUTPUT_DIR3D = "performance_diagrams_3d"

def load_and_prepare_data(allocator_files):
    all_data = []
    for filepath, allocator_name in allocator_files.items():
        print(f"Processing {filepath} for allocator '{allocator_name}'...")
        df = pd.read_csv(filepath)
        if df is None or df.empty:
            print(f"Skipping file {filepath} due to parsing errors.")
            continue
        df['Allocator'] = allocator_name
        all_data.append(df)
    if not all_data:
        return None
    combined_df = pd.concat(all_data)
    long_df = combined_df.melt(
        id_vars=['Algorithm', 'Allocator'],
        var_name='GraphSize',
        value_name='Time'
    )
    long_df['Time'] = pd.to_numeric(long_df['Time'], errors='coerce')
    long_df.dropna(subset=['Time'], inplace=True)
    long_df['Time'] = long_df['Time'].replace(0, 1e-9)
    size_extraction = long_df['GraphSize'].str.extract(r'V:(\d+),E:(\d+)')
    long_df['V'] = pd.to_numeric(size_extraction[0])
    long_df['E'] = pd.to_numeric(size_extraction[1])
    long_df.sort_values(by=['Algorithm', 'Allocator', 'V', 'E'], inplace=True)

    return long_df


def generate_plots(df, output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"Created output directory: '{output_dir}'")
    algorithms = df['Algorithm'].unique()
    for algorithm in algorithms:
        plt.style.use('seaborn-v0_8-whitegrid')
        fig, ax = plt.subplots(figsize=(12, 8))
        algo_df = df[df['Algorithm'] == algorithm]
        for allocator_name in algo_df['Allocator'].unique():
            allocator_df = algo_df[algo_df['Allocator'] == allocator_name]
            ax.plot(
                allocator_df['GraphSize'],
                allocator_df['Time'],
                marker='o',
                linestyle='-',
                label=allocator_name
            )
        ax.set_title(f'Performance of: {algorithm}', fontsize=16)
        ax.set_xlabel('Graph Size (Vertices:Edges)', fontsize=12)
        ax.set_ylabel('Execution Time (seconds) - Logarithmic Scale', fontsize=12)
        ax.legend(title='Allocator', fontsize=10)
        ax.set_yscale('log')
        plt.xticks(rotation=45, ha='right')
        fig.tight_layout()
        safe_filename = algorithm.replace('/', '_').replace('\\', '_')
        output_path = os.path.join(output_dir, f"{safe_filename}.png")
        plt.savefig(output_path)
        plt.close(fig)
        print(f"Saved plot: {output_path}")

def generate_3d_plots(df, output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"Created output directory: '{output_dir}'")
    algorithms = df['Algorithm'].unique()
    for algorithm in algorithms:
        fig = go.Figure()
        algo_df = df[df['Algorithm'] == algorithm]
        for allocator_name in algo_df['Allocator'].unique():
            allocator_df = algo_df[algo_df['Allocator'] == allocator_name]
            fig.add_trace(go.Scatter3d(
                x=allocator_df['V'],
                y=allocator_df['E'],
                z=allocator_df['Time'],
                mode='markers',
                marker=dict(
                    size=5,
                    opacity=0.8
                ),
                name=allocator_name,
                hovertemplate='<b>' + allocator_name + '</b><br>' +
                              'Vertices (V): %{x}<br>' +
                              'Edges (E): %{y}<br>' +
                              'Time (s): %{z:.6f}<extra></extra>'
            ))
        fig.update_layout(
            title=f'Performance of: {algorithm}',
            scene=dict(
                xaxis_title='Number of Vertices (V)',
                yaxis_title='Number of Edges (E)',
                zaxis_title='Execution Time (seconds) - Log Scale',
                zaxis_type='log'
            ),
            legend_title_text='Allocator',
            margin=dict(l=0, r=0, b=0, t=40)
        )
        safe_filename = algorithm.replace('/', '_').replace('\\', '_')
        output_path = os.path.join(output_dir, f"{safe_filename}.html")
        fig.write_html(output_path)
        print(f"Saved 3D plot: {output_path}")


def main():
    for f in ALLOCATOR_FILES.keys():
        if not os.path.exists(f):
            print(f"[ERROR] Input file not found: '{f}'. Please ensure it is in the same directory.")
            sys.exit(1)
    performance_data = load_and_prepare_data(ALLOCATOR_FILES)
    if performance_data is not None and not performance_data.empty:
        generate_plots(performance_data, OUTPUT_DIR)
        print(f"\nAll 2D plots have been generated in the '{OUTPUT_DIR}' directory.")
        generate_3d_plots(performance_data, OUTPUT_DIR3D)
        print(f"\nAll 3D plots have been generated in the '{OUTPUT_DIR3D}' directory.")
    else:
        print("\nNo data was processed. Exiting.")

if __name__ == '__main__':
    main()