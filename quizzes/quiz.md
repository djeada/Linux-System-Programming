
**Flashcard 1**  
**Q:** What is one fundamental difference between the `signal()` and `sigaction()` system calls?  
**A:** `sigaction()` allows blocking of specific signals while a signal handler is executing, whereas `signal()` typically does not block other signals during handler execution.

---

**Flashcard 2**  
**Q:** What is the purpose of `SIGKILL` even though it cannot be intercepted?  
**A:** `SIGKILL` is used as a last-resort signal by operating systems—often during shutdown—to forcefully terminate processes that do not exit gracefully, ensuring a prompt system shutdown.

---

**Flashcard 3**  
**Q:** How does the shell identify if a command executed successfully?  
**A:** In the lab implementation, the shell uses an `if` statement around `execvp()`. If `execvp()` produces output (indicating an error), the command did not execute successfully, so an error message is printed and the user is reprompted.

---

**Flashcard 4**  
**Q:** Under what circumstances will `exec()` return to the calling process?  
**A:** `exec()` returns to the calling process only if an error occurs during its execution. In such cases, `errno` is set with the relevant error information.

---

**Flashcard 5**  
**Q:** What are the critical sections of code in the lab problem?  
**A:** Critical sections include:
- Checking the number of people in the room.
- Verifying whether the barber or cashier is available.
- Inspecting the status of the mutex.
  
Each of these areas requires careful synchronization to avoid race conditions.

---

**Flashcard 6**  
**Q:** How should the system be modified if there is more than one barber?  
**A:** Replace the binary semaphore used for the barber with a counting semaphore, setting its value to the number of available barbers. When a customer arrives, the system must check each barber's availability before seating the customer in the waiting area.

---

**Flashcard 7**  
**Q:** What critical components of shared resources are involved in this lab?  
**A:** The lab's critical components include:
- Two portions of shared memory used among three processes.
- All semaphores that control access to these shared memory regions, which themselves are shared across processes.
  
Proper synchronization of these shared resources is essential.

---

**Flashcard 8**  
**Q:** What potential race conditions are present in the lab scenario?  
**A:** Race conditions can occur when:
- Checking the number of people in the room.
- Verifying the availability of the barber or cashier.
- Inspecting or modifying the mutex state.  
Each of these actions must be protected to avoid inconsistent or incorrect behavior.

---

**Flashcard 9**  
**Q:** How do semaphores help manage shared memory access among processes?  
**A:** Semaphores control access by allowing only one process (or a limited number in the case of counting semaphores) to enter critical sections at a time. This prevents simultaneous modifications that could lead to data corruption or race conditions.

---

**Flashcard 10**  
**Q:** What role does `errno` play when `exec()` returns unexpectedly?  
**A:** When `exec()` fails and returns, `errno` is set with an error code that provides information on why the execution failed. This allows the calling process to diagnose the error, print an appropriate error message, and potentially prompt the user for a new command.

---

**Flashcard 11**  
**Q:** What changes are required in the semaphore mechanism when the number of barbers increases?  
**A:** If there are multiple barbers:
- Replace the binary semaphore (which indicates only one barber available) with a counting semaphore.
- Initialize the counting semaphore with the number of available barbers.
- Ensure that when a customer arrives, the system checks for availability among all barbers before seating the customer.

---

**Flashcard 12**  
**Q:** Why might developers prefer `sigaction()` over `signal()` in robust applications?  
**A:** Developers prefer `sigaction()` because it:
- Allows fine-grained control over signal handling.
- Provides the ability to block certain signals while a handler is executing.
- Offers additional flags and options for more predictable behavior, making it a more reliable choice for complex applications.

---

**Flashcard 13**  
**Q:** What are some challenges associated with sharing memory and semaphores across multiple processes?  
**A:** Challenges include:
- Ensuring data consistency across concurrent accesses.
- Preventing race conditions and deadlocks.
- Coordinating proper synchronization to avoid resource starvation.
- Handling errors and edge cases in a multi-process environment.

---

**Flashcard 14**  
**Q:** What is the difference between a binary semaphore and a counting semaphore?  
**A:** A binary semaphore has only two states (locked or unlocked) and is used for mutual exclusion, while a counting semaphore can hold a range of values and is used to manage a set number of resources (e.g., multiple barbers).

---

**Flashcard 15**  
**Q:** Why is shared memory beneficial in multi-process systems?  
**A:** Shared memory allows processes to access common data directly in memory, enabling fast and efficient inter-process communication without the overhead of message passing.

---

**Flashcard 16**  
**Q:** What issues might arise if semaphores are not used correctly in concurrent systems?  
**A:** Incorrect semaphore use can lead to race conditions, deadlocks, or resource starvation, all of which can compromise the system’s stability and data integrity.

---

