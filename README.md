# JUCE BASED PHASE VOCODER
ECE484

Sam Miller

## Overview

This repository contains the source code necesarry to implement the 

## Usage
This Phase Vocoder is designed to implement a variety of features. This plugin can operate as a real time processing plugin or as a pre-processed plugin. The different adjustable items are as follows:

### Pitch Shift
This is a slider that represents the pitch shift of the phase vocoder in %. A positive pitch shifts represents a positive change in pitch (e.g. increasing the pitch), and a negative pitch shifts represents a negative change in pitch (e.g. decreasing the pitch.
This can be represented using the following  equation

$P_{out} = Pitch_{in}\times(1+Pitch_{Shift})$

It's worth noting that this only actually modifies pitch when pitch is in Pitch Shift mode.
### Window Size
This is a selector that represents the size of the FFT window that will be taken to analyze the signal. Note that the size is set to powers of 2 to optomize efficiency. Window size should be at 
least $2\times hop size$ at all times but this is not explicitley limited in code.It's also worth noting that if $Window size>hop size \times 16$ There may be some performance issues

### Hop Size
This is a selector that represents the distance of hops between FFT windows that are taken to analyze the signal. Note that the size is set to powers of 2 to optomize efficiecny. Hop size should be
less than $\frac{window size}{2}$ at all times. It's also worth noting that if $Hop size<\frac{window size}{16}$ There may be some performance issues.

### Effect
This is a selector that allows you to change the effects. A description of each mode is below:
- Pitch Shift: This modifies the pitch by the amount input in Pitch Shift
- Robitization: This modifies the phase of each fft bin to be zero which gives a robot type effect to the vocals.
- Whisperization: This modifies the phase of each fft bin to be a random value between -pi to pi

## Background and Theory
This project uses a phase vocoder based on the principles outlined in Audio Effects Theory, Implementation and Application by Joshua D. Reiss Andrew P. McPherson and DAFX: Digital Audio Effects, Second Edition edited by Udo Zolzer.

To implement the phase vocoder the following steps were taken:
- Take a window samples of window size specified by user.
- Multiply the function by a Hann window.
- Apply an in-place FFT to the window (can use out of place but the sample must be rotated in this case).
- Itterate through each frequency bin and perform operations to modify the frequency.
- Apply an in-place inverse FFT to the window.
- If pitch shifting resample the window to expand or compress the audio to change pitch.
- Move onto the next frame by moving by hop size.

Some more information on how these steps are implemented are below

### Frequency Processinng 

## Implementation



