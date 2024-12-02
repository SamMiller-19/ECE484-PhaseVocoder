/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    const int s_hop{ s_win/8 };

    const float ftfactor = s_hop * 2.0 / s_win;

    //Initialize fft size (normally just window size, multiplied by 2 because the fft is real)
    const int s_fft{ 2 * s_win };

    //Initialize an FFT object
    juce::dsp::FFT forwardFFT{(int)log2(s_win)};


    juce::AudioBuffer<float> inputBuffer;
    int s_IOBuf = 2*s_win;


    //Initialize vectors for FFT data, we get this to be a 1x2 vector to start resize # rows according to FFT size later
    std::vector<float> fftmagnitude;
    //Tracks how much has already been written to the window this output
    int winWritten{ 0 };
    //Tracks how much has already been written to the buffer this output
    int outWritten{ 0 };
    //User Defined functions


    
    //Copy audio buffer into a vector
    void ECE484PhaseVocoderAudioProcessor::copyBuffertoVector(juce::AudioBuffer<float>& buffer, int bufStart, std::vector<float>& Vector, int vecStart, int numSamples, int channel);

    //Hann window a vector of data
    void ECE484PhaseVocoderAudioProcessor::hannWindow(std::vector<float>& Vector, int size);

    //Circular shift data of size by shift t
    void ECE484PhaseVocoderAudioProcessor::circularShift(std::vector<float>& Vector, int size, unsigned int shift);

    //Update an output buffer with the circular buffer starting at the read position
    void ECE484PhaseVocoderAudioProcessor::updateOutputBuffer(std::vector<float>& Window, int windowStart, juce::AudioBuffer<float>& outputBuffer, int outputStart, int numSamples, int channel);



};
