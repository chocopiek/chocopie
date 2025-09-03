import serial
import pandas as pd

ser = serial.Serial('COM4', 115200, timeout=2)
data = []
max_samples = 2000

while len(data) < max_samples:
    line = ser.readline().decode('utf-8').strip()
    if line:
        parts = line.split(",")
        if len(parts) == 6:
            data.append(parts)

ser.close()
df = pd.DataFrame(data, columns=["time_ms", "RED", "DC", "AC", "PI", "HR" ])
df.to_excel("data_collector.xlsx", index=False)
print("Đã lưu data_collector.xlsx thành công!")
# Lưu dữ liệu vào file Excel


