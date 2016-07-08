#include "radardatasource.h"
#include "xpmon_be.h"

#include <QThread>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdint.h>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#else  //  Q_OS_WIN
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif //  Q_OS_WIN

#define WRITE_2SCANS
//#define PRINTERRORS


void qSleep(int ms) {
  if (ms <= 0) return;
#ifdef Q_OS_WIN
  Sleep(uint(ms));
#else
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
  nanosleep(&ts, NULL);
#endif
}

RadarDataSource::RadarDataSource()
    : file_curr(0)
    , syncstate(RDSS_NOTSYNC)
    , bufpool(NULL)
    , nextbuf(0)
    , rdpool(NULL)
    , rdpnext(0)
    , rdpqueued(0)
    , activescan(0)
    , processed_bearing(0)
    , last_bearing(0)
    , fd(-1)
{
  finish_flag = true;
  loadData();
  
  for(int scanidx = 0; scanidx < RDS_MAX_SCANS; scanidx++)
    scans[scanidx] = NULL;
}

RadarDataSource::~RadarDataSource() {
  finish_flag = true;
  while(workerThread.isRunning());
  if(fd != -1) {
#ifdef Q_OS_WIN
    _close(fd);
#else
    ::close(fd);
#endif
    fd = -1;
  }

  for(int scanidx = 0; scanidx < RDS_MAX_SCANS; scanidx++) {
    if(scans[scanidx]) {
      delete [] scans[scanidx];
      scans[scanidx] = NULL;
    }
  }

  if(rdpool) {
    delete [] rdpool;
    rdpool = NULL;
  }

  if(bufpool) {
    delete [] bufpool;
    bufpool = NULL;
  }
}

void RadarDataSource::start() {
  if (workerThread.isRunning())
    return;

  finish_flag = false;
  workerThread = QtConcurrent::run(this, &RadarDataSource::worker);
}

void RadarDataSource::start(const char * radarfn) {
  if(!radarfn)
    return;
  if (workerThread.isRunning())
    return;

  if(!bufpool) {
    bufpool = new BearingBuffer[BEARINGS_PER_CYCLE * (RDS_MAX_SCANS + 1)];

    if(!bufpool) {
#ifdef PRINTERRORS
      fprintf(stderr, "%s: failed to create bearing buffer pool\n", __func__);
#endif // PRINTERRORS
      return;
    }

    for(int brgidx = 0; brgidx < BEARINGS_PER_CYCLE * (RDS_MAX_SCANS + 1); brgidx++) {
      bufpool[brgidx].create();
      bufpool[brgidx].align();
    }

    nextbuf = 0;
  }

  if(!rdpool) {
    rdpool = new BearingBuffer * [RDS_RDPOOL_SIZE];

    if(!rdpool) {
#ifdef PRINTERRORS
      fprintf(stderr, "%s: failed to create read pool\n", __func__);
#endif // PRINTERRORS
      return;
    }

    memset(rdpool, 0, sizeof(BearingBuffer *) * RDS_RDPOOL_SIZE);
    rdpnext   = 0;
    rdpqueued = 0;
  }

  for(int scanidx = 0; scanidx < RDS_MAX_SCANS; scanidx++) {
    if(!scans[scanidx]) {
      //scans[scanidx] = new BearingBuffer * [BEARINGS_PER_CYCLE];
      BearingBuffer ** pptr = new BearingBuffer * [BEARINGS_PER_CYCLE];
#ifdef PRINTERRORS
      fprintf(stderr, "%s: pointer array %d: %p\n", __func__, scanidx, pptr);
#endif
      scans[scanidx] = pptr;

      if(!scans[scanidx]) {
#ifdef PRINTERRORS
        fprintf(stderr, "%s: failed to create scan container %d\n", __func__, scanidx);
#endif // PRINTERRORS
        return;
      }

      memset(scans[scanidx], 0, sizeof(BearingBuffer *) * BEARINGS_PER_CYCLE);
      activescan        = 0;
      processed_bearing = 0;
      last_bearing      = 0;
    }
  }

  if(fd == -1) {
#ifdef Q_OS_WIN
    _sopen_s(&fd, radarfn, O_RDONLY, 0, 0);
#else
    fd = ::open(radarfn, O_RDONLY);
#endif

    if(fd == -1) {
#ifdef PRINTERRORS
      perror("Failed to open radar device file (using SIMULATION mode)");
#endif // PRINTERRORS
      start();
      return;
    }
  }

  syncstate    = RDSS_NOTSYNC;
  finish_flag  = false;
  workerThread = QtConcurrent::run(this, &RadarDataSource::radar_worker);
}

