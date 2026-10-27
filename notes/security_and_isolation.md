# Credentials, capabilities, and Linux isolation

Companion: [lab15](../labs/lab15/README.md). These are mechanism exercises,
not a complete container runtime or an untrusted-code sandbox.

| Mechanism | Main question it answers |
|---|---|
| Real/effective/saved IDs and groups | Which identity participates in authorization and transitions? |
| File modes, ACLs, ownership | Who can access this filesystem object? |
| Capabilities | Which individual privileged operations are available? |
| `no_new_privs` | Can exec grant new privilege through mechanisms such as setuid/file capabilities? |
| Seccomp | Which syscall requests can this task make? |
| Namespaces | Which view/instance of a kernel resource does the task see? |
| Cgroups | Which resource-accounting and limit group contains the task? |
| Resource limits | Which per-process resource ceilings apply? |

## Observe before changing

`inspect` reads IDs, supplementary groups, and `/proc/self/status` capability
masks. Effective, permitted, inheritable, bounding, and ambient capability
sets have different roles. UID zero is not an explanation of every access
decision. A user namespace changes the interpretation of IDs and capability
scope; namespace root is not unrestricted root in the initial user namespace.
See [capabilities(7)](https://man7.org/linux/man-pages/man7/capabilities.7.html)
and [user_namespaces(7)](https://man7.org/linux/man-pages/man7/user_namespaces.7.html).

Credential dropping for a real privileged service requires a deliberate group,
UID/GID, capability, and exec policy with checked results. Lab15 starts
unprivileged and drops a disposable child's capability sets; it does not
demonstrate installing a setuid binary or changing host users/groups.

## Restrict a child, preserve the parent

The limits mode changes only the child's soft descriptor ceiling and observes
`EMFILE`. The sandbox mode sets no-new-privileges, clears capability sets, and
adds a seccomp filter. That filter checks the syscall architecture, rejects
x32 on x86-64, and denies one raw `getppid` request with `EPERM`. Most calls
remain allowed: this tiny deny rule is for learning filter execution, not a
defense against an adversarial program. Real policies require workload-specific
allow rules and careful handling of alternate ABIs and arguments. See
[seccomp(2)](https://man7.org/linux/man-pages/man2/seccomp.2.html).

The namespace mode creates user and UTS namespaces in the child, writes its
UID/GID mappings, disables setgroups there, and changes only its isolated
hostname. It leaves mounts and network configuration alone. Kernel and LSM
policy may prohibit creation; a skip is an honest host constraint.

## Delegation is required for cgroups

Reading `/proc/self/cgroup` does not need ownership of a hierarchy. Creating
and moving tasks requires a suitably delegated cgroup v2 subtree, including
enabled controllers and migration permission. Lab15 never writes a parent's
`cgroup.subtree_control`, changes a mount, or moves unrelated processes.
Given an explicit delegated directory, it creates one owned subgroup, writes
`pids.max`, writes `memory.max` if available, gates/moves its own child, waits
for exit, and removes its subgroup. `pids.max` counts kernel tasks, including
threads, rather than only process IDs. Controller availability and hierarchical
limits still apply. Follow the kernel's
[cgroup v2 delegation documentation](https://docs.kernel.org/admin-guide/cgroup-v2.html).

Tests change no global policy. Restricted/missing optional mechanisms report
exit 77 with a reason. Do not treat a namespace, cgroup, or successful syscall
filter installation as proof of a complete isolation boundary.
