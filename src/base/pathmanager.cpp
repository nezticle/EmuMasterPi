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

#include "pathmanager.h"
#include <QtCore/QDir>
#include <stdlib.h>

PathManager pathManager;

/*!
	\class PathManager
	PathManager class manages paths used across EmuMaster. It also creates
	trees of folders for user data and disks.
 */

PathManager::PathManager()
{
	m_emus << "nes";
	m_emus << "gba";
	m_emus << "snes";

	m_installationDirPath = "/opt/emumaster";
	m_userDataDirPath = QString("%1/.emumaster").arg(getenv("HOME"));
	m_diskDirBase = QString("%1/MyDocs/emumaster").arg(getenv("HOME"));
}

/*! Creates subdirs for every emulation in the given \a dir. */
void PathManager::createEmusSubtree(QDir &dir)
{
	for (int i = 0; i < m_emus.size(); i++)
		dir.mkdir(m_emus.at(i));
}

/*! Creates all dirs used by EmuMaster. */
void PathManager::buildLocalDirTree()
{
	QDir dir(getenv("HOME"));
	dir.mkdir(".emumaster");
	dir.cd(".emumaster");
	dir.mkdir("state");
	dir.mkdir("icon");
	dir.mkdir("screenshot");

	dir.cd("state");		createEmusSubtree(dir); dir.cdUp();
	dir.cd("screenshot");	createEmusSubtree(dir); dir.cdUp();

	dir = QDir(getenv("HOME"));
	dir.cd("MyDocs");
	dir.mkdir("emumaster");
	dir.cd("emumaster");
	dir.mkdir("covers");
	createEmusSubtree(dir);
}

/*! Returns a path with disks for the given emulation \a emu. */
QString PathManager::diskDirPath(const QString &emu) const
{
	return QString("%1/%2").arg(m_diskDirBase).arg(emu);
}

/*! Returns a path with disks for the current emulation. */
QString PathManager::diskDirPath() const
{
	return diskDirPath(m_currentEmu);
}

/*! Returns a path for a disk specified by its \a emu and \a title. */
QString PathManager::screenShotPath(const QString &emu,
									const QString &title) const
{
	return QString("%1/screenshot/%2/%3.jpg")
			.arg(userDataDirPath())
			.arg(emu)
			.arg(title);
}

/*! Returns a path for a disk with the given \a title for the current emulation. */
QString PathManager::screenShotPath(const QString &title) const
{
	return screenShotPath(m_currentEmu, title);
}

/*! Sets current emulation to the given \a emu. */
void PathManager::setCurrentEmu(const QString &name)
{
	Q_ASSERT(m_emus.contains(name));
	m_currentEmu = name;
}

/*! Returns a path for states of the disk specified by its \a emu and \a title. */
QString PathManager::stateDirPath(const QString &emu,
								  const QString &title) const
{
	return QString("%1/state/%2/%3")
			.arg(userDataDirPath())
			.arg(emu)
			.arg(title);
}

/*!
	Returns a path for states of the disk specified by its \a title
	in the current emulation.
 */
QString PathManager::stateDirPath(const QString &title) const
{
	return stateDirPath(m_currentEmu, title);
}

/*!
	Returns a path for home screen icon of the disk specified by
	its \a emu and \a title.
*/
QString PathManager::homeScreenIconPath(const QString &emu,
										const QString &title) const
{
	return QString("%1/icon/%2_%3.png")
			.arg(userDataDirPath())
			.arg(emu)
			.arg(title);
}

/*!
	Returns a path for desktop file of the disk specified by
	its \a emu and \a title.
*/
QString PathManager::desktopFilePath(const QString &emu,
									 const QString &title) const
{
    Q_UNUSED(emu)
    Q_UNUSED(title)
    return QString();
}

/*!
	\fn QStringList PathManager::emus() const
	Returns a list with names of available emulations.
 */

/*!
	\fn QString PathManager::installationDirPath() const
	Returns a path to the dir where EmuMaster is installed.
 */

/*!
	\fn QString PathManager::userDataDirPath() const
	Returns a path to the dir containing user data ($HOME/.emumaster).
 */
