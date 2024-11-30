/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ECE484PhaseVocoderAudioProcessor::ECE484PhaseVocoderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

ECE484PhaseVocoderAudioProcessor::~ECE484PhaseVocoderAudioProcessor()
{
}

//==============================================================================
const juce::String ECE484PhaseVocoderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ECE484PhaseVocoderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ECE484PhaseVocoderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ECE484PhaseVocoderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ECE484PhaseVocoderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ECE484PhaseVocoderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ECE484PhaseVocoderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ECE484PhaseVocoderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ECE484PhaseVocoderAudioProcessor::getProgramName (int index)
{
    return {};
}

void ECE484PhaseVocoderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ECE484PhaseVocoderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    //Initialize some information about the playback
    //Window Size


    //Set the size of the window buffer
    circBuffer.setSize(getTotalNumInputChannels(), s_circBuffer);
    

    //Output data is twice the size since it has real and comples
    fftComplex.resize(s_fft);

    

}

void ECE484PhaseVocoderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ECE484PhaseVocoderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//Note the Vector must be as large or larger than the bufferStart-BufferEnd
//Copies a section of a buffer to a vector
void ECE484PhaseVocoderAudioProcessor::copyAudiotoVector(float* bufferData, std::vector<float>& Vector,int bufferStart, int copySize) {
    
    for (int i = 0; i < copySize; i++) {
        Vector[i] = bufferData[i + bufferStart];
    }

}

void ECE484PhaseVocoderAudioProcessor::hannWindow(std::vector<float>& Vector, int size) {
    for (int i = 0; i < size; i++) {
        //multiply each index by it's sinusoidal equivilaent
        Vector[i] = Vector[i] * sin(3.14159 * (float)i / (float)size);

    }

}

void ECE484PhaseVocoderAudioProcessor::circularShift(std::vector<float>& Vector, int size,unsigned int shift) {
    std::vector<float> copy = Vector;

    int shiftedIndex;
    for (int i = 0; i < size; i++) {
        //Apply the shift
        shiftedIndex = i + shift;
        //Take the modulo as this normalizes this back to the actual size range
        shiftedIndex %= size;
        Vector[i] = copy[shiftedIndex];
    }
}

//Update the buffer by filling it with the full size of the vector
void ECE484PhaseVocoderAudioProcessor::updateCircBuffer(std::vector<float>& Vector, int numSamples, int channel) {

    //i is only used to itterate the vector write posisiton is used for the buffer
    auto* bufferData = circBuffer.getWritePointer(channel);
    int i = 0;
    //The way we have this set up the first half of the data is overlapping with past data, the second half of the data
    //Shouldn't overlap anything and will be overlapped by future data. As such they should be treated differently


    for (i = 0; i < numSamples; i++) {
        //Update the write posisiton
        writePosition++;
        if (writePosition >= s_circBuffer) {
            writePosition = 0;
        }
        if (i < numSamples / 2) {
            bufferData[writePosition] += Vector[i];
        }
        else {
            bufferData[writePosition] = Vector[i];
        }

    }
   

}
//Update a set number of samples from the circular buffer
void ECE484PhaseVocoderAudioProcessor::updateOutputBuffer(juce::AudioBuffer<float>& buffer, int outputStart, int numSamples, int channel) {
    
    if (readPosition + numSamples < s_circBuffer) {
        buffer.copyFrom(channel, outputStart, circBuffer, channel, readPosition, numSamples);
        readPosition += numSamples;

    }
    else {
        int firstCopyAmount = s_circBuffer - readPosition;
        int secondCopyAmount = numSamples - firstCopyAmount;
        buffer.copyFrom(channel, outputStart, circBuffer, channel, readPosition, firstCopyAmount);
        buffer.copyFrom(channel, outputStart+firstCopyAmount, circBuffer, channel, 0, secondCopyAmount); 
        readPosition = secondCopyAmount;
    }


}

void ECE484PhaseVocoderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    //For each sample we need to

    //1. Take a window of the data and store in window buffer

    //2. Perform FFT and IFFT of existing window using window data and put this in FFT data

    //3. Perform any phase vocoder actions

    //3. Add this to
    
     //Itterate through each window 
    for (int window = 0; window < numSamples / s_hop; window++) {
       



        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            //Get audio data
            auto* bufferData = buffer.getWritePointer(channel);
            auto* circBufferData = circBuffer.getWritePointer(channel);
            //Copy a window worth of data into the window data from the buffer


            //Copy data from the buffer to a vector for the window
            copyAudiotoVector(bufferData, fftComplex, window * s_hop, s_win);

            //apply the hann windowing
            //hannWindow(fftComplex, s_win);

            //shift data to the left, this is to ensure phase is flat
           // circularShift(fftComplex, s_win, s_win / 2);


            //Now we perform the FFT on the data stored in same location
            //forwardFFT.performFrequencyOnlyForwardTransform(fftComplex.data());

            //Now we go through each bin and do frequency processing
            for (int bin = 0; bin < s_win; bin++) {
                //Put the 2 values into a complex constructor so it's easier to modify
                std::complex<float> complexData (fftComplex[2*bin], fftComplex[2*bin+1]);
                
                //Find the phase and magnitude
                float angle = arg(complexData);
               float magnitude = abs(complexData);


                //Do processing right now this is robotization

                /* ------------------------TODO Processing--------------------------*/
                //angle = 0;

                /*------------------------TODO Processing--------------------------*/
                
                //Convert back to complex number
                complexData = std::polar(magnitude, angle);

                //Store back into the original vector
                fftComplex[2 * bin] = complexData.real();
                fftComplex[2 * bin + 1] = complexData.imag();

            }
            //Takes the inverse transform
            //forwardFFT.performRealOnlyInverseTransform(fftComplex.data());

            //Inverse shift the data so it's centered again
           // circularShift(fftComplex, s_win, s_win / 2);

            //Update the Circular buffer with data
            updateCircBuffer(fftComplex, s_win,channel);

            //Copy Data starting from one hop size 
            updateOutputBuffer(buffer, window * s_hop, s_hop, channel);


            // ..do something to the data...
        }
    }
}

//==============================================================================
bool ECE484PhaseVocoderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ECE484PhaseVocoderAudioProcessor::createEditor()
{
    //return new ECE484PhaseVocoderAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ECE484PhaseVocoderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = layout.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ECE484PhaseVocoderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(layout.state.getType()))
            layout.replaceState(juce::ValueTree::fromXml(*xmlState));
}

Pluginsettings getPluginSettings(juce::AudioProcessorValueTreeState& layout) {

    Pluginsettings settings;
    settings.pitchShift = layout.getRawParameterValue("Pitch Shift in Semitones")->load();
    settings.effect = layout.getRawParameterValue("Effect")->load();


    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout
ECE484PhaseVocoderAudioProcessor::createParamaterLayout() {

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        "Pitch Shift in Semitones",
        "Pitch Shift in Semitones",
        juce::NormalisableRange<float>(-5.f,5.f,1.f),
        0.0f));


    juce::StringArray stringArray;
    stringArray.add("Pitch Shift");
    stringArray.add("Robitization");
    stringArray.add("Whisperization");

    layout.add(std::make_unique<juce::AudioParameterChoice>("Effect", "Effect", stringArray, 0));
    return layout;

}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ECE484PhaseVocoderAudioProcessor();
}
