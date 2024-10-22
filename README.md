# STM32_Dictaphone
Bare metal project to build a simple dictaphone using a STM32_L053R8 nucleo board as main driver.

## General description
I always wanted to play around with audio signals, see how they are dealt with in the digital realm and see how one can generate and modify an audio sample at his leisure. Also, since audio streams are continuous, handling them seemed challenging, peaking my interest even higher.

As usual, there are a plethora of solutions readily available, especially for the ESP32 MCUS to help any hobbyist jump on this particular topic and quickly come up with an audio element for their setup. I, obviously, say NAY to that! Let’s see what we can do on our own!

Be aware that this is rather complex project and a very dense one, at best. I recommend taking a swing at it only if you know what you are doing.

### Audio data type and its digital flow
Let’s have a look at what new concepts we will be using within this project.
 
#### PCM
PCM – pulse code modulation - is a type of PWM signal specifically designed to store audio data in digital format. It is a rather basic type of encoding where we capture/produce an incoming/outgoing audio signal’s “digital cross section” (well, the output of an ADC or the input of a DAC, to be exact). This cross section can be a 16/24/32-bit binary value, representing the bit depth of the PCM signal (16-bit, 24-bit or 32-bit PCM, respectively).

The captured/published binary data will be in the 2’s complement format. This is necessary since audio data is understood as negative values and not positive ones. (Apologies, I didn’t do any deep dive on audio signal properties so I can’t give an explanation on why that is. Let’s just say because “reasons”.)

PCM is distributed/captured on one or two channels, meaning that it can either be mono for one channel or stereo for two channels. An audio frame is thus concluded when both channels have been activated (i.e. a frame always includes all channels and they are called/handled in an alternating fashion).

Apart from the two parameters above, the PCM signal also has something called a “sampling rate”. This sampling rate is NOT the same as the frequency of bit capture, alluded to above. Instead, sampling rate is the frequency at which we capture an entire frame of audio data… so the entirety of the 16/24/32 bits on one/two channels, depending on format. In practical sense, this means that while the capture frequency (frequency of capturing one bit) will be in the range of MHz, the sampling rate will be in the range of kHz. Using math, sampling rate will be bit capture frequency divided by channel number divided by PCM data bit depth.

Now, most off the shelf MEMS microphones don’t produce a PCM output, but something called PDM – pulse-density modulation - which is similar to PCM but follows a different sampling philosophy. PDM is then usually converted to PCM by a specialised peripheral (it is called SAI – serial audio interface – in STM32s).

Our L053R8 MCU does not have this peripheral, so we need to use a MEMS microphone that will be able to already provide us with a PCM output on something called an “I2S” bus.

#### I2S
I2S is a Philips standard just like I2C. It is used to transfer digital audio data (usually, but not restricted to, PCM) between ICs.

It runs on 3 pins, which are:
- WS (words select): the input that selects/defines the channel of the data we are transferring at that moment
- BCLK (bitclock): a few MHz clock signal that sets the speed of the bus and directly (!) adjusts the sampling rate of the audio on both capture or transfer. This must be very accurately set.
- DIN/DOUT (data): the data pin where the provide the, you guessed it, data
Now, there are no separate I2S peripherals on an MCU, instead the SPI peripheral can sometimes be recalibrated to run on the specifications provided above. This means that when we are using I2S, we will be sharing pins as well as multiple registers with an SPI peripheral. Of note, just lie with SPI, we can change the clock polarity and the clock phase, so one has to be aware what the demands are of the ICs on the I2S bus or we won’t be able to communicate with them.

FIn the L053R8 we are using for this project, only the SPI2 peripheral is I2S capable.

We will be generating the BCLK from the APB1 clock using the peripheral’s internal prescaler. (This is NOT the same for other MCUs like the F429 where the I2S has its own designated PLL. As such, the I2S driver we will have here will not be directly compatible with the other MCUs from ST.) Also, the BCLK of the I2S bus will directly define the sampling rate of the PCM data and thus must (!) be adjusted according to the demands of the audio format.

