# WaveformDrawMinMaxJuce
In order to get the perfect waveform you have to oversample by 2x then set the width of you view to a division of the sample rate procede with Min Max when zooming in just interpolate 


//Width Of this component must be a divider of the sample rate for exmaple 44100Hz the width of this component must be at least 441 pixels
//upsample the data you want to render by at least 2x
//use min max for large views samples >= width*4 else juct interpolate



Oversample the data 2x then linearly interpolate for zooming 

Your Width has to be a multiple of the SampleRate 

or resample your data before hand if you sample is 44.1khz your width is 1024.
upsample to 88.2khz 
interpolate your data to 102.4 khz
min max order interpolated data 
set ratio = (100240*length) / getWidth();
length is length of the samples 50 seconds or what you have 


<img width="1034" alt="Screenshot 2024-08-23 at 2 37 25 AM" src="https://github.com/user-attachments/assets/811fae01-3b73-4107-8f3f-abff05503074">
