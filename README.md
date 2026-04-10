# marchers

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Thanks

A big thank you to:

* ccexplore for their documentation on the original compression format which I used as a guide for writing the decompressor.
* esoteric-programmer for their 3ds implementation of the decompression algorithm, which I used to compare the correctness of my implementation.
