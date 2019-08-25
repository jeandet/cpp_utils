#ifndef SCIQLOP_STRINGUTILS_H
#define SCIQLOP_STRINGUTILS_H

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

  template<typename container>
  QString join(const container& input, const char* sep)
  {
    QStringList list;
    if constexpr(std::is_same_v<typename container::value_type, std::string>)
    {
      std::transform(
          std::cbegin(input), std::cend(input), std::back_inserter(list),
          [](const auto& item) { return QString::fromStdString(item); });
    }
    else if constexpr(std::is_same_v<typename container::value_type, QString>)
    {
      std::copy(std::cbegin(input), std::cend(input), std::back_inserter(list));
    }
    return list.join(sep);
  }

  template<typename container>
  QString join(const container& input, const char* sep,
               std::function<QString(typename container::value_type)> op)
  {
    QStringList list;
    std::transform(std::cbegin(input), std::cend(input),
                   std::back_inserter(list), op);
    return list.join(sep);
  }
} // namespace StringUtils

#endif // SCIQLOP_STRINGUTILS_H
