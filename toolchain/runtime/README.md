# The runtime

What a native Haard program stands on, where the C++ path has libc (record 0084).
`x86_64_linux.s` is assembled together with what `hdc` writes:

```
hasm x86_64_linux.s program.s -o program
```

Nothing but the kernel is below it: no libc, and system calls directly.

## What a program has to define

| label | given | gives |
|---|---|---|
| `main` | `rdi` the count of arguments, `rsi` their table | the exit code in `rax` |
| `__before_main` | | what the IR's initialisers are |
| `__after_main` | | what its finalisers are |

The calling convention is record 0084's: whole numbers and pointers in `rdi`, `rsi`,
`rdx`, `rcx`, `r8`, `r9`, floats in `xmm0` on, the result in `rax` or `xmm0`, and only
`rsp` and `rbp` are kept. Every label shares one namespace, `.L` ones too, and this file's
start with `.Lrt` or `__`.

## What it gives

`__io_open_read`, `__io_open_write`, `__io_read`, `__io_at_end`, `__io_write`,
`__io_close`, `__io_flush`, `__io_stdin`, `__io_stdout`, `__io_stderr`, `__abort`, `__alloc`,
`__free` and `__floor`, as `std.low_io` and the IR name them, and `__abort_with`, for a
message then a stop. `__pow` is not here: it becomes Haard (record 0084), and until then a
program that uses it is told its label is not defined.

**Output follows C's stdio**, since the C++ path is its oracle. `stdout` goes by the
block, or by the line on a terminal; `stderr` looks unbuffered, and is held only to the
end of a line, or until anything else would go out or wait, which costs a system call a
line and not one a byte. Everything open is flushed at the end of a program. `__abort` lets
out only the console and raises `SIGABRT`.

**The allocator** has size classes, powers of two from 32 bytes to 1 MB, each with a free
list, carved from chunks of 4 MB. With memory already mapped each call takes a bounded
time, which the WCET report needs. A larger block is a mapping of its own, given back when
freed. A freed small block never goes back to the kernel. Running out stops with a message.

## The tests

`tests/run.sh` runs programs written by hand, and `as` and `ld` judge the runtime's bytes
as an oracle. Each program runs under a time limit and a file size limit: a wrong one tends
to loop writing, and would otherwise fill the disk.

**Not proven by a case**: going on after a partial write, which nothing simple provokes.
