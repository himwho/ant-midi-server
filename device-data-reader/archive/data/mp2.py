from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import sys
import multiprocessing
from dateutil import parser

def plot_data(x,y,fileName):
    col_length = len(y.columns)
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
    #plt.show()
        # Save figure
    plt.savefig(fileName+".png")

if __name__ == '__main__':
    
    file_to_open = sys.argv[1]

    df = pd.read_csv(file_to_open,sep='\t')
    col_length = len(df.columns)
    column_names = ['DateTime']
    
    #date = file_to_open.split('-')[2][10:]
        date = parser.parse()
    print(date)

    #devID = file_to_open.split('-')[3][:7]
    #print(devID)

    for i in range(1,col_length):
        column_names.append(f'Col{i}')

    df.columns = column_names
    x = df['DateTime']
    y = df.iloc[:,1:]

    y1,y2 = np.array_split(y,2)
    x1,x2 = np.array_split(x,2)
    #print(y1)

    p1 = multiprocessing.Process(target=plot_data, args=(x1,y1,file_to_open))
    p2 = multiprocessing.Process(target=plot_data, args=(x2,y2,file_to_open))
    p1.start()
    p2.start()
    p1.join()
    p2.join()
    