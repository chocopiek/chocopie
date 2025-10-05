import pandas as pd
import matplotlib.pyplot as plt

file_path = "data_sheet.xlsx"
df = pd.read_excel(file_path)

# Đọc dữ liệu
times = df.iloc[:, 0]      # time_ms (cột 1)
ac_values = df.iloc[:, 1]  # AC (cột 2)
peak = df.iloc[:, 2]  # P1 (cột 3)

plt.figure(figsize=(12,6))
plt.plot(times, ac_values, linewidth=1, color="blue", label="AC")

## Đánh dấu các điểm P1 và P2 trên đồ thị AC
plt.scatter(times, peak, color="red", marker="o", label="P1")

plt.title("Tín hiệu AC và các đỉnh P1, P2 theo thời gian")
plt.xlabel("Time (ms)")
plt.ylabel("AC Value")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()