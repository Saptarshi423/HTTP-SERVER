# Thread Pool — Concurrent Request Handling

This document explains how the HTTP server handles multiple client connections at the same time.

---

## The Problem with a Single-Threaded Server

Before the thread pool was added, the server handled one client at a time:

```
accept() → handleClient() → accept() → handleClient() → ...
```

While `handleClient()` was running for one client, every other incoming connection sat waiting in the OS backlog. If the current request was slow (large body, slow disk write), every other client felt that delay. This is a sequential bottleneck.

---

## The Solution: Thread Pool

Instead of processing requests on the main thread, the server now keeps a fixed pool of **worker threads** running in the background. The main thread's only responsibility becomes accepting sockets and handing them off. The workers do all the actual processing — in parallel.

```
Main thread:   accept() → queue → accept() → queue → accept() → ...
                             ↓                   ↓
Worker threads:          handle()            handle()     (running concurrently)
```

---

## Pool Size

The number of worker threads is calculated once in the `HttpServer` constructor:

```cpp
THREAD_POOL_SIZE = static_cast<int>(std::max(4u, std::thread::hardware_concurrency()));
```

- `std::thread::hardware_concurrency()` returns the number of logical CPU cores on the machine.
- `std::max(4u, ...)` guarantees a floor of 4 threads even on low-core machines.
- The threads are spawned at the start of `HttpServer::start()` and live for the entire lifetime of the server.

---

## Key Data Structures

All of these are private members of `HttpServer`:

| Member | Type | Purpose |
|---|---|---|
| `workerThreads` | `std::vector<std::thread>` | Owns the pool threads |
| `clientQueue` | `std::queue<SOCKET>` | Sockets waiting to be picked up by a worker |
| `queueMutex` | `std::mutex` | Guards all reads and writes to `clientQueue` |
| `queueCV` | `std::condition_variable` | Puts idle workers to sleep and wakes them when work arrives |
| `running` | `std::atomic<bool>` | Shutdown flag, readable by all threads without a lock |

---

## Step-by-Step: What Happens When a Client Connects

### Step 1 — Client connects, main thread accepts

`HttpServer::start()` sits in a loop calling `accept()`, which blocks until a client connects. When one does, the OS returns a `SOCKET` handle for that client.

```cpp
SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&serverAddr, &addrLen);
```

### Step 2 — Socket is pushed onto the queue

The main thread locks `queueMutex`, pushes the socket onto `clientQueue`, releases the lock, then calls `notify_one()` to wake exactly one idle worker.

```cpp
{
    std::lock_guard<std::mutex> lock(queueMutex);
    clientQueue.push(clientSocket);
}
queueCV.notify_one();
```

The main thread immediately loops back to `accept()` — it does not wait for the request to be processed. This is why multiple clients can be accepted in rapid succession.

### Step 3 — A worker thread wakes up

Every worker thread runs `workerLoop()` in a loop. When idle, it blocks on the condition variable:

```cpp
std::unique_lock<std::mutex> lock(queueMutex);
queueCV.wait(lock, [this] {
    return !clientQueue.empty() || !running;
});
```

The lambda is the **predicate** — `wait()` will only return when it evaluates to `true`. This guards against spurious wakeups (the OS can wake a thread even without a `notify` call). The worker wakes only when:
- there is actually a socket in the queue, **or**
- the server is shutting down.

### Step 4 — Worker pops the socket and releases the lock

Once awake, the worker pops the socket from the front of the queue, then immediately releases `queueMutex`. The lock is held for the shortest possible time — only during the queue access, not during request handling.

```cpp
if (!running && clientQueue.empty()) return;   // exit on shutdown

SOCKET clientSocket = clientQueue.front();
clientQueue.pop();
// lock released here (unique_lock goes out of scope)
```

### Step 5 — Worker handles the request

With the lock released, the worker calls `handleClient()` which does everything on the stack — no shared mutable state:

```
recv()                  — reads up to 8192 bytes from the socket into a local buffer
HttpRequest::parse()    — splits the raw bytes into method, path, headers, and body
Router::handleRequest() — finds the matching route and calls its handler lambda
HttpResponse::build()   — serialises the response to an HTTP/1.1 string
sendResponse()          — loops over send() until all bytes are delivered
closesocket()           — closes the connection
```

