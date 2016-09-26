#include "radarscale.h"

static const rli_scale_t scales[APCTRL_SNDPULSE_TYPES][APCTRL_SCALE_NUM] =
{
    // Scales for short sounding pulse (SNDPLS_SHORT)
    {
        {  0.125f, SNDPLS_SHORT, 0x03200009, 0x001f0024, 0x001f4003,  0.025,   "0.125",  "0.025", "%.3f"},
        {  0.250f, SNDPLS_SHORT, 0x03200009, 0x001f0024, 0x001f4003,  0.050,   "0.25" ,  "0.05" , "%.3f"},
        {  0.500f, SNDPLS_SHORT, 0x0320000A, 0x001f0024, 0x001f4003,  0.100,   "0.5"  ,  "0.1"  , "%.3f"},
        {  0.750f, SNDPLS_SHORT, 0x0320000B, 0x001f0024, 0x001f4003,  0.150,   "0.75" ,  "0.15" , "%.3f"},
        {  1.500f, SNDPLS_SHORT, 0x06400013, 0x001f0024, 0x001f4003,  0.250,   "1.5"  ,  "0.25" , "%.3f"},
        {  3.000f, SNDPLS_SHORT, 0x0320000F, 0x001f0024, 0x001f4003,  0.500,   "3.0"  ,  "0.5"  , "%.3f"},
        {  6.000f, SNDPLS_SHORT, 0x06400017, 0x001f0024, 0x001f4003,  1.000,   "6.0"  ,  "1.0"  , "%.3f"},
        { 12.000f, SNDPLS_SHORT, 0x0960001E, 0x001f0024, 0x001f4007,  2.000,  "12.0"  ,  "2.0"  , "%.2f"},
        { 24.000f, SNDPLS_SHORT, 0x0960001E, 0x001f0024, 0x001f400F,  4.000,  "24.0"  ,  "4.0"  , "%.2f"},
        { 48.000f, SNDPLS_SHORT, 0x12C00036, 0x001f0024, 0x001f400F,  8.000,  "48.0"  ,  "8.0"  , "%.2f"},
        {  0.000f, SNDPLS_NONE,  0x12C00136, 0x001f0024, 0x001f400F,  0.000,   "0.0"  ,  "0.0"  , "%.1f"}, //  96 nm valid only for SNDPLS_LONG
        {  0.000f, SNDPLS_NONE,  0x19000145, 0x001f0024, 0x001f400F,  0.000,   "0.0"  ,  "0.0"  , "%.1f"}  // 144 nm valid only for SNDPLS_LONG
    },
    // Scales for long sounding pulse (SNDPLS_LONG)
    {
        {  0.000f, SNDPLS_NONE,  0x03200009, 0x001f0024, 0x001f4003,  0.025,   "0.0"  ,  "0.0"  , "%.3f"}, // 0.125 nm valid only for SNDPLS_SHORT
        {  0.000f, SNDPLS_NONE,  0x03200009, 0x001f0024, 0x001f4003,  0.050,   "0.0"  ,  "0.0"  , "%.3f"}, // 0.25  nm valid only for SNDPLS_SHORT
        {  0.500f, SNDPLS_LONG,  0x0320000A, 0x001f0024, 0x001f4003,  0.100,   "0.5"  ,  "0.1"  , "%.3f"},
        {  0.750f, SNDPLS_LONG,  0x0320000B, 0x001f0024, 0x001f4003,  0.150,   "0.75" ,  "0.15" , "%.3f"},
        {  1.500f, SNDPLS_LONG,  0x0640000D, 0x001f0024, 0x001f4003,  0.250,   "1.5"  ,  "0.25" , "%.3f"},
        {  3.000f, SNDPLS_LONG,  0x0320000F, 0x001f0024, 0x001f4003,  0.500,   "3.0"  ,  "0.5"  , "%.3f"},
        {  6.000f, SNDPLS_LONG,  0x06400017, 0x001f0024, 0x001f4003,  1.000,   "6.0"  ,  "1.0"  , "%.3f"},
        { 12.000f, SNDPLS_LONG,  0x06400017, 0x001f0024, 0x001f4007,  2.000,  "12.0"  ,  "2.0"  , "%.2f"},
        { 24.000f, SNDPLS_LONG,  0x06400017, 0x001f0024, 0x001f400F,  4.000,  "24.0"  ,  "4.0"  , "%.2f"},
        { 48.000f, SNDPLS_LONG,  0x0C800027, 0x001f0024, 0x001f400F,  8.000,  "48.0"  ,  "8.0"  , "%.2f"},
        { 96.000f, SNDPLS_LONG,  0x12C00136, 0x001f0024, 0x001f400F, 16.000,  "96.0"  , "16.0"  , "%.1f"},
        {144.000f, SNDPLS_LONG,  0x19000145, 0x001f0024, 0x001f400F, 24.000, "144.0"  , "24.0"  , "%.1f"}
    }
};

