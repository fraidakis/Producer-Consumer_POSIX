# Producer-Consumer Synchronization for RTES  

## Description  
This project implements an optimized producer-consumer synchronization mechanism using pthreads. The focus is on minimizing wait time, making it suitable for Real-Time Embedded Systems (RTES) applications.  

## Features  
- Multi-threaded producer-consumer model  
- Queue-based function execution  
- Mutex and condition variable synchronization  
- Wait time measurement and statistical analysis  

## Requirements  
- GCC Compiler  
- POSIX Threads (pthreads)  
- C Standard Library  

## Output  
The program outputs the average wait time for queue items, helping analyze the efficiency of different consumer thread counts.  
