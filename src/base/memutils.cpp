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

#include "memutils.h"
#include <sys/mman.h>
#include <unistd.h>

int AnonymousMemMapping::pageSize()
{
	return sysconf(_SC_PAGESIZE);
}

AnonymousMemMapping::AnonymousMemMapping(int size)
{
	void *mem = mmap(0, size,
					 PROT_READ | PROT_WRITE | PROT_EXEC,
					 MAP_PRIVATE | MAP_ANONYMOUS,
					 -1, 0);
	if (mem == MAP_FAILED) {
		m_address = 0;
		m_size = 0;
	} else {
		m_address = mem;
		m_size = size;
	}
}

AnonymousMemMapping::~AnonymousMemMapping()
{
	if (m_address) {
		if (munmap(m_address, m_size) < 0)
			qDebug("munmap() failed");
	}
}

bool AnonymousMemMapping::changeRights(bool r, bool w, bool x)
{
	int prot = 0;
	if (r)
		prot |= PROT_READ;
	if (w)
		prot |= PROT_WRITE;
	if (x)
		prot |= PROT_EXEC;
	return mprotect(m_address, m_size, prot) == 0;
}
