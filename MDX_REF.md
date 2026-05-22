All commands in a sequence file (.mdx) are bundled in 4 bytes. A command may or may not use all bytes, but it's still necessary to write 4 bytes before the next command. When the command's first byte (big-endian) has a value below 0x80, it's considered a note input. The first byte is the note itself, and the next bytes are STEP (the length of the note), GATE (value between 0 - 100, serves as the percentage of the note's nominal length, so any value below 100 releases the note before its nominal ending) and VELOCITY (can be considered the volume of the note itself).

When the command's first byte has a value equal or above 0x80, it should be processed before a new note input is found or until the song reaches the end. The following three bytes are used as parameters. A command doesn't necessarily use all three, if any. Here follows every special command found in the MGS2 song format:

| Command (byte) | Parameter(s) <br>(3 bytes) | Description |
| :-------: | :-------: | ------------- |
| 0xCD | <ul><li>Volume</li><li>Position/Timer</li></ul> | Set **Automation 6**.<br> See command 0xF8 for further explanation on Automations. |
| 0xCE | <ul><li>Volume</li><li>Position/Timer</li></ul> | Set **Automation 7**.<br> See command 0xF8 for further explanation on Automations. |
| 0xCF | <ul> <li>Volume</li> <li>Position/Timer</li> </ul> | Set **Automation 8**.<br> See command 0xF8 for further explanation on Automations. |
|0xD0|<ul><li>Tempo</li></ul>| Set current track's **tempo**.|
|0xD1|<ul><li>Target tempo</li></ul>|Move/slide **tempo** to target value.|
|0xD2|<ul><li>Tone number</li></ul>| Set **current instrument** to given tone number.|
|0xD3|<ul> <li>Tone number</li> </ul>| (same as above)|
|0xD4|<ul> <li>Tone number</li> </ul>| (same as above)|
|0xD5|<ul> <li>Volume</li> </ul>| Set current track **volume**. (Note volume depends on its velocity, the track's own volume and the mixer volume for current track.)|
|0xD6|<ul> <li>Steps</li> <li>Target volume</li> </ul>|Move/slide current track **volume** to target value specified, during the given duration in steps.|
|0xD7|<ul> <li>AR</li> <li>DR</li> <li>SL</li> </ul>|Set the **ADSR**'s Attack Rate (`127 - (AR & 0x7F)`), Decay Rate (`15 - (DR & 0x0F)`) and Sustain Level (`SL & 0x0F`). Attack mode is set to Linear.|
|0xD8|<ul> <li>SR</li> </ul>|Set the **ADSR**'s Sustain Rate (`127 - (SR & 0x7F)`). Sustain mode is set to linear decrease.|
|0xD9|<ul> <li>RR</li> </ul>| Set the **ADSR**'s Release Rate (`31 - (RR & 0x1F)`). Release mode is set to linear.|
|0xDA|<ul> <li>Wait Mode</li> <li>Wait Time Base</li> <li>Address</li> </ul>|    Set **FX Track Start**.                         |
|0xDB|*-&emsp;None*|Set **FX Track End**.|
|0xDC|<ul> <li>Offset 1</li> <li>Offset 2</li> </ul>| **Sample Offset**, given by the following formula: <br>&emsp;&emsp;`address = (offset1 << 12) \| (offset2 << 4)`<br>In order to get the equivalent PCM frame, multiply address by 28/16.|
|0xDD|<ul> <li>Mode</li> <li>Phase</li> </ul>| Set current track **panning**. <br>&emsp;&emsp;When mode is 0, if the current instrument is changed, phase returns to default value. <br>&emsp;&emsp;When mode is 1, phase remains with current value when instrument is changed. <br>&emsp;&emsp;When mode is 2, phase receives the value set in the mixer. <br>Phase value goes from 0 to 40, where 20 is Center. (the parameter value actually goes from 0xEC to 0x14, or -20 to 20, from left to right, then the sound driver adds 0x14 before processing it)|
|0xDE|<ul> <li>Steps</li> <li>Target phase</li> </ul>| Move/slide **panning phase** to target value. Same principle as above.|
|0xDF|<ul> <li>Semitones (signed)</li> </ul>| Set **transpose** value as specified. Adds the amount the semitones to the notes to be played.|
|0xE0|<ul> <li>Detune value (signed)</li> </ul>|Set **detune** value to be applied on the notes.|
|0xE1|<ul> <li>Hold counter</li> <li>Speed</li> <li>Depth</li> </ul>|Set **vibrato** on. Hold counter specifies how many steps to wait before applying vibrato to note. When a new note is set, vibrato is set off.|
|0xE2|<ul> <li>Value</li> </ul>|Gradually change the vibrato depth, as specified by the given value.|
|0xE3|<ul> <li>Speed</li> <li>Depth 1</li> <li>Depth 2</li> </ul>| Set **LFO** to current track. Depth value is a combination of the last two parameters (`(depth1 << 8) \| (depth2)`).|
|0xE4|<ul> <li>Hold counter</li> <li>Speed</li> <li>Target note</li> </ul>|**Slide current note** being played to target note. Hold counter specifies how many steps to wait before applying the slide.|
|0xE5|<ul> <li>Hold counter</li> <li>Speed</li> <li>Depth</li> </ul>| Set **sweep**. Hold counter specifies how many steps to wait before applying the sweep.|
|0xE6|<ul> <li>Speed</li> </ul>| Set **portamento** speed.|
|0xE7|*-&emsp;None*| Start a **block loop**. Inside a block, commands will be executed as many times as specified by the end marker.|
|0xE8|<ul> <li>Count</li> <li>Volume (signed)</li> <li>Transpose (signed)</li> </ul>|Set the end marker for a block of commands, specifying how many times the block will be executed. For every time the block executes, volume can be added/subtracted, and the notes can be transposed, provided that the parameters are specified as non-zero.|
|0xE9|*-&emsp;None*|Similar to 0xE7, but should be treated as an **outer loop** (0xE7 block loops should be inside outer loops).|
|0xEA|<ul> <li>Count</li> <li>Volume (signed)</li> <li>Transpose (signed)</li> </ul>|Equivalent to 0xE8 for outer loops.|
|0xEB|*-&emsp;None*|Set the start of an **infinite loop**. Can be considered the song's actual loop.|
|0xEC|*-&emsp;None*|Set the end of the infinite loop.|
|0xED|*-&emsp;None*|Set the start of a special type of loop (mentioned in code as **brackets**), where a single loop may have at least two end markers (there is a case in ZOE2 where more than two is used, requires further investigation). Its corresponding flag is set to 0.|
|0xEE|*-&emsp;None*|Set an end marker to the brackets loop. Its behavior depends on the value of the brackets flag at a given moment: <br> &emsp; - When the flag is 0, it just increments to 1 and continues. <br> &emsp; - When the flag is 1, saves this new position, returns to the start of the loop, and increments the flag before continuing. <br> &emsp; - When the flag is 2, moves to the position set when flag was 1, and sets it back to 1.|
|0xEF|*-&emsp;None*|Described as a command to **"start FX on separate track"**.|
|0xF0|<ul> <li>Track index</li> <li>Steps</li> <li>Target volume</li> </ul>|Move/slide the volume of specified track to target value, during the given duration in steps.|
|0xF1|<ul> <li>Attack Mode</li> <li>Sustain Mode</li> <li>Release Mode</li> </ul>| Sets the **Envelope Modes** as follows: <br> <ul> <li>Attack Mode: <ul> <li>0: linear</li> <li>else: exponential</li> </ul></li> <li>Sustain Mode: <ul> <li>0: linear decrease</li><li>1: exponential decrease</li><li>2: linear increase</li><li>else: exponential increase</li> </ul></li> <li>Release Mode: <ul> <li>0: linear</li> <li>else: exponential</li> </ul></li> </ul>|
|0xF2|<ul> <li>Steps</li> </ul>|Set a **Rest** with the given length in steps in the current track.|
|0xF3|<ul> <li>Steps</li> <li>Gate</li> </ul>|Set a **Tie** with the given length in steps. A tie is used to extend a note's duration, if one is playing, beyond what was previously defined. The gate dictates if it actually plays the entire duration or if it only plays for a fraction of it (expected values from 0 - 100, as it works as a percentage).|
|0xF4|<ul> <li>Mode</li> <li>Delay (signed?)</li> <li>Feedback (signed?)</li> </ul>|Set the SPU's **Echo** configuration. Can either be *ROOM* (1), *STUDIO A/B/C* (2/3/4), *HALL* (5), *SPACE* (6), *ECHO*(7), *DELAY* (8) and *PIPE* (9). Any other value for Mode is considered as *OFF*. Only modes *ECHO* and *DELAY* receive the delay and feedback parameters.|
|0xF5|<ul> <li>Depth (LEFT)</li> <li>Depth (RIGHT)</li> </ul>|Set the **Echo depth** for the stereo channels.|
|0xF6|*-&emsp;None*|Set **Echo ON** for current track.|
|0xF7|*-&emsp;None*|Set **Echo OFF** for current track.|
|0xF8|<ul> <li>Volume</li><li>Position/Timer</li> <li>Mode</li> </ul>|Set **Automation 1**. <br>Using any automation command automatically fills the volume and position/timers to the other following automations (volume is copied, timer is set to either 0 or 255 depending on if mode is set to 0 or 1). <br>Automations are used to allow automatic changes to mixer state during gameplay, given an expected condition is met. This is used for dynamic music.|
|0xF9|<ul><li>Volume</li><li>Position/Timer</li></ul>|Set **Automation 2**.<br> See command 0xF8 for further explanation on Automations. |
|0xFA|<ul><li>Volume</li><li>Position/Timer</li></ul>|Set **Automation 3**.<br> See command 0xF8 for further explanation on Automations. |
|0xFB|<ul><li>Volume</li><li>Position/Timer</li></ul>|Set **Automation 4**.<br> See command 0xF8 for further explanation on Automations. |
|0xFC|<ul><li>Volume</li><li>Position/Timer</li></ul>|Set **Automation 5**.<br> See command 0xF8 for further explanation on Automations. |
|0xFD|<ul> <li>Tone number</li> <li>Wave index (MSB)</li> <li>Wave index (LSB)</li> </ul>|Specify **tone for memory streaming**. The actual tone number is 0x100 + the tone number specified in the command. <br>(Wave index is only used if parameter 1 equals 0xFF and parameter 2 & 0xF0 equals 0xF0, then the bytes are combined as follows: `wave_index_LSB \| ((wave_index_MSB & 0xF) << 8))`. This is used for an address override.)|
|0xFE|<ul> <li>Flag</li> <li>Value</li> </ul>|**Flag Control Code.** When flag is set to 0, set track flag for first-person mode to specified value. When flag is set to 1, override reverb value to specified value, disregarding the current SE (Sound Effect) mode.|
|0xFF|*-&emsp;None*|Set **End of Track**.|

&emsp;\* If any of this is incorrect, feel free to add corrections to this table (and the parser as well). Some functions are disabled for MGS2HD, making it more difficult to evaluate how exactly the code should behave.