#include "iostream"
#include "SineWaveAudioChunk.h"
#include "cmath"

using namespace std;

SineWaveAudioChunk::SineWaveAudioChunk(double samplingRate,
                                       double duration,
                                       double amplitude,
                                       uint32_t frequency,
                                       double phase):
AudioChunk(samplingRate,duration),
amplitude(amplitude),
frequency(frequency),
phase(phase)
{
	cout << "Create a new Sinwave Audio chunk, with amplitude = "
         << amplitude << ", frequency = " << frequency 
         << ", phase = " << phase << endl;
	this->samplesBuffer = new uint16_t[this->samplesNumber+1];
	this->samplesBuffer[0]=(2*this->samplesNumber)+1;
}

SineWaveAudioChunk::~SineWaveAudioChunk()
{
	// cout << "Destroy SineWaveAudioChunk" << endl;

}

double SineWaveAudioChunk::getamplitude()
{
	return(this->amplitude);
}

void SineWaveAudioChunk::setamplitude(double amplitude)
{
	this->amplitude = amplitude;
}

uint32_t SineWaveAudioChunk::getfrequency()
{
	return(this->frequency);
}

void SineWaveAudioChunk::setfrequency(uint32_t amplitude)
{
	this->frequency = frequency;
}

double SineWaveAudioChunk::getphase()
{
	return(this->amplitude);
}

void SineWaveAudioChunk::setphase(double phase)
{
	this->phase = phase;
}

void SineWaveAudioChunk::genrate_samples()
{
	double t = 0;
	this->samplesBuffer[0] = samplesNumber;
	for (unsigned int i = 1; i < samplesNumber; i++){
		this->samplesBuffer[i] = this->amplitude * sin(2 * M_PI * this->frequency * t + this->phase);
		t += 1/this->samplingRate;
	}
	cout << "generate sine wave !!" << endl;
}
