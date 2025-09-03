import pandas as pd
import matplotlib.pyplot as plt

def main():
    # Đọc file Excel
    file_path = "data_collector.xlsx"
    df = pd.read_excel(file_path)

    # Chỉ lấy đúng 8 trường của bài test
    columns_to_plot = ["RED", "DC", "AC", "PI", "HR"]
    times = df['time_ms']

    fig, axes = plt.subplots(len(columns_to_plot), 1, figsize=(6, 3 * len(columns_to_plot)))

    if len(columns_to_plot) == 1:
        axes = [axes]

    for i, column in enumerate(columns_to_plot):
        axes[i].plot(times, df[column])
        axes[i].set_title(f'{column} Plot')
        axes[i].set_xlabel('Time (ms)')
        axes[i].set_ylabel(column)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()