import numpy as np
import pandas as pd
import os
import argparse
import logging

logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)

"""
The script expects the pwd to have directories with following structure:
- <expt-1>
    - FFTInputX.txt, FFTInputY.txt, FFTInputZ.txt
    - FFTOutputX.txt, FFTOutputY.txt, FFTOutput.txt
- <expt-2>
    - FFTInputX.txt, FFTInputY.txt, FFTInputZ.txt
    - FFTOutputX.txt, FFTOutputY.txt, FFTOutput.txt
...
Each <expt-n> directory contains FFTInput[X|Y|Z].txt and FFTOutput[X|Y|Z].txt files retrieved from the IDE.
Each FFTInput file contains n number of blocks of samples, and FFTOuptut file contains the corresponding n number of 
acceleration, velocity and displacement FFTs along with velocity and displacement RMS values.    
"""

parser = argparse.ArgumentParser(description="Script to convert raw data recorded on device into csv format")
parser.add_argument("-n", "--number", help="maximum number of blocks of samples to save in csv file", type=int, default=1)
parser.add_argument("directories", help="list of directories containing data fetched from IDE, 'ALL' to convert all directories",
                    type=str, nargs="+")
args = parser.parse_args()

if(args.directories == ["ALL"]):
    '''Ensure that the pwd contains only directories which have FFTInput and FFTOutput files'''
    root_directories = os.listdir()
    root_directories = [root_directory for root_directory in os.listdir()
                    if (os.path.isdir(root_directory) and "ipynb" not in root_directory and "." not in root_directory)]
    # ipynb - ignore python notebook files
    # . - ignore any config files, (for IDE configs e.g. .vscode, .idea etc)
else:
    # remove trailing '/' in case user used autocomplete to specify directories
    root_directories = args.expt_directories
    for i in range(len(root_directories)):
        if (root_directories[i][-1] == "/"):
            root_directories[i] = root_directories[i][:-1]

# Pre-process files to remove the FFT info
fft_files = {}
for root_directory in root_directories:
    fft_files[root_directory] = {}
    for direction in ["X", "Y", "Z"]:
        fft_input_filepath = os.path.join(root_directory, "FFTInput{}.txt".format(direction))
        with open(fft_input_filepath, "r") as fft_input_file:
            fft_input_data = fft_input_file.read()
        fft_input_data = fft_input_data.split("\n")[5:] # remove the FFT info lines here
        fft_input_data = "\n".join(fft_input_data)
        fft_files[root_directory][direction] = fft_input_data

"""
Format of "samples" dict - 
samples[root_directory][direction]
where root_directory = "directory with FFTInput[X|Y|Z].txt files"
      direction = X | Y | Z
samples[root_directory][direction] is an array of 25 records, each record contains 4096 samples. 
Each record is a string of float values separated by ",".
"""

record_separator = "-------------------------------------\n"
# extract atmost "args.number(default=1)" samples for each direction, each root_directory
samples = {}
for root_directory in root_directories:
    samples[root_directory] = {}
    for direction in ["X", "Y", "Z"]:
        fft_input = fft_files[root_directory][direction]
        samples[root_directory][direction] = []
        for record in fft_input.split(record_separator)[:-1]:
            record = record.split("\n")[1]  # remove the timestamp line
            if (len(samples[root_directory][direction]) != args.number):
                samples[root_directory][direction].append(record)

for root_directory in root_directories:
    for direction in ["X","Y","Z"]:
        samples[root_directory][direction] = ",".join(samples[root_directory][direction])
        samples[root_directory][direction] = list(map(float, samples[root_directory][direction].split(",")))
        samples[root_directory][direction] = np.array(samples[root_directory][direction])

# Create dataframes and save to file
dataframes = {}
for root_directory in root_directories:
    data = {"x":samples[root_directory]["X"],
            "y":samples[root_directory]["Y"],
            "z":samples[root_directory]["Z"]}
    dataframe = pd.DataFrame(data)
    dataframes[root_directory] = dataframe
    dataframe.to_csv(root_directory+".csv", header=True, index=False)
    logging.info("Saved {}.csv".format(root_directory))
