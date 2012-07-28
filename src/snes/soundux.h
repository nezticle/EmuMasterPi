/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */
#ifndef SNESSOUND_H
#define SNESSOUND_H

#include "snes.h"
#include "port.h"
#include "snes9x.h"

static const int SoundSampleRate = 44100;

extern void snesSoundSl();

enum { SOUND_SAMPLE = 0, SOUND_NOISE, SOUND_EXTRA_NOISE, SOUND_MUTE };
enum { SOUND_SILENT, SOUND_ATTACK, SOUND_DECAY, SOUND_SUSTAIN,
       SOUND_RELEASE, SOUND_GAIN, SOUND_INCREASE_LINEAR,
       SOUND_INCREASE_BENT_LINE, SOUND_DECREASE_LINEAR,
       SOUND_DECREASE_EXPONENTIAL};

enum { MODE_NONE = SOUND_SILENT, MODE_ADSR, MODE_RELEASE = SOUND_RELEASE,
       MODE_GAIN, MODE_INCREASE_LINEAR, MODE_INCREASE_BENT_LINE,
       MODE_DECREASE_LINEAR, MODE_DECREASE_EXPONENTIAL};

#define MAX_ENVELOPE_HEIGHT 127
#define ENVELOPE_SHIFT 7
#define MAX_VOLUME 127
#define VOLUME_SHIFT 7
#define VOL_DIV 128
#define SOUND_DECODE_LENGTH 16

#define NUM_CHANNELS    8
#define SOUND_BUFFER_SIZE (2*44100/50)
#define MAX_BUFFER_SIZE SOUND_BUFFER_SIZE

#define SOUND_BUFS      4


typedef struct {
	bool8 mute_sound;
    int noise_gen;
} SoundStatus;

EXTERN_C SoundStatus so;

typedef struct {
    int state;
    int type;
    short volume_left;
    short volume_right;
	u32 hertz;
	u32 frequency;
	u32 count;
    bool8 loop;
    int envx;
    short left_vol_level;
    short right_vol_level;
    short envx_target;
	u32 env_error;
	u32 erate;
    int direction;
	u32 attack_rate;
	u32 decay_rate;
	u32 sustain_rate;
	u32 release_rate;
	u32 sustain_level;
    signed short sample;
	s16 decoded [16];
    signed short *block;
	u16 sample_number;
    bool8 last_block;
    bool8 needs_decode;
	u32 block_pointer;
	u32 sample_pointer;
    int *echo_buf_ptr;
    int mode;
	s32 envxx;
    signed short next_sample;
	s32 interpolate;
	s32 previous [2];
	// notaz
	u8 env_ind_attack;
	u8 env_ind_decay;
	u8 env_ind_sustain;
	//I'll use Fatl's recovery on savestates.
	short gaussian[8];
	int   g_index;
	unsigned short last_valid_header;
} Channel;

typedef struct
{
    short master_volume_left;
    short master_volume_right;
    short echo_volume_left;
    short echo_volume_right;
    int echo_enable;
    int echo_feedback;
    int echo_ptr;
    int echo_buffer_size;
    int echo_write_enabled;
    int echo_channel_enable;
    int pitch_mod;
    Channel channels [NUM_CHANNELS];
    bool8 no_filter;
    int master_volume [2];
    int echo_volume [2];
    int noise_hertz;
} SSoundData;

EXTERN_C SSoundData SoundData;

void S9xSetEnvelopeHeight (int channel, int height);
void S9xSetSoundKeyOff (int channel);
void S9xSetSoundDecayMode (int channel);
void S9xSetSoundAttachMode (int channel);
void S9xSoundStartEnvelope (Channel *);
void S9xSetSoundSample (int channel, u16 sample_number);
void S9xSetEchoDelay (int byte);
void S9xResetSound (bool8 full);
void S9xFixSoundAfterSnapshotLoad ();
void S9xPlaybackSoundSetting (int channel);
void S9xFixEnvelope (int channel, u8 gain, u8 adsr1, u8 adsr2);
void S9xStartSample (int channel);

