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

#ifndef CRC32_H
#define CRC32_H

#include <QtCore/QtGlobal>

#if defined(CRC32_PROJECT)
# define CRC32_EXPORT Q_DECL_EXPORT
#else
# define CRC32_EXPORT Q_DECL_IMPORT
#endif

Q_DECL_EXPORT quint32 qChecksum32(const char *data, uint len);

#endif // CRC32_H