I2S can be used to transfer PCM data in packages of 16-bit or 32-bit. This means that the I2S packet size may not be the same as the PCM data package stored within it. This must be kept in mind during data processing…that is, one has to know very well, where the actual data is being stored with such a packet. (Here, for instance, the 24 bits of I2S data from the mic will be in MSB.)

Lastly, if one decides to use an MCU with the PLL on the I2S, CubeMx seems to not be able to set it up properly. If the wav file sounds weird, go and check the clocking of the mic for the culprit and then manually adjust the PLL values until the desired frequency is achieved.

#### Wav files
Wav files are used to store PCM data in an uncompressed format, similarly, to how BMP files are used to store uncompressed pixel (image) data. They are not efficient but very easy to generate and will just have to do for our particular usecase. The files will be stored on an SDcard.

### Hardware
On the input side, we will be using the SPH0645LM4H-1 MEMS microphone from , specifically, the Adafruit breakout board. On the output side, we will have the Adafruit Music Maker Feather board with the in-built amplifier. Both hardware elements are relatively cheap and will give us everything we need to record data and then drive any small speaker.

Let’s have a quick look at these hardware elements a bit more in detail.

#### Mic
I chose the Knowles mic since it is an I2S slave device and thus provides the digitalized audio output directly in I2S format. We don’t need to do any major processing on the data for our MCU to be able to deal with it.

This microphone will generate a 24-bit PCM signal, published in packages of 32 bits on one (!) channel. The WS pin of the I2S signal will be used to activate the mic, with the mic being depending on where the SEL pin of the mic is pulled by hardware. On the board I have, it is pulled to GND, so the mic will be active when WS is LOW, representing the “left” channel of the audio stream. Of note, it is possible to use the same I2S bus and two of these MEMS mics to capture a fully stereo audio, given the second mic’s SEL is pulled HIGH (so the two mic’s activation can be alternated by the WS signal). We will be using only one mic though, so this does not impact us. One mic meand one channel, thus a mono signal.

Of note, we should put a 100k resistor on the DOUT line of the mic to discharge any extra capacitance from the bus whenever we don’t have it activated. In our case, we will be discharging the bus when we have “activated” the unused right channel with the WS.

#### Codec and amplifier
The Maker has a VS1053B audio codec from VLSI solutions. It is a general audio codec that is capable to decode mp3, wma, ogg and wav. It is both controlled by and data-fed through SPI.

For the wav format, it demands that the PCM will be 16-bits and, at maximum, 48 kHz of sampling rate. This means that we will need to downsample the 24-bit PCM signal we get from the microphone to 16-bits using code so as to be compliant with the demands of the codec.

The output of the codec are two lines of data that can be directly fed into the amplifier. Just a note, it is also possible to use the codec as an I2S slave but in this case an external DAC would need to be connected to this  output I2S bus. (We won’t be using this option.)

The amplifier is a TPA2012 which is already hooked up to our codec within the Music Maker. We will be using it at the default 6 dB gain which is enough to turn the codec’s output into something audible on both a 4 ohm and an 8m ohm speaker. All in all, we will not need to bother ourselves with the amp.

There are just two small things:
1) The output of the amp is AC (a 50% PWM square wave) which will hide the audio signal when probed by an oscilloscope. One must deactivate the amp to debug the output of the codec using a scope.
2) Within the Music Maker board, the amp is powered separately (!) to the codec. This means that power will need to be provided to the BAT or the USB pins on the Maker to activate the amplifier.

#### SDcard
The SDcard shares its SPI bus with the codec on the Music Maker board. This is a limitation that must be respected since we won’t be able to read from the SDcard while we are publishing data to the codec (so, no peripheral-to-peripheral DMA transfer for us). This also means that I will be using blocking functions most of the time.

## Previous relevant projects
On how to set up the microphone and what pits to avoid, I recommend this project:
- A USB microphone for online meetings | Andys Workshop (andybrown.me.uk)

