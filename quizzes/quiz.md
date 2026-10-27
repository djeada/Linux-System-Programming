# C/POSIX and OS Concepts Quiz

Use the [modern C/POSIX guide](../notes/modern_c_patterns.md) with these questions. Current lab patterns, explicit API comparisons, and historical OS models are distinguished where they differ.

#### Flashcard 1

**Q:** Why prefer sigaction() over signal() for a configurable POSIX handler?

**A:** sigaction() specifies a handler mask and flags explicitly. signal() has fewer controls and historically varying semantics. Neither interface makes printf, allocation, or ordinary application-state changes safe in a handler; notify the main loop instead.

---

#### Flashcard 2

**Q:** What is the purpose of `SIGKILL` even though it cannot be intercepted?

**A:** `SIGKILL` is used as a last-resort signal by operating systems—often during shutdown—to forcefully terminate processes that do not exit gracefully, ensuring a prompt system shutdown.

---

#### Flashcard 3

**Q:** How does the default lab shell determine whether a command succeeded?

**A:** First check the error number returned by posix_spawnp(). If creation succeeds, waitpid() supplies the child status: inspect WIFEXITED/WEXITSTATUS or WIFSIGNALED. Output text is not a success or failure protocol.

---

#### Flashcard 4

**Q:** Under what circumstances will `exec()` return to the calling process?

**A:** `exec()` returns to the calling process only if an error occurs during its execution. In such cases, `errno` is set with the relevant error information.

---

#### Flashcard 5

**Q:** Which sleeping-barber updates belong in the critical section?

**A:** Checking waiting capacity, inserting/removing a customer in the FIFO, changing the waiting count, and accounting for remaining arrivals form shared state transitions. Protect the complete transition, not just a preliminary availability check.

---

#### Flashcard 6

**Q:** What must change when the barber example has several barbers?

**A:** Use multiple service workers sharing the protected waiting FIFO, retain customer-specific completion notifications, and ensure every idle barber can wake during shutdown. A counting semaphore alone does not associate a completed service with the correct customer.

---

#### Flashcard 7

**Q:** What critical components of shared resources are involved in this lab?

**A:** The lab's critical components include:

- Two portions of shared memory used among three processes.
- All semaphores that control access to these shared memory regions, which themselves are shared across processes.

Proper synchronization of these shared resources is essential.

---

#### Flashcard 8

**Q:** Which parking-garage transitions must be synchronized together?

**A:** Selecting a vehicle type, choosing FIFO waiters, reserving capacity, and posting their admission gates must follow one mutex-protected protocol. Counting a slot only after a waiter wakes lets another arrival claim capacity already promised to that waiter.

---

#### Flashcard 9

**Q:** Does a counting semaphore automatically make shared mutable data safe?

**A:** No. A count greater than one permits concurrent holders. Use a separate mutex for related counters or queue links. A process-shared POSIX semaphore must itself live in shared storage and be initialized for sharing between processes.

---

#### Flashcard 10

**Q:** How do fork/exec and posix_spawnp report launch errors differently?

**A:** A failed execvp() returns -1 and sets errno in the child; the comparison exits that child with _exit(127). posix_spawnp() returns an error number directly. After successful creation, inspect the eventual child status as well.

---

#### Flashcard 11

**Q:** Why use a completion semaphore per customer rather than one shared done semaphore?

**A:** A shared completion count can wake any waiter, including the wrong customer. A per-customer gate associates the notification with the exact service that finished. Extra barbers need a shared admission queue and a shutdown protocol as well.

---

#### Flashcard 12

**Q:** Why might developers prefer `sigaction()` over `signal()` in robust applications?

**A:** Developers prefer `sigaction()` because it:

- Allows fine-grained control over signal handling.
- Provides the ability to block certain signals while a handler is executing.
- Offers additional flags and options for more predictable behavior, making it a more reliable choice for complex applications.

---

#### Flashcard 13

**Q:** What are some challenges associated with sharing memory and semaphores across multiple processes?

**A:** Challenges include:

- Ensuring data consistency across concurrent accesses.
- Preventing race conditions and deadlocks.
- Coordinating proper synchronization to avoid resource starvation.
- Handling errors and edge cases in a multi-process environment.

