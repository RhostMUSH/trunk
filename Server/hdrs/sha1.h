#ifndef SHA1_H
#define SHA1_H
#ifndef UINT64
#define UINT64 unsigned long long
#endif
#ifndef UINT32
#define UINT32 unsigned int
#endif
#ifndef UINT8
#define UINT8 unsigned char
#endif

typedef struct
{
    UINT64   nTotal;
    UINT32   H[5];
    UINT8    block[64];
    size_t   nblock;
} MUSH_SHA1_CONTEXT;

void MUSH_SHA1_Init(MUSH_SHA1_CONTEXT *p);
void MUSH_SHA1_Compute(MUSH_SHA1_CONTEXT *p, size_t n, const char *buf);
void MUSH_SHA1_Final(MUSH_SHA1_CONTEXT *p);

#endif // SHA1_H
