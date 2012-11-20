#include "snes.h"
#include "snes9x.h"
#include "mem.h"
#include "spu.h"
#include "gfx.h"
#include "soundux.h"
#include "srtc.h"
#include "dma.h"
#include "sdd1.h"
#include <base/emuview.h>
#include <QtGui/QGuiApplication>

// TODO set sound enabled

SnesEmu snesEmu;

static bool romLoaded = false;
static int soundSampleCount;
static volatile bool rendered = false;

SnesEmu::SnesEmu(QObject *parent) :
	Emu("snes", parent) {
}

void setDefaultSettings() {
	// try to start emu
	ZeroMemory (&Settings, sizeof (Settings));

	Settings.CyclesPercentage = 100;
	Settings.DisableSoundEcho = FALSE;
	Settings.APUEnabled = 1;// TODO (iSettings.iEnableSpeedHack ? 3 : 1);
	Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
	Settings.Shutdown = Settings.ShutdownMaster = TRUE;
	Settings.DisableMasterVolume = FALSE;
	Settings.Mouse = FALSE;
	Settings.SuperScope = FALSE;
	Settings.MultiPlayer5 = FALSE;
	Settings.ControllerOption = 0;
	Settings.Transparency = TRUE;
	Settings.SixteenBit = TRUE;
	Settings.SupportHiRes = TRUE;
	Settings.NetPlay = FALSE;
	Settings.ServerName[0] = 0;
	Settings.ApplyCheats = FALSE;
	Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
	Settings.os9x_hack = FALSE;
	Settings.InterpolatedSound = FALSE;
	soundSampleCount = SoundSampleRate / (Settings.PAL ? 50 : 60);
	soundSampleCount *= 2;
}

bool SnesEmu::init(const QString &diskPath, QString *error) {
	S9xSetSoundMute(FALSE);
	setDefaultSettings();
	S9xSetPlaybackRate();
	m_frame = QImage(512, 239, QImage::Format_RGB16);
	setVideoSrcRect(QRectF(0.0f, 0.0f, 256.0f, 224.f));
	setFrameRate(60);

	GFX.Pitch = 512 * 2;
	GFX.RealPitch = 512 * 2;
	GFX.PPL = GFX.Pitch >> 1;
	GFX.PPLx2 = GFX.Pitch;
	GFX.Screen = (u8 *)m_frame.bits();
	GFX.SubScreen = (u8 *) malloc(GFX.RealPitch * 239);
	GFX.ZBuffer = (u8 *) malloc(GFX.PPL * 239);
	GFX.SubZBuffer = (u8 *) malloc(GFX.PPL * 239);
	GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;

	if (!GFX.Screen || !GFX.SubScreen || !GFX.ZBuffer || !Memory.Init() || !S9xInitAPU() || !GFX.SubZBuffer ) {
		*error = tr("SNES emulation init failed!");
		return false;
	}
	S9xInitSound();
	if (!S9xGraphicsInit()) {
		*error = tr("SNES emulation init failed!");
		return false;
	}
	S9xReset();
	setDefaultSettings();
	*error = setDisk(diskPath);
	return error->isEmpty();
}

void SnesEmu::shutdown() {
	m_frame = QImage();
}

void SnesEmu::reset() {
	S9xReset();
}

void SnesEmu::sync(int width, int height) {
	setVideoSrcRect(QRect(0, 0, width, height));
	rendered = true;
}

extern "C" void S9xMessage(int type, int number, const char *message)
{ qDebug("%d %d %s", type, number, message); }

extern "C" void S9xLoadSDD1Data(void) {
	Settings.SDD1Pack = TRUE;
	Memory.FreeSDD1Data();
	Settings.SDD1Pack = FALSE;
}


extern "C" const char *S9xGetFilename(const char *ex) // ex is like ".srm"
{
	static char filename[PATH_MAX];
	char *p;

	strcpy(filename, Memory.ROMFilename);
	for (p = filename + strlen(filename) - 1; p > filename + 3; p--)
		if (*p == '.' || *p == '\\')
			break;
	strcpy(p, ex);

	return filename;
}

extern "C" bool8 S9xReadMousePosition(int /* which1 */, int &/* x */,
		int & /* y */, u32 & /* buttons */)
{
	return (FALSE);
}

