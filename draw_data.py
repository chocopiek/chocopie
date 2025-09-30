import pandas as pd
import matplotlib.pyplot as plt

file_path = "data_sheet.xlsx"
df = pd.read_excel(file_path)

times = df['time_ms']
ac_values = df['AC']


plt.figure(figsize=(12,6)) 
plt.plot(times, ac_values,  linewidth=1, color="blue") 
plt.title("Tín hiệu AC theo thời gian") 
plt.xlabel("Time (ms)") 
plt.ylabel("AC Value") 
plt.grid(True) 
plt.tight_layout() 
plt.show()
