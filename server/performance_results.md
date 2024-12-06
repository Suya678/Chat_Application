
# Chat Server Performance Analysis

## System Specifications 
- **CPU**: Intel i5-12400 (12th Gen)
- **Physical Cores**: 6
- **Logical Cores**: 12 (2 threads per core)
- **Architecture**: x86_64 (32/64-bit)
- **OS**: Ubunutu Nobel


## Methodology
- Java test file was modified to simulate varying client loads
- Server configuration macros adjusted to accommodate different client counts, number of rooms and number of clients in a room.
- Each test scenario runs with different worker thread configurations:
 - Single worker thread
 - Two worker threads
 - Six worker threads (This one was skipped for 2000 clients)
- Each configuration tested 5 times times to obtain average performance
- Here each thread refers to worker thread seperated from the main thread, the main thread's only job is to accept a conneciton and hand the new connection's fd to a worker thread. So at a minumum, the program will have 2 threads in total(1 main and 1 worker).
- Tests were ran with logging of.

## Note
- Linux has a default fil descriptor limit. This was changed prior to the test - ulimit -99999
- Socket States: Disabled artificial delay between client disconnections by removing `Thread.sleep()` in test suite's `disconnect_clients()` function


## Performance Results (Time in milliseconds)

### 18,000 Clients (MAX_CLIENTS=18000, MAX_ROOMS=60, MAX_Clients=300)
| Threads | Average Time |
|---------|--------------|
| 1 thread | 66,453.2 |
| 2 threads | 69,630.0 |
| 6 threads | 69,241.0 |

### 6,000 Clients (MAX_CLIENTS=6000, MAX_ROOMS=60, MAX_Clients=100)
| Threads | Average Time |
|---------|--------------|
| 1 thread | 30,578.4 |
| 2 threads | 31,870.0 |
| 6 threads | 31,200.8 |

### 9,000 Clients (MAX_CLIENTS=9000, MAX_ROOMS=60, MAX_Clients=150)
| Threads | Average Time |
|---------|--------------|
| 1 thread | 36,516.4 |
| 2 threads | 40,076.0 |
| 6 threads | 38,589.0 |


### 2,000 Clients (MAX_CLIENTS=2000, MAX_ROOMS=50, MAX_Clients=40)
| Threads | Average Time |
|---------|--------------|
| 1 thread | 15,707.6 |
| 2 threads | 18,076.0 |

## Key Finding
Single worker thread consistently performs better across all client loads.