**Flashcard 17**  
**Q:** How does `execvp()` differ from other `exec()` family functions?  
**A:** `execvp()` searches for the executable in the directories specified by the PATH environment variable and accepts an array of arguments, simplifying command execution in shells compared to some other `exec()` variants.

---

**Flashcard 18**  
**Q:** What does blocking a signal in a signal handler achieve?  
**A:** Blocking a signal ensures that specific signals are not delivered to the process while the handler is executing, preventing interference and potential nested invocations of the same or related handlers.

---

**Flashcard 19**  
**Q:** What potential pitfalls exist when using signals for process control?  
**A:** Signals can be asynchronous, making it difficult to predict when they will occur, and if not handled carefully, they can cause race conditions, inconsistent state, or unexpected behavior in the process.

---

**Flashcard 20**  
**Q:** How is a critical section defined and why is it important?  
**A:** A critical section is a portion of code where shared resources are accessed. It is important because it must be executed atomically to prevent race conditions and maintain data integrity.

---

**Flashcard 21**  
**Q:** What role does a mutex play in the lab scenario?  
**A:** The mutex is used to control access to shared data (e.g., the room occupancy counter), ensuring that only one process modifies the shared resource at a time and thereby preventing race conditions.

---

**Flashcard 22**  
**Q:** Why is SIGKILL's inability to be intercepted significant?  
**A:** Since SIGKILL cannot be caught or ignored, it guarantees that the process receiving it will terminate immediately, which is critical during system shutdowns or when a process becomes unresponsive.

---

**Flashcard 23**  
**Q:** How do error codes set in `errno` assist developers?  
**A:** `errno` provides standardized error codes that help developers diagnose why a system call (like `exec()`) failed, enabling more precise error handling and debugging.

---

**Flashcard 24**  
**Q:** What impact does a failed `exec()` call have on process flow?  
**A:** If `exec()` fails, control returns to the calling process, allowing the program to detect the error, log an appropriate message, and take corrective action (such as reprompting for input).

---

**Flashcard 25**  
**Q:** What are the consequences of not using semaphores in a multi-process system?  
**A:** Without semaphores, concurrent processes might simultaneously access and modify shared resources, leading to race conditions, data corruption, inconsistent state, and unpredictable system behavior.

---

**Flashcard 26**  
**Q:** How does the use of shared memory improve inter-process communication efficiency?  
**A:** Shared memory allows processes to read and write directly to a common memory area, significantly reducing the latency and overhead compared to other IPC methods like sockets or pipes.

---

**Flashcard 27**  
**Q:** What is a race condition and how can it be prevented?  
**A:** A race condition occurs when multiple processes access and manipulate shared data concurrently, leading to unpredictable outcomes. It can be prevented by using synchronization mechanisms such as semaphores, mutexes, or critical sections.

---

**Flashcard 28**  
**Q:** How do you ensure that a counting semaphore accurately reflects available resources?  
**A:** By initializing the counting semaphore with the total number of available resources and carefully incrementing or decrementing its value as processes acquire or release resources, ensuring that it remains synchronized with the actual resource count.

---

**Flashcard 29**  
**Q:** What is the significance of using additional flags with `sigaction()`?  
**A:** Additional flags (such as `SA_RESTART` or `SA_NOCLDWAIT`) provide greater control over signal handling behavior, allowing developers to tailor signal processing to the application's needs for more robust and predictable execution.

---

**Flashcard 30**  
**Q:** Under what circumstances might you choose to use SIGKILL over other signals?  
**A:** SIGKILL is used when a process must be terminated immediately, such as when it becomes unresponsive or when a graceful shutdown fails, because it cannot be intercepted or ignored by the process.

---

**Flashcard 31**  
**Q:** How might a shell handle a command error aside from checking `execvp()` output?  
**A:** A shell can inspect the exit status of a process using system calls like `waitpid()` after a fork, checking for non-zero exit codes to determine if an error occurred, and then handling it appropriately.

---

**Flashcard 32**  
**Q:** What are some strategies to debug issues with shared memory in multi-process applications?  
**A:** Strategies include:
- Using logging or debugging tools to trace access to shared memory.
- Employing synchronization primitives to track concurrent access.
- Running tests to simulate heavy concurrent access scenarios.
- Utilizing memory-checking tools (e.g., Valgrind) to detect improper access or leaks.

---

**Flashcard 33**  
**Q:** What is the role of inter-process communication (IPC) in coordinating actions between multiple processes?  
**A:** IPC mechanisms (such as shared memory, pipes, message queues, and semaphores) facilitate the exchange of data and synchronization between processes, ensuring coordinated behavior and efficient resource sharing in a multi-process environment.

---

**Flashcard 34**  
**Q:** What is the significance of blocking signals while executing a signal handler?  
**A:** Blocking signals prevents the handler from being reentered before it has finished executing, which helps avoid race conditions and inconsistent program states.

