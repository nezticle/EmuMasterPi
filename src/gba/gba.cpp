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

#include "common.h"
#include "cpu.h"
#include "mem.h"
#include "gpu.h"
#include "spu.h"
#include "gba.h"
#include "input.h"
#include "cheats.h"
#include <base/emuview.h>
#include <base/pathmanager.h>
#include <QtCore/QFile>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtCore/QSemaphore>

timer_type timer[4];

u32 cpu_ticks = 0;

u32 execute_cycles = 960;
s32 video_count = 960;

u32 skip_next_frame = 0;

extern "C" u16 *screen_pixels_ptr;
extern "C" void return_to_host(u32 *returnRegs);
static u32 return_to_host_regs[2];

static QSemaphore producerSem;
static QSemaphore consumerSem;
static volatile bool requestQuit;
static QImage gpuFrame;
static GbaThread gbaThread;

GbaEmu gbaEmu;

GbaEmu::GbaEmu() :
	Emu("gba")
{
}

bool GbaEmu::init(const QString &diskPath, QString *error)
{
	gpuFrame = QImage(240, 160, QImage::Format_RGB16);
	setVideoSrcRect(gpuFrame.rect());
	setFrameRate(60);

	screen_pixels_ptr = (quint16 *)gpuFrame.bits();
	requestQuit = false;

	*error = loadBios();
	if (error->isEmpty())
		*error = setDisk(diskPath);
	return error->isEmpty();
}

void GbaEmu::shutdown()
{
	requestQuit = true;
	emulateFrame(false);
	gbaThread.wait();
	gpuFrame = QImage();
}

void GbaEmu::reset()
{
	skip_next_frame = 0;

	for (int i = 0; i < 4; i++) {
		dma[i].start_type = DMA_INACTIVE;
		dma[i].direct_sound_channel = DMA_NO_DIRECT_SOUND;
		timer[i].status = TIMER_INACTIVE;
		timer[i].reload = 0x10000;
		timer[i].stop_cpu_ticks = 0;
	}

	timer[0].direct_sound_channels = TIMER_DS_CHANNEL_BOTH;
	timer[1].direct_sound_channels = TIMER_DS_CHANNEL_NONE;

	cpu_ticks = 0;

	execute_cycles = 960;
	video_count = 960;

	init_memory();
	init_cpu();
	reset_sound();
	reg[CHANGED_PC_STATUS] = 1;

	flush_translation_cache_rom();
	flush_translation_cache_ram();
	flush_translation_cache_bios();
}

QString GbaEmu::loadBios()
{
	QString path = pathManager.diskDirPath() + "/gba_bios.bin";
	QFile biosFile(path);

	bool loaded = false;
	bool supported = false;
	if (biosFile.open(QIODevice::ReadOnly)) {
		if (biosFile.read(reinterpret_cast<char *>(bios_rom), 0x4000) == 0x4000) {
			loaded = true;
			supported = (bios_rom[0] == 0x18);
		}
	}
	if (!loaded) {
		return
			tr(	"Sorry, but emulator requires a Gameboy Advance BIOS "
				"image to run correctly. Make sure to get an "
				"authentic one, it'll be exactly 16384 bytes large "
				"and should have the following md5sum value: \n"
				"\n"
				"a860e8c0b6d573d191e4ec7db1b1e4f6\n"
				"\n"
				"When you do get it name it gba_bios.bin and put it "
                "in the \"roms/gba\" directory.");

	} else if (!supported) {
		return tr("You have an incorrect BIOS image.");
	}
	return QString();
}

QString GbaEmu::setDisk(const QString &path)
{
	init_gamepak_buffer();
	if (!gbaMemLoadGamePack(path))
		return tr("Could not load disk");
	reset();
	skip_next_frame = 1;

	gbaThread.start();
	consumerSem.acquire();
	return QString();
}

const QImage &GbaEmu::frame() const
{
	return gpuFrame;
}

void GbaEmu::emulateFrame(bool drawEnabled)
{
	gbaInputUpdate(input());
	skip_next_frame = !drawEnabled;
	screen_pixels_ptr = (quint16 *)gpuFrame.bits();
	producerSem.release();
	consumerSem.acquire();
}

static void gbaSync()
{
	consumerSem.release();
	if (requestQuit)
		return_to_host(return_to_host_regs);
	producerSem.acquire();
}

int GbaEmu::fillAudioBuffer(char *stream, int streamSize)
{
	return gbaSpu.fillBuffer(stream, streamSize);
}

QString GbaEmu::gamePackTitle() const
{
	return gbaGamePackTitle;
}

QString GbaEmu::gamePackCode() const
{
	return gbaGamePackCode;
}

QString GbaEmu::gamePackMaker() const
{
	return gbaGamePackMaker;
}

GbaCheats *GbaEmu::cheats() const
{
	return &gbaCheats;
}

void GbaEmu::resume()
{
	gbaSpu.setEnabled(isAudioEnabled());
}

void GbaThread::run()
{
	execute_arm_translate(execute_cycles, return_to_host_regs);
}

