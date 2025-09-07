import serial
import pandas as pd

ser = serial.Serial('COM6', 115200, timeout=2)
data = []
max_samples = 1000

while len(data) < max_samples:
    line = ser.readline().decode('utf-8').strip()
    if line:
        parts = line.split(",")
        if len(parts) == 15:
            data.append(parts)

ser.close()
df = pd.DataFrame(data, columns=[
    "time_ms", "RED", "DC", "AC", "PI", "HR", "P1", "P2", "Amp", "DT_up", "DT12", "W50", "RI", "AI", "SI"
])
df.to_excel("data_collector.xlsx", index=False)
print("Đã lưu dữ liệu vào data_collector.xlsx")