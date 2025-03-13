#!/bin/bash

# Experiment script for producer-consumer testing
# This script runs the program with different numbers of consumer threads
# and records the average wait time

# Compile the program
gcc -o producer_consumer ./../prod-cons.c -lpthread -lm

# Define the number of producers
PRODUCERS=2

# Define the range of consumers to test
MIN_CONSUMERS=2
MAX_CONSUMERS=100

# Number of runs per configuration to ensure stable results
RUNS_PER_CONFIG=1

# Output file
OUTPUT_FILE="wait_time_results.txt"

echo "# Consumers, Average Wait Time (microseconds)" > $OUTPUT_FILE

# Run the experiments
# for CONSUMERS in $(seq $MIN_CONSUMERS 2 $MAX_CONSUMERS); do
for CONSUMERS in 1 10 100 1000 10000; do

    echo "Testing with $PRODUCERS producers and $CONSUMERS consumers..."
    
    total_avg=0
    wait_times=()
    
    for run in $(seq 1 $RUNS_PER_CONFIG); do        
        # Run the program, let it run for a few seconds, then kill it
        ./producer_consumer $PRODUCERS $CONSUMERS > temp_output.txt

        sleep 0.1
                
        # Extract the average wait time from the output
        avg_wait=$(grep "Average wait time:" temp_output.txt | awk '{print $4}')
        elap_wait=$(grep "Elapsed time:" temp_output.txt | awk '{print $3}')

        echo "  Run $run of $RUNS_PER_CONFIG - $avg_wait us"

        
        # Add to total
        total_avg=$(echo "$total_avg + $avg_wait" | bc)
        total_elap=$(echo "$total_elap + $elap_wait" | bc)
        wait_times+=($avg_wait)

    done
    
    # Calculate the average across all runs
    final_avg=$(echo "scale=2; $total_avg / $RUNS_PER_CONFIG" | bc)
    final_elap=$(echo "scale=2; $total_elap / $RUNS_PER_CONFIG" | bc)

    # Calculate the standard deviation
    sum_sq_diff=0
    for wait_time in "${wait_times[@]}"; do
        diff=$(echo "$wait_time - $final_avg" | bc)
        sq_diff=$(echo "$diff * $diff" | bc)
        sum_sq_diff=$(echo "$sum_sq_diff + $sq_diff" | bc)
    done
    variance=$(echo "scale=2; $sum_sq_diff / $RUNS_PER_CONFIG" | bc)
    stddev=$(echo "scale=2; sqrt($variance)" | bc)



    # Write to the output file
    echo "$CONSUMERS, $final_avg" >> $OUTPUT_FILE
    
    echo "  Average wait time: $final_avg microseconds, Elapsed time: $final_elap seconds, Standard deviation: $stddev"
done

echo "Experiments completed. Results saved to $OUTPUT_FILE"

# Create a simple plot using gnuplot
echo "Creating plot..."
echo "
set terminal png
set output 'wait_time_plot.png'
set title 'Average Wait Time vs Number of Consumer Threads'
set xlabel 'Number of Consumer Threads'
set ylabel 'Average Wait Time (microseconds)'
set grid
plot '$OUTPUT_FILE' using 1:2 with linespoints title 'Wait Time'
" > plot.gnu

gnuplot plot.gnu

echo "Plot saved as wait_time_plot.png"

rm temp_output.txt plot.gnu producer_consumer