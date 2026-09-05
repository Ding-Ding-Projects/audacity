/*
* Audacity: A Digital Audio Editor
*/

#include "bulkselectionmodel.h"

using namespace au::toolkit;

BulkSelectionModel::BulkSelectionModel(QObject* parent)
    : QObject(parent)
{
}

int BulkSelectionModel::selectedCount() const
{
    return m_allMatchesSelected ? m_totalCount : m_selected.size();
}

int BulkSelectionModel::totalCount() const
{
    return m_totalCount;
}

void BulkSelectionModel::setTotalCount(int count)
{
    if (m_totalCount == count) {
        return;
    }
    m_totalCount = count;
    emit totalCountChanged();
    emit selectionChanged();
}

bool BulkSelectionModel::allMatchesSelected() const
{
    return m_allMatchesSelected;
}

bool BulkSelectionModel::isSelected(int index) const
{
    return m_allMatchesSelected || m_selected.contains(index);
}

void BulkSelectionModel::toggle(int index)
{
    if (m_allMatchesSelected) {
        // Leaving the "every match" state to deselect one row: fall back to
        // an explicit set containing everything except the toggled index.
        m_allMatchesSelected = false;
        m_selected.clear();
        for (int i = 0; i < m_totalCount; ++i) {
            if (i != index) {
                m_selected.insert(i);
            }
        }
    } else if (m_selected.contains(index)) {
        m_selected.remove(index);
    } else {
        m_selected.insert(index);
    }
    emit selectionChanged();
}

void BulkSelectionModel::selectRange(int fromIndex, int toIndex)
{
    if (m_allMatchesSelected) {
        return;
    }
    const int lo = std::min(fromIndex, toIndex);
    const int hi = std::max(fromIndex, toIndex);
    for (int i = lo; i <= hi; ++i) {
        m_selected.insert(i);
    }
    emit selectionChanged();
}

void BulkSelectionModel::selectAllOnPage(int pageStart, int pageEnd)
{
    m_allMatchesSelected = false;
    for (int i = pageStart; i <= pageEnd; ++i) {
        m_selected.insert(i);
    }
    emit selectionChanged();
}

void BulkSelectionModel::selectAllMatches()
{
    m_allMatchesSelected = true;
    m_selected.clear();
    emit selectionChanged();
}

void BulkSelectionModel::invert()
{
    if (m_allMatchesSelected) {
        m_allMatchesSelected = false;
        m_selected.clear();
        emit selectionChanged();
        return;
    }

    QSet<int> inverted;
    for (int i = 0; i < m_totalCount; ++i) {
        if (!m_selected.contains(i)) {
            inverted.insert(i);
        }
    }
    m_selected = inverted;
    emit selectionChanged();
}

void BulkSelectionModel::clearSelection()
{
    m_allMatchesSelected = false;
    m_selected.clear();
    emit selectionChanged();
}

QVariantList BulkSelectionModel::selectedIndexes() const
{
    QVariantList list;
    if (m_allMatchesSelected) {
        for (int i = 0; i < m_totalCount; ++i) {
            list << i;
        }
        return list;
    }
    QList<int> sorted = m_selected.values();
    std::sort(sorted.begin(), sorted.end());
    for (int i : sorted) {
        list << i;
    }
    return list;
}
