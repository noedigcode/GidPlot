#pragma once

#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief A floating legend overlay widget for QGeoView.
 *
 * Uses a standard QVBoxLayout with QLabels — no custom painting needed.
 * Parent it to your QGVMap and call snapToCorner() after show().
 *
 * Usage:
 *   auto* legend = new QGVLegendWidget(mapWidget);
 *   legend->addEntry(Qt::red, "Track");
 *   legend->show();
 *   legend->snapToCorner(Qt::BottomRightCorner);
 */
class QGVLegendWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGVLegendWidget(QWidget* parent = nullptr);

    void addEntry(const QColor& color, const QString& name);
    void setEntryColor(int index, QColor color);
    void setEntryName(int index, QString name);
    void removeEntry(int index);
    void clearEntries();

    void snapToCorner(Qt::Corner corner, const QPoint& margin = {10, 10});
    void setDraggable(bool draggable);

    void setPixelPos(QPoint pixelPos);
    void setNormPos(QPointF normPos);
    void updatePlacement();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event);

private:
    QVBoxLayout* mLayout = nullptr;
    bool mDraggable = true;
    bool mDragging = false;
    QPoint mDragOffset;
    QPointF mNormPos;

    struct Entry {

        Entry (QWidget* parent);

        QWidget* row = nullptr;
        QLabel* iconLabel = nullptr;
        QLabel* textLabel = nullptr;

        void setText(QString text);
        void setColor(QColor color);
    };
    typedef QSharedPointer<Entry> EntryPtr;

    QList<EntryPtr> mEntries;
};
