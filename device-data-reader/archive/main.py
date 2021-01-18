# Initial single reader script for output logs
# by Dylan Marcus

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

df = pd.read_csv('data/2020-08-23-Device0.txt')
timestamp_column = df.iloc[:, 0]

df.sample(5, random_state=0)
sns.set(rc={'figure.figsize':(11, 4)})

fig, ax = plt.subplots()
ax.plot(df[df.iloc[:,1]], marker='.', markersize=2, color='0.6', linestyle='None', label='Daily')
#opsd_7d = opsd_daily[data_columns].rolling(7, center=True).mean()
#opsd_365d = opsd_daily[data_columns].rolling(window=365, center=True, min_periods=360).mean()

#ax.plot(opsd_7d['Consumption'], linewidth=2, label='7-d Rolling Mean')
#ax.plot(opsd_365d['Consumption'], color='0.2', linewidth=3,
#label='Trend (365-d Rolling Mean)')
# Set x-ticks to yearly interval and add legend and labels
ax.legend()



#cols_plot = [df.iloc[: , [1, len(df.columns-1)]]
#for numSensors in range(len(df.columns-1))
