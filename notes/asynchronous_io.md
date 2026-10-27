# Asynchronous I/O and io_uring

Companion: [optional lab20](../labs/lab20/README.md).

## Readiness and completion

| Model | What the notification tells you | Example |
|---|---|---|
| Blocking I/O | The call returns after progress/error | Lab9 copy |
| Readiness | Try a nonblocking operation now | Lab12 epoll |
| Completion | A previously submitted operation has a result | Lab20 io_uring |

Linux io_uring exposes submission/completion queues shared between userspace
and the kernel. Liburing handles the queue mechanics; it does not own your
buffers, define the application protocol, or guarantee a particular throughput.
Read [io_uring(7)](https://man7.org/linux/man-pages/man7/io_uring.7.html).

## Own each submitted request until completion

Prepare an SQE, submit it, wait/retrieve its CQE, inspect the completion's
result, then mark the CQE consumed. Submission success means the request was
submitted, not that the I/O succeeded. Completion results use negative errno
values on errors; printing the current global `errno` can report the wrong
failure. A positive result may be short and require another operation.

The buffer and all request state must remain valid while the kernel may use
them. Reusing or freeing a buffer after submission but before completion is a
lifetime error. Multiple outstanding operations require distinct request IDs,
owned buffers, and handling of completion order. Cancellation also has a
completion/ownership protocol; requesting cancellation is not the same as
proving a buffer is unused. See
[io_uring_wait_cqe(3)](https://man7.org/linux/man-pages/man3/io_uring_wait_cqe.3.html).

## Start with a deliberately small example

Lab20 keeps one read or write outstanding and waits for its completion. It
teaches lifecycle without claiming concurrency or speed. Regular-file offset
management is explicit. The finished destination is synced; partial failure
can still leave partial contents. Build and test the copier before attempting
multi-buffer pipelining, batching, registered resources, or polling modes.

Library availability and kernel permission are separate prerequisites.
`make optional` reports a missing liburing development package. Kernel policy,
containers, or unsupported kernels can deny setup even with the library
installed; lab20 exits 77 with the setup reason before modifying a destination.
See [io_uring_setup(2)](https://man7.org/linux/man-pages/man2/io_uring_setup.2.html).
Keep host policy intact and record a skipped run accurately.
