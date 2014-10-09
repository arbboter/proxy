// NatRunner.cpp: implements for the CNatRunner class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatRunner.h"

#include "NatDebug.h"

NatRunResult NatRunResultMax(NatRunResult value1, NatRunResult value2)
{
    if (value1 < value2)
    {
        return value2;
    }
    return value1;
}

