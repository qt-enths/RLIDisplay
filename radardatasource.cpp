#include "radardatasource.h"
#include "xpmon_be.h"
#include "apctrl.h"

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

//#define WRITE_2SCANS
//#define PRINTERRORS
//#define WRITE_WHENSYNC
//#define GET_MAX_AMPL

#ifdef WRITE_WHENSYNC
FILE * scanlog = NULL;
#endif // WRITE_WHENSYNC
#ifdef GET_MAX_AMPL
uint32_t gmax = 0;
#endif // GET_MAX_AMPL


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
    , bufpoolsize(0) //nextbuf(0)
    //, rdpool(NULL)
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
    int ret = ioctl(fd, APCTRL_IOCTL_STOP);
    if (-1 == ret) {
        fprintf(stderr, "Failed to stop APCTRL: %s\n", strerror(errno));
        //return 17;
    }
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

  if(bufpool) {
    delete [] bufpool;
    bufpool     = NULL;
    bufpoolsize = 0;
  }

#ifdef WRITE_WHENSYNC
  if(scanlog)
  {
      fclose(scanlog);
      scanlog = NULL;
  }
#endif // WRITE_WHENSYNC
#ifdef GET_MAX_AMPL
  fprintf(stderr, "gmax = %u\n", gmax);
#endif // GET_MAX_AMPL
}

void RadarDataSource::start() {
  if (workerThread.isRunning())
    return;

  finish_flag = false;
  workerThread = QtConcurrent::run(this, &RadarDataSource::worker);
}

void RadarDataSource::start(const char * radarfn) {
    struct apctrl_caps caps;
    int ret;

    if(!radarfn)
      return;
    if (workerThread.isRunning())
      return;

    rdpnext   = 0;
    rdpqueued = 0;

    for(int scanidx = 0; scanidx < RDS_MAX_SCANS; scanidx++)
    {
        if(!scans[scanidx])
        {
            //scans[scanidx] = new BearingBuffer * [BEARINGS_PER_CYCLE];
            BearingBuffer ** pptr = new BearingBuffer * [BEARINGS_PER_CYCLE];
  #ifdef PRINTERRORS
            fprintf(stderr, "%s: pointer array %d: %p\n", __func__, scanidx, pptr);
  #endif
            scans[scanidx] = pptr;
            if(!scans[scanidx])
            {
  #ifdef PRINTERRORS
                fprintf(stderr, "%s: failed to create scan container %d\n", __func__, scanidx);
  #endif // PRINTERRORS
                return;
            }
            memset(scans[scanidx], 0, sizeof(BearingBuffer *) * BEARINGS_PER_CYCLE);
            activescan        = 0; // TODO: Check whether these lines may be moved out of the scope of the cycle
            processed_bearing = 0;
            last_bearing      = 0;
        }
    }

    if(fd == -1)
    {
  #ifdef Q_OS_WIN
      _sopen_s(&fd, radarfn, O_RDONLY, 0, 0);
  #else
      fd = ::open(radarfn, O_RDONLY);
  #endif

        if(fd == -1)
        {
  #ifdef PRINTERRORS
            perror("Failed to open radar device file (using SIMULATION mode)");
  #endif // PRINTERRORS
            start();
            return;
        }

        ret = ioctl(fd, APCTRL_IOCTL_GET_CAPS, &caps);
        if (-1 == ret) {
            fprintf(stderr, "Failed to get capabilities: %s\n", strerror(errno));
            return;
        }

        printf("APCTRL device capabilities:\n"
                " - bufs_nr = %lu (expected %lu)\n",
                caps.bufs_nr, (unsigned long)(BEARINGS_PER_CYCLE * RDS_MAX_SCANS));
    }

    if(!bufpool)
    {
        bufpoolsize = caps.bufs_nr;
        bufpool     = new BearingBuffer[bufpoolsize];
        if(!bufpool)
        {
  #ifdef PRINTERRORS
            fprintf(stderr, "%s: failed to create bearing buffer pool\n", __func__);
  #endif // PRINTERRORS
            bufpoolsize = 0;
            return;
        }
        //nextbuf = 0;
    }

    for (unsigned int i = 0; i < bufpoolsize; i++) {
        struct apctrl_buf_desc d;
        uint8_t * bufptr;

        d.nr = i;

        ret = ioctl(fd, APCTL_IOCTL_GET_BUF_DESC, &d);
        if (-1 == ret) {
            fprintf(stderr, "Failed to get buffer descriptor: %s\n",
                    strerror(errno));
            return;
        }

        bufptr = (uint8_t *)mmap(0, d.size, PROT_READ, MAP_SHARED, fd, d.offset);
        if (!bufptr || (-1 == (long)bufptr)) {
            fprintf(stderr, "Failed to mmap buffer %u: %s\n", i,
                    strerror(errno));
            return;
        }

        if(!bufpool[i].bind((uint32_t *)bufptr, d.nr))
        {
            fprintf(stderr, "Failed to bind kernel buffer to BearingBuffer object\n");
            return;
        }
    }

    ret = ioctl(fd, APCTL_IOCTL_GET_CUR, &rdpqueued);
    if(-1 == ret)
    {
        fprintf(stderr, "Failed to to get current buffer index: %s\n", strerror(errno));
        return;
    }
    fprintf(stderr, "Ready to start DMA with buffer %lu\n", rdpqueued);
    rdpnext = rdpqueued;

    //ret = ioctl(fd, APCTRL_IOCTL_START, 256);
    //if (-1 == ret) {
    //    fprintf(stderr, "Failed to start APCTRL: %s\n", strerror(errno));
    //    return;
    //}

#ifdef WRITE_WHENSYNC
    if(!scanlog)
    {
        scanlog = fopen("scanlog.txt", "w");
    }
#endif // WRITE_WHENSYNC

    syncstate    = RDSS_NOTSYNC;
    finish_flag  = false;
    workerThread = QtConcurrent::run(this, &RadarDataSource::radar_worker);
}

