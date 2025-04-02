//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_AUDIOFILE_H
#define DISKERROR_AUDIOFILE_H


#include <fstream>
#include <vector>

#include "Wave.h"

////////////////////////////////////////////////////////////////////////////////
class AudioFile 
{
	string			fileName	= "";
	fstream			fileObj;
// 	uint64_t		fileSize	= 0;
	big_uint32_t	fileType	= 0x00000000;

	uint16_t	dataType	= 0;
	uint16_t	numChannels	= 0;
	float		sampleRate	= 0;
	uint16_t	bytesPerSample = 0;
	uint64_t	numSamples	= 0;
	uint32_t	dataBlockStart	= 0;
	uint64_t	dataBlockSize	= 0;
	
	void	openFile();
	
	void	openRIFF();	//	RIFF/WAVE
	void	openRF64();	//	RIFF/WAVE > 4G
	void	openFORM();	//	FORM/AIFF

public:
	AudioFile(string);
	AudioFile(char*);
	
	~AudioFile() { };

	vector<float>	samples;

	uint16_t	GetDataType()		{ return this->dataType; };
	uint16_t	GetNumChannels() 	{ return this->numChannels; };
	float		GetSampleRate()  	{ return this->sampleRate; };
	uint16_t	GetBytesPerSample() { return this->bytesPerSample; };
	uint64_t	GetNumSamples()  	{ return this->numSamples; };
	uint64_t	GetDataBlockSize()  { return this->dataBlockSize; };

	char*		ReadRawData();
	void		WriteRawData(char*);
	
	void	ReadSamples();
	void	WriteSamples();
	
	inline float& operator[] (uint64_t s) { return samples[s]; }
};

#endif // DISKERROR_AUDIOFILE_H
