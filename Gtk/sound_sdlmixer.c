/*
 * sound_sdlmixer.c - SDL_mixer sound backend
 * Copyright (c) 2002 Torbjörn Andersson <d91tan@Update.UU.SE>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 */

/*
 * Both SDL and SDL_mixer are distributed under the GNU Library Public
 * License, version 2. For more information about SDL and associated
 * libraries, see <http://www.libsdl.org/>.
 */

#include <gtk/gtk.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_mixer.h"

#include "sound.h"

#define MIXER_FREQUENCY    44100

static Mix_Music *mp3 = NULL;
static Mix_Music *midi = NULL;

void sound_start_music (gchar *filename)
{
    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
	return;

    if (SDL_Init (SDL_INIT_AUDIO) < 0)
	return;

    if (Mix_OpenAudio (MIXER_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
    {
	SDL_Quit ();
	return;
    }

    mp3 = Mix_LoadMUS (filename);
    if (!mp3)
    {
	SDL_Quit ();
	return;
    }

    Mix_PlayMusic (mp3, -1);
}

void sound_stop_music ()
{
    Mix_HaltMusic ();
    if (mp3)
    {
       Mix_FreeMusic (mp3);
       mp3 = NULL;
    }
    if (midi) {
       Mix_FreeMusic (midi);
       midi = NULL;
    }
    Mix_CloseAudio ();
    SDL_QuitSubSystem (SDL_INIT_AUDIO);
}
void sound_play_midi (type8 *midi_data, type32 length, type16 tempo)
{
    SDL_RWops *rw;

    if (midi) {
    sound_stop_music();
    }

    if (!SDL_WasInit (SDL_INIT_AUDIO)) {
        if (SDL_Init (SDL_INIT_AUDIO) < 0)
            return;

        if (Mix_OpenAudio (MIXER_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
            SDL_Quit ();
            return;
        }
    }

    rw = SDL_RWFromMem(midi_data, length);

    midi = Mix_LoadMUS_RW(rw, 1);

    if (Mix_PlayMusic (midi, 1) == -1) {
        Mix_FreeMusic (midi);
        midi = NULL;
        return;
    }
}

void sound_clear()
{
    sound_stop_music();

    if (SDL_WasInit (SDL_INIT_AUDIO)) {
        Mix_CloseAudio();
        SDL_QuitSubSystem (SDL_INIT_AUDIO);
    }
}

