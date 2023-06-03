import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

Data_normal = pd.read_csv("6_Collect_Prediction_Data/Prediction_Data_normal.csv")
Data_anormal = pd.read_csv("6_Collect_Prediction_Data/Prediction_Data_anormal.csv")

Data_normal = Data_normal.values
Data_anormal = Data_anormal.values

size = len(Data_anormal)

mean_normal = np.mean(Data_normal[:size])
mean_anormal = np.mean(Data_anormal[:size])

print("Mean normal is: ", mean_normal)
print("Mean anormal is: ", mean_anormal)

plt.plot(Data_normal[:size])
plt.plot(Data_anormal[:size])
plt.show()