void RadarDataSource::finish() {
  finish_flag = true;
}

#define BLOCK_TO_SEND 32

void RadarDataSource::worker() {
  int file = 0;
  int offset = 0;

  while(!finish_flag) {
    qSleep(24);

    emit updateData(offset, BLOCK_TO_SEND, &file_divs[file][offset], &file_amps[file][offset*PELENG_SIZE]);

    offset = (offset + BLOCK_TO_SEND) % BEARINGS_PER_CYCLE;
    if (offset == 0) file = 1 - file;
  }
}

void RadarDataSource::radar_worker() {
  int       i, res;
  uint32_t bearingbuflen = BEARING_PACK_SIZE;
  //uint32_t curbuf;
  uint32_t stepbear;
  uint32_t scanidx;
  uint32_t bearidx;
  //uint32_t rdbufidx;
  //uint32_t reqidx;
  uint32_t waitnum = 0; // Number of bearings to wait for completition
  //uint32_t valcnt;
  //uint32_t firstbear;
  //uint32_t firstbearidx;
  FreeInfo  usrInfo;
  //struct timespec t1, t2;
#ifdef WRITE_2SCANS
  uint32_t prevbear = -1;
  int             writestage;
  int             dbgfd = -1;

  writestage = 0;
#endif // WRITE_2SCANS
  stepbear   = 0;
  scanidx    = 0;
  bearidx    = 0;
  //rdbufidx   = 0;
  //reqidx     = 0;
  //curbuf     = 0;
  //valcnt     = 0;
  //firstbear  = -1;
#ifdef WRITE_2SCANS

#ifdef Q_OS_WIN
  _sopen_s(&fd, "simout.bin", O_CREAT | O_RDWR, 0, 0);
#else
  dbgfd = ::open("simout.bin", O_CREAT | O_RDWR);
#endif

#endif // WRITE_2SCANS
  try {
    while(!finish_flag) {
      //clock_gettime(CLOCK_REALTIME, &t1);
      //clock_gettime(CLOCK_REALTIME, &t2);
      //double rdtime = (double)((long long)(t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec) / 1000000);
      //printf("%s: Read time %f ms (mean %f ms per bearing)\n", __func__, rdtime, rdtime / 256);
      for(i = 0; i < RDS_RDPOOL_SIZE; i++) {
          if(finish_flag)
              break;

          if(rdpool[rdpnext] != NULL)
              break;

          BearingBuffer * bbuf = getNextFreeBuffer();

          if(bbuf == NULL) {
#ifdef PRINTERRORS
            fprintf(stderr, "%s: buffer pool is exhausted\n", __func__);
#endif // PRINTERRORS
            break;
          }

#ifdef Q_OS_WIN
          res = _read(fd, bbuf->ptr, bearingbuflen);
#else
          res = ::read(fd, bbuf->ptr, bearingbuflen);
#endif

          if(res == -1) {
            throw -1;
          }

          bbuf->used  = true;
          bbuf->valid = false;
          rdpool[rdpnext] = bbuf;
          waitnum++;

          if(++rdpnext >= RDS_RDPOOL_SIZE)
              rdpnext = 0;
        }

        if(finish_flag && !waitnum)
          break;

        processBearings();

        usrInfo.expected = waitnum;
#ifndef Q_OS_WIN
        res = ioctl(fd, IGET_TRN_RXUSRINFO, &usrInfo);
        if (res < 0)
          throw -1;
#endif
        //if(usrInfo.expected)
        //	fprintf(stderr, "%u completed\n", usrInfo.expected);
        for(i = 0; i < (int)usrInfo.expected; i++) {
          if(rdpool[rdpqueued] == NULL) {
#ifdef PRINTERRORS
            fprintf(stderr, "%s: rdpqueued points at invalid buffer %d\n", __func__, rdpqueued);
#endif // PRINTERRORS
            for(int j = 0; j < RDS_RDPOOL_SIZE; j++) {
              if(++rdpqueued >= RDS_RDPOOL_SIZE)
                rdpqueued = 0;
              if(rdpool[rdpqueued] != NULL)
                break;
            }

            if(rdpool[rdpqueued] == NULL) {
#ifdef PRINTERRORS
              fprintf(stderr, "%s: Read pool is EMPTY (%d)!\n", __func__, rdpqueued);
#endif // PRINTERRORS
              break;
            }
          }

          BearingBuffer * bbuf = rdpool[rdpqueued];
          bool useData = true;
          if(setRawBearingData(rdpool[rdpqueued]))
            useData = false;

          rdpool[rdpqueued] = NULL; // Remove buffer from read pool

          if(++rdpqueued >= RDS_RDPOOL_SIZE)
            rdpqueued = 0;

          if(useData) {
            stepbear = bbuf->ptr[0];
            //valcnt++;
#ifdef WRITE_2SCANS
          if((writestage == 0) && (stepbear == 0))
            writestage = 1;

          if((writestage == 1) || (writestage == 2)) {
            if((stepbear - prevbear) == 1) {
#ifdef Q_OS_WIN
              _write(dbgfd, bbuf->ptr, (800 + 3) * 4);
#else
              ::write(dbgfd, bbuf->ptr, (800 + 3) * 4);
#endif
              prevbear = stepbear;
            }
          }
#endif // WRITE_2SCANS

          if(bbuf->ptr[0] == 4095) {
#ifdef WRITE_2SCANS
            if(prevbear == 4095) {
              if(writestage == 1) {
                prevbear = -1;
                writestage++;
              } else if(writestage == 2) {
#ifdef Q_OS_WIN
                _close(dbgfd);
#else
                ::close(dbgfd);
#endif
                dbgfd = -1;
                writestage++;
              }
            }
#endif // WRITE_2SCANS
          }
        }

        bearidx++;
        if((bearidx % 4096) == 0)
          printf("Radar scan: %u (bearing %u)\n", ++scanidx, stepbear);
      }
    }
  }
#ifdef PRINTERRORS
  catch(int err)
#else
  catch(int)
#endif
  {
#ifdef PRINTERRORS
    if(err == -1)
      perror("Error during reading radar data");
      fprintf(stderr, "%s: thread stopped because of error %d\n", __func__, err);
#endif // PRINTERRORS
      finish_flag = true;
  }

#ifdef PRINTERRORS
  printf("%s: radar thread is about to finish\n", __func__);
#endif // PRINTERRORS
  return;
}

