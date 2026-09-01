/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to nowhere... */

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_libretroaudio.h"

/* The tag name used by DUMMY audio */
#define LIBRETRO_DRIVER_NAME         "libretro"

/* defined in libretro.cpp */
extern void libretro_audio_cb(int16_t *buffer, uint32_t buffer_len);

static SDL_AudioDevice *audiodevice; 

/* Audio driver functions */
static int LIBRETRO_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void LIBRETRO_WaitAudio(_THIS);
static void LIBRETRO_PlayAudio(_THIS);
static Uint8 *LIBRETRO_GetAudioBuf(_THIS);
static void LIBRETRO_CloseAudio(_THIS);

/* Audio driver bootstrap functions */
static int LIBRETRO_Available(void)
{
	return(1);
}

static void LIBRETRO_DeleteDevice(SDL_AudioDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);

	audiodevice = NULL;
}

static SDL_AudioDevice *LIBRETRO_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->OpenAudio = LIBRETRO_OpenAudio;
	this->WaitAudio = LIBRETRO_WaitAudio;
	this->PlayAudio = LIBRETRO_PlayAudio;
	this->GetAudioBuf = LIBRETRO_GetAudioBuf;
	this->CloseAudio = LIBRETRO_CloseAudio;

	this->free = LIBRETRO_DeleteDevice;

	audiodevice = this;

	return this;
}

AudioBootStrap LIBRETRO_bootstrap = {
	LIBRETRO_DRIVER_NAME, "SDL dummy audio driver",
	LIBRETRO_Available, LIBRETRO_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void LIBRETRO_WaitAudio(_THIS)
{
	/* NOOP */
	printf("LIBRETRO_WaitAudio\n");
}

static void LIBRETRO_PlayAudio(_THIS)
{
	/* NOOP */
	printf("LIBRETRO_PlayAudio\n");
}

static Uint8 *LIBRETRO_GetAudioBuf(_THIS)
{
	return(NULL);
}

static void LIBRETRO_CloseAudio(_THIS)
{
	if ( this->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(this->hidden->mixbuf);
		this->hidden->mixbuf = NULL;
	}
}

void LIBRETRO_MixAudio()
{
	SDL_AudioDevice *audio = (SDL_AudioDevice *) audiodevice;
	SDL_AudioSpec *spec = &audio->spec;

	if (audio == NULL) {
		return;
	}

	spec = &audio->spec;

	/* Silence the buffer, since it's ours */
	SDL_memset(audio->hidden->mixbuf, spec->silence, audio->hidden->mixlen);

	/* Only do soemthing if audio is enabled */
	if (!audio->enabled) {
		return;
	}

	if (audio->convert.needed) {
		// TODO
		return;
	}

	SDL_mutexP(audio->mixer_lock);
	(*spec->callback)(spec->userdata, audio->hidden->mixbuf, audio->hidden->mixlen);
	SDL_mutexV(audio->mixer_lock);

	libretro_audio_cb((int16_t *)audio->hidden->mixbuf, spec->size / spec->channels / 2);
}

static int LIBRETRO_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	float bytes_per_sec = 0.0f;

	spec->channels = 2;
	spec->format = AUDIO_S16;
	spec->freq = 44100;             // should match retro_system_timing.sample_rate
	spec->samples = 735;            // should match retro_system_timing.fps

	/* Update the fragment size as size in bytes */
	SDL_CalculateAudioSpec(spec);

	/* Allocate mixing buffer */
	this->hidden->mixlen = spec->size;
	this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
	if ( this->hidden->mixbuf == NULL ) {
		return(-1);
	}
	SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

	bytes_per_sec = (float) (((spec->format & 0xFF) / 8) *
	                   spec->channels * spec->freq);

	/*
	 * We try to make this request more audio at the correct rate for
	 *  a given audio spec, so timing stays fairly faithful.
	 * Also, we have it not block at all for the first two calls, so
	 *  it seems like we're filling two audio fragments right out of the
	 *  gate, like other SDL drivers tend to do.
	 */
	this->hidden->initial_calls = 2;
	this->hidden->write_delay =
	               (Uint32) ((((float) spec->size) / bytes_per_sec) * 1000.0f);

	/* We're ready to rock and roll. :-) */
	return(1);
}

