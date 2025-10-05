import serial
import pandas as pd

ser = serial.Serial('COM6', 115200, timeout=2)
data = []
max_samples = 2000

while len(data) < max_samples:
    line = ser.readline().decode('utf-8').strip()
    if line:
        parts = line.split(",")
        # Ghi mọi dòng có từ 2 trường trở lên (ưu tiên đủ trường)
        if len(parts) >= 2:
            data.append(parts)

ser.close()

# Nếu file Main.ino xuất đủ 2 trường: ["time_ms", "AC"]
# Nếu xuất nhiều trường hơn, bạn có thể tự động lấy số cột từ dòng đầu tiên
num_cols = max(len(row) for row in data)
columns = [f"col_{i+1}" for i in range(num_cols)]
df = pd.DataFrame(data, columns=columns)
df.to_excel("data_sheet.xlsx", index=False)
print("Đã lưu dữ liệu vào data_sheet.xlsx")