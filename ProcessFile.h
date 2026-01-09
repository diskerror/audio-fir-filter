//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_PROCESSFILE_H
#define DISKERROR_PROCESSFILE_H

#include <filesystem>
#include <boost/cstdfloat.hpp>

namespace Diskerror {

struct FilterOptions {
	boost::float64_t    freq;
	boost::float64_t    slope;
	bool                normalize;
	bool                verbose;
	unsigned int        num_threads;
};

void process_file(const std::filesystem::path& file_path, const FilterOptions& opts);

} // namespace Diskerror

#endif // DISKERROR_PROCESSFILE_H