void RadarDataSource::finish() {
  finish_flag = true;
}

//#define BLOCK_TO_SEND 32
#define BLOCK_TO_SEND 16

void RadarDataSource::worker() {
  int file = 0;
  int offset = 0;

  while(!finish_flag) {
    qSleep(19);

    emit updateData(offset, BLOCK_TO_SEND, &file_divs[file][offset], &file_amps[file][offset*PELENG_SIZE]);

    offset = (offset + BLOCK_TO_SEND) % BEARINGS_PER_CYCLE;
    if (offset == 0) file = 1 - file;
  }
}

uint32_t bearidx;

void RadarDataSource::radar_worker() {
    int      res;
    uint32_t stepbear;
    uint32_t scanidx;
    //uint32_t bearidx;
    //struct timespec t1, t2;
#ifdef WRITE_2SCANS
    uint32_t prevbear = -1;
    int      writestage;
    int      dbgfd = -1;

    writestage = 0;
#endif // WRITE_2SCANS
    stepbear   = 0;
    scanidx    = 0;
    bearidx    = 0;
#ifdef WRITE_2SCANS

#ifdef PRINTERRORS
	printf("%s: thread is starting.\n", __func__);
#endif // PRINTERRORS

#ifdef Q_OS_WIN
    _sopen_s(&fd, "simout.bin", O_CREAT | O_RDWR, 0, 0);
#else
    dbgfd = ::open("simout.bin", O_CREAT | O_RDWR);
#endif

#endif // WRITE_2SCANS
    try
    {

#define SLEEPTIMESEC 20
#ifdef PRINTERRORS
			printf("%s: Sleeping %d s while engine initializes.\n", __func__, SLEEPTIMESEC);
#endif // PRINTERRORS
        sleep(SLEEPTIMESEC);
        res = ioctl(fd, APCTRL_IOCTL_START, 256);
        if (-1 == res) {
            fprintf(stderr, "Failed to start APCTRL: %s\n", strerror(errno));
            return;
        }

#ifdef PRINTERRORS
		printf("%s: IOCTL_START succeeded.\n", __func__);
#endif // PRINTERRORS

        while(!finish_flag)
        {
            //clock_gettime(CLOCK_REALTIME, &t1);
            //clock_gettime(CLOCK_REALTIME, &t2);
            //double rdtime = (double)((long long)(t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec) / 1000000);
            //printf("%s: Read time %f ms (mean %f ms per bearing)\n", __func__, rdtime, rdtime / 256);

#ifndef Q_OS_WIN
            res = ioctl(fd, APCTL_IOCTL_WAIT, &rdpnext);
            if (-1 == res) {
                fprintf(stderr, "%s: Failed to get current buffer: %s\n",
                        __func__, strerror(errno));
                throw -18;
            }
#endif

#ifdef PRINTERRORS
			printf("%s: IOCTL_WAIT returned (%lu).\n", __func__, rdpnext);
#endif // PRINTERRORS

            if(rdpnext > bufpoolsize)
            {
                fprintf(stderr, "Updated buffer id %lu is out of bound\n", rdpnext);
                throw -19;
            }

            for(; rdpqueued != rdpnext; rdpqueued = (rdpqueued + 1) % bufpoolsize)
            {
                BearingBuffer * bbuf = &bufpool[rdpqueued];
                bool useData = true;
                bbuf->used   = true;

//				if(bearidx > 4095)
				{

#ifdef PRINTERRORS
				printf("%s: setting raw data (%lu).\n", __func__, rdpqueued);
#endif // PRINTERRORS

                if(setRawBearingData(&bufpool[rdpqueued]))
                    useData = false;

#ifdef PRINTERRORS
				printf("%s: raw data set (useData = %d).\n", __func__, (int)useData);
#endif // PRINTERRORS

				}
				//else
				//{
				//	bbuf->used = false;
				//	useData = false;
				//}

                if(useData)
                {
                    stepbear = bbuf->ptr[0];
#ifdef WRITE_2SCANS
                    if((writestage == 0) && (stepbear == 0))
                        writestage = 1;
                    if((writestage == 1) || (writestage == 2))
                    {
                        if((stepbear - prevbear) == 1)
                        {
#ifdef Q_OS_WIN
                           _write(dbgfd, bbuf->ptr, (800 + 3) * 4);
#else
                           ::write(dbgfd, bbuf->ptr, (800 + 3) * 4);
#endif
                           prevbear = stepbear;
                        }
                    }
#endif // WRITE_2SCANS

                    if(bbuf->ptr[0] == 4095)
                    {
#ifdef WRITE_2SCANS
                        if(prevbear == 4095)
                        {
                            if(writestage == 1)
                            {
                                prevbear = -1;
                                writestage++;
                            }
                            else if(writestage == 2)
                            {
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

#ifdef PRINTERRORS
			printf("%s: calling preocessBearings.\n", __func__);
#endif // PRINTERRORS

            processBearings();

#ifdef PRINTERRORS
			printf("%s: .ssBearings returned\n", __func__);
#endif // PRINTERRORS
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
    printf("%s: radar thread is about to finish (rdpqueued = %lu)\n", __func__, rdpqueued);
#endif // PRINTERRORS
    res = ioctl(fd, APCTRL_IOCTL_STOP);
    if (-1 == res) {
        fprintf(stderr, "Failed to stop APCTRL: %s\n", strerror(errno));
        //return 17;
    }
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
  static uint32_t prev_bear = 0;

#ifdef PRINTERRORS
    printf("%s: Entered\n", __func__);
#endif // PRINTERRORS

  if(!bearing)
    return -1;

  if(!bearing->used)
    return -2;

#ifdef PRINTERRORS
    printf("%s: Getting bearing value\n", __func__);
#endif // PRINTERRORS

  brg = bearing->ptr[0];
  bearing->used  = false;

#ifdef PRINTERRORS
    printf("%s: Bearing: %u\n", __func__, brg);
#endif // PRINTERRORS

  // TODO: Delete after debugging (start)
  if(((brg - prev_bear) != 1) && (brg != 0) && (prev_bear != 4095))
  {
      fprintf(stdout, "Non-sequential! cur %u, prev %u\n", brg, prev_bear);
      //fprintf(stderr, "Non-sequential! cur %u, prev %u\n", brg, prev_bear);
  }
  prev_bear = brg;
  // TODO: Delete after debugging (end)


  if (brg >= BEARINGS_PER_CYCLE) {
    bearing->valid = false;
#ifdef PRINTERRORS
    fprintf(stdout, "%s: Invalid bearing (%u)\n", __func__, brg);
    //fprintf(stderr, "%s: Invalid bearing (%u)\n", __func__, brg);
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
#ifdef PRINTERRORS
            printf("In sync state (%u %u)!\n", brg, last_bearing);
#endif // PRINTERRORS
            syncstate = RDSS_SYNC;
            break;
          }

          syncstate = RDSS_NOTSYNC; // Gap in bearings. Restart synchronization process
#ifdef PRINTERRORS
          //fprintf(stderr, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
          fprintf(stdout, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
#ifdef WRITE_WHENSYNC
          if(!brg && !last_bearing)
          {
              fprintf(scanlog, "BIDX: %u. %u, %u, %u", bearidx, bearing->ptr[0], bearing->ptr[1], bearing->ptr[2]);
              for(int i = 3; i < 803; i++)
              {
                  if(((i - 3) % 8) == 0)
                      fprintf(scanlog, "\n\t");
                  fprintf(scanlog, "0x%08X ", bearing->ptr[i]);
              }
              fprintf(scanlog, "\n");
          }
#endif // WRITE_WHENSYNC
#endif
        } else {
          if (brg != last_bearing + 1) {
            syncstate = RDSS_NOTSYNC; // Gap in bearings. Restart synchronization process
#ifdef PRINTERRORS
            //fprintf(stderr, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
            fprintf(stdout, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
#ifdef WRITE_WHENSYNC
            if(!brg && !last_bearing)
            {
                fprintf(scanlog, "%s: Restart synchronization: cur %u, prev %u\n", __func__, brg, last_bearing);
                fprintf(scanlog, "BIDX: %u. %u, %u, %u", bearidx, bearing->ptr[0], bearing->ptr[1], bearing->ptr[2]);
                for(int i = 3; i < 803; i++)
                {
                    if(((i - 3) % 8) == 0)
                        fprintf(scanlog, "\n\t");
                    fprintf(scanlog, "0x%08X ", bearing->ptr[i]);
                }
                fprintf(scanlog, "\n");
            }
#endif // WRITE_WHENSYNC
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

#ifdef WRITE_WHENSYNC
  fprintf(scanlog, "%u, %u, %u", bearing->ptr[0], bearing->ptr[1], bearing->ptr[2]);
  for(int i = 3; i < 803; i++)
  {
      if(((i - 3) % 8) == 0)
          fprintf(scanlog, "\n\t");
      fprintf(scanlog, "0x%08X ", bearing->ptr[i]);
  }
  fprintf(scanlog, "\n");
#endif // WRITE_WHENSYNC

  bearing->valid = true;
  if(scans[activescan][brg] && (scans[activescan][brg]->used || scans[activescan][brg]->valid)) {
    // The bearing has not been processed yet but we have new data for this bearing
    // so we should return the old buffer back to pool
    scans[activescan][brg]->used  = false;
    scans[activescan][brg]->valid = false;
#ifdef PRINTERRORS
    //fprintf(stderr, "%s: data at BRG %u has been overwritten\n", __func__, brg);
    fprintf(stdout, "%s: data at BRG %u has been overwritten (%c)\n", __func__, brg,
            (scans[activescan][brg]->used ? 'u' : 'v'));
#endif // PRINTERRORS
  }

  scans[activescan][brg] = bearing;
#ifdef PRINTERRORS
  if (brg == 0) {
    if (last_bearing != (BEARINGS_PER_CYCLE - 1))
    {
        fprintf(stdout, "%s: Gap in bearings: cur %u, prev %u\n", __func__, brg, last_bearing);
        //fprintf(stderr, "%s: Gap in bearings: cur %u, prev %u\n", __func__, brg, last_bearing);
        syncstate = RDSS_NOTSYNC; // Gap in bearings. Restart synchronization process
    }
  } else if(brg != (last_bearing + 1))
  {
      fprintf(stdout, "%s: Gap in bearings: cur %u, prev %u\n", __func__, brg, last_bearing);
      //fprintf(stderr, "%s: Gap in bearings: cur %u, prev %u\n", __func__, brg, last_bearing);
      syncstate = RDSS_NOTSYNC; // Gap in bearings. Restart synchronization process
  }
#endif // PRINTERRORS

  last_bearing = brg;
  return 0;
}

BearingBuffer * RadarDataSource::getNextFreeBuffer(void) {
    //TODO: Remove this function
    // There is no read pool anymore. Buffers are created and managed by driver now.
    // We need just to keep track of buffer indexes (last processed must be kept by us
    // and latest newly populated index is returned by ioctl)
    /*
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
  */

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


#ifdef GET_MAX_AMPL
    uint32_t max = scans[activescan][processed_bearing]->ptr[3];
    for(int i = 4; i < 803; i++)
    {
        if(scans[activescan][processed_bearing]->ptr[i] > max)
            max = scans[activescan][processed_bearing]->ptr[i];
    }
    if(max > 255)
    {
        div = max / 255 + 1;
    }

    if(max > gmax)
        gmax = max;
#endif // GET_MAX_AMPL
    div = 4; // 10 bit data downgraded to 8 bit //1600 / 255 + 1;


    if (amps > PELENG_SIZE)
      amps = PELENG_SIZE;

    file_divs[0][processed_bearing] = div;
    //(div < 32) ? div : 1; // After debugging "div"
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

