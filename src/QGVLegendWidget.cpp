#include "QGVLegendWidget.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

QGVLegendWidget::QGVLegendWidget(QWidget* parent)
    : QWidget(parent)
{
    // Float on top of the map
    setAttribute(Qt::WA_AlwaysStackOnTop);

    mLayout = new QVBoxLayout(this);
    mLayout->setContentsMargins(mContentMargins);
    mLayout->setSpacing(mRowSpacing);
}

void QGVLegendWidget::addEntry(const QColor& color, const QString& name)
{
    EntryPtr entry = EntryPtr::create(this);

    entry->setColor(color);
    entry->setText(name);
    mLayout->addWidget(entry->row);

    queueUpdateSizeAndPlacement();

    mEntries.append(entry);
}

void QGVLegendWidget::setEntryColor(int index, QColor color)
{
    EntryPtr entry = mEntries.value(index);
    if (!entry) { return; }

    entry->setColor(color);
}

void QGVLegendWidget::setEntryName(int index, QString name)
{
    EntryPtr entry = mEntries.value(index);
    if (!entry) { return; }

    entry->setText(name);
    queueUpdateSizeAndPlacement();
}

void QGVLegendWidget::removeEntry(int index)
{
    QLayoutItem* item = mLayout->takeAt(index);
    if (item) {
        delete item->widget();
        delete item;
        if ((index >= 0) && (index < mEntries.count())) {
            mEntries.removeAt(index);
        }
    }
    queueUpdateSizeAndPlacement();
}

void QGVLegendWidget::snapToCorner(Qt::Corner corner, const QPoint& margin)
{
    if (!parentWidget()) { return; }

    adjustSize(); // ensure size() is up to date
    const QSize p = parentWidget()->size();
    const QSize s = size();

    QPoint pos;
    switch (corner) {
    case Qt::TopLeftCorner:
        pos = {margin.x(), margin.y()};
        break;
    case Qt::TopRightCorner:
        pos = {p.width() - s.width() - margin.x(), margin.y()};
        break;
    case Qt::BottomLeftCorner:
        pos = {margin.x(), p.height() - s.height() - margin.y()};
        break;
    case Qt::BottomRightCorner:
        pos = {p.width()  - s.width()  - margin.x(),
               p.height() - s.height() - margin.y()};
        break;
    }
    setPixelPos(pos);
}

void QGVLegendWidget::setDraggable(bool draggable)
{
    mDraggable = draggable;
}

void QGVLegendWidget::setPixelPos(QPoint pixelPos)
{
    if (!parentWidget()) { return; }

    QSize parentSize = parentWidget()->size();

    setNormPos(QPointF(
                   pixelPos.x() / (double)parentSize.width(),
                   pixelPos.y() / (double)parentSize.height()));
}

void QGVLegendWidget::setNormPos(QPointF normPos)
{
    mNormPos = normPos;
    updatePlacement();
}

void QGVLegendWidget::updatePlacement()
{
    if (!parentWidget()) { return; }

    QSize parentSize = parentWidget()->size();
    QSize legendSize = this->size();

    QPoint pixelPos(
                mNormPos.x() * (double)parentSize.width(),
                mNormPos.y() * (double)parentSize.height());

    // Ensure legend is within view
    if ((pixelPos.x() + legendSize.width()) > parentSize.width()) {
        pixelPos.setX(parentSize.width() - legendSize.width());
    }
    if (pixelPos.x() < 0) { pixelPos.setX(0); }
    if ((pixelPos.y() + legendSize.height()) > parentSize.height()) {
        pixelPos.setY(parentSize.height() - legendSize.height());
    }
    if (pixelPos.y() < 0) { pixelPos.setY(0); }

    move(pixelPos);
}

void QGVLegendWidget::queueUpdateSizeAndPlacement()
{
    QMetaObject::invokeMethod(this, [=]()
    {
        adjustSize();
        updatePlacement();
    }, Qt::QueuedConnection);
}

void QGVLegendWidget::mousePressEvent(QMouseEvent* event)
{
    if (mDraggable && event->button() == Qt::LeftButton) {
        mDragging   = true;
        mDragOffset = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void QGVLegendWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mDragging) {
        QPoint newPos = mapToParent(event->pos()) - mDragOffset;
        if (parentWidget()) {
            const QRect bounds = parentWidget()->rect();
            newPos.setX(qBound(0, newPos.x(), bounds.width()  - width()));
            newPos.setY(qBound(0, newPos.y(), bounds.height() - height()));
        }
        setPixelPos(newPos);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void QGVLegendWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (mDragging && event->button() == Qt::LeftButton) {
        mDragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void QGVLegendWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

QGVLegendWidget::Entry::Entry(QWidget *parent)
{
    row = new QWidget(parent);
    QHBoxLayout* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(iconTextPadding);

    iconLabel = new QLabel(row);
    iconLabel->setFixedSize(iconSize);

    textLabel = new QLabel(row);

    rowLayout->addWidget(iconLabel);
    rowLayout->addWidget(textLabel);
    rowLayout->addStretch();
}

void QGVLegendWidget::Entry::setText(QString text)
{
    if (!textLabel) { return; }

    textLabel->setText(text);
}

void QGVLegendWidget::Entry::setColor(QColor color)
{
    if (!iconLabel) { return; }

    QPixmap pixmap(iconSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(color, 2));
    painter.drawLine(0, pixmap.height() / 2, pixmap.width(), pixmap.height() / 2);
    painter.end();

    iconLabel->setPixmap(pixmap);
}
