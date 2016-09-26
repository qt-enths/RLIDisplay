#ifndef RADARSCALE_H
#define RADARSCALE_H

#include <utility>
#include <sys/types.h>
#include <qbytearray.h>

#define APCTRL_SCALE_NUM         12 // Number of RLI scales
#define APCTRL_SNDPULSE_TYPES    2  // Numer of sounding pulse types (now only two: short and long)
#define APCTRL_SNDPULSE_SHORT    0  // Index in scales table for short sounding pulse
#define APCTRL_SNDPULSE_LONG     1  // Index in scales table for long sounding pulse

// Data types for managing scales
enum soundingpulses
{
    SNDPLS_NONE  = -1,
    SNDPLS_FIRST =  0,
    SNDPLS_SHORT =  0,
    SNDPLS_LONG  =  1,
    SNDPLS_LAST  =  1
};

typedef soundingpulses sndpls_t;

struct rli_scale
{
    float        len;      // Length of this scale in miles; 0 means invalid scale
    sndpls_t     sndpls;   // Type of sounding pulse
    u_int32_t    pkidpkod; // Settings for PKID and PKOD
    u_int32_t    gen_addr; // Value to set the address of ADC Frequency Register
    u_int32_t    gen_dat;  // Value to set the contents of ADC Frequency Register
    float        dist_rng; // Space in miles between distance rings
    const char * display;  // Name of the scale for displaying
    const char * dist_rng_display; // How to display space size between distance rings
    const char * val_fmt;  // printf format to display float value
};

typedef struct rli_scale rli_scale_t;

class RadarScale
{
public:
    RadarScale();

    sndpls_t sndpls;
    const rli_scale_t * current;

    static sndpls_t getNextSndPlsType(sndpls_t sndpls);
    static sndpls_t getPrevSndPlsType(sndpls_t sndpls);
    static const rli_scale_t * getScale(float len, sndpls_t sndpls);
    std::pair<QByteArray, QByteArray> getCurScaleText(void);
    const rli_scale_t * getCurScale(void);
    int nextScale(void);
    int prevScale(void);
};

#endif // RADARSCALE_H
