/*
 * Copyright 2025 Wisp
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WISP_AUDIO_H_
#define _WISP_AUDIO_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * Audio operations.
 */
struct gui_audio_table {
    /**
     * Initialize audio output.
     *
     * \param rate      Sample rate (e.g., 44100)
     * \param channels  Number of channels (e.g., 2)
     * \return true on success, false on error.
     */
    bool (*init)(int rate, int channels);

    /**
     * Play audio data.
     *
     * \param data  Pointer to PCM data (S16LE)
     * \param size  Size of data in bytes
     */
    void (*play)(const void *data, size_t size);

    /**
     * Finalize audio output.
     */
    void (*fini)(void);
};

#endif
