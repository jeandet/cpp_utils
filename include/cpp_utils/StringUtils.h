#pragma once
/*------------------------------------------------------------------------------
--  This file is a part of cpp_utils
--  Copyright (C) 2019, Plasma Physics Laboratory - CNRS
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
-------------------------------------------------------------------------------*/
/*--                  Author : Alexis Jeandet
--                     Mail : alexis.jeandet@lpp.polytechnique.fr
--                            alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/

#include "CoreGlobal.h"

#include <QRegExp>
#include <QString>
#include <QStringList>
#include <functional>
#include <set>
#include <vector>

/**
 * Utility class with methods for strings
 */
namespace StringUtils
{
  /**
   * Generates a unique name from a default name and a set of forbidden names.
   *
   * Generating the unique name is done by adding an index to the default name
   * and stopping at the first index for which the generated name is not in the
   * forbidden names.
   *
   * Examples (defaultName, forbiddenNames -> result):
   * - "FGM", {"FGM"} -> "FGM1"
   * - "FGM", {"ABC"} -> "FGM"
   * - "FGM", {"FGM", "FGM1"} -> "FGM2"
   * - "FGM", {"FGM", "FGM2"} -> "FGM1"
   * - "", {"ABC"} -> "1"
   *
   * @param defaultName the default name
   * @param forbiddenNames the set of forbidden names
   * @return the unique name generated
   */
  static QString uniqueName(const QString& defaultName,
                            const std::vector<QString>& forbiddenNames) noexcept
  {
    // Gets the base of the unique name to generate, by removing trailing number
    // (for example, base name of "FGM12" is "FGM")
    auto baseName = defaultName;
    baseName.remove(QRegExp{QStringLiteral("\\d*$")});

    // Finds the unique name by adding an index to the base name and stops when
    // the generated name isn't forbidden
    QString newName{};
    auto forbidden = true;
    for(auto i = 0; forbidden; ++i)
    {
      newName = (i == 0) ? baseName : baseName + QString::number(i);
      forbidden =
          newName.isEmpty() ||
          std::any_of(forbiddenNames.cbegin(), forbiddenNames.cend(),
                      [&newName](const auto& name) {
                        return name.compare(newName, Qt::CaseInsensitive) == 0;
                      });
    }

    return newName;
  }

} // namespace StringUtils