bool RadarDataSource::loadData() {
  char file1[25] = "res/pelengs/r1nm3h0_4096";
  char file2[25] = "res/pelengs/r1nm6h0_4096";
  //char file3[23] = "res/pelengs/simout.bin";

  if (!loadObserves1(file1, file_divs[0], file_amps[0]))
    return false;
  if (!loadObserves1(file2, file_divs[1], file_amps[1]))
    return false;

  /*
  if (!loadObserves2(file3, file_divs[0], file_amps[0]))
    return false;
  if (!initWithDummy(file_divs[1], file_amps[1]))
    return false;
  */

  return true;
}

bool RadarDataSource::loadObserves2(char* filename, float* divs, float* amps) {
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
  //uint32_t bearing - номер пеленга (0...4095)
  //uint32_t datalen - 800 (800 32-битных амплитуд)
  //uint32_t bearlen - 803 (общая длина пакета в 32-битных словах)
  //uint32_t amps[800]

  if (file.is_open()) {
    std::streampos size = file.tellg();
    int32_t* memblock = new int32_t[size/4];

    //qDebug() << "size: " << size;

    file.seekg(0, std::ios::beg);
    file.read((char*) memblock, size);
    file.close();

    //int max_amp = -1000000;
    //int min_amp = 1000000;

    //6424*4096 байт
    for (int i = 0; i < BEARINGS_PER_CYCLE; i++) {
      //int32_t bearing = memblock[803*BEARINGS_PER_CYCLE+0];
      //int32_t datalen = memblock[803*BEARINGS_PER_CYCLE+1];
      //int32_t bearlen = memblock[803*BEARINGS_PER_CYCLE+2];

      divs[i] = 4;
      for (int j = 0; j < PELENG_SIZE; j++) {
        if (j < 800) {
          amps[i*PELENG_SIZE + j] = memblock[803*BEARINGS_PER_CYCLE+3+j];

          //if (amps[i*PELENG_SIZE + j] > max_amp)
          //  max_amp = amps[i*PELENG_SIZE + j];

          //if (amps[i*PELENG_SIZE + j] < min_amp)
          //  min_amp = amps[i*PELENG_SIZE + j];
        } else {
          amps[i*PELENG_SIZE + j] = 0;
        }
      }
    }

    //qDebug() << "Max amp: " << max_amp;
    //qDebug() << "Min amp: " << min_amp;

    delete[] memblock;
    //exit(0);
    return true;
  }
  else {
    std::cerr << "Unable to open file";
    return false;
  }

  return true;
}

