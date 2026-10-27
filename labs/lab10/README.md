# Lab 10: Filesystems and durable replacement

Explore directory descriptors, metadata, hard links, symlinks, permissions,
and the difference between atomic visibility and persistence. See
[filesystem notes](../../notes/file_io_and_filesystems.md).

```sh
make build/lab10
./build/lab10 list labs/lab10
printf 'old\n' > /tmp/lab10-target
printf 'new\n' > /tmp/lab10-contents
./build/lab10 replace /tmp/lab10-target /tmp/lab10-contents
```

`list` uses `fdopendir`, `readdir`, `fstatat(AT_SYMLINK_NOFOLLOW)`, and
`openat(O_NOFOLLOW)` for each directory descent. It prints type, mode, link
count, size, and relative name. Enumeration order is unspecified. Symlinks
are listed without recursion; depth greater than 64 reports an error.

`replace` creates an exclusive temporary file in the target's directory,
copies all bytes, syncs and closes the file, renames it onto the target, then
syncs the directory. A fresh private `0600` file replaces the old inode; old
permissions, ownership, ACLs, and extended attributes are not copied. Other
hard links still name the previous inode. Existing nonregular targets are
rejected. Temporary names belong to this process and are removed on errors
before rename.

**Current pattern:** same-directory replacement and explicit sync boundaries.
**Fragile recipe:** truncate the live file first, or call `rename` and claim
that power-loss persistence is established. A directory-sync failure after
rename reports that the new file is visible but durability is unconfirmed.

Use a directory controlled by you. Descriptor-relative traversal reduces
pathname races, but this is not a security boundary for a directory tree an
adversary can concurrently replace. Test hard links and symlinks, then use
`strace` to locate the two syncs. Tests inject write and sync failures;
successful calls alone cannot simulate a physical power-loss test.
