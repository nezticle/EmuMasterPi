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

#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include "base_global.h"
#include <QtCore/QStringList>
class QDir;

class BASE_EXPORT PathManager
{
public:
	PathManager();
	QString installationDirPath() const;
	QString userDataDirPath() const;
	QString diskDirPath(const QString &emu) const;
	QString screenShotPath(const QString &emu,
						   const QString &title) const;
	QString stateDirPath(const QString &emu,
						 const QString &title) const;
	QString homeScreenIconPath(const QString &emu,
							   const QString &title) const;
	QString desktopFilePath(const QString &emu,
							const QString &title) const;

	void buildLocalDirTree();

	void setCurrentEmu(const QString &name);

	QString diskDirPath() const;
	QString screenShotPath(const QString &title) const;
	QString stateDirPath(const QString &title) const;

	QStringList emus() const;
private:
	Q_DISABLE_COPY(PathManager)
	void createEmusSubtree(QDir &dir);

	QString m_currentEmu;
	QStringList m_emus;
	QString m_installationDirPath;
	QString m_userDataDirPath;
	QString m_diskDirBase;
};

inline QStringList PathManager::emus() const
{ return m_emus; }

inline QString PathManager::installationDirPath() const
{ return m_installationDirPath; }
inline QString PathManager::userDataDirPath() const
{ return m_userDataDirPath; }

BASE_EXPORT extern PathManager pathManager;

#endif // PATHMANAGER_H
