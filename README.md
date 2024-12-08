# JUCE BASED PHASE VOCODER
This Vocoder was developed by Sam Miller in completion of the requirements of ECE 484 at the University of Victoria During the Fall of 2024. To see the full project please go to https://github.com/SamMiller-19/ECE484-PhaseVocoder.

## Overview

This repository contains the source code necessary to implement a phase vocoder using the JUCE framework. The repository contains several files. If you want to use the plugin, there is an ECE484-phaseVocoder.vst3 file in the repository which can be imported into audio processing software. To do this audacity in windows copy the file into C:$\backslash$ Program Files$\backslash$ Common Files$\backslash$ VST3 (Make a VST3 folder if it doesn't exist).

Beyond this, there is a .jucer file which can be used with projucer to build the project, as well as PluginProcessor.cpp/h files which do the audio processing, and PluginEditor.cpp/h files that can be used to modify the appearance of the plugin.

To read more about the JUCE framework see their documentation at https://juce.com/.

## Using the Plugin
This Phase Vocoder is designed to implement several effects including whisperization, robotization, and pitch shifting. To implement all these effects in one plugin without compromising on quality, a variety of selectors and sliders were included. The function of each of these user controls are outlined below.

### Effect Selector
This is a selector that allows you to change the effects. A description of each mode as well as the recommended window and hop sizes are below:
|Parameter|Pitch Shift|Robotization|Whisperation|
|:---------:|:-----------:|:------------:|:-----------:|
|Description|Modifies the pitch by the amount input in Pitch Shift|modifies the phase of each fft bin to be zero which gives a robot type effect to the vocals|modifies the phase of each fft bin to be a random value between -pi to pi, removes most of the pitch and vowel sounds.|
|Window Size (samples)| 1024-4096 | <=2048 | 64-256 |
|Hop Size (fraction of Window Size)| $<=\frac{1}{8} Window Size$|$<=\frac{1}{2} Window Size$, changing will vary pitch| $<=\frac{1}{2} Window Size$|


### Pitch Shift Slider
This slider represents the ratio of the modified pitch divided by the original pitch. A pitch greater than one represents a positive change in pitch (increasing the pitch), and a less than one represents a negative change in pitch (decreasing the pitch).

The change in pitch can be represented with the below equation:

$P_{out} = Pitch_{in}\times Shift$

It's worth noting that this only modifies pitch when the pitch is in Pitch Shift mode. In the other modes, this slider does nothing.
### Window Size Selector
This selector represents the size of the FFT window that will be taken to analyze the signal. Note that the size is set to powers of 2 to optimize efficiency. Window size should be at 
least $2\times hop size$ at all times but this is not explicitly limited in code. It's also worth noting that if $Window size>hop size \times 16$, there may be some performance issues if processing in real-time due to the number of FFTs that must be taken. If you need to go outside these bounds it's recommended to pre-process the audio. It's also worth noting that if $Hop Size < 2\times Window Size$ in any case the overlap-add will not fully reconstruct the signal so this should be avoided. Due to the potential for phase error in pitch shifting this should be increased to  $Hop Size < 8\times Window Size$ to limit audible artifacts from the pitch shift.

### Hop Size Selector
This selector represents the distance of hops between FFT windows that are taken to analyze the signal. Note that the size is set to powers of 2 to optimize efficiency. Hop size should be
less than $\frac{window size}{2}$ at all times. It's also worth noting that if $Hop size<\frac{window size}{16}$, there may be some performance issues if processing in real-time due to the number of FFTs that must be taken. If you need to go outside these bounds it's recommended to pre-process the audio. It's also worth noting that if $Hop Size < 2\times Window Size$ in any case the overlap-add will not fully reconstruct the signal so this should be avoided. Due to the potential for phase error in pitch shifting this should be increased to  $Hop Size < 8\times Window Size$ to limit audible artifacts from the pitch shift.


## Get Started Building and Editing the Plugin
To work on this project you will first need to install Juce and Projucer. This can be installed from https://juce.com/download/.

Next, you need to install the repository https://github.com/SamMiller-19/ECE484-PhaseVocoder to some location on your desktop. Once this has been cloned run the ECE484-PhaseVocoder.jucer file using projucer, this will load all the required modules into the project and build it for you. From here you should be able to open the files in an IDE and begin working on it. To see more about starting a project in JUCE see https://docs.juce.com/master/tutorial_new_projucer_project.html.

When you open the project you will notice a PluginProcess.cpp/h file and a PluginEditor.cpp/h file. In this case, the PluginProcessor was used for all the audio processing. For this project, the PluginEditor files were not edited but they could be modified to change the appearance of the plugin.

## Technical Information About this Plugin
This project uses a phase vocoder based on the principles outlined in Audio Effects Theory, Implementation and Application by Joshua D. Reiss Andrew P. McPherson(https://www.taylorfrancis.com/books/mono/10.1201/b17593/audio-effects-joshua-reiss-andrew-mcpherson) and DAFX: Digital Audio Effects, Second Edition edited by Udo Zolzer(https://www.dafx.de/DAFX_Book_Page_2nd_edition/index.html).

It's worth noting that this implementation also uses circular buffers to store the output and input information. These circular buffers are not a part of C++ or the JUCE framework so all of the processing of these data structres is written in this project. The idea of a circular buffer is that each buffer has a stored read and/or write position, when this exceeds the buffer size it loops back to zero. This means old data is constantly being overwritten by the circular buffer. Therefore if the size is correctly selected this stores all required data and limits the amount of memory required by the program.

### Step One: Copy input buffer into circular buffer.

The JUCE framework supports real-time processing by taking input samples in audio buffers of a size determined by the environment the plugin is running in. However, the phase vocoder needs not only the current information, but also information of at least one window size into the future, and one window size into the past. To do this we make circular input and output buffers of size $2\times s_{win max}+s_{buffer}$ Where $s_{win max}$ is the max window size and $s_{buffer}$ is the size of the buffer we are reading from.

Using this buffer we copy one buffer of data into the input buffer and advance the buffer position by the hop size.

### Step Two: Hann Windowing

To Process the audio data, the input data is broken into windows. For each buffer windows are taken by advancing the start of the windows by the hop size. If the end of the window exceeds the edge of the buffer then it stops the window. For each window, a Hanning function is applied. This makes the transitions between windows much less jarring and limits audio artifacts from the vocoder. To window the function each sample is multiplied by the below function where n represents the current sample of the window and N is the sample size.

$1-cos(\frac{2\pi n}{N})$ 

### Step Three: Apply the Forward FFT
To transform the function into the frequency domain a fast Fourier transform (FFT) is used. In our case, the built-in JUCE FFT class was used to compute the transform. This allows us to process the signals in the frequency domain. It's worth noting that this real-only frequency transform was used since we are only interested in real signals in the time domain. The function used to do this is found in the juce::dsp::fft class and is called performRealOnlyForwardTransform()

### Step Four: Apply Frequency Modulation
To modulate the frequency a different Algorithm is applied for each effect.

For Pitch shifting we must calculate what the phase should be after pitch shifting to ensure the signal is clean. To do this we iterate through each frequency bin and calculate what its phase should be based on the pitch shift. To do this we calculate the unwrapped phase difference of the unmodified phasem, divide this by the pitch shift, then add this to the previous phase for this bin. For more information on this see the doPitchShift function or read more in the McPherson textbook at https://www.taylorfrancis.com/books/mono/10.1201/b17593/audio-effects-joshua-reiss-andrew-mcpherson.

For robotization, the phase of each sample is set to 0. This flattens the pitch which gives vocals a robotic-like effect.

For whisperization, the phase of each sample is instead set to a random value between -pi and pi. This removes the vowel sounds from the vocals especially when the window size is low. 

### Step Five: Apply Inverse FFT
For this, take the inverse FFT by invoking the same JUCE object used in the forward FFT.  The function used to do this is found in the juce::dsp::fft class and is called performRealOnlyInverseTransform().

### Step Six: Resample the Window to Apply Pitch Shift (Pitch Shifting only)

To shift the pitch we need to resample the window. To raise the signal pitch we need to shrink the window size and to lower the pitch we need to increase the window size. To do this the window is compressed/expanded by the pitch shift. However, in most cases, this leads to some resampled indexes being non-integer. To compensate for this we use linear interpolation from the two nearest values to calculate the interpolated values. The formula for this is shown below.

$f_{resample}[n]=f_{original}[n/shift-trunc(n/shift)]\times (1-frac(n/shift)) +f_{original}[trunc(n/shift)+1]frac(n/shift)$

Where trunc truncates any decimal places, and frac takes only the values after the decimal place.

### Step 7: Overlap and Add

To recombine the windows, take the processed window and add it to the output circular buffer starting at the write position of the output buffer. Advance the write position of the output buffer by the hop size.

### Step 8: Copy Data Back to Audio Buffer

For the audio to play data, we need to copy the data into the same buffer that we read the input from. However, to ensure that all overlapping windows from the buffer have been processed, there must be a delay of the maximum window size from where the input data is being read. This does put a slight delay on the real-time processing but it is the only way to process the data in real-time without heavy distortion of the input signal. Once the signal has been read from the output buffer, the buffer index is cleared to ensure the new signal is not added on top of it.



