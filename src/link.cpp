#include "link.h"


bool Link::match(int otherGroup)
{
    return (group && (group == otherGroup));
}
