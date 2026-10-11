#ifndef PORTSELECTION_H
#define PORTSELECTION_H
#include <QComboBox>
#include <QStringList>

// Rebuild from an authoritative snapshot. Never silently select a replacement
// for a disconnected device; identity is stored as item data, not list position.
void refreshPortSelection(QComboBox *combo, const QStringList &names,
                          const QStringList &labels);
#endif
