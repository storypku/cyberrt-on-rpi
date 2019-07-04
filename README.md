## CyberRT Port to RPi3
- Based on Apollo r3.5.0
- Bazel version was 0.16.1@PX2, 0.18.0/rPi3
- Experiment carried out mainly on PX2, Nvidia

## How to start
```
bash cyber_l10n.sh
```

### Coroutine
- reference/coroutine.h was adapted from https://github.com/tonbit/coroutine
- swapcontext(3) manpage

## Note
- It seems that the Apollo Team has added `aarch64` support for CyberRT in the master branch.
- Scheduler::ParseCpuset expects well-formatted cpuset. Otherwise, it will either throw or exit.
- //cyber/logger and //cyber/common:log are unrelated logging facilities.

## Known problems
- AtomicHashMap isn't thread-safe.
- ConcurrentObjectPool sacrifices effeciency for not so commonly used `size()` functionality. Worth or Not?