---

#### Flashcard 14

**Q:** How do a binary-semaphore protocol and a mutex differ?

**A:** A binary protocol balances a semaphore around values zero and one; a mutex has an owning thread that must unlock it. A semaphore can be posted by another participant and can also represent several permits. Balanced posting is an application invariant.

---

#### Flashcard 15

**Q:** When can shared memory help IPC performance?

**A:** A shared mapping can avoid explicit message copies for large or frequently accessed data. It still needs synchronization, correct lifetime, and measurement; contention and coordination costs can outweigh copying savings.

---

#### Flashcard 16

**Q:** What issues might arise if semaphores are not used correctly in concurrent systems?

**A:** Incorrect semaphore use can lead to race conditions, deadlocks, or resource starvation, all of which can compromise the system’s stability and data integrity.

---

#### Flashcard 17

**Q:** How does `execvp()` differ from other `exec()` family functions?

**A:** `execvp()` searches for the executable in the directories specified by the PATH environment variable and accepts an array of arguments, simplifying command execution in shells compared to some other `exec()` variants.

---

#### Flashcard 18

**Q:** What does a signal mask do in a multithreaded process?

**A:** It controls delivery to the calling thread. A blocked signal can remain pending, and other eligible threads may still receive process-directed signals. A handler mask can restrict nested delivery in that thread; it is not a process-wide lock.

---

#### Flashcard 19

**Q:** What potential pitfalls exist when using signals for process control?

**A:** Signals can be asynchronous, making it difficult to predict when they will occur, and if not handled carefully, they can cause race conditions, inconsistent state, or unexpected behavior in the process.

---

#### Flashcard 20

**Q:** Does a critical section have to be one atomic CPU instruction?

**A:** No. It can contain many operations. A correct lock protocol prevents conflicting participants from entering simultaneously and orders the accesses. Every participant must follow the same synchronization rule.

---

#### Flashcard 21

**Q:** What role does a mutex play in the lab scenario?

**A:** The mutex is used to control access to shared data (e.g., the room occupancy counter), ensuring that only one process modifies the shared resource at a time and thereby preventing race conditions.

---

#### Flashcard 22

**Q:** Does SIGKILL guarantee that cleanup completes immediately?

**A:** No. SIGKILL cannot be caught, blocked, or ignored, but termination may be delayed while the task is in an uninterruptible kernel operation. Application cleanup handlers do not run.

---

#### Flashcard 23

**Q:** How do error codes set in `errno` assist developers?

**A:** `errno` provides standardized error codes that help developers diagnose why a system call (like `exec()`) failed, enabling more precise error handling and debugging.

---

#### Flashcard 24

**Q:** What should the child do after a failed exec in the comparison shell?

**A:** Report the failure using a safe child path and call _exit with a failure status. Do not return into the parent shell loop or run inherited parent exit cleanup. The parent reaps the child and can continue reading commands.

---

#### Flashcard 25

**Q:** Are semaphores required for every multiprocess program?

**A:** No. Independent processes may share no mutable data; message passing or process-shared mutexes can also coordinate work. Shared mutable state needs a suitable protocol, not necessarily a semaphore.

---

#### Flashcard 26

**Q:** Does shared memory always outperform pipes or sockets?

**A:** No. It can reduce explicit copying, but lock contention, cache effects, record size, and coordination determine actual performance. Choose the simplest correct communication model and measure representative workloads.

---

#### Flashcard 27

**Q:** How does a C11 data race differ from a broader race condition?

**A:** A data race involves unsynchronized conflicting thread accesses to the same object, with at least one write, and has undefined behavior for ordinary non-atomic data. A broader race can be a timing-dependent logic error even when individual accesses are locked. Protect the whole invariant.

---

#### Flashcard 28

**Q:** How do you ensure that a counting semaphore accurately reflects available resources?

**A:** By initializing the counting semaphore with the total number of available resources and carefully incrementing or decrementing its value as processes acquire or release resources, ensuring that it remains synchronized with the actual resource count.

---

#### Flashcard 29

**Q:** What is the significance of using additional flags with `sigaction()`?

