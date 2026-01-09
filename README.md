# Audio FIR Filter

A high-precision C++ command-line tool designed to apply a low-cut (high-pass) Finite Impulse Response (FIR) filter to WAVE and AIFF audio files. It is primarily used to remove DC offset and low-frequency noise from digital recordings while strictly preserving all non-audio metadata.

## Key Features

*   **Metadata Preservation:** Copies input file headers and extra chunks exactly before processing audio data.
*   **High Precision:** Utilizes `long double` (`float64_t`) arithmetic for filter kernel stability and accuracy, capable of handling steep slopes (narrow transition bands).
*   **Cross-Platform:** Built with Boost.Endian to handle WAVE (Little Endian) and AIFF (Big Endian) formats on various architectures.
*   **Performance:** Multi-threaded processing for faster-than-realtime filtering on modern hardware.

## Building

The project requires a C++23 compiler and the Boost libraries. It depends on the sibling `c_lib` project for core audio I/O.

```bash
make
```

## Usage

```bash
./lowcut [options] <input_file> <output_file>
./lowcut [options] <input_files...> <output_directory>
```

### Scenarios
1.  **Single File:** Provide one input file and one output file path.
2.  **Batch Processing:** Provide multiple input files; the final argument must be a destination directory.

### Options
*   `-f, --frequency <Hz>`: Filter cutoff frequency (default: 15Hz).
*   `-s, --slope <Hz>`: Filter transition band width (default: 10Hz).
*   `-n, --normalize`: Normalize the output to the maximum bit depth.
*   `-O, --overwrite`: Overwrite existing files.
*   `-t, --threads <N>`: Set number of threads (defaults to ~70% of cores).
*   `-v, --verbose`: Enable verbose status output.

## Architecture & Algorithm

The core algorithm is a Windowed-sinc FIR filter, using Blackman window.

*   **Tech Stack:** C++23, Boost (Endian, cstdfloat), `c_lib`.
*   **Structure:**
    *   `main.cp`: Argument parsing and scenario logic.
    *   `ProcessFile.cp`: File I/O orchestration.
    *   `FilterCore.h`: The parallelized FIR filter kernel implementation.

## Acknowledgements

The windowed sinc algorithm implementation was derived from [The Scientist and Engineer's Guide to Digital Signal Processing](http://www.dspguide.com) (Second Edition) by Steven W. Smith.