# WaveformDrawMinMaxJuce
In order to get the perfect waveform you have to oversample by 2x then set the width of you view to a division of the sample rate procede with Min Max when zooming in just interpolate 



Your Width has to be a multiple of the SampleRate
resample your data before hand if your sample is 44.1khz and your width is 1024.
interpolate your data to 50.24 khz with linear interpolation before drawing.
min max order interpolated data 
set ratio = (sampleRate*length) / getWidth();
length is length of the samples 50 seconds or what you have
[![voice render wave demo](https://youtu.be/fmK6tGakLWA/0.jpg)](https://youtu.be/fmK6tGakLWA)
