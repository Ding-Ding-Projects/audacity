/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QSet>
#include <QVariantList>

namespace au::toolkit {
//! A reusable multi-selection helper for any list model owned by a QML
//! surface. It tracks selected row indices, supports shift-range selection,
//! select-all with an explicit "this page" versus "every match" distinction,
//! inversion, and reports an honest count for a bulk-action preview.
//!
//! The controller never performs a destructive action itself; the caller
//! (a page's QML) decides what "delete" or "export" means for its own rows
//! and is responsible for routing a destructive action through its own
//! super confirmation surface before calling clearSelection().
class BulkSelectionModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(int totalCount READ totalCount WRITE setTotalCount NOTIFY totalCountChanged)
    Q_PROPERTY(bool allMatchesSelected READ allMatchesSelected NOTIFY selectionChanged)

public:
    explicit BulkSelectionModel(QObject* parent = nullptr);

    int selectedCount() const;
    int totalCount() const;
    void setTotalCount(int count);
    bool allMatchesSelected() const;

    Q_INVOKABLE bool isSelected(int index) const;
    Q_INVOKABLE void toggle(int index);
    Q_INVOKABLE void selectRange(int fromIndex, int toIndex);
    Q_INVOKABLE void selectAllOnPage(int pageStart, int pageEnd);
    Q_INVOKABLE void selectAllMatches();
    Q_INVOKABLE void invert();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE QVariantList selectedIndexes() const;

signals:
    void selectionChanged();
    void totalCountChanged();

private:
    QSet<int> m_selected;
    int m_totalCount = 0;
    bool m_allMatchesSelected = false;
};
}
