This is an experimental pipeline to generate a PSX (Playstation 1) demo executable featuring an RWX (per Active Worlds usage) prop.

## Requirements

- nodejs (and npm)
- PSn00bSDK (in `/usr/local/lib` by default) with mipsel-none-elf-toolchain
- CMake
- make
- gcc

## Serializing the RWX model

Tu pick which RWX model to serialize, two environment variables need to be defined:
- `PSX_RWX_PATH`: The AW object path meant to serve the object (in its `rwx/` subfolder);
- `PSX_RWX_PROP`: The RWX file to load (available in said `rwx/` subfolder from the path above).

Optionally: the path for the output binary-serialized file can be specified with `PSX_RWX_OUTPUT`, which is `./rwx.bin` by default.

Once those variables are set, go to the `rwx2binary` subfolder and run the following:

```bash
npm install
./rwx2base64.sh | node base64rwx2binary.js
```

This ought to install all the _npm_ dependencies and run the whole serialization pipeline.

The resulting `./rwx.bin` file (or whatever you specified with `PSX_RWX_OUTPUT`) must then be copied to the root `src/` folder along with the other source file(s).

## Generating the ISO file

Once the `rwx.bin` file has been placed into `src/`, run the following from the root of the project to build the PSX demo featuring the RWX prop:

```bash
export PSN00BSDK_LIBS=/usr/local/lib/libpsn00b
cmake -B build --preset default
make -C build
```

Once completed, `build/src/test.exe` can then be opened with a working PSX emulator (like _duckstation_).
