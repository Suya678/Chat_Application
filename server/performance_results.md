# Chat Server Performance Analysis

## System Specifications

- **CPU**: Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz
- **Cores**: 6
- **Architecture**: x86_64
- **OS**: Ubuntu 22.04.4 LTS Jammy

## Methodology

- Java test file was modified to simulate varying client loads.
- Server configuration macros adjusted to accommodate different client counts, number of rooms and
  number of clients in a room.
- Each test scenario runs with different worker thread configurations:
  **`Single worker thread`**  
  **`Two worker threads`**  
  **`Four worker threads`**
- Each configuration was tested 6 times to obtain average time for the test to finish
- Here each thread refers to worker thread seperated from the main thread, the main thread's only
  job is to accept a connection and hand the new connection's fd to a worker thread. So at a
  minimum, the program will have 2 threads in total(1 main and 1 worker).
- Tests were ran with logging of.


## Note

- Linux has a default fil descriptor limit. This was changed prior to the test: ulimit -n 99999

---

## Performance Results (Time in milliseconds)

### 2000 Clients

### 2000 Clients (50 rooms, 40 clients per room)

| Test #  | 1 Thread time | 2 Threads time | 4 Threads time |
|---------|---------------|----------------|----------------|
| 1       | 5470          | 5375           | 5371           |
| 2       | 5470          | 5451           | 5403           |
| 3       | 5470          | 5179           | 5387           |
| 4       | 5470          | 5343           | 5278           |
| 5       | 5564          | 5317           | 5545           |
| 6       | 5556          | 5541           | 5267           |
| **AVG** | **5500**      | **5368**       | **5375**       |

### 6000 Clients (50 rooms, 120 clients per room)

| Test #  | 1 Thread time | 2 Threads time | 4 Threads time |
|---------|---------------|----------------|----------------|
| 1       | 15555         | 13701          | 13493          |
| 2       | 15201         | 13846          | 13382          |
| 3       | 18086         | 13968          | 13587          |
| 4       | 15363         | 13749          | 13254          |
| 5       | 15530         | 13999          | 13467          |
| 6       | 15491         | 13651          | 13768          |
| **AVG** | **15871**     | **13819**      | **13492**      |

### 12000 Clients (60 rooms, 200 clients per room)

| Test #  | 1 Thread time | 2 Threads time | 4 Threads time |
|---------|---------------|----------------|----------------|
| 1       | 32173         | 29563          | 29006          |
| 2       | 32008         | 29412          | 28701          |
| 3       | 32086         | 30011          | 29212          |
| 4       | 32034         | 29562          | 29033          |
| 5       | 32209         | 29202          | 29266          |
| 6       | 32539         | 29885          | 28811          |
| **AVG** | **32175**     | **29606**      | **29005**      |

### Total Averages

| Threads   | Average Time |
|-----------|--------------|
| 1 Thread  | **53546**    |
| 2 Threads | **48793**    |
| 4 Threads | **47872**    |

---

## Results

1. **Small Scale (2000 Clients)**

- Minimal performance difference between thread configurations
- 2 threads perform marginally better than 1 and 4

2. **Medium Scale (6000 Clients)**

- 2 threads perform ~13% better than a single worker thread
- 4 threads performs marginally better than 2 threads and ~15% better than a single worker thread

3. **Large Scale (12000 Clients)**

- 2 threads performed ~8% compared to single thread
- 4 threads performs marginally better than 2 threads and ~9.9% better than a single worker thread

### Overall

- Multi-threading benefits become more pronounced with larger client loads
- Total average time shows ~9% improvement with 2 threads and ~11% with 4 threads compared to single
  thread

---

## Test Limitations

- The Java test client performs operations synchronously, which may not reflect real-world usage
  patterns
- Asynchronous client behavior and differnt test cases could produce significantly different results
- Additional testing with concurrent client operations would provide more comprehensive
  performance data
- The sample size was small(6 per thread per configuration).
- The test was done on a single system, other systems may produce different results
