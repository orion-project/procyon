#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <QLabel>

namespace Z {
namespace Gui {

inline void setValueFont(QWidget *widget)
{
    QFont f = widget->font();
    if (f.pointSize() < 10)
    {
        f.setPointSize(10);
        widget->setFont(f);
    }
}

inline void setSymbolFont(QWidget *widget)
{
    QFont f = widget->font();
    f.setBold(true);
    f.setPointSize(14);
    f.setFamily("Times New Roman");
    widget->setFont(f);
}

inline QLabel* makeSymbolLabel(const QString& text)
{
    auto label = new QLabel(text);
    setSymbolFont(label);
    return label;
}

inline QLabel* makeHeaderLabel(const QString& title)
{
    QLabel* label = new QLabel(title);
    auto font = label->font();
    font.setBold(true);
    font.setPointSize(font.pointSize()+1);
    label->setFont(font);
    return label;
}

} // namespace Gui
} // namespace Z

#endif // APPEARANCE_H
