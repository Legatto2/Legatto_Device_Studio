#include "portselection.h"
#include <QSignalBlocker>

void refreshPortSelection(QComboBox *combo, const QStringList &names,
                          const QStringList &labels)
{
    if (!combo) return;
    const QString selectedName = combo->currentData().toString();
    const QSignalBlocker blocker(combo);
    combo->clear();
    if (names.size() != labels.size()) return;
    for (int index = 0; index < names.size(); ++index)
        combo->addItem(labels.at(index), names.at(index));
    combo->setCurrentIndex(selectedName.isEmpty() ? -1 : combo->findData(selectedName));
}
