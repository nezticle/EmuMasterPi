/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef EMU_H
#define EMU_H

#include <QObject>
#include <QHash>
#include <QRectF>
class QImage;

typedef qint8 s8;
typedef quint8 u8;
typedef qint16 s16;
typedef quint16 u16;
typedef qint32 s32;
typedef quint32 u32;
typedef qint64 s64;
typedef quint64 u64;

// TODO convert to static cast
#define S8_MIN SCHAR_MIN
#define S8_MAX SCHAR_MAX
#define U8_MAX UCHAR_MAX
#define S16_MIN SHRT_MIN
#define S16_MAX SHRT_MAX
#define U16_MAX USHRT_MAX
#define S32_MIN INT_MIN
#define S32_MAX INT_MAX
#define U32_MAX UINT_MAX

static const int KB = 1024;
static const int MB = KB * KB;
static const int GB = KB * KB * KB;

#define EM_MSG_DISK_LOAD_FAILED QObject::tr("Could not load the disk")
#define EM_MSG_OPEN_FILE_FAILED QObject::tr("Unable to open the file")
#define EM_MSG_FILE_CORRUPTED QObject::tr("File is corrupted")
#define EM_MSG_STATE_DIFFERS QObject::tr("Configuration of loaded state differs from the current one. Mismatch in")

#define DISABLE_IMPLICIT_CONSTRUCTORS(Class) \
	Class(); \
	Q_DISABLE_COPY(Class)

#define UNREACHABLE() Q_ASSERT(false)

#endif // EMU_H