**A:** Additional flags (such as `SA_RESTART` or `SA_NOCLDWAIT`) provide greater control over signal handling behavior, allowing developers to tailor signal processing to the application's needs for more robust and predictable execution.

---

#### Flashcard 30

**Q:** When is SIGKILL appropriate compared with an orderly stop?

**A:** Use an orderly stop when the program can close intake, drain or cancel work, and join its participants. SIGKILL is a last resort when those mechanisms cannot complete; it does not provide application-level cleanup or a real-time completion guarantee.

---

#### Flashcard 31

**Q:** What should a parent inspect after successfully starting a program?

**A:** Use waitpid(), retry EINTR, then inspect WIFEXITED before WEXITSTATUS and handle WIFSIGNALED separately. A command may produce no output and succeed, or produce output and fail.

---

#### Flashcard 32

**Q:** What are some strategies to debug issues with shared memory in multi-process applications?

**A:** Strategies include:

- Using logging or debugging tools to trace access to shared memory.
- Employing synchronization primitives to track concurrent access.
- Running tests to simulate heavy concurrent access scenarios.
- Utilizing memory-checking tools (e.g., Valgrind) to detect improper access or leaks.

---

#### Flashcard 33

**Q:** What is the role of inter-process communication (IPC) in coordinating actions between multiple processes?

**A:** IPC mechanisms (such as shared memory, pipes, message queues, and semaphores) facilitate the exchange of data and synchronization between processes, ensuring coordinated behavior and efficient resource sharing in a multi-process environment.

---

#### Flashcard 34

**Q:** Does masking nested signals make printf safe in a handler?

**A:** No. A signal can interrupt a stdio operation already running in normal code, leaving library state mid-update. In lab1, the handler sets sig_atomic_t flags and writes to a self-pipe; the main loop performs output and maze updates.

---

#### Flashcard 35

**Q:** How does the maze avoid complex work in signal handlers?

**A:** Handlers preserve errno, record the request, and notify a nonblocking self-pipe. poll() wakes the main loop, which handles the request in ordinary control flow. The pipe also closes the check-then-wait race around notification.

---

#### Flashcard 36

**Q:** How is an argv vector terminated for execvp or posix_spawnp?

**A:** Each argument is a NUL-terminated string, argv[0] names the program, and the pointer array ends with a NULL pointer. The shell grows that array instead of silently truncating long commands.

---

#### Flashcard 37

**Q:** Why is reprompting the user important after a failed command in a shell?

**A:** Reprompting the user provides immediate feedback about the error and allows the user to enter a corrected or different command, improving the shell’s usability.

---

#### Flashcard 38

**Q:** What is the role of `waitpid()` in process management?

**A:** `waitpid()` allows a parent process to wait for a child process to change state (e.g., termination), enabling the parent to retrieve the child’s exit status and perform necessary cleanup.

---

#### Flashcard 39

**Q:** When is a counting semaphore more appropriate than a mutex?

**A:** A counting semaphore is ideal when managing multiple identical resources (like several barbers or shared memory slots), as it can track the availability of a specified number of resources, unlike a mutex which only allows exclusive access.

---

#### Flashcard 40

**Q:** How can the explicit fork/exec comparison detect execvp failure?

**A:** A successful execvp replaces the child program and does not return. Returning indicates failure and sets errno. This comparison is separate from the default shell, which uses posix_spawnp and checks its returned error number.

---

#### Flashcard 41

**Q:** What problems can arise from asynchronous signals if not properly managed?

**A:** Asynchronous signals can lead to unpredictable interruptions, potentially causing race conditions, corrupted data, or inconsistent program states if the handlers are not designed to manage concurrent access.

---

#### Flashcard 42

**Q:** What is a deadlock in a multi-process system, and how might it occur?

**A:** A deadlock occurs when processes wait indefinitely for each other to release resources. It can happen if processes acquire locks in an inconsistent order, leading to a circular wait.

---

#### Flashcard 43

**Q:** What strategies can help prevent deadlocks?

**A:** Strategies include enforcing a strict resource acquisition order, using timeout mechanisms, employing deadlock detection algorithms, and avoiding circular wait conditions.

