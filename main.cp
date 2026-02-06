//
// Created by Reid Woodbury.
//

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <DiskerrorExceptions.h>
#include <filesystem>
#include <format>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/cstdfloat.hpp>
#include <boost/program_options.hpp>

#include "ProcessFile.h"

using namespace boost;
namespace po = program_options;
namespace fs = std::filesystem;
using namespace Diskerror;


constexpr std::string_view HELP = R"#(
Applies low-cut (high-pass) FIR filter to WAVE or AIFF file.
Usage:
  lowcut [options] <input_file> <output_file>
  lowcut [options] <input_file1> [input_file2 ...] <output_directory>

    -f, --frequency <Hz>   Filter cutoff frequency (default: 15).
    -s, --slope <Hz>       Filter slope width (default: 10).
    -n, --normalize        Normalize output to maximum level.
    -v, --verbose          Verbose output.
    -t, --threads <N>      Number of threads.
    -O, --overwrite        Overwrite existing files.
    -h, --help             Display this help message.
)#";


int main(const int argc, char** argv) {
	auto exitVal = EXIT_SUCCESS;

	try {
		FilterOptions opts={0,0,false,false,0};
		bool          overwrite = false;

		po::options_description p_opt("Allowed options");
		p_opt.add_options()
		    ("help,h", po::bool_switch(), "Display this help message.")
		    ("frequency,f", po::value<float64_t>(&opts.freq)->default_value(15),
		    	"Filter cutoff frequency (default: 15).")
		    ("slope,s", po::value<float64_t>(&opts.slope)->default_value(10),
		    	"Filter slope width (default: 10).")
		    ("normalize,n", po::bool_switch(&opts.normalize),
		    	"Normalize output to maximum level.")
		    ("verbose,v", po::bool_switch(&opts.verbose),
		    	"Verbose output.")
		    ("threads,t", po::value<unsigned int>(&opts.num_threads)->default_value(0),
		    	"Number of threads (default is 2/3 of the processors available).")
		    ("overwrite,O", po::bool_switch(&overwrite),
		    	"Overwrite existing files.")
		;

		// Hidden option for positional file args
		po::options_description hidden;
		hidden.add_options()
			("paths", po::value<std::vector<std::string>>(), "Input/output paths.");

		// Combine for parsing (but only show p_opt in help)
		po::options_description all_options;
		all_options.add(p_opt).add(hidden);

		po::positional_options_description pos;
		pos.add("paths", -1);  // -1 = unlimited positional args

		po::variables_map vm;

		po::store(
			  po::command_line_parser(argc, argv)
				  .options(all_options)
				  .positional(pos)
				  .run(),
			  vm);

		po::notify(vm);

		if (vm["help"].as<bool>()) {
			throw StopNoError(HELP.data());
		}

		// Show status when verbose is set.
		std::function<void(std::string const&)> show_status =
			opts.verbose ?
				[](std::string const&) { return; } :
				[](std::string const& s) { std::cout << s << std::endl; };


		if (opts.num_threads == 0) opts.num_threads = floor(std::thread::hardware_concurrency() * 0.7);
		if (opts.num_threads == 0) opts.num_threads = 4; // Fallback

		show_status(std::format("Using {} threads.", opts.num_threads));

		// Collect non-option arguments
		std::vector<fs::path> paths;
		for (auto& s : vm["paths"].as<std::vector<std::string>>()) {
			paths.push_back(s);
		}

		if (paths.size() == 2) {
			// Scenario 1: Input File -> Output File
			const fs::path& inputPath  = paths[0];
			const fs::path& outputPath = paths[1];

			if (!fs::exists(inputPath) || !fs::is_regular_file(inputPath)) {
				throw std::runtime_error(
					std::format("Input file does not exist or is not a file: {}", inputPath.string()));
			}

			if (fs::exists(outputPath) && fs::is_directory(outputPath)) {
				throw std::runtime_error(
					"With two parameters the second parameter must be a file path, not a directory.");
			}

			if (inputPath.extension() != outputPath.extension()) {
				throw std::runtime_error(
					"Input and output file types (WAVE or AIFF) must be the same (extensions must match).");
			}

			if (fs::exists(outputPath) && !overwrite) {
				throw std::runtime_error(std::format("File exists: {}", outputPath.string()));
			}

			if (fs::exists(outputPath)) fs::remove(outputPath);

			process_file(inputPath, outputPath, opts);

		}
		else if (paths.size() > 2) {
			// Scenario 2: Input Files -> Output Directory
			const fs::path& destDir = paths.back();

			if (fs::exists(destDir)) {
				if (!fs::is_directory(destDir)) {
					throw std::runtime_error(
						std::format("Destination exists but is not a directory: {}", destDir.string()));
				}
			}
			else {
				if (destDir.has_extension()) {
					throw std::runtime_error(std::format(
						"Destination directory '{}' does not exist and has a suffix. Undefined scenario.",
						destDir.string()));
				}
				show_status(std::format("Creating directory: {}", destDir.string()));
				fs::create_directories(destDir);
			}

			for (size_t i = 0; i < paths.size() - 1; ++i) {
				const fs::path& inputPath = paths[i];
				if (!fs::exists(inputPath) || !fs::is_regular_file(inputPath)) {
					throw std::runtime_error(std::format("Input file does not exist or is not a file: {}",
													inputPath.string()));
				}

				fs::path destPath = destDir / inputPath.filename();

				if (fs::exists(destPath) && !overwrite) {
					throw std::runtime_error(std::format("File exists: {}", destPath.string()));
				}

				if (fs::exists(destPath)) fs::remove(destPath);

				process_file(inputPath, destPath, opts);
			}
		}
		else {
			throw std::runtime_error("Invalid number of parameters. Need at least 2.");
		}
	} // End try
	catch (StopNoError &e) {
		std::cerr << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		exitVal = EXIT_FAILURE;
	}
	catch (...) {
		std::cerr << "Caught an unknown exception." << std::endl;
		exitVal = EXIT_FAILURE;
	}

	return exitVal;
} // End main