And for playing back a wav file, this one:
- WAVEPLAYER using STM32 || I2S AUDIO || CS43L22 || F4 DISCOVERY (youtube.com)

## To read
There is a lot of reading involved for this project due to all the new elements we will be using.

For the I2S specs, I recommend looking at this:
- I2S bus specification (nxp.com)

I also suggest going through a general description on how to construct a wav file: 
- https://learn.microsoft.com/en-gb/archive/blogs/dawate/intro-to-audio-programming-part-2-demystifying-the-wav-format

Some sample files can be found here for testing purposes:
- https://samplelib.com/sample-wav.html

Audacity is a free tool to look at audio files and format them to the desired output:
- https://learn.adafruit.com/microcontroller-compatible-audio-file-conversion/check-your-files

As usual, the available Adafruit information on both the mic and the Music Maker is golden (datasheets for all ICs can be found here as well):
- Overview | Adafruit I2S MEMS Microphone Breakout | Adafruit Learning System
- Overview | Adafruit Music Maker FeatherWing | Adafruit Learning System

Lastly, validating the file we generate using a hex reader:
- HexEd.it - Browser-based Online and Offline Hex Editing

## Particularities
### SDcard
Something I have run into when I have ported my SDcard driver from the F429 was that, well, it did not work. It took me a lot of time to figure this one out but, apparently, the OSPEED values for the GPIOs on the F429 are reset (or generally are set as) to be significantly higher than the reset values of the L053R8. In other words, the OSPEED for the SPI GPIO on the F429 can be left untouched and the F429 will be able to communicate with the SDcard without a problem. For the L053R8 though, it MUST be set to HIGH_SPEED, otherwise the MCU will not be able to meet the timing demands of the card, neither at the kHz init speed, nor at MHz data transfer speed.

Another issue is with the mutual SPI bus. Since the codec and the card are on the same SPI bus, we will have to alternate between talking to one and the other during data streaming. This means that a data buffer – and, for good practice, an adequately-sized ping-pong one – will need to be defined.

Lastly, we will have the problem with sector changes. Reading data out from an SDcard is fast, unless the data is on the boundary of two sectors. SDcards process their data in sectors (for a fatfs compatible 32 GB one, it will be 512 bytes) and changing from one sector to the next takes relatively lot of time. Multiple milliseconds to be exact. As such, during the streaming of our data, we must always be sure that we have at least a few milliseconds worth of data stored locally on the MCU to bridge any kind of hole within the input side of the stream.

I generally try working in sectors when dealing with an SDcard to always have the same amount of hole in the data stream. If a full sector might take too much RAM, half a sector generally also works well. Just to give an estimate, loading a full sector will always lead to the same amount of delay on the input which will  be, at 4 MHz SPI, around 7 ms.

Just a note: I am not sure, why, but my code may lead to Windows freezing when it is attempting to read the card using certain microSDcard readers. I have not had a problem with this when using the MCU or Linux to read the card, plus the problem was also removed when I changed the card reader to something more professional (official SanDisk reader). This may just be a fatfs compatibility problem with some readers. Formatting the card in Windows does recover ithe card though no matter the reader, so…a mystery.

### Finding the right timing
As mentioned above, we will have a few milliseconds of delay in our data stream after each sector transfer. We can decrease this delay by clocking the SPI at as high a speed as possible, which we can easily do by clocking our SPI up to 8 MHz (see baud rate prescaling from the APB2 clock). This will push the delay down to about 4 ms.

Now, whatever amount of audio data is stored in 512 bytes, it must be more than this approximately 4 ms, otherwise we will run out of data in the stream during a sector change. We know that we will have mono PCM 16-bit data type. These two information should be enough to define the sampling rate. After a bit of calculation, we can conclude that 44100 Hz sampling rate will be just too fast and we won’t be able to bridge the gap (512 bytes of data is roughly 3.5 ms of mono 16-bit PCM audio data). Luckily though, we won’t need to sacrifice too much of the quality since 22050 Hz will do the trick comfortably (7 ms of audio data).

