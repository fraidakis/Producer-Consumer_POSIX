## Producers and Consumers Are Not CPU-Bound

- If your threads were purely CPU-bound (e.g., matrix multiplication), performance would drop after ~12 threads.
- But in a producer-consumer model, there are many synchronization and waiting operations (mutex locks, condition variables, etc.).
- More threads can help keep the queue busy while others wait.
