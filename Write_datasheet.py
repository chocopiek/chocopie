import serial
import pandas as pd

ser = serial.Serial('COM6', 115200, timeout=2)
data = []
max_samples = 2000



while len(data) < max_samples:
    line = ser.readline().decode('latin-1').strip()
    if line:
        parts = line.split(",")
        if len(parts) ==  2:
            data.append(parts)

ser.close()
df = pd.DataFrame(data, columns=["time_ms", "AC"])
df.to_excel("data_sheet.xlsx", index=False)
print("Đã lưu dữ liệu vào data_sheet.xlsx")