Thus, to comply to all demands of hardware and interface, we will have to generate 16-bit PCM mono 22050 Hz audio files.

### Ride the wav
I have found the wav file specifications to be somewhat confusing since the naming convention is just not intuitive whatsoever. Eventually, I did manage to figure it out by investigating some of the file I have generated from existing samples using the Audacity software (link above in the “To read” section) and read the links shared above.

Let’s look at the header.

- We have “RIFF”, “WAVE”, “fmt “ and “data” elements which are always the same and are always at the same place within the header, so they don’t need any explanation.

- “FileLength” and the “Datachunksize” section can be left as “0xFFFFFFFF” if we are lazy. The codec would not mind, but a PC might if you intend to extract the wav files afterwards from the SDcard. Anyway, these values should be exactly the number of data bytes we have captured for “datachunksize” and “datachunkzise + 44 bytes (header size)” for the “FileLength”. It is obviously possible to edit the file afterward the data has been captured to implement the exact file size values, though I have omitted that here.

- “fmtchunksize” is 0x10000000, which – due to the endian swap - will represent an “fmtchunksize” of 16 bits. We can ignore this, I think it is a constant since it seems to describe the package sizes the decoder should read from the file when it is going through the header.

- “FormatTag” and “channelnumber” sections should be self-explanatory. PCM format will be 0x0100 and mono channel will be 0x0100 (both values being “1”, just endian swapped).

- “Samplingrate” should be the sampling rate, while “byterate” should be the frequency at which one byte is to be transferred/recorded. For example, if we have 16-bit PCM data at 22050 Hz, then one byte will have to be transferred at twice that speed (44100 Hz) to sustain the constant sampling rate of 22050 Hz.

- “Blockalign” describes how many bytes we have in the channel. In case of 16-bit PCM, this will be 2 (0x0200).

- “Bitspersample” will be 0x1000 (or 16) for PCM 16-bit.

And that’s pretty much it. Now, please be aware that the example above is for a 16-bit mono audio file which is arguably rather simple. When we have more bits, the “byterate” as well as the “bitspersample” will need to be adjusted accordingly. This can become especially complicated when we are to transfer 32 bits in a channel but with only 24 bits of audio data stored in each package. To be honest, I recommend generating the desired wav output type using Audacity and then copying the hex values from that file’s header to avoid any mistakes. If the wav file is not set up correctly, the output will be very dodgy in quality or would not be there at all when being read out with the codec.

### Mic shenanigans
As mentioned before, the mic we are using is an I2S slave device. We also know already that the sampling rate of our data will need to be 22050 Hz and we will be using only one channel.

In practical sense, once we have set up the I2S bus properly (BCLK will be 1.4 MHz or so, not 22050 Hz!), we will be seeing something like “0xFB28C000” for the left channel (WS is LOW) and “0x00000000” for the right channel (WS is HIGH) when we check the DOUT of the mic with an oscilloscope. Despite how awkward this may seem, it is actually correct:
1) Yes, one frame will be 64-bits since the I2S will be 32-bit wide for each channel and the mic “assumes” that it is part of a stereo setup to comply to the I2S standard.
2) The right channel will have nothing in it since our mic is only active on the left channel (SEL is pulled LOW on the breakout board).
3) On the left channel, we will have only 18 bits changing out of the 32 that are designated for that channel. This is because the mic generates 24-bit PCM data of which the 6 LSBs are kept as “0” (see mic’s datasheet). The additional 8 bits that are also “0” since the I2S bus can only work with 16- or 32-bit packages on the channel thus the mic expands the 24 bits with some dummy data to 32 bits.
4) The output is in 2’s complement format.
5) The output is LSB first, so we have an endian swap within the frame that must be undone during processing.

First and foremost, the start of the mic audio stream (activation of I2S) should be aligned to the start of the data capture. If it is kept asynchronous – because we want the mic to be active before we start capturing – we aren’t ensured that the data processing would occur on the right part of the captured frame. (Yes, this can be mended by some additional processing, but I want to keep things simple here.)