Because all of this runs on the worker's own stack with no globals or shared objects involved (except `FileStorage`, handled separately below), multiple workers can run `handleClient()` simultaneously without any interference.

---

## Thread Safety of Shared State

Most components are naturally safe for concurrent use. The one exception required an explicit fix.

### Router — safe, no lock needed

Routes are registered in `main()` before `server.start()` is called. After that, no thread ever writes to the route list. All workers only read from a stable `std::vector` — concurrent reads with no concurrent writes are safe.

### HttpRequest / HttpResponse — safe, stack-local

Both objects are created fresh inside `handleClient()` on the worker's own stack. They have no connection to any other thread and require no synchronisation.

### FileStorage — mutex protected

`FileStorage` is the only object shared across worker threads. Two routes capture it by reference:

- `POST /data` calls `save()` — writes to `post_data.txt`
- `GET /data` calls `readAll()` — reads from `post_data.txt`

Without protection, two concurrent POST requests could interleave their writes and corrupt the file. To prevent this, a `std::mutex fileMutex` was added inside `FileStorage`, and both methods acquire it before touching the file:

```cpp
bool FileStorage::save(const std::string& data) {
    std::unique_lock<std::mutex> lock(fileMutex);  // blocks if another thread is in save/readAll
    std::ofstream file(filename, std::ios::app);
    // ... write ...
}

std::string FileStorage::readAll() {
    std::unique_lock<std::mutex> lock(fileMutex);  // blocks if another thread is in save/readAll
    std::ifstream file(filename);
    // ... read ...
}
```

The lock is held until the file is closed (when `lock` goes out of scope). All file access is fully serialised — only one thread touches the file at a time.

### running flag — std::atomic\<bool\>

The `running` flag is written by the main thread (in `stop()`) and read by all worker threads (inside the condition variable predicate). Workers read it without holding `queueMutex`, so it must be `std::atomic<bool>` to avoid a data race.

---

## Shutdown Sequence

Shutdown needs to happen in the right order to avoid deadlocks:

```
stop() called
  │
  ├─ running = false
  │
  ├─ queueCV.notify_all()        ← wakes all sleeping workers
  │                                 they see running==false, exit cleanly
  │
  ├─ closesocket(serverSocket)   ← causes accept() in start() to return INVALID_SOCKET
  │                                 start()'s while(running) loop exits
  │
  └─ WSACleanup()

start() loop exits
  │
  ├─ queueCV.notify_all()        ← second notify in case any worker is still waiting
  │
  └─ for each thread: t.join()   ← waits for all workers to finish
```

Workers that are mid-request when shutdown is called will finish serving that client before checking `running` and exiting. Sockets already in the queue are drained before workers exit — no connected client is dropped mid-response.

---

## Full Flow Diagram

```
Client A ──connects──► main thread: accept()
                              │
                              ▼
                        lock queueMutex
                        clientQueue.push(socketA)
                        unlock
                        notify_one()
                              │
                              ▼
                        loop back to accept()  ◄── Client B connects immediately

Client B ──connects──► accept() returns socketB
                              │
                              ▼
                        push socketB → notify_one()

                   Worker 1 (woken by notify)        Worker 2 (woken by notify)
                        │                                   │
                        ▼                                   ▼
                   pop socketA                         pop socketB
                        │                                   │
                        ▼                                   ▼
                  handleClient(A)                    handleClient(B)
                  (running in parallel)              (running in parallel)
                        │                                   │
                        ▼                                   ▼
                  closesocket(A)                     closesocket(B)
                        │                                   │
                        ▼                                   ▼
                  back to wait()                     back to wait()
```

---

## The Logic Behind Concurrent Request Serving

### The Core Idea

The server separates two concerns that used to be one:

- **Accepting** connections — done by the main thread only
- **Processing** connections — done by worker threads in parallel

This means the main thread is never blocked waiting for a request to finish. It just keeps accepting.

---

### How Workers Stay Ready Without Wasting CPU

When there are no clients, workers shouldn't be spinning in a busy loop burning CPU. They need to sleep and wake up instantly when work arrives. This is done with a **condition variable**.