#define check_count(count_var) \
	if (count_var < execute_cycles) \
		execute_cycles = count_var;

#define check_timer(timer_number) \
	if (timer[timer_number].status == TIMER_PRESCALE) \
		check_count(timer[timer_number].count);

#define update_timer(timer_number) \
	if (timer[timer_number].status != TIMER_INACTIVE) { \
		if (timer[timer_number].status != TIMER_CASCADE) { \
			timer[timer_number].count -= execute_cycles; \
			io_registers[REG_TM##timer_number##D] = -(timer[timer_number].count >> timer[timer_number].prescale); \
		} \
		if (timer[timer_number].count <= 0) { \
			if (timer[timer_number].irq == TIMER_TRIGGER_IRQ) \
				irq_raised |= IRQ_TIMER##timer_number; \
			if ((timer_number != 3) && (timer[timer_number + 1].status == TIMER_CASCADE)) { \
				timer[timer_number + 1].count--; \
				io_registers[REG_TM0D + (timer_number + 1) * 2] = -(timer[timer_number + 1].count); \
			} \
			if (timer_number < 2) { \
				if (timer[timer_number].direct_sound_channels & 0x01) \
					sound_timer(timer[timer_number].frequency_step, 0); \
				if (timer[timer_number].direct_sound_channels & 0x02) \
					sound_timer(timer[timer_number].frequency_step, 1); \
			} \
			timer[timer_number].count += (timer[timer_number].reload << timer[timer_number].prescale); \
		} \
	}

u32 update_gba()
{
	int irq_raised = IRQ_NONE;
	do {
		cpu_ticks += execute_cycles;
		reg[CHANGED_PC_STATUS] = 0;
		if (gbc_sound_update) {
			update_gbc_sound(cpu_ticks);
			gbc_sound_update = 0;
		}
		update_timer(0);
		update_timer(1);
		update_timer(2);
		update_timer(3);

		video_count -= execute_cycles;

		if (video_count <= 0) {
			u32 vcount = io_registers[REG_VCOUNT];
			u32 dispstat = io_registers[REG_DISPSTAT];

			if ((dispstat & 0x02) == 0) {
				// Transition from hrefresh to hblank
				video_count += (272);
				dispstat |= 0x02;

				if ((dispstat & 0x01) == 0) {
					update_scanline();
					// If in visible area also fire HDMA
					for (int i = 0; i < 4; i++) {
						if (dma[i].start_type == DMA_START_HBLANK)
							dma_transfer(dma + i);
					}
				}
				if (dispstat & 0x10)
					irq_raised |= IRQ_HBLANK;
			} else {
				// Transition from hblank to next line
				video_count += 960;
				dispstat &= ~0x02;
				vcount++;
				if (vcount == 160) {
					// Transition from vrefresh to vblank
					dispstat |= 0x01;

					if (dispstat & 0x8)
						irq_raised |= IRQ_VBLANK;

					affine_reference_x[0] = (s32)(address32(io_registers, 0x28) << 4) >> 4;
					affine_reference_y[0] = (s32)(address32(io_registers, 0x2C) << 4) >> 4;
					affine_reference_x[1] = (s32)(address32(io_registers, 0x38) << 4) >> 4;
					affine_reference_y[1] = (s32)(address32(io_registers, 0x3C) << 4) >> 4;

					for (int i = 0; i < 4; i++) {
						if (dma[i].start_type == DMA_START_VBLANK)
							dma_transfer(dma + i);
					}
				} else if (vcount == 228) {
					// Transition from vblank to next screen
					dispstat &= ~0x01;

					gbaSync();

					update_gbc_sound(cpu_ticks);
					gbaCheatsProcess();
					vcount = 0;
				}
				if (vcount == (dispstat >> 8)) {
					// vcount trigger
					dispstat |= 0x04;
					if (dispstat & 0x20)
						irq_raised |= IRQ_VCOUNT;
				} else {
					dispstat &= ~0x04;
				}
				io_registers[REG_VCOUNT] = vcount;
			}
			io_registers[REG_DISPSTAT] = dispstat;
		}
		if (irq_raised)
			raise_interrupt(static_cast<irq_type>(irq_raised));
		execute_cycles = video_count;

		check_timer(0);
		check_timer(1);
		check_timer(2);
		check_timer(3);
	} while (reg[CPU_HALT_STATE] != CPU_ACTIVE);
	return execute_cycles;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
    QGuiApplication app(argc, argv);
	EmuView view(&gbaEmu, argv[1]);
	return app.exec();
}

void GbaEmu::sl()
{
	emsl.begin("machine");
	emsl.var("cpu_ticks", cpu_ticks);
	emsl.var("execute_cycles", execute_cycles);
	emsl.var("video_count", video_count);
	emsl.array("timer", timer, sizeof(timer));
	emsl.end();

	gbaCpu.sl();
	gbaSpu.sl();
	gbaGpu.sl();
	gbaMemSl();
	gbaCheats.sl();
}
