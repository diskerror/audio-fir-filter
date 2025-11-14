//
// Created by Reid Woodbury.
//

#include "AudioSamples.h"

#include <boost/cstdfloat.hpp>
#include <boost/endian.hpp>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <random>
#include <stdexcept>

namespace Diskerror {

using namespace std;
using namespace boost;
using namespace boost::endian;


void AudioSamples::assertDataFormat() const {
    if (this->dataEncoding != 'PCM ' && this->dataEncoding != 'IEEE') {
        throw runtime_error(
            "ERROR: This method only handles PCM (integer) or IEEE (32-bit float) data in audio files.");
    }

    if (this->dataEncoding == 'PCM ' && this->bitsPerSample >= 32) {
        throw runtime_error(
            "ERROR: Cannot convert 32-bit integer (PCM) data to 32-bit float without loss of resolution.");
    }

    if (this->bitsPerSample > 32) {
        throw runtime_error("ERROR: Cannot read data without loss of resolution.");
    }
}


//	Maximum value storable == 2^(nBits-1) - 1
float32_t AudioSamples::GetSampleMaxMagnitude() {
    switch (this->getBitsPerSample()) {
        case 8:
            return MAX_VALUE_8_BIT; //         127

        case 16:
            return MAX_VALUE_16_BIT; //     32,767

        case 24:
            return MAX_VALUE_24_BIT; //  8,388,607

        case 32:
            return 1.0; //  Assume IEEE

        default:
            throw runtime_error("Can't handle PCM files with this bit depth.");
    }
}


void AudioSamples::ReverseCopy4Bytes(unsigned char* dest, const unsigned char* src) {
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
}


float32_t AudioSamples::Dither() {
    static std::random_device                        rd;
    static std::mt19937                              gen(rd());
    static std::uniform_real_distribution<float32_t> low(-1, 0);
    static std::uniform_real_distribution<float32_t> hi(0, 1);

    return low(gen) + hi(gen);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * ReadSamples
 * This method converts 8-, 16-, 24-bit integers (PCM), and returns 32-bit machine native floats (IEEE).
 */
void AudioSamples::ReadSamples() {
    this->assertDataFormat();
    auto dataBlock = this->ReadAllData();
    const auto tempFloat = reinterpret_cast<float32_t*>(dataBlock);

    uint_fast64_t  s; //	Index variable for samples.
    unsigned char* dPtr;

    samples.resize(this->numSamples);
    samples.shrink_to_fit();


    if (this->bitsPerSample == 8) {
        for (s = 0; s < this->numSamples; ++s) {
            this->samples[s] = static_cast<float32_t>(dataBlock[s] - 127);
        }
    }
    else if (this->bitsPerSample > 32) {
        throw runtime_error("ERROR: Cannot read data without loss of resolution.");
    }
    else {
        dPtr = dataBlock;
        switch (this->dataEndianess) {
            case 'litl':
                switch (this->bitsPerSample) {
                    case 16:
                        for (auto& elem : this->samples) {
                            elem = static_cast<float32_t>(load_little_s16(dPtr));
                            dPtr += 2;
                        }
                        break;

                    case 24:
                        for (auto& elem : this->samples) {
                            elem = static_cast<float32_t>(load_little_s24(dPtr));
                            dPtr += 3;
                        }
                        break;

                    //  Data is 32-bit float.
                    case 32:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        for (s = 0; s < this->numSamples; ++s) {
                            this->samples[s] = tempFloat[s];
                        }
#else   //  __ORDER_BIG_ENDIAN__
                        for (s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr += 4) {
                            this->ReverseCopy4Bytes((unsigned char*)&this->samples[s], dPtr);
                        }
#endif
                        break;

                    default:
                        throw runtime_error("Should not get here.");
                }
                break;

            case 'big ':
                switch (this->bitsPerSample) {
                    case 16:
                        for (auto& elem : this->samples) {
                            elem = static_cast<float32_t>(load_big_s16(dPtr));
                            dPtr += 2;
                        }
                        break;

                    case 24:
                        for (auto& elem : this->samples) {
                            elem = static_cast<float32_t>(load_big_s24(dPtr));
                            dPtr += 3;
                        }
                        break;

                    //  Data is 32-bit float.
                    case 32:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        for (s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr += 4) {
                            ReverseCopy4Bytes(reinterpret_cast<unsigned char*>(&this->samples[s]), dPtr);
                        }
#else   //  __ORDER_BIG_ENDIAN__
                        for (s = 0; s < this->numSamples;) {
                            this->samples[s] = tempFloat[s];
                        }
#endif
                        break;

                    default:
                        throw runtime_error("Should not get here.");
                }
                break;

            default:
                throw runtime_error("Should not get here.");
        }
    }

    delete dataBlock;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//	Assumes data will be the same size as the buffer that was read.
void AudioSamples::WriteSamples(const bool do_dither) {
    this->assertDataFormat();
    auto dataBlock = static_cast<unsigned char*>(calloc(this->dataBlockSize, 1));

    uint_fast64_t  s; //	Index variable for samples.
    unsigned char* dPtr;

    //	Decide whether to add dither.
    std::function<float32_t(void)> getDither = []()-> float32_t { return 0.0; };
    if (do_dither) getDither = []()-> float32_t { return Dither(); };

    if (this->bitsPerSample == 8) {
        for (s = 0; s < this->numSamples; ++s) {
            dataBlock[s] = static_cast<uint8_t>(this->samples[s] + getDither()) + 127;
        }
    }
    else {
        dPtr = dataBlock;

        switch (this->dataEndianess) {
            case 'litl':
                switch (this->bitsPerSample) {
                    case 16:
                        for (auto& elem : this->samples) {
                            store_little_s16(dPtr, static_cast<int16_t>(elem + getDither()));
                            dPtr += 2;
                        }
                        break;

                    case 24:
                        for (auto& elem : this->samples) {
                            store_little_s24(dPtr, static_cast<int32_t>(elem + getDither()));
                            dPtr += 3;
                        }
                        break;

                    case 32:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        delete dataBlock;
                        dataBlock = reinterpret_cast<unsigned char*>(this->samples.data());
#else   //  __ORDER_BIG_ENDIAN__
                        for (s = 0, dPtr = dataBlock; s < this->numSamples; ++s, dPtr += 4) {
                            this->ReverseCopy4Bytes(dPtr, reinterpret_cast<unsigned char*>(&this->samples[s]));
                        }
#endif
                        break;

                    default:
                        throw runtime_error("Should not get here.");
                }
                break;

            case 'big ':
                switch (this->bitsPerSample) {
                    case 16:
                        for (auto& elem : this->samples) {
                            store_big_s16(dPtr, static_cast<int16_t>(elem + getDither()));
                            dPtr += 2;
                        }
                        break;

                    case 24:
                        for (auto& elem : this->samples) {
                            store_big_s24(dPtr, static_cast<int32_t>(elem + getDither()));
                            dPtr += 3;
                        }
                        break;

                    case 32:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        for (auto& elem : this->samples) {
                            AudioSamples::ReverseCopy4Bytes(dPtr, reinterpret_cast<unsigned char*>(&elem));
                            dPtr += 4;
                        }
#else   //  __ORDER_BIG_ENDIAN__
                        delete dataBlock;
                        dataBlock = reinterpret_cast<unsigned char*>(this->samples.data());
#endif
                        break;

                    default:
                        throw runtime_error("Should not get here.");
                }
                break;

            default:
                throw runtime_error("Should not get here.");
        }
    }

    WriteAllData(dataBlock);
    if (dataBlock != reinterpret_cast<unsigned char*>(samples.data())) delete dataBlock;
}

} //  namespace Diskerror
