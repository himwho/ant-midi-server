from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import sys
file_to_open = sys.argv[1]


df = pd.read_csv(file_to_open,sep='\t')
col_length = len(df.columns)
column_names = ['DateTime']
for i in range(1,col_length):
    column_names.append(f'Col{i}')


df.columns = column_names
x = df['DateTime']
y = df.iloc[:,1:]
fig, axs = plt.subplots(11)
fig.suptitle('Vertically stacked subplots')

for i in range(1,col_length):
    y_ = y[column_names[i]]
    axs[i-1].plot(x, y_)

plt.show()



