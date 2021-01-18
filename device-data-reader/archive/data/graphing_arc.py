from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


file_to_open ="2020-09-08-Device0.txt"

file = df = pd.read_csv(file_to_open,sep='\t',names=["DateTime", "Col1", "Col2", "Col3","Col4","Col5","Col6","Col7","Col8","Col9","Col10","Col11"])

x = df['DateTime']
y = df.iloc[:,1:]
for col in y.columns:
    y_ = y[col]
    plt.plot(x,y_,label=f'{col}')
plt.xlabel('TimeDate')
# Set the y axis label of the current axis.
plt.ylabel('Readings')
# Set a title of the current axes.
plt.title('Reading of 11 columns')
# show a legend on the plot
plt.legend()
# Display a figure.
plt.show()
