#pragma once

#include <QString>
#include <stl/string.h>

inline QString str2q(const sf::stl::string& str) { return QString::fromLatin1(str.data(), str.size()); }

inline sf::stl::string q2str(const QString& q) { return sf::stl::string{q.toLatin1().data()}; }