---

**Flashcard 35**  
**Q:** How do you ensure that signal handlers do not interfere with each other?  
**A:** By using proper signal masks with `sigaction()`, you can block specific signals during the execution of a handler, ensuring that signals are processed sequentially rather than concurrently.

---

**Flashcard 36**  
**Q:** How does `execvp()` handle command-line arguments?  
**A:** `execvp()` accepts an array of null-terminated strings where the first element is the command and the subsequent elements are its arguments, allowing for flexible command execution.

---

**Flashcard 37**  
**Q:** Why is reprompting the user important after a failed command in a shell?  
**A:** Reprompting the user provides immediate feedback about the error and allows the user to enter a corrected or different command, improving the shell’s usability.

---

**Flashcard 38**  
**Q:** What is the role of `waitpid()` in process management?  
**A:** `waitpid()` allows a parent process to wait for a child process to change state (e.g., termination), enabling the parent to retrieve the child’s exit status and perform necessary cleanup.

---

**Flashcard 39**  
**Q:** When is a counting semaphore more appropriate than a mutex?  
**A:** A counting semaphore is ideal when managing multiple identical resources (like several barbers or shared memory slots), as it can track the availability of a specified number of resources, unlike a mutex which only allows exclusive access.

---

**Flashcard 40**  
**Q:** How can a shell detect if `execvp()` fails?  
**A:** Since a successful `execvp()` call replaces the process image and does not return, any return from `execvp()` indicates an error; the shell can then inspect `errno` for the error details.

---

**Flashcard 41**  
**Q:** What problems can arise from asynchronous signals if not properly managed?  
**A:** Asynchronous signals can lead to unpredictable interruptions, potentially causing race conditions, corrupted data, or inconsistent program states if the handlers are not designed to manage concurrent access.

---

**Flashcard 42**  
**Q:** What is a deadlock in a multi-process system, and how might it occur?  
**A:** A deadlock occurs when processes wait indefinitely for each other to release resources. It can happen if processes acquire locks in an inconsistent order, leading to a circular wait.

---

**Flashcard 43**  
**Q:** What strategies can help prevent deadlocks?  
**A:** Strategies include enforcing a strict resource acquisition order, using timeout mechanisms, employing deadlock detection algorithms, and avoiding circular wait conditions.

---

**Flashcard 44**  
**Q:** What is inter-process communication (IPC) and why is it necessary?  
**A:** IPC encompasses the mechanisms (such as shared memory, pipes, and message queues) that allow processes to communicate and share data, which is crucial for coordination in multi-process applications.

---

**Flashcard 45**  
**Q:** How do semaphores differ from mutexes in resource management?  
**A:** Mutexes provide exclusive access to a resource (one process at a time), while semaphores (especially counting semaphores) can allow a specified number of processes to access a resource concurrently.

---

**Flashcard 46**  
**Q:** How can developers ensure error messages are useful for debugging?  
**A:** By providing clear, descriptive messages that indicate the error type, context, and possible corrective actions, developers can facilitate faster diagnosis and resolution of issues.

---

**Flashcard 47**  
**Q:** What is the primary purpose of using shared memory in multi-process systems?  
**A:** Shared memory allows processes to access common data directly, significantly reducing communication overhead compared to other IPC methods, thus improving performance.

---

**Flashcard 48**  
**Q:** How can shared memory be used safely among multiple processes?  
**A:** Safety is ensured by combining shared memory with synchronization mechanisms (such as semaphores or mutexes) to control access and prevent concurrent modifications that might lead to data corruption.

---

**Flashcard 49**  
**Q:** What is the benefit of the PATH environment variable when executing commands?  
**A:** The PATH environment variable lets programs like `execvp()` locate executables without needing full paths, streamlining command execution and enhancing user convenience.

---

**Flashcard 50**  
**Q:** Why is it critical to check for errors after system calls like `execvp()` or `fork()`?  
**A:** Checking for errors ensures that any issues are detected early, allowing the program to handle them gracefully rather than crashing or proceeding in an unpredictable state.

---

**Flashcard 51**  
**Q:** What role do exit statuses play in process management?  
**A:** Exit statuses inform the parent process whether a child process terminated successfully or encountered errors, enabling proper cleanup, error handling, or subsequent actions based on the outcome.

---

**Flashcard 52**  
**Q:** What happens to a process when `exec()` successfully replaces its image?  
**A:** On successful execution, the process image is replaced entirely by the new program, and the original code ceases to run; control does not return to the calling process.

---

**Flashcard 53**  
**Q:** How do signal handlers improve an application’s reliability?  
**A:** Signal handlers allow an application to catch and manage asynchronous events—such as errors or external interrupts—enabling it to clean up resources, log events, or restart operations as needed, thereby enhancing robustness.
