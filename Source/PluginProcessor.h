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
    bool effect{ Shift };

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
    const int s_win{ 128 };
    const int s_hop{ s_win/4 };

    const float ftfactor = s_hop * 2.0 / s_win;

    //Initialize fft size (normally just window size, multiplied by 2 because the fft is real)
    const int s_fft{ 2 * s_win };

    //Initialize an FFT object
    juce::dsp::FFT forwardFFT{(int)log2(s_win)};


    juce::AudioBuffer<float> inputBuffer;
    int inWrite{ 0 };
    int inRead{ 0 };

    juce::AudioBuffer<float>  outputBuffer;
    int outWrite{ 0 };
    int outRead{ 0 };

    int s_IOBuf{ 2 * s_win };

    int startLastWindow;


    
    //Copy audio buffer into a vector
    void ECE484PhaseVocoderAudioProcessor::copyBuffertoVector(juce::AudioBuffer<float>& buffer, int bufStart, std::vector<float>& Vector, int vecStart, int numSamples, int channel);

    //Hann window a vector of data
    void ECE484PhaseVocoderAudioProcessor::hannWindow(std::vector<float>& Vector, int size);

    //Circular shift data of size by shift t
    void ECE484PhaseVocoderAudioProcessor::circularShift(std::vector<float>& Vector, int size, unsigned int shift);

    //Update an output buffer with the circular buffer starting at the read position
    void ECE484PhaseVocoderAudioProcessor::updateOutputBuffer(std::vector<float>& Window, int windowStart, juce::AudioBuffer<float>& outputBuffer, int outputStart, int numSamples, int channel);

    void ECE484PhaseVocoderAudioProcessor::updateCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int writePosition, int channel);
    void ECE484PhaseVocoderAudioProcessor::updateFromCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int readPosition, int channel);    
    void ECE484PhaseVocoderAudioProcessor::addToCircBuffer(float* Data, int numSamples, juce::AudioBuffer<float>& circBuffer, int writePosition, int channel, float gain);
    void ECE484PhaseVocoderAudioProcessor::clearCircBuffer(juce::AudioBuffer<float>& circBuffer, int numSamples, int writePosition, int channel);

    void ECE484PhaseVocoderAudioProcessor::updateBufferIndex(int& index, int increment, unsigned int size);

};