With all that considered, we can set up a DMA to channel these incoming 64-bit frames into an input buffer. Watch out that the I2S is transferring 16 bits on one transfer, so the DMA will need to be dealing with 16-bit wide transfers on both the peripheral and the memory sides. The input (ping-pong) buffer is set up to be 512 half-words, which will then be exactly 128 full frames. The transfer width of the DMA should be reset between recordings to ensure that the buffer is always loaded full. (CNDTR register does not reset in case we interrupt the DMA.)

These 128 frames will then be processed – endian swapped and downsampled to 16-bits – before being written to the SDcard in chunks of 256 bytes (so one sector change will occur after every 2 transfers; this is to save a bit on RAM use). We are using the DMA’s TC and HT flags to time the transfer and ensure that while we are working on the SDcard, the DMA will still be able to load the data into the input buffer.

Optionally, during processing here, we can add software amplification to the data by simply multiplying the 16 bits we have selected by a constant value (easy trick is to use the “<< 3” to multiply by 8, for instance). This is a rather crude way to amplify since we will be amplifying noise with the signal. Nevertheless, my experience suggests that some amplification is recommended, otherwise the codec and the amp within the Music Maker may not be able to provide a loud enough output. One could also potentially add some noise filtering here.

On the input side of the process, we are using the fatfs “f_write” function with “file_name” as the key parameter. We will use later in the readout part the “file_name” to find the position of the file within memory using fatfs commands.

Here within the audio capture function is where we will be defining the length of our file. The easiest is to just set a standard length for each captured file and have all of them be the same length. Otherwise, it must be respected that the “recording over” input from, say, a button, will be asynchronous to the data transfer and should not take effect until the half-page data transfer is concluded. Also, for a proper wav file, a byte counter should be implemented and the appropriate wav header values updated accordingly as the recording concludes.

### Codec shenanigans
#### Speeding on dual busses
The VS1053 codec has two SPI busses: one for the commands (SCI) and one for the data (SDI). These busses have their own CS pins, activating depending on which pin is pulled LOW (active LOW pins!!!) They also must be regularly toggled to allow the codec to synchronize with the busses.

Just like the SDcard, the codec also is a bit of a mess regarding speed. The SPI speed cannot exceed CLKI/4 for write on the SCI/SDI bus and CLKI/7 for read on the SCI bus (see datasheet). Since on the Music Maker, the starting CLKI value is 12 MHz thanks to an external oscillator, the SPI clocking cannot initially exceed 3 MHz and 1.7 MHz. Luckily, during the configuration of the codec, we can write to the CLOCKF register and activate the chip’s internal PLL, multiplying the input clock.

Since I wanted to keep the 8 MHz SPI for the codec as well as the SDcard, I multiplied the input by x3.5, putting CLKI to 42 MHZ. That allowed 10.5 MHz maximum SPI speed for write processes. For reads, each time I want to do it, I manually slow down the SPI back to the original 400 kHz.

Generally, we don’t need the read action on the SCI bus when everything is going fine, except for after an audio file has been fully sent over. There we need to check the state of the CANCEL bit within the codec’s MODE register to ensure that everything is properly reset. We also need to read the “ENDFILLBYTE” from the codec’s own memory, though for our particular case here, it will be “0x0” for the wav file and thus can simply be ignored.

Of note, there are a whole bunch of useful registers that could be checked to ensure that everything is going well with the codec. Unfortunately, due to the speed change and the SCI/SDI using the same SPI peripheral on the MCU, this cannot be done during audio streaming. I have left the debug functions in the file, though they should be used considering the limitations I have described above.

Ohh, one more thing: if someone wishes to do an SDI test (just to generate a sine wave output on the codec), the codec MUST BE hard reset prior! This can only be done on the Music Maker using the reset button without doing any soldering or trace cutting, a software reset would not do the trick.

