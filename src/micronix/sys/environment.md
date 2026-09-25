# Environment variables in exec (V7-style)

V7's defining addition over V6 was the environment: `exec` carries a
third argument, `envp`, an array of `NAME=value` strings copied onto the
child's stack next to `argv`.  Micronix has none of it.  This is the plan
to add it.  It is mostly plumbing - `putargs()` already builds the stack;
envp is just a second pointer array with a string payload.

## The stack at process entry

Today `putargs()` builds (low to high addresses):

        u.sp -> argc
                argv[0] .. argv[nargs-1]
                NULL                          (argv terminator)
                ... argv strings ...

`crt0.s` does `pop de` (argc), then `hl = sp` (argv), and calls
`main(argc, argv)`.

The V7 layout the child must see instead:

        u.sp -> argc
                argv[0] .. argv[nargs-1]
                NULL                          (argv terminator)
                envp[0] .. envp[nenv-1]
                NULL                          (envp terminator)
                ... argv strings, then env strings ...

so `crt0` can compute `envp = argv + argc + 1` (argv plus its NULL) and
`environ = envp`.

## Kernel (`sys/exec.c`)

The syscall dispatch in `system.c` already passes three register words:
`(*syssw[call].call)(arg[0], arg[1], arg[2])`, and `exec` is syscall
`0x0b`.  So the kernel side is a signature change, not a new syscall.

- `exec(name, args, envp)` - third parameter.  (Or, to keep the ABI,
  add `execve(name, argv, envp)` and make `exec` a two-arg wrapper that
  passes `u.environ`; V7 does the latter.  Since every current caller is
  the libu `exec.s` stub, either works - decide on `execve` for the V7
  shape and a thin `exec` shim.)
- `getargs(args, envp)` - walk both pointer arrays into the `bp[]`
  argument buffers, counting `nargs`/`nbytes` and `nenv`/`nenvbytes`.
  Bump `NBLKS` if needed (4 blocks may be tight with an environment).
- `fit()` - the room check `4 + nargs + nargs + nbytes` must become
  `4 + nargs + nargs + nenv + nenv + nbytes + nenvbytes`, and the two
  NULLs the new layout adds.
- `putargs()` - build the two arrays:

      d_env  = &usrtop - nenvbytes          (env strings)
      d_argv = d_env - nbytes               (argv strings)
      envp   = d_argv - (nenv + 1) words
      av     = envp    - (nargs + 1) words
      u.sp   = &av[-1]                       (argc)

  fill `av[]` and `envp[]` with the string pointers (the same two passes
  that now fill `av[]`, one per array), `putword(NULL)` after each, and
  `putword(nargs, u.sp)` last.  Careful with the `rem`-count loop: the
  fix that removed the `+ 512` wrap applies to every block pass.

## libc / libu

- `execve.s` (new 3-arg stub, `0x0b`) - name in hl, argv and envp on the
  stack, like `exec.s` but one more pop.  `exec.s` becomes (or gains) a
  two-arg wrapper that pushes `environ`.
- `crt0.s` - after `pop de` (argc) and `hl = sp` (argv), compute
  `envp = sp + 2*(argc+1)`, store it in a new `_environ` global, push it,
  and call `main(argc, argv, envp)` (or keep `main(argc, argv)` and let
  users read `environ` - decide whether ccc's `main` takes three).
- New `getenv.c` / `putenv.c` (or `setenv`), plus `extern char **environ`.
  `getenv` scans `NAME=value` for `NAME=`.  `putenv`/`setenv` grow or
  mutate `environ`.

## Shell (`/bin/sh`)

- `name=value` assignment and `export` builtins; keep an internal env
  list and pass it to `execve` (via `exec`'s `environ`).  `sh` today has
  "no variables, no environment" (BUGS file), so this is the largest
  visible piece.

## Order of work

1. Kernel `execve` + stack layout + `fit`/`NBLKS`; the libu stub and
   `crt0`, so a 3-arg exec round-trips and `environ` is set.
2. `getenv`/`putenv` and `environ` in libc.
3. Shell variables + `export`, wiring its env into exec.

## Gotchas

- `putargs()` uses `static` locals (see sys/TODO re: `static`); the whole
  function is single-threaded under the exec lock, so that's fine, but
  any new helper must not sleep while those are live.
- The `+ 512` 16-bit wrap: any new block-walk must use a count, not an
  end pointer, or it bites at the top of the BUFSEG window (0xee00).
- `ps` (u.p->args, 8 bytes) and the `0xd1` exec trace both read the
  old layout - update them for the third array.
