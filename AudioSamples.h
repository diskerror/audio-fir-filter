//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_AUDIOSAMPLES_H
#define DISKERROR_AUDIOSAMPLES_H


#include <boost/cstdfloat.hpp>
#include <boost/endian.hpp>

#include "AudioFile.h"
#include <VectorMath.h>

namespace Diskerror {


using namespace std;
using namespace boost;
using namespace boost::endian;

/**
 *	class AudioSamples
 */
class AudioSamples : public AudioFile {
    static void ReverseCopy4Bytes(unsigned char*, const unsigned char*);

    void assertDataFormat() const;

public:
    // Constructor
    explicit AudioSamples(const filesystem::path& fPath) : AudioFile(fPath) {};

    // Destructor
    ~AudioSamples() = default;


    // Exposing these members because of their useful methods.
    VectorMath<float32_t> samples;

    float32_t GetSampleMaxMagnitude();

    void ReadSamples();

    void Normalize() { samples.normalize_mag(GetSampleMaxMagnitude()); }

    static float32_t Dither();

    void WriteSamples(bool do_dither = true);

    float32_t& operator[](const uint64_t s) { return samples[s]; }
};


} // namespace Diskerror

#endif // DISKERROR_AUDIOSAMPLES_H
