//
// Created by Reid Woodbury.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <cmath>
#include <filesystem>
#include <format>
#include <boost/cstdfloat.hpp>

#include "ProcessFile.h"

using namespace std;
using namespace boost;
namespace fs = std::filesystem;
using namespace Diskerror;

//	Default values.
#define TEMP_FREQ  15
#define TEMP_SLOPE 10


int main(const int argc, char** argv) {
	auto exitVal = EXIT_SUCCESS;

	try {
		int          opt            = 0;
		const option long_options[] = {
			{"help", no_argument, nullptr, 'h'},
			{"frequency", required_argument, nullptr, 'f'},
			{"slope", required_argument, nullptr, 's'},
			{"normalize", no_argument, nullptr, 'n'},
			{"verbose", no_argument, nullptr, 'v'},
			{"threads", required_argument, nullptr, 't'},
			{"overwrite", no_argument, nullptr, 'O'},
			{nullptr, 0, nullptr, 0}
		};

		FilterOptions opts{TEMP_FREQ, TEMP_SLOPE, false, false, 0};
		bool         help      = false;
		bool         overwrite = false;
		
		opts.num_threads = floorl(std::thread::hardware_concurrency() * 0.7);
		if (opts.num_threads == 0) opts.num_threads = 4; // Fallback

		// Show status when verbose is true.
		std::function<void(string const&)> show_status = [](string const&) { return; };

		while (opt != -1) {
			int option_index = 0;
			opt              = getopt_long(argc, argv, "hf:s:nvt:O", long_options, &option_index);

			switch (opt) {
				case 'h':
					help = true;
					opt = -1; //  cause the while-loop to stop
					break;

				case 'f':
					opts.freq = strtod(optarg, nullptr);
					break;

				case 's':
					opts.slope = strtod(optarg, nullptr);
					break;

				case 'n':
					opts.normalize = true;
					break;

				case 'v':
					opts.verbose = true;
					show_status = [](string const& s) { cout << s << endl; };
					break;

				case 't': {
					long val = strtol(optarg, nullptr, 10);
					if (val > 0) opts.num_threads = static_cast<unsigned int>(val);
				}
				break;

				case 'O':
					overwrite = true;
					break;

				case ':':
					cerr << "option needs a value" << endl;
					break;

				case '?':
					if (optopt == 'f' || optopt == 's' || optopt == 't') {
						throw invalid_argument("Argument missing.");
					}

					throw invalid_argument("Unknown option.");

				default:
					break;
			}
		}

		//  Check for input parameter.
		if (help || optind >= argc) {
			cout << "Applies low-cut (high-pass) FIR filter to WAVE or AIFF file." << endl;
			cout << "Usage:" << endl;
			cout << "  lowcut [options] <input_file> <output_file>" << endl;
			cout << "  lowcut [options] <input_file1> [input_file2 ...] <output_directory>" << endl;
			cout << endl;
			cout << "    -f, --frequency <Hz>   Filter cutoff frequency (default: 15)." << endl;
			cout << "    -s, --slope <Hz>       Filter slope width (default: 10)." << endl;
			cout << "    -n, --normalize        Normalize output to maximum bit depth." << endl;
			cout << "    -v, --verbose          Verbose output." << endl;
			cout << "    -t, --threads <N>      Number of threads." << endl;
			cout << "    -O, --overwrite        Overwrite existing files." << endl;
			cout << "    -h, --help             Display this help message." << endl;
			return exitVal;
		}

		show_status(std::format("Using {} threads.", opts.num_threads));

		// Collect non-option arguments
		std::vector<fs::path> paths;
		for (int i = optind; i < argc; ++i) {
			paths.push_back(argv[i]);
		}

		if (paths.size() == 2) {
			// Scenario 1: Input File -> Output File
			const fs::path& inputPath = paths[0];
			const fs::path& outputPath = paths[1];

			if (!fs::exists(inputPath) || !fs::is_regular_file(inputPath)) {
				throw runtime_error(std::format("Input file does not exist or is not a file: {}", inputPath.string()));
			}

			if (fs::exists(outputPath) && fs::is_directory(outputPath)) {
				throw runtime_error("In Scenario 1 (two parameters), the second parameter must be a file path, not an existing directory.");
			}

			if (inputPath.extension() != outputPath.extension()) {
				throw runtime_error("Input and output file types (WAVE or AIFF) must be the same (extensions must match).");
			}

			if (fs::exists(outputPath) && !overwrite) {
				throw runtime_error(std::format("File exists: {}", outputPath.string()));
			}

			if (fs::exists(outputPath)) fs::remove(outputPath);

			process_file(inputPath, outputPath, opts);

		} else if (paths.size() > 2) {
			// Scenario 2: Input Files -> Output Directory
			const fs::path& destDir = paths.back();

			if (fs::exists(destDir)) {
				if (!fs::is_directory(destDir)) {
					throw runtime_error(std::format("Destination exists but is not a directory: {}", destDir.string()));
				}
			} else {
				if (destDir.has_extension()) {
					throw runtime_error(std::format("Destination directory '{}' does not exist and has a suffix. Undefined scenario.", destDir.string()));
				}
				show_status(std::format("Creating directory: {}", destDir.string()));
				fs::create_directories(destDir);
			}

			for (size_t i = 0; i < paths.size() - 1; ++i) {
				const fs::path& inputPath = paths[i];
				if (!fs::exists(inputPath) || !fs::is_regular_file(inputPath)) {
					throw runtime_error(std::format("Input file does not exist or is not a file: {}", inputPath.string()));
				}

				fs::path destPath = destDir / inputPath.filename();

				if (fs::exists(destPath) && !overwrite) {
					throw runtime_error(std::format("File exists: {}", destPath.string()));
				}

				if (fs::exists(destPath)) fs::remove(destPath);

				process_file(inputPath, destPath, opts);
			}
		} else {
			throw runtime_error("Invalid number of parameters. Need at least 2.");
		}
	}     // End try
	catch (const std::exception& e) {
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}
	catch (...) {
		cerr << "Caught an unknown exception." << endl;
		exitVal = EXIT_FAILURE;
	}

	return exitVal;
} // End main