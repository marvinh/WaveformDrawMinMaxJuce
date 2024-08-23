# WaveformDrawMinMaxJuce
In order to get the perfect waveform you have to oversample by 2x then set the width of you view to a division of the sample rate procede with Min Max when zooming in just interpolate 


//Width Of this component must be a divider of the sample rate for exmaple 44100Hz the width of this component must be at least 441 pixels
//upsample the data you want to render by at least 2x
//use min max for large views samples >= width*4 else juct interpolate
