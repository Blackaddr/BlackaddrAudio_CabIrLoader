/*
 * Company: Blackaddr Audio
 * Effect Name: Cab IR Loader
 * Description: 
 *
 * This file was auto-generated by Aviate Audio Effect Creator for the Multiverse.
 */
#include <cmath>
#include "Aviate/LibBasicFunctions.h"
#include "CabIrLoader.h"

using namespace Aviate;

namespace BlackaddrAudio_CabIrLoader {

void CabIrLoader::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
    if (parameter >= NUM_CONTROLS) {
        return ; // Invalid midi parameter
    }
    m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
    m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

void CabIrLoader::setParam(int paramIndex, float paramValue)
{
    switch(paramIndex) {
    case 0 : bypass( (paramValue - 0.000000) / (1.000000 - 0.000000) ); break;
    case 1 : volume( (paramValue - 0.000000) / (10.000000 - 0.000000) ); break;
    case 2 : cabselect( (paramValue - 1.000000) / (16.000000 - 1.000000) ); break;
    case 3 : hipass1hz( (paramValue - 20.000000) / (200.000000 - 20.000000) ); break;
    case 4 : lopass1khz( (paramValue - 1.000000) / (16.000000 - 1.000000) ); break;
    case 5 : delay1ms( (paramValue - 0.000000) / (20.000000 - 0.000000) ); break;
    default : break;
    }
}

float CabIrLoader::getUserParamValue(int paramIndex, float normalizedParamValue)
{
    switch(paramIndex) {
    case 0 : return ( ((1.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // bypass
    case 1 : return ( ((10.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // volume
    case 2 : return ( ((16.000000 - 1.000000) * normalizedParamValue) + 1.000000 ); // cabselect
    case 3 : return ( ((200.000000 - 20.000000) * normalizedParamValue) + 20.000000 ); // hipass1hz
    case 4 : return ( ((16.000000 - 1.000000) * normalizedParamValue) + 1.000000 ); // lopass1khz
    case 5 : return ( ((20.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // delay1ms
    default : return 0.0f;
    }
}

void CabIrLoader::processMidi(int channel, int control, int value)
{
    float val = (float)value / 127.0f;

    if ((m_midiConfig[Bypass_e][MIDI_CHANNEL] == channel) && (m_midiConfig[Bypass_e][MIDI_CONTROL] == control)) {
        bypass(val);
        return;
    }

    if ((m_midiConfig[Volume_e][MIDI_CHANNEL] == channel) && (m_midiConfig[Volume_e][MIDI_CONTROL] == control)) {
        volume(val);
        return;
    }

    if ((m_midiConfig[CabSelect_e][MIDI_CHANNEL] == channel) && (m_midiConfig[CabSelect_e][MIDI_CONTROL] == control)) {
        cabselect(val);
        return;
    }

    if ((m_midiConfig[HiPass1Hz_e][MIDI_CHANNEL] == channel) && (m_midiConfig[HiPass1Hz_e][MIDI_CONTROL] == control)) {
        hipass1hz(val);
        return;
    }

    if ((m_midiConfig[LoPass1KHz_e][MIDI_CHANNEL] == channel) && (m_midiConfig[LoPass1KHz_e][MIDI_CONTROL] == control)) {
        lopass1khz(val);
        return;
    }

    if ((m_midiConfig[Delay1ms_e][MIDI_CHANNEL] == channel) && (m_midiConfig[Delay1ms_e][MIDI_CONTROL] == control)) {
        delay1ms(val);
        return;
    }

}

audio_block_t* CabIrLoader::m_basicInputCheck(audio_block_t* inputAudioBlock, unsigned outputChannel)
{
    // Check if effect is disabled
    if (m_enable == false) {
        // do not transmit or process any audio, return as quickly as possible after releasing the inputs
        if (inputAudioBlock) { release(inputAudioBlock); }
        return nullptr; // disabled, no further EFX processing in update()
    }  // end of enable check

    // check if effect is in bypass
    if (m_bypass == true) {
        // drive input directly to the specified output. ie. bypass
        if (inputAudioBlock != nullptr) {
            // valid input, drive to outputChannel if specified
            if (outputChannel >= 0) {
                transmit(inputAudioBlock, outputChannel); // drive to specified output
            }
            release(inputAudioBlock); // release the input block as we are done with it
        } else { // invalid input block, allocate a block and drive silence if specified
            if (outputChannel >= 0) {
                audio_block_t* silenceBlock = allocate();
                if (silenceBlock) {
                    clearAudioBlock(silenceBlock);  // create silence in the buffer
                    transmit(silenceBlock, outputChannel);
                    release(silenceBlock);
                }
            }
        }
        return nullptr;  // bypassed, no further EFX processing in update()
    }  // end of bypass check

    // If not disabled or bypassed, create silence if the input block is invalid then
    // return the valid audio block so update() can continue.
    if (inputAudioBlock == nullptr) {
        inputAudioBlock = allocate();
        if (inputAudioBlock == nullptr) { return nullptr; } // check if allocate was unsuccessful
        // else
        clearAudioBlock(inputAudioBlock);
    }
    return inputAudioBlock; // inputAudioBLock is valid and ready for update() processing
}

const uint8_t rblk[256] = { 0xc3, 0x68, 0x9d, 0x0a, 0x3d, 0x70, 0x07, 0xe7, 0x31, 0x17, 0x3d, 0xec, 0x77, 0x83, 0x26, 0x68, 0x6f, 0x32, 0x62, 0xf0, 0x79, 0x1f, 0xbd, 0x43, 0x00, 0x3b, 0x19, 0x63, 0x48, 0xfa, 0xc9, 0x5f, 0xcc, 0x3a, 0x0f, 0x37, 0x8b, 0x54, 0x6e, 0xa2, 0xd5, 0x36, 0x5c, 0xef, 0x03, 0xac, 0x67, 0xba, 0x8f, 0x0a, 0xa3, 0x9d, 0x2b, 0x87, 0x24, 0xc5, 0xa9, 0x40, 0x4d, 0xd0, 0x17, 0x5c, 0xcd, 0x03, 0x50, 0x6c, 0x33, 0x79, 0x10, 0x89, 0x12, 0xe5, 0xe6, 0xba, 0x9f, 0xd5, 0x90, 0xe0, 0xc1, 0x7b, 0xc6, 0x17, 0x32, 0xcc, 0xe3, 0xc1, 0x36, 0x2c, 0x30, 0x16, 0x3f, 0xd1, 0x82, 0xd9, 0xcb, 0x44, 0x01, 0xf9, 0x05, 0xdc, 0x18, 0x36, 0x2e, 0x58, 0x6c, 0x9b, 0xf1, 0x0b, 0xf4, 0x02, 0x69, 0x12, 0xa3, 0x44, 0xa3, 0x34, 0x16, 0x26, 0x02, 0x94, 0x25, 0x02, 0xc4, 0xf6, 0x16, 0x46, 0x56, 0x27, 0x00, 0x73, 0xca, 0x47, 0x36, 0xdc, 0xe2, 0x98, 0x1e, 0x2c, 0xb9, 0xc3, 0xb4, 0x0e, 0x6c, 0xa1, 0xc8, 0x8b, 0x31, 0x56, 0x5d, 0xca, 0xb7, 0x91, 0xde, 0x9d, 0xf7, 0x38, 0x07, 0x3f, 0xed, 0x8f, 0xb2, 0xb7, 0xd6, 0xe8, 0x93, 0xb8, 0x80, 0xb2, 0xe4, 0x3a, 0x75, 0x98, 0x48, 0xe1, 0x39, 0x10, 0x6c, 0x6b, 0x66, 0xc9, 0x35, 0x1e, 0x5a, 0x13, 0xbb, 0x52, 0x4c, 0xc2, 0x91, 0x39, 0x51, 0x43, 0xf0, 0x27, 0x2c, 0x83, 0xdf, 0xac, 0x35, 0xc3, 0xe6, 0xaa, 0x5b, 0x2e, 0x8b, 0x94, 0x3f, 0xf7, 0xff, 0xa5, 0xc1, 0x35, 0xc3, 0x1b, 0x48, 0x7e, 0x6d, 0x94, 0x40, 0xfe, 0xcd, 0x91, 0x42, 0xbd, 0xb8, 0x6e, 0x41, 0x97, 0x1a, 0x76, 0x5a, 0x01, 0x21, 0xb5, 0x2f, 0xac, 0x4a, 0x6e, 0xa4, 0x49, 0x14, 0x65, 0x7e, 0xd7, 0x80, 0xc7, 0x56, 0xee, 0x5b, 0x96, 0xec, 0x29, 0x28, 0x2e, 0xe6, 0xe0};
const uint8_t* CabIrLoader::getRblk() { return rblk; }
static constexpr char PROGMEM CabIrLoader_name[] = {0x42, 0x6c, 0x61, 0x63, 0x6b, 0x61, 0x64, 0x64, 0x72, 0x20, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x3a, 0x44, 0x3a, 0x43, 0x61, 0x62, 0x20, 0x49, 0x52, 0x20, 0x4c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x0};
const char* CabIrLoader::getName() { return CabIrLoader_name; }

}