EXTERN_C void S9xMixSamples (signed short *buffer, int sample_count);
void S9xSetPlaybackRate();
EXTERN_C bool8 S9xInitSound (void);

// notaz: some stuff from soundux.cpp to enable their inlining
#include "spu.h"
//#define DEBUG
//#include <dprintf.h>

extern int Echo [24000];
extern int Loop [16];
extern int FilterTaps [8];
extern int EchoBuffer [SOUND_BUFFER_SIZE];
extern int NoiseFreq [32];

// precalculated env rates for S9xSetEnvRate
extern unsigned long AttackERate     [16][10];
extern unsigned long DecayERate       [8][10];
extern unsigned long SustainERate    [32][10];
extern unsigned long IncreaseERate   [32][10];
extern unsigned long DecreaseERateExp[32][10];
extern unsigned long KeyOffERate[10];


#define FIXED_POINT 0x10000UL
#define CLIP8(v) \
if ((v) < -128) \
    (v) = -128; \
else \
if ((v) > 127) \
    (v) = 127

static inline void S9xSetSoundMute (bool8 mute)
{
    so.mute_sound = mute;
}

static inline void S9xSetEnvRate (Channel *ch, unsigned long rate, int direction, int target, unsigned int mode)
{
    ch->envx_target = target;

    if (rate == ~0UL)
    {
	ch->direction = 0;
	rate = 0;
    }
    else
	ch->direction = direction;


	if (rate == 0)
		ch->erate = 0;
    else
    {
		switch(mode >> 28) {
			case 0: // attack
			ch->erate = AttackERate[ch->env_ind_attack][ch->state];
			break;

			case 1: // Decay
			ch->erate = DecayERate[ch->env_ind_decay][ch->state];
			break;

			case 2: // Sustain
			ch->erate = SustainERate[ch->env_ind_sustain][ch->state];
			break;

			case 3: // Increase
			ch->erate = IncreaseERate[mode&0x1f][ch->state];
			break;

			case 4: // DecreaseExp
			ch->erate = DecreaseERateExp[mode&0x1f][ch->state];
			break;

			case 5: // KeyOff
			ch->erate = KeyOffERate[ch->state];
			break;
		}
	}
}

static inline void S9xSetEchoEnable (u8 byte)
{
    SoundData.echo_channel_enable = byte;
    if (!SoundData.echo_write_enabled || Settings.DisableSoundEcho)
	byte = 0;
    if (byte && !SoundData.echo_enable)
    {
	memset (Echo, 0, sizeof (Echo));
	memset (Loop, 0, sizeof (Loop));
    }

    SoundData.echo_enable = byte;
    for (int i = 0; i < 8; i++)
    {
	if (byte & (1 << i))
	    SoundData.channels [i].echo_buf_ptr = EchoBuffer;
	else
	    SoundData.channels [i].echo_buf_ptr = 0;
    }
}

static inline void S9xSetEchoFeedback (int feedback)
{
    CLIP8(feedback);
    SoundData.echo_feedback = feedback;
}

static inline void S9xSetFilterCoefficient (int tap, int value)
{
    FilterTaps [tap & 7] = value;
    SoundData.no_filter = (FilterTaps [0] == 127 || FilterTaps [0] == 0) && 
			   FilterTaps [1] == 0   &&
			   FilterTaps [2] == 0   &&
			   FilterTaps [3] == 0   &&
			   FilterTaps [4] == 0   &&
			   FilterTaps [5] == 0   &&
			   FilterTaps [6] == 0   &&
			   FilterTaps [7] == 0;
}

static inline u16 *S9xGetSampleAddress (int sample_number)
{
	u32 addr = (((APU.DSP[APU_DIR] << 8) + (sample_number << 2)) & 0xffff);
	return (u16 *)(IAPU.RAM + addr);
}

static inline void S9xSetSoundFrequency (int channel, int hertz) // hertz [0~64K<<1]
{
	if (SoundData.channels[channel].type == SOUND_NOISE)
		hertz = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
	SoundData.channels[channel].frequency = (hertz * ((FIXED_POINT << 11) / SoundSampleRate)) >> 11;
}

#endif // SNESSOUND_h
