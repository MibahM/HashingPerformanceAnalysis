# Hashing Performance Analysis
An experiment was conducted to analyze the effects of varying thread counts on the processing times and speed-up rates while hashing files of different sizes.

**The script/program is written in C**

**Overview:**
This repository contains the code, data, and analysis for an experiment designed to study the impact of different thread counts on the processing times and speed-up achieved when hashing files of various sizes. The aim is to understand how efficiently multi-threading can be utilized to enhance hashing performance and to identify optimal thread configurations for different data volumes.


**Instructions:**
Download the repo files to your UNIX directory


**Compilation:**
Use this line to compile the file:

    gcc htree.c –o htree -Wall -Werror -std=gnu99 -pthread


**Usage:**
Running the script will output results to a file. To get results for a specific file output to the terminal, run the program with this usage.

    ./htree filename num_threads


**Running:**
3 Test Files are provided and a script to run the program
Test files:
- p2_tc0
- p2_tc1
- p2_tc2

The Script will out results to their respective files:
- hash_values_tc0
- hash_values_tc1
- hash_values_tc2

The script provided is: 'runp2'
Run with:  ./runp2





