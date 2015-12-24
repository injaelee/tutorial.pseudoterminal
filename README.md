# Pseudo Terminal (PTY) Tutorial
OS PseudoTTY (PTY) explanations, mechanics, and C code

## Overview

## TODO
Experiment with 'openpty' and 'login_tty' which simplify the handling of creating master and slave PTY file descriptors.

### Key Concepts/Points
```
-- PTY Related API's
posix_openpt
grantpt
unlockpt
ptsname

-- Process Related API's
fork

-- Terminal Related API's
tcgetattr
cfmakeraw
tcsetattr

-- I/O Related API's
close
read
write
dup
FD_ZERO
FD_SET
select
ioctl

-- Symbolic Related API's
setsid
```
## Instructions
```
gcc bash.pty.c -o bash.pty
gcc ping.back.c -o ping.back
```
