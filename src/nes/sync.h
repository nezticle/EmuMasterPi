/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef NESSYNC_H
#define NESSYNC_H

#include <base/emu.h>

static const int NesDmaCycles = 513;

typedef int NesSync(int additionalCycles);

extern bool nesSyncInit(QString *error);
extern void nesSyncShutdown();

extern void nesSyncReset();
extern void nesSyncFrame(bool drawEnabled);

extern u64 nesSyncCpuCycles();

extern void nesSyncSl();

#endif // NESSYNC_H
