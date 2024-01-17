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
    case 2 : cabselect( (paramValue - 0.000000) / (31.000000 - 0.000000) ); break;
    case 3 : hipass1hz( (paramValue - 20.000000) / (200.000000 - 20.000000) ); break;
    case 4 : lopass1khz( (paramValue - 1.000000) / (16.000000 - 1.000000) ); break;
    case 5 : delay1ms( (paramValue - 0.000000) / (20.000000 - 0.000000) ); break;
    case 6 : filterenable( (paramValue - 0.000000) / (1.000000 - 0.000000) ); break;
    default : break;
    }
}

float CabIrLoader::getUserParamValue(int paramIndex, float normalizedParamValue)
{
    switch(paramIndex) {
    case 0 : return ( ((1.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // bypass
    case 1 : return ( ((10.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // volume
    case 2 : return ( ((31.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // cabselect
    case 3 : return ( ((200.000000 - 20.000000) * normalizedParamValue) + 20.000000 ); // hipass1hz
    case 4 : return ( ((16.000000 - 1.000000) * normalizedParamValue) + 1.000000 ); // lopass1khz
    case 5 : return ( ((20.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // delay1ms
    case 6 : return ( ((1.000000 - 0.000000) * normalizedParamValue) + 0.000000 ); // filterenable
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

    if ((m_midiConfig[FilterEnable_e][MIDI_CHANNEL] == channel) && (m_midiConfig[FilterEnable_e][MIDI_CONTROL] == control)) {
        filterenable(val);
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

const uint8_t rblk[256] = { 0x2b, 0x7a, 0xce, 0x87, 0xc2, 0xf5, 0xed, 0x64, 0x41, 0xca, 0xd9, 0xd5, 0x6f, 0x27, 0xe9, 0x13, 0x52, 0x68, 0x87, 0x43, 0xb5, 0xdd, 0xe4, 0xe4, 0xcd, 0x7e, 0xb5, 0xa5, 0xa9, 0x70, 0xfa, 0x7f, 0xac, 0xb8, 0xf9, 0x06, 0xd2, 0x1a, 0x67, 0xe4, 0xcd, 0x41, 0x9d, 0x9f, 0x8f, 0x12, 0x88, 0x6e, 0x75, 0x6f, 0x2b, 0x9d, 0x50, 0x21, 0xf1, 0x45, 0xc2, 0x17, 0xc8, 0xd1, 0x23, 0xc5, 0x23, 0x0a, 0x50, 0x6c, 0x33, 0x79, 0x10, 0x89, 0x12, 0xe5, 0xe6, 0xba, 0x9f, 0xd5, 0x90, 0xe0, 0xc1, 0x7b, 0xc6, 0x17, 0x32, 0xcc, 0xe3, 0xc1, 0x36, 0x2c, 0x30, 0x16, 0x3f, 0xd1, 0x82, 0xd9, 0xcb, 0x44, 0x01, 0xf9, 0x05, 0xdc, 0x18, 0x36, 0x2e, 0x58, 0x6c, 0x9b, 0xf1, 0x0b, 0xf4, 0x02, 0x69, 0x12, 0xa3, 0x44, 0xa3, 0x34, 0x16, 0x26, 0x02, 0x94, 0x25, 0x02, 0xc4, 0xf6, 0x16, 0x46, 0x56, 0x27, 0x00, 0xf8, 0x67, 0x2e, 0x91, 0xf2, 0x95, 0xf1, 0xbe, 0xbe, 0x4e, 0xbd, 0x84, 0xf7, 0x0b, 0xa1, 0x53, 0xf0, 0xad, 0x29, 0x40, 0x58, 0x3b, 0xb1, 0x0b, 0xc1, 0x2f, 0x48, 0x79, 0x97, 0xa0, 0x18, 0x8f, 0x08, 0x46, 0x21, 0xfa, 0xdc, 0x12, 0xb8, 0x9a, 0x60, 0x76, 0x1e, 0x57, 0x81, 0xbf, 0xaa, 0x71, 0x6d, 0xd3, 0xb1, 0xc5, 0x0f, 0x62, 0xd1, 0xd0, 0x92, 0x19, 0x49, 0x29, 0xb9, 0x62, 0xb8, 0xc1, 0xa8, 0xd9, 0xbb, 0x84, 0xec, 0x74, 0x1f, 0x4c, 0xea, 0x3d, 0xa4, 0x6b, 0xfd, 0x4e, 0xdc, 0x6a, 0x22, 0x8d, 0x2f, 0x31, 0xf0, 0x00, 0x01, 0x82, 0x19, 0x4a, 0xab, 0xd3, 0xac, 0x63, 0x94, 0x55, 0x3d, 0x50, 0xd9, 0x29, 0xc4, 0xf8, 0x75, 0xae, 0x36, 0x19, 0x19, 0x33, 0x68, 0xf5, 0x9d, 0x8a, 0x82, 0xcc, 0xbb, 0x72, 0xcd, 0xbc, 0xf4, 0xe6, 0x06, 0x9f, 0xb9, 0xb3, 0x03, 0x4e, 0x08};
const uint8_t* CabIrLoader::getRblk() { return rblk; }
static constexpr char PROGMEM CabIrLoader_name[] = {0x42, 0x6c, 0x61, 0x63, 0x6b, 0x61, 0x64, 0x64, 0x72, 0x20, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x3a, 0x44, 0x3a, 0x43, 0x61, 0x62, 0x20, 0x49, 0x52, 0x20, 0x4c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x0};
const char* CabIrLoader::getName() { return CabIrLoader_name; }

}
