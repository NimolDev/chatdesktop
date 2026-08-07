#ifndef SHARED_THEME_FONT_MANAGER_HPP
#define SHARED_THEME_FONT_MANAGER_HPP

#include <QString>

namespace shared {
namespace theme {

class FontManager
{
public:
    static bool initialize();
    static QString family();
};

} //namespace theme
} // namespace shared

#endif // SHARED_THEME_FONT_MANAGER_HPP