```cpp
queueCV.wait(lock, [this] {
    return !clientQueue.empty() || !running;
});
```

A condition variable is a signalling mechanism. A sleeping worker tells the OS *"wake me up when this condition becomes true"*. Until then it consumes zero CPU. When the main thread pushes a socket and calls `notify_one()`, the OS wakes exactly one worker — no more.

The predicate lambda `!clientQueue.empty() || !running` has two purposes:
- `!clientQueue.empty()` — there is actual work to do
- `!running` — the server is shutting down, wake up so you can exit

The predicate also guards against **spurious wakeups** — the OS can occasionally wake a thread for no reason. Without it, the worker might pop from an empty queue and crash. `wait()` re-checks the predicate and goes back to sleep if it is still false.

---

### Why the Lock Is Released Before Handling

```cpp
{
    std::unique_lock<std::mutex> lock(queueMutex);
    queueCV.wait(lock, ...);        // sleeps here, lock released while sleeping
    clientSocket = clientQueue.front();
    clientQueue.pop();
}                                   // lock released here
handleClient(clientSocket);         // runs without any lock held
```

The lock is only held while touching the queue — not during the actual request handling. If the lock were held through `handleClient()`, only one worker could ever process a request at a time, completely defeating the purpose of the thread pool.

---

### What Makes handleClient() Safe to Run in Parallel

Every variable inside `handleClient()` lives on the **stack** of the calling worker thread:

```
Worker 1 stack          Worker 2 stack
─────────────────       ─────────────────
char buffer[8192]       char buffer[8192]
HttpRequest req         HttpRequest req
HttpResponse res        HttpResponse res
```

Each worker has its own independent copy of everything. They do not share memory, so they cannot interfere with each other. This is the key property that makes the parallelism safe — stateless, stack-local processing.

---

### The One Shared Object: FileStorage

`FileStorage` is the only object that multiple workers genuinely share, because it was created once in `main()` and captured by reference in two route lambdas:

```cpp
FileStorage storage("post_data.txt");

router.addRoute("POST", "/data", [&storage](...) { storage.save(...); });
router.addRoute("GET",  "/data", [&storage](...) { storage.readAll(); });
```

Without protection, Worker 1 could be halfway through writing to the file while Worker 2 starts reading it — producing garbled output. The fix is a mutex inside `FileStorage`:

```cpp
bool FileStorage::save(const std::string& data) {
    std::unique_lock<std::mutex> lock(fileMutex); // Worker 2 blocks here if Worker 1 is inside
    // ... file write ...
}
```

Only one thread can be inside `save()` or `readAll()` at any moment. All others wait at the lock.

---

### How Multiple Clients Are Served at Once

Say 4 clients connect almost simultaneously:

```
t=0ms  Client A connects → main thread pushes socketA → notifies Worker 1
t=1ms  Client B connects → main thread pushes socketB → notifies Worker 2
t=1ms  Client C connects → main thread pushes socketC → notifies Worker 3
t=2ms  Client D connects → main thread pushes socketD → notifies Worker 4
```

All 4 workers are now running `handleClient()` in parallel. The main thread has already looped back to `accept()` ready for a 5th client. None of them wait for each other (unless they both hit `FileStorage`, in which case one waits briefly for the other to finish the file operation).

---

## Summary

| Concern | How it is handled |
|---|---|
| Multiple clients at once | Thread pool — N workers handle requests in parallel |
| Worker coordination | `std::mutex` + `std::condition_variable` on a shared `std::queue<SOCKET>` |
| Idle worker efficiency | Workers sleep on `queueCV`; zero CPU usage when no clients are connected |
| Spurious wakeups | Predicate lambda re-checked on every wake — worker goes back to sleep if queue is still empty |
| Lock held minimally | Lock released before `handleClient()` so all workers run in parallel |
| Parallel request processing | Stack-local state in `handleClient()` — each worker has its own buffer, request, and response |
| Shared file access | `std::mutex fileMutex` inside `FileStorage` serialises all disk I/O |
| Shutdown safety | `running = false` + `notify_all()` before joining threads; in-flight requests complete normally |