#### DREQ
One of the most important elements within the entire codec drive sequence is the DREQ output. This is a pin on the codec which goes LOW whenever the codec is busy executing an SCI command OR when the 2048-byte FIFO on the SDI bus is, I quote the datasheet, “too full”. For the record, the very highly specific description of “too full” covers 32 bytes away from being full.

During SCI commands, the DREQ should be checked before every command to ensure that the codec is not busy with something else. The DREQ should also be checked during SDI data transfer to ensure that we aren’t overrunning the FIFO.
As recommended by the datasheet, I am checking the DREQ between each 32 bytes of SDI data transfers. This is enough since the FIFO’s input side has a small 32-byte buffer in addition to the 2048 bytes, thus the moment the DREQ is triggered, we can still comfortably send another 32 bytes to the codec. Of note, we WILL trigger the DREQ during data transfer since we will be loading the chip faster than how fast it is going to process it. Not doing so would mean implementing “holes” in the audio output stream when the FIFO inevitably runs empty.

I have put the DREQ on an EXTI, just to be sure that any change in it would be detected at the highest priority. Unlike general push button EXTIs, this DREQ EXTI is activated on the falling edge of the signal. (Technically, it should be on the rising edge for SCI and the falling edge for SDI, but leaving it on only the falling edge seems to work anyway…)

The DREQ flag generated by the EXTI is reset at the start of every SCI command. It is also manually reset around SDI data transfers.

#### Transferring data
I have decided not to rely on fatfs for the readout of the file. There were multiple parameters in the “FIL” struct – the struct that stores the file parameters, such as where the file’s own read/write pointer is placed – that I could not make sense of. As such, I have decided to extract the position of the file in memory by checking where the file pointer is pointing during an “OPEN_APPEND” open parameter (i.e. where the last byte of the file is in memory) and then calculated from there, where the first byte needs to be. This then allowed me to do a sector readout using memory positions (sector addresses) instead of moving the fatfs file pointer (which, by the way, refused to do what I wanted it to do…). The “file_name” parameter was used to do all this investigation.

The same timing limitations apply as on the input side, meaning that we will be reading out from the SDcard one sector at a time and then have the SDcard’s sector change time to deal with. As a safety measure, this demands a 1024 ping-pong buffer during SDI transfer, so we always have at least a full sector in memory to send to the codec. Just imagine filling the FIFO up when we only have one 32-byte chunk left in memory. That won’t cover the millisecond-sized that delay the sector changes would present, leading to a “hole” again in the audio stream.

To be fair, adding a ping-pong buffer is a bit of an overkill here. The limitation above only applies in case we have the codec and the SDcard on two different busses that could run parallel to each other using DMA. This is not the case here since we have both the codec and the SDcard on the same SPI bus. Also, initially I did not know what the “too full” would mean and assumed that the FIFO would run completely empty before we were allowed to publish into it again. Also-also, a fully loaded FIFO should worth about 10 ms of audio data, so no need to rush whatsoever. We could just read out from the SDcard while the codec is busy emptying its FIFO.

## User guide
We have two push buttons (the second should be added externally, the first is the blue push button of the nucleo). While we push the first, we record a 16-bit 22050 Hz mono wav file through the mic. 

The blue push button has a crude debouncing element in it with its EXTI activate on both the falling and rising edge of the pin, effectively detecting both button pushes and releases. Recording stops when the button is released.
We can push the blue button 5 times to record 5 different wav files, afterwards the recording section stops working and no new files can be captured.

Pushing the second button cycles through these wav files, playing back one after the other upon each button push.

There is no interrupt in the playback so once we started playing. We will have to wait until it ends.

The generated wav files are removed when the code is reset. In case these recorded files are to be extracted from the card, the card should be removed from the setup before the code is reset.

## Conclusion
Obviously, this isn’t the most practical dictaphone one could build since it lacks filtering, flexible playback functions and some form of visual feedback to offer the user some understanding, what file they are playing back. Nevertheless, with a little bit of extra work, it is possible to turn it into a fully capable one. The purpose of these project is to showcase a functionality, not to develop a fully functional product, so I leave that kind of refining to anyone interested. Let me know, how it went!

