# File descriptors, filesystems, and persistence

Companion labs: [9: descriptor I/O](../labs/lab9/README.md) and
[10: filesystem operations](../labs/lab10/README.md).

## Descriptors and open file descriptions

A file descriptor is a process-local integer referring to an open file
description. That description holds state such as the current offset and
file status flags. `dup` and inherited descriptors after `fork` can refer to
the same description; independent `open` calls create independent ones.
`FD_CLOEXEC` belongs to a descriptor. Use `O_CLOEXEC` at creation to avoid a
window where a concurrently executed program could inherit it.

`read` may return fewer bytes than requested; zero means EOF for a regular
file/stream read with a nonzero request size. `write` may consume only a
prefix. Move the buffer pointer by the actual count, retry `EINTR`, and treat
other failures according to the operation. A failed operation may follow
earlier successful partial progress. `pread`/`pwrite` take an explicit offset
without changing the description's current offset. Do not open a positional
writer with `O_APPEND` and assume offsets override append behavior on Linux.

On Linux, release a descriptor once with `close` and check its error; retrying
`close` can close a descriptor number another thread has already reused. A
successful close is not a data-persistence guarantee. See
[open(2)](https://man7.org/linux/man-pages/man2/open.2.html) and
[close(2)](https://man7.org/linux/man-pages/man2/close.2.html).

## Names, metadata, and permissions

A directory entry names an inode. Hard links are different names for the
same inode; unlinking one name need not remove the object while other links
or open references exist. A symlink contains a pathname interpreted during
lookup. `stat` normally follows it; `fstatat(..., AT_SYMLINK_NOFOLLOW)` observes
the link itself. Mode bits, ownership, umask, ACLs, and mount options jointly
affect access. Umask removes requested creation bits; it does not grant them.

Use `openat` relative to an owned directory descriptor when walking a tree.
Check the object actually opened rather than assuming an earlier pathname
check remains true. `O_NOFOLLOW` protects the final component of that open;
it does not constrain all ancestors or freeze a directory tree. Lab10 opens
each descent separately without following directory symlinks. For hostile
trees, study Linux `openat2` resolution restrictions and define the threat
model rather than treating this listing tool as confinement.

## Atomic visibility and crash persistence

For a whole-file update in a directory you control:

1. Create a new exclusive temporary file in that directory.
2. Write complete contents and check all errors.
3. `fsync` the temporary file and close it, checking both.
4. Rename that name onto the destination within the same filesystem.
5. `fsync` the directory containing the changed name.

The rename provides the visibility boundary; synchronization establishes the
requested persistence boundaries. Open readers can keep the previous inode.
If step 5 fails, the replacement may already be visible: report uncertain
durability rather than claiming the previous file is intact. Filesystem,
storage, and error behavior still matter. See
[fsync(2)](https://man7.org/linux/man-pages/man2/fsync.2.html) and
[rename(2)](https://man7.org/linux/man-pages/man2/rename.2.html).

Lab10 intentionally creates a fresh private inode, without copying the old
metadata. Multi-file transactions, concurrent writers, filesystem snapshots,
and database recovery need additional protocols. Truncating the live file and
writing in place is a useful simple copier behavior, but it has different
failure semantics from replacement.