bool RadarDataSource::initWithDummy(float* divs, float* amps) {
  for (uint i = 0; i < BEARINGS_PER_CYCLE; i++) {
    for (uint j = 0; j < PELENG_SIZE; j++) {
      amps[i*PELENG_SIZE+j] = (255.f * ((j + i/2) % PELENG_SIZE)) / PELENG_SIZE;
      divs[i] = 1;
    }
  }

  return true;
}

bool RadarDataSource::loadObserves1(char* filename, float* divs, float* amps) {
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);

  // 16 and 3204 in bytes, we will use INT16
  const uint headerSize = 16 / 2;
  const uint dataSize = 3204 / 2;

  if (file.is_open()) {
    std::streampos size = file.tellg();
    int16_t* memblock = new int16_t[size/2];

    file.seekg(0, std::ios::beg);
    file.read((char*) memblock, size);
    file.close();

    for (int i = 0; i < BEARINGS_PER_CYCLE; i++) {
      divs[i] = memblock[headerSize + i*dataSize + 1];
      for (int j = 0; j < PELENG_SIZE; j++)
        amps[i*PELENG_SIZE + j] = memblock[headerSize + i*dataSize + 2 + j];
    }

    delete[] memblock;
    return true;
  }

  std::cerr << "Unable to open file";
  return false;
}

