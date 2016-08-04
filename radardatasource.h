#ifndef RADARDATASOURCE_H
#define RADARDATASOURCE_H

#include <stdint.h>
#include <QObject>

#if QT_VERSION >= 0x050000
    #include <QtConcurrent/QtConcurrentRun>
#else
    #include <QtConcurrentRun>
#endif

const int PELENG_SIZE         = 800;
const int BEARINGS_PER_CYCLE  = 4096;
const int BEARING_PACK_SIZE   = (800 + 3) * sizeof(uint32_t); // Number of words in DMA transaction
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
    uint32_t * buf;
};

#endif //Q_OS_WIN
// ------------------------------------------------------

class RadarDataSource : public QObject {
  Q_OBJECT
public:
  explicit RadarDataSource();
  virtual ~RadarDataSource();

  void start();
  void start(const char * radarfn);

  void finish();

signals:
  void updateData(uint offset, uint count, float* divs, float* amps);

private:
  bool loadData();
  bool initWithDummy(float* divs, float* amps);
  bool loadObserves1(char* filename, float* divs, float* amps);
  bool loadObserves2(char* filename, float* divs, float* amps);

  bool finish_flag;
  float file_divs[2][BEARINGS_PER_CYCLE];
  float file_amps[2][BEARINGS_PER_CYCLE * PELENG_SIZE];
  uint  file_curr;

  void worker();
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
  BearingBuffer  * bufpool;
  unsigned long     bufpoolsize;
  //int              nextbuf; // Next available buffer

  // Pool of buffers awaiting for DMA read completition
  //BearingBuffer ** rdpool;
  unsigned long    rdpnext;
  unsigned long    rdpqueued;

  // Scans
  BearingBuffer ** scans[RDS_MAX_SCANS];
  int              activescan;
  uint32_t        processed_bearing;
  uint32_t        last_bearing;

  int              fd;         // Radar device file descriptor
#endif //Q_OS_WIN
// ------------------------------------------------------
};

#endif // RADARDATASOURCE_H

