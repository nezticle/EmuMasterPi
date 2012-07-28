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

#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <base/emu.h>
#include <arm/constants.h>

static inline void memset8(void *p, u8 b, int n)
{ memset(p, b, n); }
static inline void memcpy8(void *dst, void *src, int n)
{ memcpy(dst, src, n); }

static inline void memset32(void *p, u32 d, int n)
{
	u32 *p32 = (u32 *)p;
	for (; n > 0; p32++, n--)
		*p32 = d;
}

class AnonymousMemMapping
{
public:
	static int pageSize();

	AnonymousMemMapping(int size);
	~AnonymousMemMapping();

	void *address() const;
	int size() const;
	bool changeRights(bool r, bool w, bool x);
private:
	void *m_address;
	int m_size;
};

inline void *AnonymousMemMapping::address() const
{ return m_address; }
inline int AnonymousMemMapping::size() const
{ return m_size; }

#endif // MEMUTILS_H