int RadarDataSource::setRawBearingData(BearingBuffer * bearing) {
  uint32_t brg;

  if(!bearing)
    return -1;

  if(!bearing->used)
    return -2;

  brg = bearing->ptr[0];
  bearing->used  = false;

  if (brg >= BEARINGS_PER_CYCLE) {
    bearing->valid = false;
#ifdef PRINTERRORS
    fprintf(stderr, "%s: Invalid bearing (%u)\n", __func__, brg);
#endif // PRINTERRORS
    return -3; // Invalid bearing
  }

  if(syncstate != RDSS_SYNC) {
    switch(syncstate) {
      case RDSS_NOTSYNC:
        if (brg == 0)
          syncstate = RDSS_WAIT4STAB;
        break;
      case RDSS_WAIT4STAB:
        if(brg == 0) {
          if(last_bearing == (BEARINGS_PER_CYCLE - 1)) {
            // Full scan completed. Radar controller is synchronized
            processed_bearing = BEARINGS_PER_CYCLE - 1;
            syncstate = RDSS_SYNC;
            break;
          }

          syncstate = RDSS_NOTSYNC; // Gap in bearings. Restart synchronization process
#ifdef PRINTERRORS
          fprintf(stderr, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
#endif
        } else {
          if (brg != last_bearing + 1) {
            syncstate = RDSS_NOTSYNC; // Gap in bearings. Restart synchronization process
#ifdef PRINTERRORS
            fprintf(stderr, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
#endif
          }
        }
        break;
      default:
        break;
      }

    if(syncstate != RDSS_SYNC) {
      last_bearing = brg;
      bearing->valid = false;
      return 0;
    }
  }

  bearing->valid = true;
  if(scans[activescan][brg] && (scans[activescan][brg]->used || scans[activescan][brg]->valid)) {
    // The bearing has not been processed yet but we have new data for this bearing
    // so we should return the old buffer back to pool
    scans[activescan][brg]->used  = false;
    scans[activescan][brg]->valid = false;
#ifdef PRINTERRORS
    fprintf(stderr, "%s: data at BRG %u has been overwritten\n", __func__, brg);
#endif // PRINTERRORS
  }

  scans[activescan][brg] = bearing;
#ifdef PRINTERRORS
  if (brg == 0) {
    if (last_bearing != (BEARINGS_PER_CYCLE - 1))
      fprintf(stderr, "%s: Gap in bearings: cur %u, prev %u\n", __func__, brg, last_bearing);
  } else if(brg != (last_bearing + 1))
    fprintf(stderr, "%s: Gap in bearings: cur %u, prev %u\n", __func__, brg, last_bearing);
#endif // PRINTERRORS

  last_bearing = brg;
  return 0;
}

BearingBuffer * RadarDataSource::getNextFreeBuffer(void) {
  if (nextbuf >= (BEARINGS_PER_CYCLE * (RDS_MAX_SCANS + 1)))
    nextbuf = 0;

  for (int i = 0; i < BEARINGS_PER_CYCLE * (RDS_MAX_SCANS + 1); i++) {
    if (bufpool[nextbuf].used || (bufpool[nextbuf].valid)) {
      if (++nextbuf >= (BEARINGS_PER_CYCLE * RDS_MAX_SCANS))
        nextbuf = 0;
      continue;
    }

    return &bufpool[nextbuf++];
  }

  return NULL;
}

int RadarDataSource::processBearings(void) {
  uint32_t   div;
  uint32_t   amps;
  uint32_t * ptr;
  uint32_t   firstbrg;
  int         num;
  int         res = 0;

  if (syncstate != RDSS_SYNC)
    return res;

  firstbrg = processed_bearing + 1;
  if (firstbrg >= BEARINGS_PER_CYCLE)
    firstbrg = 0;

  if (last_bearing < processed_bearing) {
    num = BEARINGS_PER_CYCLE - processed_bearing - 1;
    for(int i = 0; i < num; i++) {
      processed_bearing++;
      amps = scans[activescan][processed_bearing]->ptr[1];
      div  = scans[activescan][processed_bearing]->ptr[2];

      if(amps > PELENG_SIZE)
        amps = PELENG_SIZE;

      file_divs[0][processed_bearing] = (div < 32) ? div : 1; // After debugging "div"
      ptr = &(scans[activescan][processed_bearing]->ptr[3]);

      for(uint32_t j = 0; j < amps; j++)
        file_amps[0][processed_bearing * PELENG_SIZE + j] = *ptr++;

      scans[activescan][processed_bearing]->valid = false;
    }

    if(num) {
      emit updateData(firstbrg, num, &file_divs[0][firstbrg], &file_amps[0][firstbrg * PELENG_SIZE]);
    }

    res      += num;
    num       = last_bearing + 1;
    firstbrg  = 0;
  } else
    num = last_bearing - processed_bearing;

  res      += num;

  for(int i = 0; i < num; i++) {
    if(++processed_bearing >= BEARINGS_PER_CYCLE)
      processed_bearing = 0;

    amps = scans[activescan][processed_bearing]->ptr[1];
    div  = scans[activescan][processed_bearing]->ptr[2];

    if (amps > PELENG_SIZE)
      amps = PELENG_SIZE;

    file_divs[0][processed_bearing] = (div < 32) ? div : 1; // After debugging "div"
    ptr = &(scans[activescan][processed_bearing]->ptr[3]);

    for(uint32_t j = 0; j < amps; j++)
      file_amps[0][processed_bearing * PELENG_SIZE + j] = *ptr++;

    scans[activescan][processed_bearing]->valid = false;
  }

  if(num) {
    emit updateData(firstbrg, num, &file_divs[0][firstbrg], &file_amps[0][firstbrg * PELENG_SIZE]);
  }

  return res;
}

