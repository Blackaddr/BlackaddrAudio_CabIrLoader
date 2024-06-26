/*
 * Company: Blackaddr Audio
 * Effect Name: Cab IR Loader
 * Description: A IR loader for guitar cabs up to 42ms impulse time.
 
 * This design is based heavily on a copy-and-refactor approach to the great work by bmillier
 * for implementing uniformly partition convolution on a Teensy.
 * Ref: https://github.com/bmillier/FFT-Convolution-Filter-Uniformly-partitioned/tree/master
*/
#include <vector>
#include <cmath>
#include <arm_math.h>
#include <arm_const_structs.h>
#include "Aviate/EfxPrint.h"
#include "Aviate/IrLoader.h"
#include "CabIrLoader.h"

using namespace Aviate;
using namespace IrLoader;

//#define DEBUG_CAB_IR_PRINT

namespace BlackaddrAudio_CabIrLoader {

#if defined(DEBUG_CAB_IR_PRINT)
static void debugPrintIrCabs();  // for printing Cab IR info
#endif

constexpr float PI_F = static_cast<float>(M_PI);
constexpr float TWO_PI_F = static_cast<float>(2.0 * M_PI);

enum class FilterType : unsigned {
	LOW_PASS,
	HIGH_PASS
};

static void updateFilterFreq(FilterType filterType, LowPassFilter_t* filter, float cutoffFrequency)
{
	float ratio        = cutoffFrequency / AUDIO_SAMPLE_RATE_HZ;
	if ( (ratio < 0.0) || (ratio > 0.5f) ) {
		switch(filterType) {
		case FilterType::HIGH_PASS : ratio = 0.0f;
		case FilterType::LOW_PASS  :
		default                    : ratio = 0.5f;

		}
	}
    float ratioRadians = TWO_PI_F * ratio;
	switch(filterType) {
	case FilterType::HIGH_PASS : filter->alpha = (1.0f / (ratioRadians + 1.0f)); break;
	case FilterType::LOW_PASS  :
	default                    :
	    filter->alpha = ratioRadians / (ratioRadians + 1.0f);
	}
}

static void initializeFilter(FilterType filterType, LowPassFilter_t *filter, float cutoffFrequency) {
	filter->yPrev = 0.0f;
	filter->xPrev = 0.0f;
	updateFilterFreq(filterType, filter, cutoffFrequency);
}

static float applyFilter(FilterType filterType, LowPassFilter_t *filter, float input) {
	switch(filterType) {
	case FilterType::HIGH_PASS :
	{
		float y = filter->alpha * (filter->yPrev + input - filter->xPrev);
		filter->yPrev = y;
		filter->xPrev = input;
		return y;
	}
	case FilterType::LOW_PASS  :
	default                    :
	{
		float y = filter->yPrev + (filter->alpha * (input - filter->yPrev));
		filter->yPrev = y;
		return y;
	}
	}
}

#if 0
static std::vector<float> generateHannWindow(int windowSize) {
    std::vector<float> hannWindow(windowSize);

    for (int i = 0; i < windowSize; ++i) {
        hannWindow[i] = 0.5 * (1 - std::cos(2 * M_PI * i / (windowSize - 1)));
    }

    return hannWindow;
}
#endif

static bool isCabIndexValid(int idx)
{
	if ((NUM_IMPULSE_RESPONSES < 1) || (idx >= (int)NUM_IMPULSE_RESPONSES)) { return false; }
	if (IR_SAMPLE_SIZES[idx] > 0) { return true; }
	return false;
}

static int getFirstValidCab()
{
	int validIndex = -1;
	for (unsigned idx=0; idx < NUM_IMPULSE_RESPONSES; idx++) {
		if (isCabIndexValid(idx)) {
			validIndex = idx;
			break;
		}
	}
	return validIndex;
}

static size_t getCabNumSamples(int idx) {
	if (!isCabIndexValid(idx)) { return 0; }
	return IR_SAMPLE_SIZES[idx];
}


CabIrLoader::CabIrLoader()
: AudioStream(NUM_INPUTS, m_inputQueueArray)
{
	m_partitionSize = AUDIO_SAMPLES_PER_BLOCK;
	m_fftLength = 2 * m_partitionSize;

	last_sample_buffer_L = (float*)malloc(AUDIO_SAMPLES_PER_BLOCK * sizeof(float));
	last_sample_buffer_R = (float*)malloc(AUDIO_SAMPLES_PER_BLOCK * sizeof(float));

	fftout = (float*)malloc(MAX_IR_PARTITIONS * 512 * sizeof(float));
	maskgen = (float*)malloc(m_fftLength * 2 * sizeof(float));
}

CabIrLoader::~CabIrLoader()
{
    if (fftout) { free(fftout); }
	if (maskgen) { free(maskgen); }

	if (last_sample_buffer_L) { free (last_sample_buffer_L); }
	if (last_sample_buffer_R) { free (last_sample_buffer_R); }
}


bool CabIrLoader::m_config(unsigned selectedIr)
{
#if defined(DEBUG_CAB_IR_PRINT)
	debugPrintIrCabs();
#endif

	initializeFilter(FilterType::LOW_PASS,  &m_lpf_L, 16000.0f);
	initializeFilter(FilterType::HIGH_PASS, &m_hpf_L, 20.0f);
	initializeFilter(FilterType::LOW_PASS,  &m_lpf_R, 16000.0f);
	initializeFilter(FilterType::HIGH_PASS, &m_hpf_R, 20.0f);

    m_configComplete = false;
	size_t impulseSizeSamples = getCabNumSamples(selectedIr);
	if (impulseSizeSamples < 1) { return false; }  // invalid cab

	const size_t MAX_IR_SAMPLES = m_partitionSize * CabIrLoader::MAX_IR_PARTITIONS;

	m_numSamplesIr = (impulseSizeSamples > MAX_IR_SAMPLES) ? MAX_IR_SAMPLES : impulseSizeSamples;
	n_numPartitionsIr = m_numSamplesIr / m_partitionSize;

    float32_t* fo = (float32_t*)fftout;

	m_impulseLoaded = false;
	buffidx=0;

	if (n_numPartitionsIr > MAX_IR_PARTITIONS) {
		n_numPartitionsIr = MAX_IR_PARTITIONS;
	}

    // define the pointer that is used to access the main program DMAMEM array fftout[]
	ptr_fftout = fo;
	ptr_fmask = &fmask[0][0];

	memset(ptr_fftout, 0, n_numPartitionsIr*512*4);  // clear fftout array
	memset(fftin, 0,  512 * 4);  // clear fftin array


    // Calculate the FFT of the FIR filter coefficients once to produce the FIR filter mask
    // convolution.impulse(guitar_cabinet_impulse, maskgen,nc);
	m_impulse(IrLoader::IMPULSE_RESPONSE_PTRS[selectedIr], maskgen, m_numSamplesIr);

    m_configComplete = true;
	return m_configComplete;
}


void CabIrLoader::update(void)
{
    if (!m_configComplete) {
		if (!m_config(getFirstValidCab())) {
			m_bypass = true;
		}
	}
    if (!m_enable) { return; }

    audio_block_t *blockL = receiveWritable(0);
    audio_block_t *blockR = receiveWritable(1);

	if ( (!blockL && !blockR) || m_bypass || !m_impulseLoaded ) {
		// attempt bypass
		if (blockL) {
			transmit(blockL, 0);
			release(blockL);
		}
		if (blockR) {
			transmit(blockR, 1);
			release(blockR);
		}
		return;
	}

    // At this point we are guarenteed to have at least one channel to process
	bool processLeft  = false;
	bool processRight = false;
	if (blockL) { processLeft  = true; m_updateInputPeak(blockL); }
	if (blockR) {
		processRight = true;
		if (!processLeft) { m_updateInputPeak(blockR); }  // use right for levels
	}

	const size_t AUDIO_BLOCK_SIZE = AUDIO_SAMPLES_PER_BLOCK;
	float float_buffer_L[AUDIO_BLOCK_SIZE];
	float float_buffer_R[AUDIO_BLOCK_SIZE];

	if (processLeft)  { arm_q15_to_float(blockL->data, &float_buffer_L[0], AUDIO_SAMPLES_PER_BLOCK); }
	else { memset(&float_buffer_L[0], 0, AUDIO_SAMPLES_PER_BLOCK * sizeof(float)); }

	if (processRight) { arm_q15_to_float(blockR->data, &float_buffer_R[0], AUDIO_SAMPLES_PER_BLOCK); }
	else { memset(&float_buffer_R[0], 0, AUDIO_SAMPLES_PER_BLOCK * sizeof(float)); }

	for (unsigned i = 0; i < m_partitionSize; i++)
	{
		fftin[i * 2] = last_sample_buffer_L[i]; // real is left chan
		fftin[i * 2 + 1] = last_sample_buffer_R[i]; // imaginary is right chan
		// copy recent samples to last_sample_buffer for next time!
		last_sample_buffer_L[i] = float_buffer_L[i];
		last_sample_buffer_R[i] = float_buffer_R[i];
		// now fill recent audio samples into FFT_buffer (left channel: re, right channel: im)
		fftin[256 + i * 2] = float_buffer_L[i]; // real = left chan
		fftin[256 + i * 2 + 1] = float_buffer_R[i]; // imaginary = right chan
	}

	//**********************************************************************************
	//	  Complex Forward FFT
	// calculation is performed in-place the FFT_buffer [re, im, re, im, re, im . . .]
	// complex FFT with the new library CMSIS V4.5
	// **********************************************************************************
	const static arm_cfft_instance_f32 *S;
	S = &arm_cfft_sR_f32_len256;
	arm_cfft_f32(S, fftin, 0, 1);
	int buffidx512 = buffidx * 512;
	float* ptr1 = ptr_fftout + (buffidx512);   // set pointer to proper segment of fftout array
	memcpy(ptr1, fftin, 2048);  // copy 512 samples from fftin to fftout (at proper segment)

	//**********************************************************************************
	//	 Complex multiplication with filter mask (precalculated coefficients subjected to an FFT)
	//	 this is taken from wdsp library by Warren Pratt firmin.c
	//  **********************************************************************************
	int k = buffidx;
	memset(accum, 0, m_partitionSize * 16);  // clear accum array
	int k512 = k * 512;  // save 7 k*512 multiplications per inner loop
	int j512 = 0;

	for (unsigned j = 0; j < n_numPartitionsIr; j++)      //BM np was nfor
	{
			ptr1 = ptr_fftout + k512;
			float* ptr2 = ptr_fmask + j512;
			// do a complex MAC (multiply/accumulate)
			arm_cmplx_mult_cmplx_f32(ptr1, ptr2, ac2, 256);  // This is the complex multiply
			for (int q = 0; q < 512; q=q+8) {    // this is the accumulate
				accum[q] += ac2[q];
				accum[q+1] += ac2[q+1];
				accum[q+2] += ac2[q+2];
				accum[q+3] += ac2[q+3];
				accum[q+4] += ac2[q+4];
				accum[q + 5] += ac2[q + 5];
				accum[q + 6] += ac2[q + 6];
				accum[q + 7] += ac2[q + 7];
			}
		k--;
		if (k < 0)
		{
			k = n_numPartitionsIr - 1;
		}
		k512 = k * 512;
		j512 += 512;
	} // end np loop

	buffidx++;
	buffidx = buffidx % n_numPartitionsIr;

	//*********************************************************************************
	//	 Complex inverse FFT
	//  ********************************************************************************

	// complex iFFT with the new library CMSIS V4.5
	const static arm_cfft_instance_f32 *iS;
	iS = &arm_cfft_sR_f32_len256;
	arm_cfft_f32(iS, accum, 1, 1);

	//**********************************************************************************
	//	  Overlap and save algorithm, which simply means y�u take only half of the buffer
	//	  and discard the other half (which contains unusable time-aliased audio).
	//	  Whether you take the left or the right part is determined by the position
	//	  of the zero-padding in the filter-mask-buffer before doing the FFT of the
	//	  impulse response coefficients
	//   *********************************************************************************
	for (unsigned i = 0; i < m_partitionSize; i++)
	{
		float_buffer_L[i] = accum[i * 2 + 0] * m_volume;
		float_buffer_R[i] = accum[i * 2 + 1] * m_volume;
	}

	if (processLeft) {
		if (m_filterEnable) {
			for (unsigned idx=0; idx < AUDIO_SAMPLES_PER_BLOCK; idx++) {
				float_buffer_L[idx]= applyFilter(FilterType::LOW_PASS, &m_lpf_L, float_buffer_L[idx]);
			}
			for (unsigned idx=0; idx < AUDIO_SAMPLES_PER_BLOCK; idx++) {
				float_buffer_L[idx] = applyFilter(FilterType::HIGH_PASS, &m_hpf_L, float_buffer_L[idx]);
			}
		}

		arm_float_to_q15(&float_buffer_L[0], blockL->data, AUDIO_SAMPLES_PER_BLOCK);
		m_updateOutputPeak(blockL);
		transmit(blockL, 0);
		release(blockL);
	}

	if (processRight) {
		if (m_filterEnable) {
			for (unsigned idx=0; idx < AUDIO_SAMPLES_PER_BLOCK; idx++) {
				float_buffer_R[idx] = applyFilter(FilterType::LOW_PASS, &m_lpf_R, float_buffer_R[idx]);
			}
			for (unsigned idx=0; idx < AUDIO_SAMPLES_PER_BLOCK; idx++) {
				float_buffer_R[idx] = applyFilter(FilterType::HIGH_PASS, &m_hpf_R, float_buffer_R[idx]);
			}
		}

		arm_float_to_q15(&float_buffer_R[0], blockR->data, AUDIO_SAMPLES_PER_BLOCK);
		if (!processLeft) { m_updateOutputPeak(blockR); }
		transmit(blockR, 1);
		release(blockR);
	}
}

void CabIrLoader::volume(float value)
{
    float volDbValue = -40.0f + (value * 50.0f);  // remap the normalized value to represent -40dB to +10dB
    volumeDb(volDbValue);  // AudioEffectWrapper has built-in volume function in dB

    // Demonstrate the Serial.printf() function to print over the USB serial port
    // EFX_PRINT must be used to ensure prints only appear in DEBUG builds.
    // NOTE: Serial.printf() requires explicit newline and carriage return \n\r
    EFX_PRINT(Serial.printf("CabIrLoader(): volume change: normalized value: %f  dB value: %f\n\r", value, volDbValue));
}

void CabIrLoader::cabselect(float value)
{
	//int requestedCab = static_cast<int>(getUserParamValue(CabSelect_e, value)) - 1;
	int requestedCab = static_cast<int>(getUserParamValue(CabSelect_e, value));
	if (requestedCab < 0) { return; } // invalid cab
	unsigned newIdx = ((unsigned)requestedCab % NUM_IMPULSE_RESPONSES);  /// User value cab select starts at 1, but we are 0 indexed

	if (!m_config(newIdx)) {
		EFX_PRINT(Serial.printf("CabIrLoader(): invalid cab select: %d\n\r", newIdx));
		return;
	}
	EFX_PRINT(Serial.printf("CabIrLoader(): Requested cab: %d cab selected: %d name: %s\n\r", requestedCab, newIdx, IMPULSE_RESPONSE_NAME_PTRS[newIdx]));
}

void CabIrLoader::hipass1hz(float value)
{
	m_hipass1hz = getUserParamValue(HiPass1Hz_e, value);
	updateFilterFreq(FilterType::HIGH_PASS, &m_hpf_L, m_hipass1hz);
	updateFilterFreq(FilterType::HIGH_PASS, &m_hpf_R, m_hipass1hz);
	EFX_PRINT(Serial.printf("CabIrLoader::hipass1hz(): lowpass alpha is %f\n\r", m_hpf_L.alpha));
}

void CabIrLoader::lopass1khz(float value)
{
	m_lopass1khz = getUserParamValue(LoPass1KHz_e, value);
	updateFilterFreq(FilterType::LOW_PASS, &m_lpf_L, 1000.0f * m_lopass1khz);
	updateFilterFreq(FilterType::LOW_PASS, &m_lpf_R, 1000.0f * m_lopass1khz);
}

void CabIrLoader::filterenable(float value)
{
	m_filterenable = value;
	if (m_filterenable) { m_filterEnable = true; }
	else { m_filterEnable = false; }
}

void CabIrLoader::m_impulse(const float32_t *coefs,float32_t *maskgen,int size)
{
	const static arm_cfft_instance_f32* maskS;
	maskS = &arm_cfft_sR_f32_len256;
	int ptr;

	for (unsigned j = 0; j < n_numPartitionsIr; j++)
	{
		memset(maskgen, 0, m_partitionSize * 16);  // zero out maskgen
		// take part of impulse response and fill into maskgen
		for (unsigned i = 0; i < m_partitionSize; i++)
		{
			// THIS IS FOR REAL IMPULSE RESPONSES OR FIR COEFFICIENTS
			// the position of the impulse response coeffs (right or left aligned)
			// determines the useable part of the audio in the overlap-and-save (left or right part of the iFFT buffer)
			ptr = i + j * m_partitionSize;
			if (ptr < size) {
				maskgen[i * 2 + m_partitionSize * 2] = coefs[ptr];
			}
			else  // Impulse is smaller than the fixed (defined) filter length (taps)
			{
				maskgen[i * 2 + m_partitionSize * 2] = 0.0;  // pad the last part of filter mask with zeros
			}
		}

		// perform complex FFT on maskgen
		arm_cfft_f32(maskS, maskgen, 0, 1);

		// fill into fmask array
		float (*fmaskPtr)[512] = (float (*)[512])fmask;
		for (unsigned i = 0; i < m_partitionSize * 4; i++)
		{
			fmaskPtr[j][i] = maskgen[i];
		}
	}

	m_impulseLoaded = true;
}

#if defined(DEBUG_CAB_IR_PRINT)
static void debugPrintIrCabs()
{
	EFX_PRINT(Serial.printf("\nCabIrLoader::debugPrintIrCabs():\n\r"));
	EFX_PRINT(Serial.printf("- NUM_IMPULSE_RESPONSES: %d\n\r", NUM_IMPULSE_RESPONSES));
	EFX_PRINT(Serial.printf("- IMPULSE_SAMPLE_RATE_F: %f\n\r", IMPULSE_SAMPLE_RATE_F));

	for (unsigned idx=0; idx < NUM_IMPULSE_RESPONSES; idx++) {
		EFX_PRINT(Serial.printf("-- Name:%s  Size:%u\n\r", IMPULSE_RESPONSE_NAME_PTRS[idx], IR_SAMPLE_SIZES[idx]));
	}
}
#endif

}
