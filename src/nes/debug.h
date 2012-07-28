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

#ifndef NESDEBUG_H
#define NESDEBUG_H

#include <base/emu.h>

enum DebugEvent {
	DebugCpuOp,
	DebugNmi,
	DebugIrq,
	DebugBankSwitch,
	DebugBreakpoint
};

enum Message {
	Continue,
	InsertBreakpoint,
	RemoveBreakpoint,
	SetEventMask,
	SendCpuRegisters,
	SendCpuBanks,
	SendRomMemory,
	SendProfiler,
	ClearProfiler
};

static const u16 DefaultPort = 5496;

extern void nesDebugInit();
extern void nesDebugPostInit();
extern void nesDebugShutdown();
extern void nesDebugCpuOp(u16 pc);
extern void nesDebugNmi();
extern void nesDebugIrq();
extern void nesDebugBankSwitch(u8 page, u8 romBank);

#endif // NESDEBUG_H
