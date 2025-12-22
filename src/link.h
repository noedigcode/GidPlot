#ifndef LINK_H
#define LINK_H

#include <QSharedPointer>
#include <QString>

class Link
{
public:
    QString tag;
    int group = 0;
    bool supportPosZoom = false;
    bool linkXpos = false;
    bool linkYpos = false;
    bool linkXzoom = false;
    bool linkYzoom = false;

    bool match(int otherGroup);
};

typedef QSharedPointer<Link> LinkPtr;

#endif // LINK_H
