#ifndef _REPLACE_PATTERN_H_
#define _REPLACE_PATTERN_H_
#include <iostream>
#include "Tools.h"


class ReplacePattern
{
private:
    ulong* answer;
    unsigned sampleRate;
    void createtable(unsigned);

public:
    ReplacePattern(unsigned, unsigned);
    ~ReplacePattern();
    ulong returnWord(ulong *, ulong, ulong);
    ulong* returnRP(ulong *, ulong, ulong, ulong);
};

#endif