extern "C" bool8 S9xReadSuperScopePosition(int & /* x */, int & /* y */,
		u32 & /* buttons */)
{
	return (FALSE);
}

extern "C" const char *S9xGetFilenameInc(const char *e)
{
	return e;
}

extern "C" const char *S9xBasename(const char *f)
{
	const char *p;
	if ((p = strrchr(f, '/')) != NULL || (p = strrchr(f, '\\')) != NULL)
		return (p + 1);
	return (f);
}

void _splitpath(const char *path, char *drive, char *dir, char *fname,
		char *ext)
{
	//TODO!


	*drive = 0;

	char *slash = (char*) strrchr(path, '/');
	if (!slash)
		slash = (char*) strrchr(path, '\\');

	char *dot = (char*) strrchr(path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		strcpy(dir, "");
		strcpy(fname, path);
		if (dot)
		{
			*(fname + (dot - path)) = 0;
			strcpy(ext, dot + 1);
		}
		else
			strcpy(ext, "");
	}
	else
	{
		strcpy(dir, path);
		*(dir + (slash - path)) = 0;
		strcpy(fname, slash + 1);
		if (dot)
		{
			*(fname + (dot - slash) - 1) = 0;
			strcpy(ext, dot + 1);
		}
		else
			strcpy(ext, "");
	}
}

void _makepath(char *path, const char *, const char *dir, const char *fname,
		const char *ext)
{
	if (dir && *dir)
	{
		strcpy(path, dir);
		strcat(path, "/");
	}
	else
		*path = 0;
	strcat(path, fname);
	if (ext && *ext)
	{
		strcat(path, ".");
		strcat(path, ext);
	}
}

QString SnesEmu::setDisk(const QString &path) {
    if (!Memory.LoadROM(path.toLocal8Bit().constData()))
		return tr("Load disk failed.");
	Memory.ROMFramesPerSecond = Settings.PAL ? 50 : 60;
	setFrameRate(Memory.ROMFramesPerSecond);

	romLoaded = true;
	S9xMainLoop();
	return QString();
}

void SnesEmu::emulateFrame(bool drawEnabled) {
	if (drawEnabled)
		GFX.Screen = (u8 *)m_frame.bits();
	IPPU.RenderThisFrame = drawEnabled;
	rendered = false;
	while (!rendered)
		S9xMainLoop();
}

const QImage &SnesEmu::frame() const {
	return m_frame;
}

int SnesEmu::fillAudioBuffer(char *stream, int streamSize) {
	int count = qMin(streamSize/4, soundSampleCount);
	S9xMixSamples((s16 *)stream, count * 2);
	return count * 4;
}

const int SnesEmu::m_keyMapping[16] = {
	0,
	0,
	0,
	0,
	EmuPad::Button_R1,
	EmuPad::Button_L1,
	EmuPad::Button_X,
	EmuPad::Button_A,
	EmuPad::Button_Right,
	EmuPad::Button_Left,
	EmuPad::Button_Down,
	EmuPad::Button_Up,
	EmuPad::Button_Start,
	EmuPad::Button_Select,
	EmuPad::Button_Y,
	EmuPad::Button_B
};

int SnesEmu::gamePad(int i) const {
	if (i > 1)
		return 0;

	int pad = input()->pad[i].buttons();
	int snesPad = 0;
	for (int i = 0; i < 16; i++) {
		if (pad & m_keyMapping[i])
			snesPad |= 1 << i;
	}
	return snesPad;
}

void SnesEmu::sl() {
	if (emsl.save) {
		S9xSRTCPreSaveState();
	} else {
		S9xReset();
	}
	snesCpuSl();
	snesMemSl();
	snesPpuSl();
	snesDmaSl();
	snesSoundSl();
	snesSpuSl();
	if (!emsl.save) {
		S9xFixSoundAfterSnapshotLoad();
		S9xSetPCBase(ICPU.ShiftedPB + Registers.PC);
		S9xReschedule();
		S9xSRTCPostLoadState();
		if (Settings.SDD1)
			S9xSDD1PostLoadState();
		S9xSetSoundMute(FALSE);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2)
		return -1;
    QGuiApplication app(argc, argv);
	EmuView view(&snesEmu, argv[1]);
	return app.exec();
}
