//
// Created by Reid Woodbury.
//

#include <cmath>
#include <cstdlib>
#include <DiskerrorExceptions.h>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <boost/cstdfloat.hpp>
#include <ProgramOptions.h>

#include "ProcessFile.h"

namespace fs = std::filesystem;
using namespace Diskerror;
using boost::float64_t;


constexpr std::string_view HELP_TEXT = R"#(
Applies low-cut (high-pass) FIR filter to WAVE or AIFF file.
Usage:
  lowcut [options] <input_file> <output_file>
  lowcut [options] <input_file1> [input_file2 ...] <output_directory>
Options)#";


int main(const int argc, char** argv) {
	auto exitVal = EXIT_SUCCESS;

	try {
		FilterOptions opts={0,0,false,false,0};
		bool          overwrite = false;

		ProgramOptions p_opt(HELP_TEXT);
		p_opt.add_options()
		    ("frequency,f", po::value<float64_t>(&opts.freq)->default_value(15),
		    	"Filter cutoff frequency in Hz.")
		    ("slope,s", po::value<float64_t>(&opts.slope)->default_value(10),
		    	"Filter slope width in Hz.")
		    ("normalize,n", po::bool_switch(&opts.normalize),
		    	"Normalize output to maximum level.")
		    ("verbose,v", po::bool_switch(&opts.verbose),
		    	"Verbose output.")
		    ("threads,t", po::value<unsigned int>(&opts.num_threads)->default_value(0),
		    	"Number of threads (default is 2/3 of the processors available).")
		    ("overwrite,O", po::bool_switch(&overwrite),
		    	"Overwrite existing files.")
		    ("help,h", po::bool_switch(), "Display this help message.")
		;

		p_opt.add_hidden_options()
			("paths", po::value<std::vector<std::string>>(), "Input/output paths.");
		p_opt.add_positional("paths", -1);

		p_opt.run(argc, argv);

		if (p_opt["help"].as<bool>()) {
			throw StopNoError(p_opt.to_string());
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
		auto pathStrings = p_opt.getParams("paths");
		std::vector<fs::path> paths(pathStrings.begin(), pathStrings.end());

		if (paths.size() == 2) {
			// Scenario 1: Input File -> Output File
			const fs::path& inputPath  = paths[0];
			const fs::path& outputPath = paths[1];

			if (!fs::exists(inputPath) || !fs::is_regular_file(inputPath)) {
				throw FileNotFound(inputPath.string());
			}

			if (fs::exists(outputPath) && fs::is_directory(outputPath)) {
				throw UsageError(
					"With two parameters the second parameter must be a file path, not a directory.");
			}

			if (inputPath.extension() != outputPath.extension()) {
				throw UsageError(
					"Input and output file types (WAVE or AIFF) must be the same (extensions must match).");
			}

			if (fs::exists(outputPath) && !overwrite) {
				throw FileExists(outputPath.string());
			}

			if (fs::exists(outputPath)) fs::remove(outputPath);

			process_file(inputPath, outputPath, opts);

		}
		else if (paths.size() > 2) {
			// Scenario 2: Input Files -> Output Directory
			const fs::path& destDir = paths.back();

			if (fs::exists(destDir)) {
				if (!fs::is_directory(destDir)) {
					throw UsageError(
						std::format("Destination exists but is not a directory: {}", destDir.string()));
				}
			}
			else {
				if (destDir.has_extension()) {
					throw UsageError(std::format(
						"Destination directory '{}' does not exist and has a suffix. Undefined scenario.",
						destDir.string()));
				}
				show_status(std::format("Creating directory: {}", destDir.string()));
				fs::create_directories(destDir);
			}

			for (size_t i = 0; i < paths.size() - 1; ++i) {
				const fs::path& inputPath = paths[i];
				if (!fs::exists(inputPath) || !fs::is_regular_file(inputPath)) {
					throw FileNotFound(inputPath.string());
				}

				fs::path destPath = destDir / inputPath.filename();

				if (fs::exists(destPath) && !overwrite) {
					throw FileExists(destPath.string());
				}

				if (fs::exists(destPath)) fs::remove(destPath);

				process_file(inputPath, destPath, opts);
			}
		}
		else {
			throw UsageError("Invalid number of parameters. Need at least 2.");
		}
	} // End try
	catch (StopNoError &e) {
		std::string s = e.what();
		if (s.length()) std::cout << s << std::endl;
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
