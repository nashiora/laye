#include "layec/diagnostic.h"

layec_location layec_location_combine(layec_location a, layec_location b)
{
    assert(a.fileId == b.fileId);

    usize startOffset = 0;
    usize endOffset = 0;

    if (a.offset < b.offset)
    {
        startOffset = a.offset;
        endOffset = b.offset + b.length;
    }
    else
    {
        startOffset = b.offset;
        endOffset = a.offset + a.length;
    }

    return (layec_location){
        .fileId = a.fileId,
        .offset = startOffset,
        .length = endOffset - startOffset,
    };
}
