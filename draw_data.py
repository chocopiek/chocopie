import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def main():
    # Đọc file Excel
    file_path = "data_collector.xlsx"  # đổi lại đúng đường dẫn file của bạn
    df = pd.read_excel(file_path)

    # Xử lý dữ liệu
    times = df['time_ms']
    df = df.drop(columns=['time_ms'])
    num_columns = len(df.columns)

    # Vẽ dữ liệu
    fig, axes = plt.subplots(num_columns, 1, figsize=(6, 4 * num_columns))

    if num_columns == 1:
        axes = [axes]  # nếu chỉ có 1 cột thì chuyển thành list để dễ xử lý

    for i, column in enumerate(df.columns):
        axes[i].plot(times, df[column])
        axes[i].set_title(f'{column} Plot')
        axes[i].set_xlabel('Time (ms)')
        axes[i].set_ylabel(column)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
