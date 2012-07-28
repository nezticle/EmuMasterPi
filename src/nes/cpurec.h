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

#ifndef NESCPUREC_H
#define NESCPUREC_H

#include "cpubase.h"

class NesCpuRecompiler : public NesCpuBase
{
public:
	bool init(QString *error);
	void shutdown();

	void run(NesSync *nesSync);
	void reset();

	s32 ticks() const;
	void setSignal(InterruptSignal sig, bool on);
	void dma();
	void clearPage(int pageIndex);
#if defined(ENABLE_DEBUGGING)
	void storeRegistersToBase();
#endif

	void sl();
private:
	void saveStateToBase();
	void loadStateFromBase();

	void setInterrupt(Interrupt interrupt, bool on);
};

extern NesCpuRecompiler nesCpuRecompiler;

#endif // NESCPUREC_H
