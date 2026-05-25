/*
	written by Missingno_force a.k.a. Missingmew
	Copyright (c) 2014-2018
	see LICENSE for details
*/

#include "kojimasound.h"

typedef struct {
	uint32_t offset;
	char s_note; //sample note
	char s_tune; //sample tune
	unsigned char adsr[10];
}__attribute__((packed)) sampleTableEntry;

typedef struct {
	char magic[4];
	unsigned char version[4];
	unsigned char reserved1[4];
	unsigned char size[4];
	char samplerate[4];
	unsigned char reserved2[12];
	char name[0x10];
}__attribute__((packed)) vagHeader;

//A is defined as 450Hz for some reason
unsigned int freq_tbl[]	= {												//ratio here!!!!
// C0   C#0    D0   D#0    E0    F0   F#0    G0   G#0    A0   A#0    B0  1.0594631*/
  267,  283,  300,  318,  337,  357,  378,  401,  425,  450,  477,  505, /* C0 */
  535,  567,  601,  637,  675,  715,  757,  802,  850,  901,  954, 1011, /* C1 */
 1071, 1135, 1202, 1274, 1350, 1430, 1515, 1605, 1701, 1802, 1909, 2022, /* C2 */
 2143, 2270, 2405, 2548, 2700, 2860, 3030, 3211, 3402, 3604, 3818, 4045, /* C3 */
 4286, 4541, 4811, 5097, 5400, 5721, 6061, 6422, 6804, 7208, 7637, 8091, /* C4 */
 8572, 9082, 9622,10194,10800,11442,12122,12844,13608,14416,15276,16182, /* C5 */
/* overflow handling (forces extra check when calculating the sample rate) */
17144,18164,19244,                           11,   12,   13,   14,   15, /* C-5 */
   16,   18,   19,   20,   21,   22,   24,   25,   27,   28,   30,   32, /* C-4 */
   33,   35,   38,   40,   42,   45,   47,   50,   53,   56,   60,   63, /* C-3 */
   66,   70,   75,   79,   84,   89,   94,  100,  106,  112,  119,  126, /* C-2 */
  133,  141,  150,  159,  168,  178,  189,  200,  212,  225,  238,  252, /* C-1 */
  267	//shouldn't be reached normally
};

unsigned char vagEnd[0x10] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void vagPrepareHeader( vagHeader *header, uint32_t size, uint32_t samplerate ) {
	header->version[3] = 0x20;
	memset( header->name, 0, 0x10 );
	
	header->size[0] = (size >> 24) & 0xFF; 
	header->size[1] = (size >> 16) & 0xFF; 
	header->size[2] = (size >> 8)  & 0xFF; 
	header->size[3] =  size 	   & 0xFF;
	
	header->samplerate[0] = (samplerate >> 24) & 0xFF; 
	header->samplerate[1] = (samplerate >> 16) & 0xFF; 
	header->samplerate[2] = (samplerate >> 8)  & 0xFF; 
	header->samplerate[3] =  samplerate        & 0xFF;
	
	memset( header->reserved1, 0, 4 );
	memset( header->reserved2, 0, 12 );
	strncpy( header->magic, "VAGp", 4 );
	return;
}

void processWvx( FILE *f, unsigned int baseoffset, char *folder, unsigned int numSamples, unsigned int wvxgame ) {
	FILE *o;
	unsigned int i, offsub, samplerate, vagsize, psx = 0;
	char filename[1024];
	unsigned char vagbuf[0x10];
	sampleTableEntry sampleentry;
	vagHeader vagheader;
	
	switch(wvxgame) {
		case mgs1: {
			psx = 1;
			break;
		}
		case zoe1:
		default: {
			break;
		}
	}
	for( i = 0; i < numSamples; i++ ) {
		fseek(f, baseoffset+0x10+(i*0x10), SEEK_SET);
		
		sprintf(filename, "%s/%08d-%08x.vag", folder, i, i);
		if( !(o = fopen( filename, "wb" ))) {
				printf("Couldnt open file %s\n", filename);
				return;
		}
		
		memset(&sampleentry, 0, 0x10);
		fread(&sampleentry, 0x10, 1, f);
		printf("Sample offset: %08x - Micro: %02x, Macro: %02x\n", 
			sampleentry.offset, sampleentry.s_tune & 0xff, sampleentry.s_note & 0xff);
		if( i == 0 ) offsub = sampleentry.offset;
		sampleentry.offset -= offsub;
		if(psx) {
			if(sampleentry.s_note != 0 && sampleentry.s_tune != 0) samplerate = 22050;
			else samplerate = 11025;
		}
		else {
			//if(sampleentry.flags > 0x7F000000) {
			if(sampleentry.s_note != 0 && sampleentry.s_tune != 0){
				
				//so we don't need to use its full name everytime
				char macro = sampleentry.s_note; 
				unsigned char tune = (unsigned char) sampleentry.s_tune;

				unsigned char note = 0x30 + macro; //combine everything
				note &= 0x7f; //enforce limit
				
				unsigned int freq = freq_tbl[note]; //get note frequency
				if(freq != 19244){ //special case check
					freq = freq_tbl[note+1] - freq;
				}else{
					freq = freq*0.0594631; //used ratio between notes minus 1
				}
				
				if ((freq & 0x8000) != 0) freq = 0xc9;
				
				unsigned char pl, ph;
				
				/* Split the frequency (1 word) into (2 bytes) for calculation */
				pl = (unsigned char) freq&0xff;
				ph = (unsigned char)(freq >> 8);
				
				freq = ((pl * tune) >> 8) + (ph * tune);   /* Tuning * Semitone frequency */
				freq += freq_tbl[note];		/* Add the frequency corresponding to the note to <freq> */
				
				//vgmstream doesn't seem to like when the .vag's sample rate is too high, I wonder why?
				//using 3761 as 44100Hz, likely incorrect but the error is small enough
				samplerate = freq*44100.0/3761; 
			}
			else samplerate = 22050;
		}
		fseek(f, baseoffset+0x20+(numSamples*0x10)+sampleentry.offset, SEEK_SET);
		
		memset(&vagheader, 0, 0x30);
		fwrite(&vagheader, 0x30, 1, o);
		vagsize = 0x30;
		memset(vagbuf, 0xFF, 0x10);
		while(1) {
			if(!fread(vagbuf, 0x10, 1, f)) break;
			if(!memcmp(vagbuf, vagEnd, 0x10) && (vagsize!=0x30)) break;
			fwrite(vagbuf, 0x10, 1, o);
			vagsize+=0x10;
		}
		vagPrepareHeader(&vagheader, vagsize, samplerate);
		fseek(o, 0, SEEK_SET);
		fwrite(&vagheader, 0x30, 1, o);
		fclose(o);
	}
	return;
}
