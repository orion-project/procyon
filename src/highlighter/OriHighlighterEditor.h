#ifndef ORI_HIGHLIGHTER_EDITOR_H
#define ORI_HIGHLIGHTER_EDITOR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Ori {
namespace Highlighter {

struct Spec;
class SpecStorage;

class EditorDialog : public QWidget
{
    Q_OBJECT

public:
    static EditorDialog* create(const QSharedPointer<SpecStorage>& storage, const QSharedPointer<Spec>& base);
    static EditorDialog* edit(const QSharedPointer<Spec>& spec);

private:
    QSharedPointer<SpecStorage> _storage;
    QSharedPointer<Spec> _spec, _target;
    QPlainTextEdit *_code, *_sample;

    explicit EditorDialog();
    void createWidgets();
};

} // namespace Highlighter
} // namespace Ori

#endif // ORI_HIGHLIGHTER_EDITOR_H