RadarScale::RadarScale()
{
    sndpls  = SNDPLS_SHORT;
    current = getScale(12.0f, sndpls);
    if(current == NULL)
        current = &scales[sndpls][0];
}

sndpls_t RadarScale::getNextSndPlsType(sndpls_t sndpls)
{
    if(sndpls >= SNDPLS_LAST)
        sndpls = SNDPLS_FIRST;
    else
        sndpls = (sndpls_t)(sndpls + 1);
    return sndpls;
}

sndpls_t RadarScale::getPrevSndPlsType(sndpls_t sndpls)
{
    if(sndpls <= SNDPLS_FIRST)
        sndpls = SNDPLS_LAST;
    else
        sndpls = (sndpls_t)(sndpls - 1);
    return sndpls;
}

const rli_scale_t * RadarScale::getScale(float len, sndpls_t sndpls)
{
    const rli_scale_t * pscale = NULL;
    if((sndpls < SNDPLS_FIRST) || (sndpls > SNDPLS_LAST))
        return NULL;
    for(int i = 0; i < APCTRL_SCALE_NUM; i++)
    {
        if(scales[sndpls][i].len == len)
        {
            pscale = &scales[sndpls][i];
            break;
        }
    }

    return pscale;
}

std::pair<QByteArray, QByteArray> RadarScale::getCurScaleText(void)
{
    std::pair<QByteArray, QByteArray> s;
    s.first  = current->display;
    s.second = current->dist_rng_display;

    return s;
}

const rli_scale_t *RadarScale::getCurScale(void)
{
    return current;
}

int RadarScale::nextScale(void)
{
    int                 res = 0;
    const rli_scale_t * nsc = NULL; // Pointer to a new scale

    if(current == NULL)
    {
        sndpls  = SNDPLS_SHORT;
        current = getScale(12.0f, sndpls);
        nsc = current;
    }
    else
    {
        for(int i = 0; i < APCTRL_SCALE_NUM - 1; i++)
        {
            if(current == &scales[sndpls][i])
            {
                nsc = &scales[sndpls][i + 1];
                if(nsc->sndpls == SNDPLS_NONE)
                {
                    sndpls = getPrevSndPlsType(sndpls);
                    nsc = &scales[sndpls][i + 1];
                }
            }
        }
    }

    if(nsc == NULL)
        return 1;

    current = nsc;

    return 0;
}

int RadarScale::prevScale(void)
{
    int                 res = 0;
    const rli_scale_t * nsc = NULL; // Pointer to a new scale

    if(current == NULL)
    {
        sndpls  = SNDPLS_SHORT;
        current = getScale(12.0f, sndpls);
        nsc = current;
    }
    else
    {
        for(int i = 1; i < APCTRL_SCALE_NUM; i++)
        {
            if(current == &scales[sndpls][i])
            {
                nsc = &scales[sndpls][i - 1];
                if(nsc->sndpls == SNDPLS_NONE)
                {
                    sndpls = getNextSndPlsType(sndpls);
                    nsc = &scales[sndpls][i - 1];
                }
            }
        }
    }

    if(nsc == NULL)
        return 1;

    current = nsc;

    return 0;
}