---

#### Flashcard 44

**Q:** What is inter-process communication (IPC) and why is it necessary?

**A:** IPC encompasses the mechanisms (such as shared memory, pipes, and message queues) that allow processes to communicate and share data, which is crucial for coordination in multi-process applications.

---

#### Flashcard 45

**Q:** How do mutexes and semaphores differ across threads and processes?

**A:** A mutex protects a critical-section invariant and has an owner; a semaphore tracks permits or notifications without an owning thread. Ordinary pthread mutexes are process-private unless initialized for shared use in shared storage. The process labs use process-shared POSIX semaphores.

---

#### Flashcard 46

**Q:** How can developers ensure error messages are useful for debugging?

**A:** By providing clear, descriptive messages that indicate the error type, context, and possible corrective actions, developers can facilitate faster diagnosis and resolution of issues.

---

#### Flashcard 47

**Q:** What is the main design responsibility when using shared memory?

**A:** Define who owns the mapping, how all conflicting accesses synchronize, and when it may be released. The parent reaps every child before destroying process-shared synchronization and unmapping its storage. Sharing bytes alone does not establish that protocol.

---

#### Flashcard 48

**Q:** How does the garage safely use shared state?

**A:** All admission and departure transitions hold the state semaphore. Capacity is reserved before posting a selected vehicle gate, and type batches change only after all outstanding reservations have left. Shared memory is released after every vehicle is reaped.

---

#### Flashcard 49

**Q:** What is the benefit of the PATH environment variable when executing commands?

**A:** The PATH environment variable lets programs like `execvp()` locate executables without needing full paths, streamlining command execution and enhancing user convenience.

---

#### Flashcard 50

**Q:** What cleanup follows a partial process or thread startup failure?

**A:** Track only workers that were actually started. Stop and reap process peers, or close the work queue and join started threads. Destroy only synchronization objects whose initialization succeeded, after their last user has finished.

---

#### Flashcard 51

**Q:** What role do exit statuses play in process management?

**A:** Exit statuses inform the parent process whether a child process terminated successfully or encountered errors, enabling proper cleanup, error handling, or subsequent actions based on the outcome.

---

#### Flashcard 52

**Q:** What happens to a process when `exec()` successfully replaces its image?

**A:** On successful execution, the process image is replaced entirely by the new program, and the original code ceases to run; control does not return to the calling process.

---

#### Flashcard 53

**Q:** What work belongs in an asynchronous signal handler?

**A:** Keep the handler minimal: record a sig_atomic_t request and use an async-signal-safe notification such as write(). Normal code then performs logging, application cleanup, or state changes. General allocation, stdio, and exit() do not belong in this handler.

---

#### Flashcard 54

**Q:** Why does a work queue need a capacity limit?

**A:** If producers outpace consumers, an unbounded queue can keep allocating memory. A bounded queue blocks or otherwise rejects new work until capacity is available, making overload part of the protocol.

---

#### Flashcard 55

**Q:** What happens when the lab work queue is closed?

**A:** Blocked producers and consumers wake. New pushes are rejected, accepted items can still be popped, and consumers exit only when the closed queue is empty. The owner joins all users before destroying the queue.

---

#### Flashcard 56

**Q:** Why is perror often wrong after pthread_create fails?

**A:** Pthread functions return the error number directly and need not set errno. Pass that returned value to strerror; otherwise the message can incorrectly say Success.

---

#### Flashcard 57

**Q:** Which clock should an elapsed-time condition deadline use?

**A:** Use CLOCK_MONOTONIC and initialize the condition variable with the matching clock attribute. Recheck predicates after wakeups against the same absolute deadline rather than repeatedly extending it.

---

#### Flashcard 58

**Q:** Why can an empty restaurant queue still have unfinished orders?

**A:** A chef may have removed an order and still be preparing it. Main joins all chefs before allowing the final reporter snapshot, so completion includes work in progress.

---

#### Flashcard 59

**Q:** Why do SCAN and LOOK diagrams not determine an SSD scheduler choice?

**A:** They model mechanical head travel on spinning disks. Modern Linux storage uses a device-dependent multi-queue stack; inspect supported scheduler options and measure the actual workload.

