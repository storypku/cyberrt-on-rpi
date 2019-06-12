## CyberRT Port
- Based on Apollo r3.5.0
- Bazel version was 0.16.1@PX2, 0.18.0/rPi3
- Experiment carried out mainly on one PX2, Nvidia

## Known problems
- AtomicHashMap isn't thread-safe.
- ConcurrentObjectPool sacrifices effeciency for not so commonly used `size()` functionality. Worth or Not?

