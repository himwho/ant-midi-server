import os
import time
import matplotlib
import pandas as pd
import seaborn as sns
from os import listdir
from datetime import datetime
import matplotlib.pyplot as plt
from os.path import isfile, join
from matplotlib.colors import ListedColormap
from sys import platform

import warnings
warnings.filterwarnings('ignore')

# %matplotlib inline
matplotlib.use('Agg')
sns.set(font_scale = 3)

# TODO: CREATE GLOBAL THRESHOLD FOR DELTAS
# TODO: CREATE STRING INPUTS FOR DEVICE NAME
# TODO: SCALE FUNCTION FOR INT INPUT (NUM_SENSORS)

# Use this for device
# Arrays to be used in this code
arr1 = []
d,t,s1,s2,s3,s4,s5,s6 = ([] for i in range(8))
c_s1,d_s1,c_s2,d_s2,c_s3,d_s3,c_s4,d_s4,c_s5,d_s5,c_s6,d_s6 = ([] for i in range(12))
da,ti,se1,se2,se3,se4,se5,se6 = ([] for i in range(8))

# Read the input files
def read_file(filename):
    f = open(filename)
    for line in f.readlines():
        l = line.split("\t")
        d.append(l[0][1:20])
        t.append(l[0][12:20].replace(":",""))
        s1.append(int(l[1]));s2.append(int(l[2]));s3.append(int(l[3]));s4.append(int(l[4]));s5.append(int(l[5]));s6.append(int(l[6]))


# Finding deltas/change in sensor values by the value of 10
def find_deltas():
    for i in range(1,len(s1)-1):
        if (abs(s1[i+1]-s1[i]) >9):
            c_s1.append(abs(s1[i+1]-s1[i]));d_s1.append(d[i+1])
        if (abs(s2[i+1]-s2[i]) >9):
            c_s2.append(abs(s2[i+1]-s2[i]));d_s2.append(d[i+1])
        if (abs(s3[i+1]-s3[i]) >9):
            c_s3.append(abs(s3[i+1]-s3[i]));d_s3.append(d[i+1])
        if (abs(s4[i+1]-s4[i]) >9):
            c_s4.append(abs(s4[i+1]-s4[i]));d_s4.append(d[i+1])
        if (abs(s5[i+1]-s5[i]) >9):
            c_s5.append(abs(s5[i+1]-s5[i]));d_s5.append(d[i+1])
        if (abs(s6[i+1]-s6[i]) >9):
            c_s6.append(abs(s6[i+1]-s6[i]));d_s6.append(d[i+1])

# Calculating average values for each sensor
def find_avg():
    div = 500
    for i in range(0,len(s1),div):
        if (i+(500+1) <= len(s1)):
            da.append(d[i+div])
            ti.append(t[i+div])
            se1.append(int((sum(s1[i:i+div]))/div))
            se2.append(int((sum(s2[i:i+div]))/div))
            se3.append(int((sum(s3[i:i+div]))/div))
            se4.append(int((sum(s4[i:i+div]))/div))
            se5.append(int((sum(s5[i:i+div]))/div))
            se6.append(int((sum(s6[i:i+div]))/div))
        else:
            pass

def plot_write_delta(c,d,sen,file):
    if(len(c)>10):
        dates =['02:30','05:00','07:30','10:00','12:30','15:00','17:30','20:00','22:30'] # TODO: CHANGE TO SCALED TIME RANGE
        sd = {'date':d,'value':c}
        temp_df = pd.DataFrame(data=sd)
        temp_df['date']= pd.to_datetime(temp_df['date'])
        plt.figure(figsize=(30,16))
        g = sns.scatterplot(data = temp_df,x='date',y='value',hue ='value',s = 100)
        plt.xlabel('Time')
        plt.ylabel(sen)
        g.set_xticklabels(labels = dates)
        cwd = os.getcwd()
        if platform == "win32":
            cwd = cwd+'\dev1\Graphs_of_deltas\\'
        else:
            cwd = cwd+'/dev1/Graphs_of_deltas/'
        print(cwd)
        plt.savefig(cwd+sen +file+ '_deltas.png', bbox_inches='tight')
        cwd = os.getcwd()
        if platform == "win32":
            cwd = cwd+'\dev1\Reports_of_deltas\\'
        else:
            cwd = cwd+'/dev1/Reports_of_deltas/'
        f = open(cwd+sen+file+"_avg.txt",'w')
        f.write("Date and TIme")
        f.write('\t\t')
        f.write('Sensor Value')
        f.write('\n')
        for i in range(len(temp_df['date'])):
            f.write(str(temp_df['date'][i]))
            f.write("\t")
            f.write(str(temp_df['value'][i]))
            f.write('\n')
        f.close()

def plot_write_avg(sen,file):
    df1 = df[['date',sen]]
    dates =['02:30','05:00','07:30','10:00','12:30','15:00','17:30','20:00','22:30']
    plt.figure(figsize=(22,15))
    g = sns.scatterplot(data = df1,x = "date", y = sen,hue = sen)
    g.set_xticklabels(labels = dates,rotation=60)
    cwd = os.getcwd()
    if platform == "win32":
        cwd = cwd+'\dev1\Graphs_of_averages\\'
    else:
        cwd = cwd+'/dev1/Graphs_of_averages/'
    print(cwd)
    plt.savefig(cwd+sen +file+ "_avg.png", bbox_inches='tight')
    cwd = os.getcwd()
    if platform == "win32":
        cwd = cwd+'\dev1\Reports_of_averages\\'
    else:
        cwd = cwd+'/dev1/Reports_of_averages/'
    f = open(cwd+sen+file+"_avg.txt",'w')
    f.write("Date and TIme")
    f.write('\t\t')
    f.write('Sensor Value')
    f.write('\n')
    for i in range(len(df['date'])):
        f.write(str(df['date'][i]))
        f.write("\t")
        f.write(str(df[sen][i]))
        f.write('\n')
    f.close()


cwd = os.getcwd()
if platform == "win32":
    cwd = cwd + '\dev1'
else:
    cwd = cwd + '/dev1'
mypath = cwd
files = [f for f in listdir(mypath) if isfile(join(mypath, f))]
for file in files:
    file_n = file.split(".")[0]
    print("Reading the file",file)
    if platform == "win32":
        read_file(cwd +'\\'+ file)
    else:
        read_file(cwd +'/'+ file)
    print("Calculating Deltas by difference of 10")
    find_deltas()
    print("Calculating average")
    find_avg()
    dic = {'date':da,'time':ti,'sensor1':se1,'sensor2':se2,'sensor3':se3,'sensor4':se4,'sensor5':se5,'sensor6':se6}
    df = pd.DataFrame(data=dic)
    df['date']= pd.to_datetime(df['date'])
    print("saving deltas for sensor1")
    plot_write_delta(c_s1,d_s1,'sensor1',file_n)
    print("saving deltas for sensor2")
    plot_write_delta(c_s2,d_s2,'sensor2',file_n)
    print("saving deltas for sensor3")
    plot_write_delta(c_s3,d_s3,'sensor3',file_n)
    print("saving deltas for sensor4")
    plot_write_delta(c_s4,d_s4,'sensor4',file_n)
    print("saving deltas for sensor5")
    plot_write_delta(c_s5,d_s5,'sensor5',file_n)
    print("saving deltas for sensor6")
    plot_write_delta(c_s6,d_s6,'sensor6',file_n) 

    for i in range(1,7):
        print("Saving average for sensor" + str(i))
        plot_write_avg('sensor'+str(i),file_n)