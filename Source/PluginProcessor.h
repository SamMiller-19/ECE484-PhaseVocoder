/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#ifndef M_PI
    #define M_PI 3.14159267
#endif


enum {
    Shift=0,
    Robitization = 1,
    Whisperization = 2
};

struct Pluginsettings
{
    float pitchShift{ 0 };
    int effect{ Shift };
    int s_win{ 0 };
    int s_hop{ 0 };

};

Pluginsettings getPluginSettings(juce::AudioProcessorValueTreeState& layout);
//==============================================================================
/**
*/
class ECE484PhaseVocoderAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ECE484PhaseVocoderAudioProcessor();
    ~ECE484PhaseVocoderAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParamaterLayout();
    juce::AudioProcessorValueTreeState layout{ *this, nullptr, "Paramaters", createParamaterLayout() };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ECE484PhaseVocoderAudioProcessor)
  

    //Initialize another vector for the FFT data of each window





  


    //Initialize hop and window size
    const int s_win_max{ 4096 };
    const int s_hop_max{ 2048 };

    juce::AudioBuffer<float> inputBuffer;
    int inWrite = 0;
    int inRead= 0;

    juce::AudioBuffer<float>  outputBuffer;
    int outWrite= 0 ;
    int outRead=0 ;

    
    /*********************************************************Time DOmain Processing**************************************************/
    //Hann window a vector of data
    void ECE484PhaseVocoderAudioProcessor::hannWindow(std::vector<float>& Vector, int size);

    //Circular shift data of size by shift t
    void ECE484PhaseVocoderAudioProcessor::circularShift(std::vector<float>& Vector, int size, unsigned int shift);

    /*****************************************************Freq domain Processing ****************************************************/
    //Take single complex sample input and apply robotization
    std::complex<float> ECE484PhaseVocoderAudioProcessor::doRobitization(std::complex<float> input);

    //Take single complex sample input and apply Whisperization
    std::complex<float> ECE484PhaseVocoderAudioProcessor::doWhisperization(std::complex<float> input);

    //Take single complex sample input and % pitch shift and apply Whisperization
    std::complex<float> ECE484PhaseVocoderAudioProcessor::doWhisperization(std::complex<float> input, float pPitchShift);


    /*****************************************************Circular Buffer ****************************************************/

    //Update the Circular buffer with the data provided for num Samples starting at the beginning of the data and at write Position of the circular buffer
    void ECE484PhaseVocoderAudioProcessor::updateCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int writePosition, int channel);

    //Add numSamples of data to CircularBuffer adding to existing data from the circular buffer starting at at read Position
    void ECE484PhaseVocoderAudioProcessor::addToCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int writePosition, int channel, float gain);

    //Update numSamples of Data starting from the beginning of data and from the circular buffer at read Position
    void ECE484PhaseVocoderAudioProcessor::updateFromCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int readPosition, int channel);    

    //Set numSamples of channel of circular buffer starting from writePosition to zero
    void ECE484PhaseVocoderAudioProcessor::clearCircBuffer(juce::AudioBuffer<float>& circBuffer, int numSamples, int writePosition, int channel);

    //Update index by of buffer of size by increment
    void ECE484PhaseVocoderAudioProcessor::updateBufferIndex(int& index, int increment, unsigned int size);

   


};
