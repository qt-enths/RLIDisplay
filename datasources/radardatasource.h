#ifndef RADARDATASOURCE_H
#define RADARDATASOURCE_H

#include "radarscale.h"

#include <stdint.h>
#include <QObject>
#include <QtOpenGL>

#if QT_VERSION >= 0x050000
    #include <QtConcurrent/QtConcurrentRun>
#else
    #include <QtConcurrentRun>
#endif

const int PELENG_SIZE         = 800;
const int BEARINGS_PER_CYCLE  = 4096;
const int BEARING_PACK_SIZE   = (800 + 3) * sizeof(uint32_t); // Number of information bytes in DMA transaction
const int BEARING_PACK_WORDS  = 1024; // Number of words in DMA transaction
const int RDS_MAX_SCANS       = 3;
const int RDS_RDPOOL_SIZE     = 256;

#include <QtGlobal>

#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------

class BearingBuffer {
public:
    BearingBuffer(size_t datasize = BEARING_PACK_SIZE, size_t pagesize = 4096) : nr(0), ptr(NULL), used(false), valid(false), buf(NULL) {this->datasize = datasize; this->pagesize = pagesize;}
    ~BearingBuffer(){/*if(buf) delete [] buf; */buf = ptr = NULL; used = false; valid = false;}

    bool create(void){buf = new uint32_t[pagesize * 2 / sizeof(uint32_t)]; ptr = buf; if(!buf) return false; else return true;}
    bool align(void){/*if(!buf) return false; if(((uint64_t)buf) & (pagesize - 1)) ptr = (uint32_t *)((((uint64_t)buf) & ~(pagesize - 1)) + pagesize); */return true;}
    bool bind(uint32_t * b, uint32_t number) {if(buf) return false; ptr = buf = b; nr = number; return true;}


    uint32_t   nr;
    uint32_t * ptr;
    bool       used;
    bool       valid;

    size_t     datasize;
    size_t     pagesize;

protected:
    u_int32_t * buf;
};

#endif //Q_OS_WIN
// ------------------------------------------------------

class RadarDataSource : public QObject {
  Q_OBJECT
public:
  explicit RadarDataSource();
  virtual ~RadarDataSource();

  void start();
  void start_dump();
  void start(const char * radarfn);

  void finish();

  int getAmpsOffset(void);
  int preprocessBearing(u_int32_t * brgdata, bool inv);
  int simulate(bool sim);

public slots:
  void updateHeading(float hdg);
  void setGain(u_int32_t gain);
  void setAmpsOffset(int off);
  void onSimulationChanged(const QByteArray& str);

signals:
  void updateData(uint offset, uint count, GLfloat* amps);

private:
  bool loadData();
  bool initWithDummy(GLfloat* amps);
  bool loadObserves1(char* filename, GLfloat* amps);
  bool loadObserves2(char* filename, GLfloat* amps);

  bool finish_flag;
  GLfloat file_amps[2][BEARINGS_PER_CYCLE * PELENG_SIZE];
  uint  file_curr;

  void worker();
  void dump_worker();
  void radar_worker();

  QFuture<void> workerThread;

#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------

  int setRawBearingData(BearingBuffer * bearing);
  BearingBuffer * getNextFreeBuffer(void);
  int processBearings(void);

  enum radar_sync_stage
  {
      RDSS_FIRST     = 0,
      RDSS_SYNC      = 0, // Radar controller is synchronized
      RDSS_NOTSYNC   = 1, // No synchronization with radar controller
      RDSS_WAIT4STAB = 2, // Zero bearing has been found; now check for stable bearing stream
      RDSS_LAST      = 2
  };

  enum radar_sync_stage syncstate;

  // Buffer pool
  BearingBuffer   * bufpool;
  unsigned long     bufpoolsize;
  //int              nextbuf; // Next available buffer

  // Pool of buffers awaiting for DMA read completition
  //BearingBuffer ** rdpool;
  unsigned long    rdpnext;
  unsigned long    rdpqueued;

  // Scans
  BearingBuffer ** scans[RDS_MAX_SCANS];
  int              activescan;
  uint32_t         processed_bearing;
  uint32_t         last_bearing;

  int              fd;         // Radar device file descriptor
#endif // !Q_OS_WIN
// ------------------------------------------------------

  // Zero offset
  int              ampoffset; // Radar data offset

  u_int32_t * dump;
  off_t       dsize;

  int findZeroBrg(int start);
  int chkScans(int start);

  u_int32_t gain_level; // Amplification level 0..max_alevel

#ifndef Q_OS_WIN
  int initGen(bool usetimeout);
  int initADC(void);
  int initDAC(void);
  int apctrl_regwr(u_int32_t regaddr, u_int32_t regval);
  int apctrl_regrd(u_int32_t regaddr, u_int32_t * regval);

  // ADC SPI manipulation data and routines
  timer_t adcspi_tmid;
  int apctrl_adcspi_send(u_int32_t baseaddr, u_int32_t addrv, u_int32_t datav, bool usetimeout);
#endif // !Q_OS_WIN

public:

  int setupScale(const rli_scale_t * pscale);
  static const u_int32_t max_gain_level; // Maximum amplification level
  int amplify(u_int32_t * brg);

  enum hip_t
  {
      HIP_FIRST  = 1,
      HIP_NONE   = 1,
      HIP_WEAK   = 2,
      HIP_STRONG = 3,
      HIP_LAST   = 3
  };

  enum hip_channel_t
  {
      HIPC_FIRST = 0,
      HIPC_MAIN  = 0,
      HIPC_SARP  = 1,
      HIPC_LAST  = 1
  };

  int nextHIP(hip_channel_t hipch);

protected:
  u_int32_t hipregv;
  hip_t hip_main;
  hip_t hip_sarp;
  int setupHIP(hip_t hiptype, hip_channel_t hipch);

  bool simulation;

  u_int32_t gyroReg;
};

#endif // RADARDATASOURCE_H

