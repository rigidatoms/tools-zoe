/*
	written by Missingno_force a.k.a. Missingmew
	Copyright (c) 2014-2018
	see LICENSE for details
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//~ #include "itreference/itstuff.h"
#include "kojimasound/kojimasound.h"
//~ #include <smf.h>

//this might be incorrect, as the frequency table for MGS2 wraps around after D6, and goes into negative octaves (!)
//the table itself has size of 129, where the last frequency is equal to the first one
//won't modify this yet, unless I find a case where negative octaves are actually used (I think it's unlikely)
char *midinotes[128] = {
	"C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
	"C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
	"C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
	"C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
	"C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
	"C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
	"C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
	"C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
	"C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",
	"C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "G#9", "A9", "A#9", "B9",
	"C10", "C#10", "D10", "D#10", "E10", "F10", "F#10", "G10"
};

unsigned int snap0(unsigned int data) {
	if(data < 26) return 5*data+0x80;
	else if(data < 52) return 5*data+0x8100;
	else if(data < 76) return 5*data+0x8180;
	else if(data < 102) return 5*data+0x8200;
	else if(data < 127) return 5*data+0x8280;
	else if(data < 153) return 5*data+0x8300;
	else if(data < 179) return 5*data+0x8380;
	else if(data < 205) return 5*data+0x8400;
	else if(data < 283) return 5*data+0x8480;
	else {
		printf("snap0: data is %d\n", data);
		return 0;
	}
}	

int main( int argc, char **argv ) {
	
	if ( argc < 3 ) {
		printf("Not enough args!\nUse: %s MDX-file game\n", argv[0]);
		printf("supported games are:\n%d (MGS1)\n%d (ZoE1 and MGS2HD)\n", mgs1, zoe1);
		return 1;
	}
	
	
	FILE *f, *o;
	char outputname[512] = {0};
	unsigned int game, tracklistsize, numtracks;
	unsigned int cursong, curtrack;
	unsigned int event, param0, param1, param2, note, velocity, length, snap;
	uint32_t numsongs, songoffset, *tracklist;
	uint32_t curtoken;
	game = strtoul(argv[2], NULL, 10);
	if(!(game<NUMWVXGAMES)) {
		printf("Unsupported game %d!\nRun without args to see supported types.\n", game);
		return 1;
	}
	
	
	switch(game) {
		case mgs1: {
			tracklistsize = 0x60;
			tracklist = malloc(0x60);
			break;
		}
		case zoe1: {
			tracklistsize = 0x80;
			tracklist = malloc(0x80);
			break;
		}
	}
	numtracks = tracklistsize / 4;
	
	if( !(f = fopen( argv[1], "rb" ))) {
		printf("Couldnt open file %s\n", argv[1]);
		return 1;
	}
	
	fseek(f, 0, SEEK_SET);
	fread(&numsongs, 4, 1, f);
	printf("Number of songs in MDX: %d\n", numsongs);
	
	for(cursong = 0;cursong < numsongs;cursong++) {
		fseek(f, 4+(4*cursong), SEEK_SET);
		fread(&songoffset, 4, 1, f);
		printf("Song %d - Offset: %#08x\n", cursong, songoffset);
		fseek(f, songoffset, SEEK_SET);
		fread(tracklist, tracklistsize, 1, f);
		for(curtrack = 0;curtrack < numtracks;curtrack++) {
			int tabs = 1, loop1 = 0, loop2 = 0, loop3 = 0, bracket = 0, fx_track = 0, 
				kakko_flag = 0;
			sprintf(outputname, "song%02d-track%02d.bin", cursong, curtrack);
			curtoken = 0;
			if( !(o = fopen( outputname, "wb" ))) {
				printf("Couldnt open file %s\n", outputname);
				return 1;
			}
			printf("Song %d - Track %02d - Offset: %#08x\n", cursong, curtrack, tracklist[curtrack]);
			fseek(f, tracklist[curtrack], SEEK_SET);
			
			/* smf stuffs */
			//~ track = smf_track_new();
			
			while((curtoken & 0xFFFF0000) != 0xFFFE0000) { //might need to change this to (curtoken & 0xFF000000) != 0xFF000000
								
				if (fread(&curtoken, 4, 1, f) == 0){
					break;
				}
				
				event = curtoken >> 24;
				param0 = (curtoken >> 16) & 0xFF;
				param1 = (curtoken >> 8) & 0xFF;
				param2 = curtoken & 0xFF;
				
				switch(event){
					case 0xDB:
						if(fx_track > 0) fx_track -= 1;
						break;
					case 0xE8:
						if (loop1 > 0) loop1 -= 1;
						break;
					case 0xEA:
						if (loop2 > 0) loop2 -= 1;
						break;
					case 0xEC:
						if (loop3 > 0) loop3 -= 1;
						break;
					case 0xEE:
						if (bracket > 0 && (kakko_flag == 1)) bracket -= 1;
						break;
				}
				
				tabs = 1 + loop1 + loop2 + loop3 + bracket + fx_track;
					
				for (int i = 0; i < tabs; i++){
					printf("\t");
				}
				
				if(event < 0x80 ) {
					/* possible midi note */
					note = event;
					snap = param0; //internal name is STEP
					length = param1; //internal name is GATE
					velocity = param2;
					printf("Note %#x(%s) === ", note, midinotes[note]);
					printf("steps:%#x - gate:%#x - vel:%#x\n", snap, length, velocity);
				}
				else {
					switch(event) {
						case 0xCD:{
							//for explanation on Automations, see command 0xF8
							printf("Automation 6 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xCE:{
							printf("Automation 7 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xCF:{
							printf("Automation 8 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xD0: {
							printf("Tempo Set at %#08x, params: 0x%02X\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xD1: {
							printf("Tempo Slide (++/--) at %#08x, params 0x%02X (steps) 0x%02X (target tempo)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						
						case 0xD2: {
							printf("Set Instrument A at %#08x, params 0x%02X (tone)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xD3: {
							printf("Set Instrument B at %#08x, params 0x%02X (tone)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xD4: {
							printf("Set Instrument C at %#08x, params 0x%02X (tone)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xD5: {
							printf("Track Volume Set at %#08x, params 0x%02X (vol)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xD6: {
							printf("Volume Slide at %#08x, params 0x%02X (steps) 0x%02X (target)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						
						case 0xD7: {
							//sets attack mode to linear
							//AR = 127 - (param0 & 0x7f)
							//DR = 15 - (param1 & 0x0f)
							//SL = (param2 & 0x0f)
							printf("ADS Envelope Set at %#08x, params 0x%02X(AR) 0x%02X(DR) 0x%02X(SL)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xD8: {
							//sets sustain mode to LINEAR DECREASE
							//SR = 127 - (param0 & 0x7f)
							printf("Sustain Rate Set at %#08x, params 0x%02X (SR)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xD9: {
							//sets release mode to linear
							//RR = 31 - (param0 & 0x1f)
							printf("Release Rate Set at %#08x, params 0x%02X (RR)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xDA: {
							printf("FX Track Start Set at %#08x, params 0x%02X (WaitMode) 0x%02X (WaitTimeBase) 0x%02X (addr)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							fx_track += 1;
							break;
						}
						
						case 0xDB: {
							printf("FX Track End Set at %#08x\n", (unsigned int)ftell(f)-4);
							break;
						}
						
						case 0xDC: { //to get the correct frame, multiply by 28/16
						unsigned int offset_addr = (param0 << 12) | (param1 << 4);
							printf("Sample Address Offset Set at %#08x, address:%#04x (PCM frame:%d)\n", (unsigned int)ftell(f)-4, offset_addr, (offset_addr>>4)*28);
							break;
						}
						
						case 0xDD: {
							//when mode == 0, if the current instrument is changed, the panning returns to default position
							//when mode == 1, panning remains in this set position, regardless of instrument change
							//when mode == 2, uses the panning set in the mixer
							unsigned int left, right;
							unsigned int panning = (param1 + 0x14) & 0xff; 
							if (panning > 40) panning = 40;
							left = 40 - panning, right = panning;
							
							printf("Panning Set at %#08x, params 0x%02X (mode) 0x%02X (phase) :: ", (unsigned int)ftell(f)-4, param0, param1);
							if(left == right) { printf("CENTER"); }
							else if (left > right) { printf("%d%% left", (int)((20-right)*5)); }
							else 				   { printf("%d%% right",(int)((20-left)*5));  }
							printf("\n");
							
							break;
						}
						

						case 0xDE: {
							
							unsigned int left, right;
							unsigned int panning = (param1 + 0x14) & 0xff; 
							if (panning > 40) panning = 40;
							left = 40 - panning, right = panning;
							
							printf("Panning Slide at %#08x, params 0x%02X (steps) 0x%02X (target) :: ", (unsigned int)ftell(f)-4, param0, param1);
							if(left == right) { printf("CENTER"); }
							else if (left > right) { printf("%d%% left", (int)((right)*5)); }
							else { printf("%d%% right", (int)((left)*5)); }
							printf("\n");
							
							break;
						}

						
						case 0xDF: {
							printf("Transpose Set at %#08x, params 0x%02X (value)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xE0: {
							printf("Detune Set at %#08x, params 0x%02X (value)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xE1: {
							//hold counter: how many steps the command must wait to be executed
							printf("Vibrato Set at %#08x, params 0x%02X (hold counter) 0x%02X(speed) 0x%02X (depth)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xE2: {
							printf("Vibrato Change at %#08x, params 0x%02X (change counter)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xE3: {
							printf("LFO Set at %#08x, params 0x%02X (speed) 0x%02X 0x%02X :: DEPTH == 0x%04X\n", (unsigned int)ftell(f)-4, param0, param1, param2, (param1 << 8) | param2);
							break;
						}
						
						case 0xE4: { //the command table had a placeholder for this command, and the actual method is called within the sound handler  
							printf("Slide to Note event at %#08x, params 0x%02X (hold counter) 0x%02X (speed) 0x%02X (target)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xE5: {
							printf("Sweep Set at %#08x, params 0x%02X (hold counter) 0x%02X (speed) 0x%02X (depth)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xE6: {
							printf("Portamento Set at %#08x, params 0x%02X (speed)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xE7: { //(inner block)
							printf("Set Block Loop Start event at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							loop1 += 1;
							break;
						}
						
						case 0xE8: {
							printf("Set Block Loop End (repeat PARAM0 times) at %#08x, params %02d (count) 0x%02X (add vol) 0x%02X (add freq)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xE9: { //so it seems that this is to allow block loops (outer) inside block loops (inner)
							printf("Set (Outer) Block Loop Start at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							loop2 += 1;
							break;
						}
						
						case 0xEA: {
							printf("Set (Outer) Loop End (repeat PARAM0 times) at %#08x, params %02d (count) 0x%02X (add vol) 0x%02X (add freq)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xEB: {
							printf("Set Song Loop Start at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							loop3 += 1;
							break;
						}
						
						case 0xEC: {
							printf("Set Song Loop End at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							break;
						}
						
						case 0xED: {
							printf("Set Loop Start (brackets) at %#08x, save current position\n", (unsigned int)ftell(f)-4);
							bracket += 1;
							kakko_flag = 0;
							break;
						}
						
						case 0xEE: {
							//this command works as follows:
							//when ED is used, it saves the current position on data, and sets a flag to 0
							//this bracket flag is then used by the EE command to dictate its behavior:
							//when flag == 0, flag += 1, end of operation
							//when flag == 1, saves this new position, moves the pointer to the position set by ED 
								//and flag += 1, end of operation
							//when flag == 2, moves the pointer to the position set when flag was 1, then flag -=1, end of operation
							printf("Set Loop End (brackets) event at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							if(kakko_flag < 2){
								kakko_flag += 1;
							}
							else{
								kakko_flag -= 1;
							}
							break;
						}
						
						case 0xEF: {
							printf("Start FX on separate track at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							break;
						}
						
						case 0xF0: {
							printf("Track-specific volume shift at %#08x, params 0x%02X (track index) 0x%02X (steps) 0x%02X (target)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xF1: {
							//ATTACK MODES:
							//		0: LINEAR
							//		else: EXPONENTIAL
							//SUSTAIN MODES:
							//		0: LINEAR DECREASE(?)
							//		1: EXPONENTIAL DECREASE(?)
							//		2: LINEAR INCREASE(?)
							//		else: EXPONENTIAL INCREASE(?)
							//RELEASE MODES:
							//		0: LINEAR
							//		else: EXPONENTIAL
							printf("Envelope Modes Set at %#08x, params 0x%02X (AM) 0x%02X (SM) 0x%02X (RM)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xF2: { //note off in this sequence format
							printf("Rest Set at %#08x, params 0x%02X (steps)\n", (unsigned int)ftell(f)-4, param0);
							break;
						}
						
						case 0xF3: { //its purpose is to extend the current note's duration
									//you can apply effects like vol slide or sweep during the tie duration too
							printf("Tie Set at %#08x, params 0x%02X (steps) 0x%02X (gate)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						
						case 0xF4: {
							//only ECHO (7) and DELAY (8) modes receive the parameters for delay and feedback
							//ROOM (1), STUDIO A/B/C (2,3,4), HALL (5), SPACE (6) and PIPE (9) remain with default values
							//any value outside of [1, 9] is treated as OFF
							//unless I'm remembering it wrong, default being used by the game is HALL
							printf("Echo Mode Set event at %#08x, params 0x%02X (mode) 0x%02X (delay) 0x%02X (feedback)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						
						case 0xF5: {
							printf("Echo Depth (Stereo) Set at %#08x, params 0x%02X (depth LEFT) 0x%02X (depth RIGHT)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						
						case 0xF6: { 
							printf("Echo ON at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							break;
						}
						
						case 0xF7: { 
							printf("Echo OFF at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							break;
						}
											//MIXER AUTOMATION COMMANDS
						case 0xF8: {
							//when using an automation command, the volume value set carries over
							//for the next automations, while the timers are either set to 0 or 0xff, 
							//depending on what mode is set when command 0xF8 is executed (0 or 1)
							//FOR EXAMPLE: the volume set with command 0xF9 (which is Automation 2 Set)
							//would also be applied to Automation 3-8 (copied over to the rest).
							//Automation 2 would have its timer set to whatever param1 is
							//and every other automation after 2 (so from 3 to 8) would be either 0 or 0xff
							//if I need Automation 3 to be different, it would require usage of command 0xFA
							//(by logic, that would overwrite the volumes for Automations 4 to 8 too)
							printf("Automation 1 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer) 0x%02X (mode)\n", (unsigned int)ftell(f)-4, param0, param1, param2);
							break;
						}
						case 0xF9: {
							printf("Automation 2 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xFA: {
							printf("Automation 3 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xFB: {
							printf("Automation 4 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xFC: {
							printf("Automation 5 Set at %#08x, params 0x%02X (volume) 0x%02X (position/timer)\n", (unsigned int)ftell(f)-4, param0, param1);
							break;
						}
						case 0xFD:{
							//this is seemingly a command to override a track playback with a memory stream
							//not clear how it's used yet, would need to find an example in a file
							printf("Tone Set MNO Set at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							break;
						}
						case 0xFE:{
							//another command I can't quite grasp fully
							printf("Flag Control Code at %#08x, ", (unsigned int)ftell(f)-4);
							switch(param0){
								case 0:{
									printf("set track flag for first-person mode to 0x%02X\n", param2);
									//flag to prevent volume from being lowered in first-person mode
									break;
								}
								case 1:{
									printf("override reverb value to 0x%02X\n", param2);
									//sets reverb based on track information regardless of SE mode
									break;
								}
							}
							break;
						}
						case 0xFF: {
							printf("End Of Track at %#08x, complete event %#08x\n", (unsigned int)ftell(f)-4, curtoken);
							break;
						}
						
						default: {
							printf("Unknown event %#x at %#08x\n", curtoken, (unsigned int)ftell(f)-4);
							break;
						}
					}
				}
				fwrite(&curtoken, 4, 1, o);
			}
			fclose(o);
		}
	}
	
	printf("Done.\n");
	
	fclose(f);
	return 0;
}