---

#### Flashcard 60

**Q:** How are historical models separated from recommended C implementation patterns?

**A:** The pattern guide identifies current application choices and a separate fork/exec comparison. Notes label classical allocation, scheduling, and kernel mechanisms as models rather than userspace code templates.

---

#### Flashcard 61

**Q:** Does a successful write(fd, buffer, size) always consume size bytes?

**A:** No. A positive shorter result is valid. Advance by that count and write the remaining bytes; retry an interrupted operation when appropriate, and report other errors without forgetting earlier progress. Binary data uses explicit lengths.

---

#### Flashcard 62

**Q:** What offset do descriptors created by dup share, and how does pread differ?

**A:** Dup descriptors refer to the same open file description and share its current offset. Pread supplies an explicit offset without changing that current offset. Two independent opens normally have independent descriptions.

---

#### Flashcard 63

**Q:** Why check source/destination inode identity before truncating a copy destination?

**A:** Different path strings can name the same inode through hard links. Compare device and inode metadata on the opened descriptors so the copier cannot truncate its own source through an alias.

---

#### Flashcard 64

**Q:** Why is O_CLOEXEC preferable to setting FD_CLOEXEC sometime after opening?

**A:** It sets the inheritance boundary atomically with descriptor creation. A later fcntl leaves a window in which another thread could launch an executable that inherits the descriptor.

---

#### Flashcard 65

**Q:** Do hard links and symlinks refer to files in the same way?

**A:** Hard links are directory names for the same inode. A symlink stores a pathname resolved during lookup. Replacing one hard-linked name with a new inode leaves the other names referring to the previous inode.

---

#### Flashcard 66

**Q:** What does descriptor-relative traversal improve, and what does O_NOFOLLOW not guarantee?

**A:** It makes operations relative to an owned directory rather than repeatedly resolving a full path. O_NOFOLLOW applies to the final component of that open; it does not constrain all ancestors or freeze a tree against concurrent mutation.

---

#### Flashcard 67

**Q:** Which boundaries establish a whole-file replacement's visibility and requested persistence?

**A:** Write and sync an exclusive same-directory temporary file, close it, rename it onto the destination, then sync the directory. Rename is the visibility boundary; the file and directory syncs cover different persistence requirements.

---

#### Flashcard 68

**Q:** If directory fsync fails after rename, can the program claim the original file is still installed?

**A:** No. The replacement may already be visible. Report that visibility changed while durability is unconfirmed. Removing the temporary name cannot undo a rename that has already committed.

---

#### Flashcard 69

**Q:** Why does TCP need an application framing protocol?

**A:** TCP exposes an ordered byte stream, without preserving individual application write boundaries. A receiver must accumulate and validate a complete message using lengths, delimiters, or another explicit grammar.

---

#### Flashcard 70

**Q:** What does UDP preserve, and what does it not promise?

**A:** It preserves datagram boundaries, while delivery, ordering, and uniqueness are not guaranteed. Applications must define loss/retry behavior and detect/reject datagrams larger than their receive policy.

---

#### Flashcard 71

**Q:** Does readiness guarantee that a nonblocking operation will complete a whole frame?

**A:** No. Try the operation, keep partial progress, and return to waiting on EAGAIN. Readiness may change before the operation; EINTR, peer closure, and errors also need explicit handling.

---

#### Flashcard 72

**Q:** How does the epoll server bound memory and apply backpressure?

**A:** It limits active clients and keeps one bounded frame per client. It pauses reads while a response is pending and pauses accepts while the table is full. Socket buffers and timeouts carry pressure back to peers.

---

#### Flashcard 73

**Q:** Why register EPOLLOUT only while output is blocked?

**A:** A socket is often writable. Permanent writable interest can produce repeated notifications with no useful work. Attempt sends first; request writable events only for an unfinished send that would block.

---

#### Flashcard 74

**Q:** What extra obligation commonly comes with edge-triggered epoll?

**A:** Use nonblocking operations and drain the relevant state until EAGAIN so an already-present readiness condition is not left waiting for a new edge. Level-triggered behavior is the simpler starting point in lab12.

