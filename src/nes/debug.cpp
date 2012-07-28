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

#include "debug.h"
#include "mapper.h"
#include "cpubase.h"
#include <base/configuration.h>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QDataStream>
#include <QSet>

static int activeEvents = 0;
static int *profilerExecCount = 0;
static u8 currentCpuBanks[8];
static QTcpServer *server = 0;
static QTcpSocket *client = 0;
static QDataStream *stream = 0;
static QSet<u16> breakpoints;

static void waitForBytes(int n)
{
	while (n > client->bytesAvailable()) {
		if (!client->waitForReadyRead(-1))
			abort();
	}
}

static void insertBreakpoint()
{
	waitForBytes(2);
	u16 pc;
	*stream >> pc;
	breakpoints.insert(pc);
}

static void removeBreakpoint()
{
	waitForBytes(2);
	u16 pc;
	*stream >> pc;
	breakpoints.remove(pc);
}

static void sendCpuRegisters()
{
	nesCpu->storeRegistersToBase();
	NesCpuBaseRegisters regs = nesCpu->stateRegs();
	*stream << regs.a;
	*stream << regs.x;
	*stream << regs.y;
//	u8 cyc = nesSyncCpuCycles();
//	*stream << cyc;
	*stream << regs.s;
	*stream << regs.p;
}

static void sendRomMemory()
{
	*stream << nesRomSizeInBytes;
	stream->writeRawData((char *)nesRom, nesRomSizeInBytes);
}

static void stepBegin(DebugEvent ev)
{
	u8 evByte = ev;
	*stream << evByte;
}

static void stepEnd()
{
	for (;;) {
		waitForBytes(1);
		u8 cmd;
		*stream >> cmd;

		switch (cmd) {
		case Continue:
			return;
		case InsertBreakpoint:
			insertBreakpoint();
			break;
		case RemoveBreakpoint:
			removeBreakpoint();
			break;
		case SetEventMask:
			waitForBytes(4);
			*stream >> activeEvents;
			break;
		case SendCpuRegisters:
			sendCpuRegisters();
			break;
		case SendCpuBanks:
			stream->writeRawData((const char *)currentCpuBanks, 8);
			break;
		case SendRomMemory:
			sendRomMemory();
			break;
		case SendProfiler:
			stream->writeRawData((char *)profilerExecCount,
								 nesRomSizeInBytes * sizeof(int));

			break;
		case ClearProfiler:
			memset(profilerExecCount, 0, nesRomSizeInBytes * sizeof(int));
			break;
		}
	}
}

void nesDebugInit()
{
	activeEvents = 0;
	profilerExecCount = new int[nesRomSizeInBytes];
	memset(profilerExecCount, 0, nesRomSizeInBytes * sizeof(int));
}

void nesDebugPostInit()
{
	if (!server) {
		server = new QTcpServer();
		u16 port = emConf.value("debugPort", DefaultPort).toInt();
		qDebug("debug: waiting for a client");
		server->listen(QHostAddress::Any, port);
		server->waitForNewConnection(100000);
		qDebug("debug: connected with a client");

		client = server->nextPendingConnection();
		Q_ASSERT(client != 0);

		stream = new QDataStream(client);
		stream->setByteOrder(QDataStream::LittleEndian);

		*stream << nesCpu->stateRegs().pc;
		stepEnd();
	}
}

void nesDebugShutdown()
{
	delete[] profilerExecCount;
}

#define CHECK_EVENT_ACTIVE(ev) \
	if (!(activeEvents & (1<<ev))) \
		return

static void checkBreakpoint(u16 pc)
{
	CHECK_EVENT_ACTIVE(DebugBreakpoint);
	if (breakpoints.contains(pc)) {
		stepBegin(DebugBreakpoint);
		*stream << pc;
		stepEnd();
	}
}

void nesDebugCpuOp(u16 pc)
{
	int bank = currentCpuBanks[nesCpuPageByAddr(pc)];
	int offset = bank * NesCpuBankSize + (pc & NesCpuBankMask);
	profilerExecCount[offset]++;

	checkBreakpoint(pc);

	CHECK_EVENT_ACTIVE(DebugCpuOp);
	stepBegin(DebugCpuOp);
	*stream << pc;
	stepEnd();
}

void nesDebugNmi()
{
	CHECK_EVENT_ACTIVE(DebugNmi);
	stepBegin(DebugNmi);
	stepEnd();
}

void nesDebugIrq()
{
	CHECK_EVENT_ACTIVE(DebugIrq);
	stepBegin(DebugIrq);
	stepEnd();
}

void nesDebugBankSwitch(u8 page, u8 romBank)
{
	currentCpuBanks[page] = romBank;

	CHECK_EVENT_ACTIVE(DebugBankSwitch);
	stepBegin(DebugBankSwitch);
	*stream << page;
	*stream << romBank;
	stepEnd();
}
