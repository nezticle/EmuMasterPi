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

#include "configuration.h"
#include "emu.h"
#include <QtCore/QCoreApplication>
#include <QtGui/QColor>

Configuration emConf;

/*!
	\class Configuration
	Configuration class holds configuration values.
	Configuration is saved/loaded to/from the state before any other data.
 */

/*! Creates a Configuration object. */
Configuration::Configuration()
{
	constructDefaults();
}

/*! Sets configuration item \a name to the given \a value. */
void Configuration::setValue(const QString &name, const QVariant &value)
{
	m_data[name] = value;
}

/*!
	Returns configuration item \a name.
	If item doesn't exist \a defaultValue is returned.
 */
QVariant Configuration::value(const QString &name,
							  const QVariant &defaultValue) const
{
	if (!m_data.contains(name))
		return defaultValue;
	return m_data.value(name);
}

/*!
	Some items of configurations are global, and this function returns
	their default value. For given \a name its default value is returned.
 */
QVariant Configuration::defaultValue(const QString &name) const
{
	return m_defaultData.value(name);
}

/*! Save/load functionality */
void Configuration::sl()
{
	emsl.begin("conf");
	emsl.var("data", m_data);
	emsl.end();
}

/*! \internal */
void Configuration::constructDefaults()
{
    m_defaultData.insert("audioEnable", false);
	m_defaultData.insert("autoSaveLoadEnable", true);
    m_defaultData.insert("buttonsVisible", false);
    m_defaultData.insert("frameSkip", 0);
    m_defaultData.insert("fpsVisible", false);
	m_defaultData.insert("gridColor", QColor(Qt::white));
	m_defaultData.insert("gridVisible", false);
	m_defaultData.insert("hapticFeedbackEnable", false);
	m_defaultData.insert("keepAspectRatio", true);
	m_defaultData.insert("lrButtonsVisible", false);
	m_defaultData.insert("padOpacity", 0.45f);
	m_defaultData.insert("runInBackground", false);
	m_defaultData.insert("swipeEnable", false);
	m_defaultData.insert("touchAreaSize", 240);
	m_defaultData.insert("touchAreaDiagonalSize", 240/4);
    m_defaultData.insert("videoFilter", "hq4x");
}

/*!
	Setups basic informations about the application.
	It should be called right after creation of QApplication object.
	It's used by QSettings later.
 */
void Configuration::setupAppInfo()
{
    QCoreApplication::setOrganizationName("nezticle");
    QCoreApplication::setOrganizationDomain("bsquask.com");
    QCoreApplication::setApplicationName("emumaster");
	// TODO change on every release
    QCoreApplication::setApplicationVersion("0.0.1");
}
