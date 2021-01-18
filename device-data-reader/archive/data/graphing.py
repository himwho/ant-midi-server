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
for col in y.columns:
    y_ = y[col]
    plt.plot(x,y_,label=f'{col}')

plt.xlabel('DateTime')
# Set the y axis label of the current axis.
plt.ylabel('Readings')
# Set a title of the current axes.
plt.title(f'Reading of {col_length} columns')
# show a legend on the plot
plt.legend()
# Display a figure.
plt.show()
