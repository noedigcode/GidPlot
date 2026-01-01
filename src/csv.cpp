#include "csv.h"


QList<RangePtr> Csv::ranges()
{
    return mRanges;
}

void Csv::addRange(RangePtr range)
{
    mRanges.append(range);
    emit rangeAdded(range);
}

Range Csv::allRange()
{
    return Range("All", 0, matrix->rowCount());
}
