# Lab 15: Credentials, restrictions, and isolation

Observe credentials, then apply restrictions to disposable child processes.
Read [security and isolation notes](../../notes/security_and_isolation.md).

```sh
make build/lab15
./build/lab15 inspect
./build/lab15 limits
./build/lab15 sandbox
./build/lab15 namespaces
./build/lab15 cgroup
```

| Mode | Behavior |
|---|---|
| `inspect` | Print real/effective IDs, supplementary groups, capability masks, seccomp state |
| `limits` | Lower a child's descriptor soft limit, reach `EMFILE`, close its descriptors |
| `sandbox` | Child sets no-new-privileges, drops capabilities, installs an architecture-checked seccomp filter that rejects `getppid` with `EPERM` |
| `namespaces` | Child creates user/UTS namespaces, maps the invoking UID/GID, changes its isolated hostname |
| `cgroup` | Read current membership without changing it |
| `cgroup DIRECTORY` | Create/remove an owned subgroup in an explicitly delegated cgroup v2 directory; limit tasks to 8 and memory to 32 MiB when its controller is available; move only the owned child |

The parent checks that its descriptor limit and hostname remain unchanged.
No global limits, sysctls, parent controllers, mounts, or host credentials are
changed. Namespace restrictions and missing cgroup delegation produce
`SKIP: reason` with exit status **77**. The seccomp example supports x86-64
and AArch64; other architectures also report a skip. For cgroup testing, set
`LAB_CGROUP_PARENT` only to a directory already delegated to your user.

**Scope:** the deliberately small seccomp deny rule demonstrates a mechanism;
it is not a policy for executing untrusted programs. Capabilities, namespaces,
seccomp, cgroups, and resource limits solve different parts of isolation.
Do not run the lab as root to bypass a skip or weaken host policy to make a
test pass. Exercises: compare effective vs permitted capabilities; explain why
a namespaced UID zero is not host UID zero; observe the child's cgroup path.