---

#### Flashcard 75

**Q:** Why can a complete request followed by a peer half-close still receive a response?

**A:** Closing the peer's sending direction does not close its receiving direction. The server should finish processing the complete request and sending its response instead of discarding it solely because read EOF or RDHUP occurred.

---

#### Flashcard 76

**Q:** What two pipe-lifetime mistakes commonly hang a shell pipeline?

**A:** Waiting for a producer before launching its consumer can fill the pipe. Keeping an unused writer open can prevent EOF. Launch all stages, close unused ends in every process, and reap every member.

---

#### Flashcard 77

**Q:** Why does terminal job control use a process group for each pipeline?

**A:** The terminal sends job-control signals to its foreground group. Group ownership lets Ctrl+C, Ctrl+Z, fg, and bg operate on the whole pipeline. The shell must reclaim terminal ownership/settings after the foreground job stops or exits.

---

#### Flashcard 78

**Q:** What race does the job-control shell's startup gate prevent?

**A:** Children wait until the parent has formed/registered the group and transferred the terminal. Otherwise a fast foreground child could read before its group owns the terminal and be stopped with SIGTTIN.

---

#### Flashcard 79

**Q:** When should a Type=notify service send READY=1?

**A:** After configuration and required resources are usable and normal work can begin. Merely creating a process is not readiness. Under the service manager, the example stays in the foreground and lets the manager track it.

---

#### Flashcard 80

**Q:** What makes a configuration reload preserve valid running state?

**A:** Parse and validate a separate candidate, then publish it only after validation succeeds. A rejected candidate leaves the active settings intact. SIGHUP requests reload; a signal-send succeeding is not proof that reload completed.

---

#### Flashcard 81

**Q:** How do namespaces, capabilities, seccomp, and cgroups differ?

**A:** Namespaces select resource views, capabilities divide privileged operations, seccomp filters syscall requests, and cgroups account for/limit resources. They complement one another; none alone establishes a complete sandbox.

---

#### Flashcard 82

**Q:** Why does the cgroup lab require an explicitly delegated directory?

**A:** Creating groups and migrating tasks requires permissions and available controllers in a delegated subtree. The lab creates one owned subgroup and moves only its child. It must not enable parent controllers, move unrelated tasks, or change host policy to pass a test.

---

#### Flashcard 83

**Q:** Is volatile sufficient to publish an ordinary payload to another thread?

**A:** No. The producer can write the payload then release-store a ready flag; an acquire load observing that value establishes the relationship before the consumer reads the payload. Volatile does not provide this synchronization.

---

#### Flashcard 84

**Q:** Why is relaxed ordering enough for lab17's independent counter?

**A:** Each increment only needs atomicity for the counter itself; it does not publish other data. Main joins workers before reading their ordinary results. This reasoning does not justify relaxed ordering for an unrelated publication protocol.

---

#### Flashcard 85

**Q:** What lifetime rule applies to a function or string pointer obtained from a plugin?

**A:** Keep the library loaded while using any of its pointers or objects. Validate its ABI, finish callbacks, release plugin-owned objects, then dlclose. Loading a trusted module is in-process execution, not sandboxing.

---

#### Flashcard 86

**Q:** What are MAP_PRIVATE and MAP_SHARED edit semantics, and what can truncation do?

**A:** Private edits are copy-on-write and are not saved to the backing file; shared edits affect the shared backing object. Neither prevents external truncation, which can make a later access raise SIGBUS. Advisory locks help only cooperating users.

---

#### Flashcard 87

**Q:** How do io_uring submission success and completion success differ?

**A:** Submission accepts a request; its CQE later reports the actual operation result, including negative errno or a short byte count. Retain buffers until completion, inspect the result, and consume the CQE. One request followed by a blocking wait does not demonstrate pipelined throughput.

---

#### Flashcard 88

**Q:** Which diagnostic would you start with for a leaked descriptor versus an invalid memory access?

**A:** Trace opens/closes and inspect /proc/PID/fd for descriptor ownership. Use a suitable sanitizer and debugger for invalid memory access. Match the tool to the question, account for instrumentation effects, and record host restrictions as explicit skips.
