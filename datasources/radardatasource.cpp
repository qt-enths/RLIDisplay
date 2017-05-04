#include "radardatasource.h"
#include "../mainwindow.h"

#include <QThread>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdint.h>
#include <QDebug>

#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------
#include "xpmon_be.h"
#include "apctrl.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#endif //  Q_OS_WIN
// ------------------------------------------------------

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// APCTRL registers
#define APCTRL_CTRL_BASEADDR     0xe000
#define APCTRL_PKIDPKOD_BASEADDR 0xe004
#define APCTRL_HIP_BASEADDR      0xe024
#define APCTRL_GEN_BASEADDR      0xe02c
#define APCTRL_ADC_BASEADDR      0xe030
#define APCTRL_E038_BASEADDR     0xe038
#define APCTRL_GYROREG_BASEADDR  0xe04C

#define APCTRL_CTRL_INTIZIEN     0x0001
#define APCTRL_CTRL_NOISEON      0x0002
#define APCTRL_CTRL_MRPEN        0x0004
#define APCTRL_CTRL_SIMULEN      0x0008
#define APCTRL_CTRL_STATEN       0x0010
#define APCTRL_CTRL_UARTEN       0x0020

#define APCTRL_SPIWR_TIMEOUT     (unsigned long long)1000 * 1000000

#define APCTRL_HIP_MASK          0x0003
#define APCTRL_HIP_NONE          0
#define APCTRL_HIP_WEAK          0x0002
#define APCTRL_HIP_STRONG        0x0003
#define APCTRL_HIP_SHIFT         0
#define APCTRL_HIP_SARP_SHIFT    0

static int spi_timeout = 0;

void qSleep(int ms) {
  if (ms <= 0) return;
#ifdef Q_OS_WIN
  Sleep(uint(ms));
#else
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
  nanosleep(&ts, NULL);
#endif
}

#ifndef Q_OS_WIN
static void tmr_hndlr(int sig, siginfo_t * si, void * uc)
{
    Q_UNUSED(sig);
    Q_UNUSED(si);
    Q_UNUSED(uc);
    spi_timeout = 1;
}
#endif

const u_int32_t RadarDataSource::max_gain_level = 255;

RadarDataSource::RadarDataSource() {
  finish_flag = true;
  _radar_scale = new RadarScale();

  loadData();

#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------
  file_curr         = 0;
  syncstate         = RDSS_NOTSYNC;
  bufpool           = NULL;
  bufpoolsize       = 0; //nextbuf(0)
  // rdpool = NULL;
  rdpnext           = 0;
  rdpqueued         = 0;
  activescan        = 0;
  processed_bearing = 0;
  last_bearing      = 0;
  fd                = -1;

  for(int scanidx = 0; scanidx < RDS_MAX_SCANS; scanidx++)
    scans[scanidx] = NULL;

  adcspi_tmid       = NULL;
#endif //Q_OS_WIN
// ------------------------------------------------------
  ampoffset         = 0;
  dump              = NULL;

  gain_level        = 0; // Initial amplification level
  hip_main          = HIP_NONE;
  hip_sarp          = HIP_NONE;

#ifndef Q_OS_WIN
  simulation        = false;
#else  // Q_OS_WIN
  simulation        = true;
#endif // Q_OS_WIN
  gyroReg           = 0;
}

RadarDataSource::~RadarDataSource() {
  finish_flag = true;
  while(workerThread.isRunning());

#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------
  if(fd != -1) {
    int ret = ioctl(fd, APCTRL_IOCTL_STOP);
    if (-1 == ret) {
        fprintf(stderr, "Failed to stop APCTRL: %s\n", strerror(errno));
        //return 17;
    }

    ::close(fd);
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

  if(adcspi_tmid != NULL)
      timer_delete(adcspi_tmid);

#endif //Q_OS_WIN
// ------------------------------------------------------

  if(dump)
  {
      delete [] dump;
      dump = NULL;
  }
}

void RadarDataSource::start() {
  if (workerThread.isRunning())
    return;

  finish_flag = false;
  workerThread = QtConcurrent::run(this, &RadarDataSource::worker);
}


void RadarDataSource::start_dump() {
  if (workerThread.isRunning())
    return;

#ifndef Q_OS_WIN
  int dmpd = open("dump.bin", O_RDONLY);
  try
  {
      if(dmpd == -1)
          throw 0;
      struct stat sbuf;
      int res = fstat(dmpd, &sbuf);
      if(res == -1)
          throw 0;
      if(dump)
      {
          fprintf(stderr, "Strange but memory block with dump exists. It will be deleted and new memory allocated.\n");
          dsize = 0;
          delete [] dump;
      }
      dsize = sbuf.st_size / (BEARING_PACK_WORDS * sizeof(u_int32_t));
      dsize *= BEARING_PACK_WORDS;
      dump = new u_int32_t[dsize];
      if(dump == NULL)
          throw -1;
      res = read(dmpd, dump, dsize * sizeof(u_int32_t));
      if(res == -1)
          throw 0;
  }
  catch(int e)
  {
      if(e)
          fprintf(stderr, "Error %d reading dump file\n", e);
      else
          fprintf(stderr, "Dump file read failed: %s\n", strerror(errno));
      if(dump)
      {
          delete [] dump;
          dump = NULL;
      }
  }

  if(dmpd != -1)
  {
      close(dmpd);
      dmpd = -1;
  }

#endif // Q_OS_WIN

  if(dump)
  {
    finish_flag = false;
    workerThread = QtConcurrent::run(this, &RadarDataSource::dump_worker);
  }
  else
  {
      finish_flag = true;
      fprintf(stderr, "dump_worker did not start\n");
  }
}


void RadarDataSource::start(const char * radarfn) {
  Q_UNUSED(radarfn);
#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------
    struct apctrl_caps caps;
    struct sigaction   sa;
    struct sigevent    tev;
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
      fd = ::open(radarfn, O_RDONLY);

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

    // Create a timer for ADC SPI timeout
    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = tmr_hndlr;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGRTMIN, &sa, NULL) != -1)
    {
        // Create timer
        tev.sigev_notify = SIGEV_SIGNAL;
        tev.sigev_signo  = SIGRTMIN;
        tev.sigev_value.sival_ptr = &adcspi_tmid;
        if(timer_create(CLOCK_REALTIME, &tev, &adcspi_tmid) == -1)
        {
            fprintf(stderr, "Failed to create timer for ADC SPI timeout handler: %s\n", strerror(errno));
            fprintf(stderr, "WARNING: Creation of the timer for ADC SPI timeout failed and the operation my hang.\n");
            fprintf(stderr, "\t Use Ctrl+C to terminate in such case.\n");

            adcspi_tmid = NULL;
        }
    }
    else
    {
        fprintf(stderr, "Failed to setup signal handler for ADC SPI timeout handler: %s\n", strerror(errno));
        fprintf(stderr, "WARNING: since there's no timer for ADC SPI timeout the operation my hang.\n");
        fprintf(stderr, "\t Use Ctrl+C to terminate in such case.\n");
    }

    // Initialize AP generator
    ret = initGen(true);
    if(ret != 0)
    {
        fprintf(stderr, "Failed to initialize AP generator (%d)\n", ret);
        return;
    }

	printf("Setting frequency.\n");
    apctrl_adcspi_send(APCTRL_GEN_BASEADDR, 0x1f0024, 0x1f4007, true);
	printf("Initializing ADC.\n");
    initADC();
	printf("Initializing DAC.\n");
    initDAC();

    // Some more settings for APCTRL
	printf("Some settings for APCTRL.\n");
    apctrl_regwr(APCTRL_PKIDPKOD_BASEADDR, 0x0960001E);
	hipregv = 0x0015;
    apctrl_regwr(APCTRL_HIP_BASEADDR, hipregv);
    apctrl_regwr(APCTRL_E038_BASEADDR, 0x1000040);

    printf("Setup current scale\n");
    MainWindow * mainWnd = dynamic_cast<MainWindow *>(parent());
    if(mainWnd)
    {
        const rli_scale_t * curscale = mainWnd->getRadarScale()->getCurScale();
        printf("Current scale %f nm\n", curscale->len);
        if(setupScale(curscale) != 0)
            fprintf(stderr, "%s: Failed to setup current scale\n", __func__);
    }
	else
	{
		fprintf(stderr, "%s: mainWnd is NULL!!!\n", __func__);
	}

    apctrl_regwr(APCTRL_GYROREG_BASEADDR, gyroReg);
	printf("APCTRL_GYROREG_BASEADDR written with 0x%X\n", gyroReg);

    syncstate    = RDSS_NOTSYNC;
    finish_flag  = false;
    workerThread = QtConcurrent::run(this, &RadarDataSource::radar_worker);

#endif //Q_OS_WIN
// ------------------------------------------------------
}

void RadarDataSource::finish() {
  finish_flag = true;
}

#define BLOCK_TO_SEND 32

void RadarDataSource::worker() {
  int file = 0;
  int offset = 0;

  while(!finish_flag) {
    qSleep(9);

    if(simulation)
    {
        static u_int32_t iamps[PELENG_SIZE + 3];
        static float famps[BEARINGS_PER_CYCLE * PELENG_SIZE];
        for(int i = 0; i < BLOCK_TO_SEND; i++)
        {
            iamps[0] = offset + i;
            iamps[1] = 800;
            iamps[2] = 1;
            for(int j = 3; j < PELENG_SIZE + 3; j++)
                iamps[j] = file_amps[file][(offset + i) * PELENG_SIZE + j - 3];
            amplify(iamps);
            for(int j = 3; j < PELENG_SIZE + 3; j++)
                famps[(offset + i) * PELENG_SIZE + j - 3] = (float)iamps[j];
        }

        //emit updateData(offset, BLOCK_TO_SEND, /*&file_divs[file][offset], */&file_amps[file][offset*PELENG_SIZE]);
        emit updateData(offset, BLOCK_TO_SEND, &famps[offset*PELENG_SIZE]);

        offset = (offset + BLOCK_TO_SEND) % BEARINGS_PER_CYCLE;
        if (offset == 0) file = 1 - file;
    }
  }
}

void RadarDataSource::dump_worker() {
    u_int32_t good_start, good_end;
    u_int32_t tmp_start, tmp_end;
    u_int32_t div;
    u_int32_t amps;
    u_int32_t brg;
    int curpos;
    int offset;
    int res;
    static u_int32_t tmp_brg[BLOCK_TO_SEND][PELENG_SIZE];

    if(dump == NULL)
    {
        fprintf(stderr, "Unexpected error: no dump data\n");
        finish_flag = 1;
        return;
    }

    printf("Simulation using dump file is starting\n");

    // Search for biggest segment of good scans in the dump
    good_start = good_end = 0;
    for(curpos = 0; curpos < dsize; curpos += BEARING_PACK_WORDS)
    {
        if(finish_flag)
            return;

        curpos = findZeroBrg(curpos);
        if(curpos < 0)
            break;
        tmp_start = curpos;

        res = chkScans(curpos);
        if(res < 0)
            continue; // Search for zero bearing again
        curpos = res;
        tmp_end = curpos;
        if((tmp_end - tmp_start) > (good_end - good_start))
        {
            good_start = tmp_start;
            good_end   = tmp_end;
            printf("Start: 0x%08lX, End: 0x%08lX\n", good_start * sizeof(u_int32_t), good_end * sizeof(u_int32_t));
        }
    }

    if(good_end == good_start)
    {
        // No good scans in the dump
        fprintf(stderr, "No valid scan has been found. Simulation stopped.\n");
        finish_flag = true;
        return;
    }

    // Preprocess data
    //for(curpos = good_start; curpos <= (int)good_end; curpos += BEARING_PACK_WORDS)
    //    preprocessBearing(&dump[curpos], true);


    qSleep(10000);
    printf("Simulation using dump file has been started\n");


    curpos = good_start;
    while(!finish_flag)
    {
        qSleep(19);
        if(simulation)
            continue;

        offset = dump[curpos + 0];
        for(int i = 0; i < BLOCK_TO_SEND; i++)
        {
            brg  = dump[curpos + 0];
            amps = dump[curpos + 1];
            div  = 1;//dump[curpos + 2];

            if(amps > PELENG_SIZE)
                amps = PELENG_SIZE;

            if(div == 0)
            {
                div = 1;
                fprintf(stderr, "ZERO divisor! BRG: %u, Dump position: %d\n", brg, curpos);
            }

            //file_divs[0][brg] = 1; //(div < 32) ? div : 1; // After debugging "div"

            memcpy(&tmp_brg[i], &dump[curpos], BEARING_PACK_SIZE);
            preprocessBearing(&tmp_brg[i][0], true);
            amplify(tmp_brg[i]);

            for(u_int32_t j = 0; j < amps; j++)
            {
                int32_t a = *((u_int32_t *)&tmp_brg[i][j + 3]);
                file_amps[0][brg * PELENG_SIZE + j] = a;
                //int32_t a = *((u_int32_t *)&dump[curpos + j + 3]);
                //file_amps[0][brg * PELENG_SIZE + j] = a;
                //file_amps[0][brg * PELENG_SIZE + j] = (float)(*((u_int32_t *)&dump[curpos + j + 3]));
            }

            curpos += BEARING_PACK_WORDS;
            if((u_int32_t)curpos > good_end)
                curpos = good_start;
        }

        if(!simulation)
            emit updateData(offset, BLOCK_TO_SEND, /*&file_divs[0][offset], */&file_amps[0][offset*PELENG_SIZE]);
    }
}

#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------

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

	dbgfd = ::open("simout.bin", O_CREAT | O_RDWR);

#endif // WRITE_2SCANS
    try
    {

#define SLEEPTIMESEC 30
//#ifdef PRINTERRORS
			printf("%s: Sleeping %d s while engine initializes.\n", __func__, SLEEPTIMESEC);
//#endif // PRINTERRORS
        sleep(SLEEPTIMESEC);
        //res = ioctl(fd, APCTRL_IOCTL_START, 256);
        res = ioctl(fd, APCTRL_IOCTL_START, 2048);
        if (-1 == res) {
            fprintf(stderr, "Failed to start APCTRL: %s\n", strerror(errno));
            return;
        }

		res = apctrl_regrd(0xe000, &scanidx);
		scanidx |= 0x20;
		res = apctrl_regwr(0xe000, scanidx);
		scanidx = 0;

//#ifdef PRINTERRORS
		printf("%s: IOCTL_START succeeded.\n", __func__);
//#endif // PRINTERRORS

        while(!finish_flag)
        {
            //clock_gettime(CLOCK_REALTIME, &t1);
            //clock_gettime(CLOCK_REALTIME, &t2);
            //double rdtime = (double)((long long)(t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec) / 1000000);
            //printf("%s: Read time %f ms (mean %f ms per bearing)\n", __func__, rdtime, rdtime / 256);

            res = ioctl(fd, APCTL_IOCTL_WAIT, &rdpnext);
            if (-1 == res) {
                fprintf(stderr, "%s: Failed to get current buffer: %s\n",
                        __func__, strerror(errno));
                throw -18;
            }

            if(simulation)
                continue;

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
                           ::write(dbgfd, bbuf->ptr, (800 + 3) * 4);
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
                                ::close(dbgfd);
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

            if(!simulation)
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
#endif //Q_OS_WIN
// ------------------------------------------------------


bool RadarDataSource::loadData() {
  char file1[25] = "res/pelengs/r1nm3h0_4096";
  char file2[25] = "res/pelengs/r1nm6h0_4096";
  //char file3[23] = "res/pelengs/simout.bin";

  if (!loadObserves1(file1/*, file_divs[0]*/, file_amps[0]))
    return false;
  if (!loadObserves1(file2/*, file_divs[1]*/, file_amps[1]))
    return false;

  /*
  if (!loadObserves2(file3, file_amps[0]))
    return false;
  */
  if (!initWithDummy(file_amps[1]))
    return false;


  /*
  for(int i = 0; i < 2; i++) {
      float amp = (i + 1) * 64;
      for(int j = 0; j < BEARINGS_PER_CYCLE; j++) {
          //file_divs[i][j] = 1.0;
          for(int k = 0; k < PELENG_SIZE; k++) {
              file_amps[i][(j * PELENG_SIZE) + k] = amp;
          }
      }
  }
  */

  return true;
}

bool RadarDataSource::loadObserves2(char* filename, GLfloat* amps) {
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
  //uint32_t bearing - номер пеленга (0...4095)
  //uint32_t datalen - 800 (800 32-битных амплитуд)
  //uint32_t bearlen - 803 (общая длина пакета в 32-битных словах)
  //uint32_t amps[800]

  if (file.is_open()) {
    std::streampos size = file.tellg();
    int32_t* memblock = new int32_t[size/4];

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

      //divs[i] = 4;
      for (int j = 0; j < PELENG_SIZE; j++) {
        if (j < 800) {
          amps[i*PELENG_SIZE + j] = memblock[803*BEARINGS_PER_CYCLE+3+j] / 4;

          //if (amps[i*PELENG_SIZE + j] > max_amp)
          //  max_amp = amps[i*PELENG_SIZE + j];

          //if (amps[i*PELENG_SIZE + j] < min_amp)
          //  min_amp = amps[i*PELENG_SIZE + j];
        } else {
          amps[i*PELENG_SIZE + j] = 0;
        }
      }
    }

    delete[] memblock;
    return true;
  }
  else {
    std::cerr << "Unable to open file";
    return false;
  }

  return true;
}

bool RadarDataSource::initWithDummy(GLfloat* amps) {
  for (uint i = 0; i < BEARINGS_PER_CYCLE; i++)
    for (uint j = 0; j < PELENG_SIZE; j++)
      //amps[i*PELENG_SIZE+j] = (255.f * ((j + i/2) % PELENG_SIZE)) / PELENG_SIZE;
      amps[i*PELENG_SIZE+j] = (255.f * j) / PELENG_SIZE;

  return true;
}

bool RadarDataSource::loadObserves1(char* filename, GLfloat* amps) {
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

    for (int i = 0; i < BEARINGS_PER_CYCLE/2; i++) {
      float div = memblock[headerSize + i*dataSize + 1];
      for (int j = 0; j < PELENG_SIZE; j++) {
        amps[(2*i)*PELENG_SIZE + j] = memblock[headerSize + i*dataSize + 2 + j] / div;
        amps[(2*i + 1)*PELENG_SIZE + j] = memblock[headerSize + i*dataSize + 2 + j] / div;
      }
    }

    delete[] memblock;
    return true;
  }

  std::cerr << "Unable to open file";
  return false;
}


#ifndef Q_OS_WIN
// Disable radar device for win OS
// ------------------------------------------------------
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
    // TODO: Remove this function
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

  static uint32_t tmp_brgbuf[800 + 3];

  if (syncstate != RDSS_SYNC)
    return res;

  //fprintf(stderr, "%s: Entered\n", __func__);

  firstbrg = processed_bearing + 1;
  if (firstbrg >= BEARINGS_PER_CYCLE)
    firstbrg = 0;

  if (last_bearing < processed_bearing) {
    num = BEARINGS_PER_CYCLE - processed_bearing - 1;
    for(int i = 0; i < num; i++) {
      processed_bearing++;
      amps = scans[activescan][processed_bearing]->ptr[1];
      div  = 1; //scans[activescan][processed_bearing]->ptr[2];

      if(amps > PELENG_SIZE)
        amps = PELENG_SIZE;

//	  fprintf(stderr, "%s: 1. Copy radar data to temporal buffer\n", __func__);
	  for(uint32_t j = 0; j < amps + 3; j++)
		  tmp_brgbuf[j] = scans[activescan][processed_bearing]->ptr[j];
	  //fprintf(stderr, "%s: 1. Copy done.\n", __func__);
      preprocessBearing(tmp_brgbuf, true);
	  //fprintf(stderr, "%s: 1. Data preprocessed.\n", __func__);
      amplify(tmp_brgbuf);

	  if(div == 0)
		  div = 1;
      //file_divs[0][processed_bearing] = (div < 32) ? div : 1; // After debugging "div"
      ptr = &(tmp_brgbuf[3]);
      //ptr = &(scans[activescan][processed_bearing]->ptr[3]);

      for(uint32_t j = 0; j < amps; j++)
        file_amps[0][processed_bearing * PELENG_SIZE + j] = *ptr++;

      scans[activescan][processed_bearing]->valid = false;
    }

    if(num) {
      emit updateData(firstbrg, num, /*&file_divs[0][firstbrg], */&file_amps[0][firstbrg * PELENG_SIZE]);
    }

    res      += num;
    num       = last_bearing + 1;
    firstbrg  = 0;
  } else
    num = last_bearing - processed_bearing;

  res      += num;

  //fprintf(stderr, "%s: 2. About to precess bearings.\n", __func__);

  for(int i = 0; i < num; i++) {
    if(++processed_bearing >= BEARINGS_PER_CYCLE)
      processed_bearing = 0;

	//fprintf(stderr, "%s: 2. Getting apms and div (BRG: %04u, scan: %d)\n", __func__, processed_bearing, activescan);

    amps = scans[activescan][processed_bearing]->ptr[1];
    div  = 1;// scans[activescan][processed_bearing]->ptr[2];

	//fprintf(stderr, "%s: 2. apms %u. div %u\n", __func__,amps, div);


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
    //div = 4; // 10 bit data downgraded to 8 bit //1600 / 255 + 1;


    if (amps > PELENG_SIZE)
      amps = PELENG_SIZE;

	//fprintf(stderr, "%s: 2. Copy radar data to temporal buffer\n", __func__);
    for(uint32_t j = 0; j < amps + 3; j++)
		tmp_brgbuf[j] = scans[activescan][processed_bearing]->ptr[j];
	//fprintf(stderr, "%s: 2. Copy done.\n", __func__);
    preprocessBearing(tmp_brgbuf, true);
	//fprintf(stderr, "%s: 2. Data preprocessed.\n", __func__);
    amplify(tmp_brgbuf);

	if(div == 0)
		div = 1;
    //file_divs[0][processed_bearing] = (div < 32) ? div : 1; // After debugging "div"
    ptr = &(tmp_brgbuf[3]);
    //ptr = &(scans[activescan][processed_bearing]->ptr[3]);
	//fprintf(stderr, "%s: 2. Preparation for copying amplitudes is done.\n", __func__);

    for(uint32_t j = 0; j < amps; j++)
      file_amps[0][processed_bearing * PELENG_SIZE + j] = *ptr++;

    scans[activescan][processed_bearing]->valid = false;
  }

  //fprintf(stderr, "%s: 2. Bearings done.\n", __func__);

  if(num) {
	//fprintf(stderr, "%s: emit updateData.\n", __func__);
    emit updateData(firstbrg, num, /*&file_divs[0][firstbrg], */&file_amps[0][firstbrg * PELENG_SIZE]);
	//fprintf(stderr, "%s: emit updateData done.\n", __func__);
  }

  //fprintf(stderr, "%s: Leaving.\n", __func__);
  return res;
}
#endif // !Q_OS_WIN
// ------------------------------------------------------

int RadarDataSource::findZeroBrg(int start)
{
    if(dump == NULL)
        return -1;
    for(; start < dsize; start += BEARING_PACK_WORDS)
    {
        if(dump[start] == 0)
            return start;
    }

    return -1;
}


int RadarDataSource::chkScans(int start)
{
    u_int32_t prevbrg;
    int       scans;
    int       good_end = -4;

    if(dump == NULL)
        return -1;
    if(start >= dsize)
        return -2;
    prevbrg = dump[start];
    if(prevbrg != 0)
        return -3;

    scans = 0;
    for(start += BEARING_PACK_WORDS; start < dsize; start += BEARING_PACK_WORDS)
    {
        if(dump[start] == 0)
        {
            if(prevbrg != (BEARINGS_PER_CYCLE - 1))
                break; //return good_end;
            scans++;
        }
        else if(dump[start] == (BEARINGS_PER_CYCLE - 1))
            good_end = start;
        else if((dump[start] - prevbrg) != 1)
            break; //return good_end;
        prevbrg = dump[start];
    }

    if(scans > 0)
        printf("Found %d successive good scans\n", scans);
    return good_end;
}

void RadarDataSource::setAmpsOffset(int off) {
  ampoffset = off;
}

int RadarDataSource::getAmpsOffset(void) {
  return ampoffset;
}

int RadarDataSource::preprocessBearing(u_int32_t * brgdata, bool inv)
{
    int32_t v;
    int32_t div;

    div = *(int32_t *)&brgdata[2];

    if(inv)
    {
        for(int i = 3; i < PELENG_SIZE + 3; i++)
        {
            v = *(int32_t *)&brgdata[i] * (-1) / div - ampoffset;
            if(v < 0)
                v = 0;
            v /= 16;
            brgdata[i] = *(u_int32_t *)&v;
        }
    }
    else
    {
        for(int i = 3; i < PELENG_SIZE + 3; i++)
        {
            v = *(int32_t *)&brgdata[i] / div - ampoffset;
            if(v < 0)
                v = 0;
            v /= 16;
            brgdata[i] = *(u_int32_t *)&v;
        }
    }

    return 0;
}

#ifndef Q_OS_WIN

int RadarDataSource::initGen(bool usetimeout)
{
    unsigned int      i;
    int               res = 0;
    static const struct genval
    {
        u32 offset;
        u32 val;
    } gval[] = {
                  {  0, 0x54},
                  {  1, 0xe1},
                  {  2, 0x42},
                  {  3, 0x55},
                  {  4, 0x00},
                  {  5, 0x3f},
                  { 21, 0x7c},
                  { 25, 0x80},
                  { 33, 0x03},
                  { 36, 0x03},
                  { 40, 0x40},
                  { 41, 0xca},
                  { 42, 0x7b},
                  { 44, 0x04},
                  { 45, 0xbe},
                  { 47, 0x09},
                  { 48, 0xc3},
                  { 55, 0x08},
                  {131, 0x18},
                  {138, 0x0c},
                  {139, 0xcc}
                };

    if(fd == -1)
    {
       fprintf(stderr, "AP controller device file is not opened.\n");
       return -1;
    }

    printf("Initializing generator (Cmd reg: 0x%08X)...\n", APCTRL_GEN_BASEADDR);
    usetimeout = true;
    for(i = 0; i < sizeof(gval) / sizeof(gval[0]); i++)
    {
		res = apctrl_adcspi_send(
				APCTRL_GEN_BASEADDR,
				0x1f0000 + (gval[i].offset & 0x000000ff),
				0x1f4000 + (gval[i].val & 0x000000ff), usetimeout);
		if(res != 0)
		{
			fprintf(stderr, "%s: apctrl_adcspi_send failed (%d)\n", __func__, res);
			return res;
		}
    }

    printf("Generator initialization done.\n");

    return res;
}


int RadarDataSource::initADC(void)
{
    int res;
    static const u_int32_t regv[] =
    {
        0x2e001409,
        0x2e001680,
        0x2e000f02,
        0x2e00ff01
    };

    for(unsigned int i = 0; i < sizeof(regv) / sizeof(regv[0]); i += 2)
    {
        res = apctrl_adcspi_send(APCTRL_ADC_BASEADDR, regv[i], regv[i + 1], true);
        if(res != 0)
        {
            fprintf(stderr, "%s: apctrl_adcspi_send failed (%d)\n", __func__, res);
            break;
        }
    }

    return res;
}

int RadarDataSource::initDAC(void)
{
    int res;
    static const u_int32_t regv[] =
    {
        0x0e00A000,
        0x0e008000,
        0x0e048000,
        0x0e0a8300
    };

    for(unsigned int i = 0; i < sizeof(regv) / sizeof(regv[0]); i += 2)
    {
        res = apctrl_adcspi_send(APCTRL_ADC_BASEADDR, regv[i], regv[i + 1], true);
        if(res != 0)
        {
            fprintf(stderr, "%s: apctrl_adcspi_send failed (%d)\n", __func__, res);
            break;
        }
    }

    return res;
}

int RadarDataSource::apctrl_regwr(u_int32_t regaddr, u_int32_t regval)
{
    int res;
    struct apctrl_reg r;

    if(fd == -1)
        return 1;

    r.offset = regaddr;
    r.val    = regval;
    r.mask   = 0xffffffff;
    res = ioctl(fd, APCTL_IOCTL_SETREG, &r);
    return res;
}


int RadarDataSource::apctrl_regrd(u_int32_t regaddr, u_int32_t * regval)
{
    int res;
    struct apctrl_reg r;

    if(fd == -1)
        return 1;
    if(regval == NULL)
        return 2;

    r.offset = regaddr;
    r.mask   = 0xffffffff;
    res = ioctl(fd, APCTL_IOCTL_GETREG, &r);
	if(res == 0)
		*regval = r.val;
    return res;
}

int RadarDataSource::apctrl_adcspi_send(u_int32_t baseaddr, u_int32_t addrv, u_int32_t datav, bool usetimeout)
{
    int               res;
    u_int32_t         v;
    struct itimerspec its;

    if(fd == -1)
       return 1;

    if(adcspi_tmid == NULL)
        usetimeout = false;

	//printf("SPI: Address write\n");
    // Write address to ADC SPI command register
    if((res = apctrl_regwr(baseaddr, addrv)) != 0)
    {
        if(res > 0)
            fprintf(stderr, "apctrl_regrd failed: %d\n", res);
        else
            fprintf(stderr, "apctrl_regrd failed: %s\n", strerror(errno));
        fprintf(stderr, "\tADDR: 0x%08X, DATA: 0x%08X\n", baseaddr, addrv);
        return res;
    }

    if(usetimeout)
    {
        // Start SPI timeout timer
        its.it_value.tv_sec     = APCTRL_SPIWR_TIMEOUT / 1000000000;
        its.it_value.tv_nsec    = APCTRL_SPIWR_TIMEOUT % 1000000000;
        its.it_interval.tv_sec  = 0;
        its.it_interval.tv_nsec = 0;

        spi_timeout = 0;
        if(timer_settime(adcspi_tmid, 0, &its, NULL) != -1)
        {
            v = 0;
            do
            {
                if(spi_timeout)
                {
                    fprintf(stderr, "%s: SPI TIMOUT elapsed!\n", __func__);
                    break;
                }

                res = apctrl_regrd(baseaddr, &v);
                if(res != 0)
                {
                    if(res > 0)
                        fprintf(stderr, "apctrl_regrd failed: %d\n", res);
                    else
                        fprintf(stderr, "apctrl_regrd failed: %s\n", strerror(errno));
                    fprintf(stderr, "\tADDR: 0x%08X, DATA: 0x%08X\n", baseaddr, addrv);
                    break;
                }
            }while(!(v & 0x80000000));

            // Disarm timer
            its.it_value.tv_sec     = 0;
            its.it_value.tv_nsec    = 0;
            its.it_interval.tv_sec  = 0;
            its.it_interval.tv_nsec = 0;
            if(timer_settime(adcspi_tmid, 0, &its, NULL) == -1)
				fprintf(stderr, "Timer disarm failed: %s\n", strerror(errno));
        }
        else
        {
            fprintf(stderr, "Failed to start timer for SPI timeout handler: %s\n", strerror(errno));
            fprintf(stderr, "WARNING: Start of the timer for SPI timeout failed and the operation my hang.\n");
            fprintf(stderr, "\t Use Ctrl+C to terminate in such case.\n");
        }
    }

	//printf("SPI: Command write\n");
    // Write address to ADC SPI command register
    if((res = apctrl_regwr(baseaddr, datav)) != 0)
    {
        if(res > 0)
            fprintf(stderr, "apctrl_regrd (datav) failed: %d\n", res);
        else
            fprintf(stderr, "apctrl_regrd (datav) failed: %s\n", strerror(errno));
        fprintf(stderr, "\tADDR: 0x%08X, DATA: 0x%08X\n", baseaddr, datav);
        return res;
    }

    if(usetimeout)
    {
        // Start SPI timeout timer
        its.it_value.tv_sec     = APCTRL_SPIWR_TIMEOUT / 1000000000;
        its.it_value.tv_nsec    = APCTRL_SPIWR_TIMEOUT % 1000000000;
        its.it_interval.tv_sec  = 0;
        its.it_interval.tv_nsec = 0;

        spi_timeout = 0;
        if(timer_settime(adcspi_tmid, 0, &its, NULL) != -1)
        {
            v = 0;
            do
            {
                if(spi_timeout)
                {
                    fprintf(stderr, "%s: SPI TIMOUT elapsed!\n", __func__);
                    break;
                }

                res = apctrl_regrd(baseaddr, &v);
                if(res != 0)
                {
                    if(res > 0)
                        fprintf(stderr, "apctrl_regrd (datav) failed: %d\n", res);
                    else
                        fprintf(stderr, "apctrl_regrd (datav) failed: %s\n", strerror(errno));
                    fprintf(stderr, "\tADDR: 0x%08X, DATA: 0x%08X\n", baseaddr, datav);
                    break;
                }
            }while(!(v & 0x80000000));

            // Disarm timer
            its.it_value.tv_sec     = 0;
            its.it_value.tv_nsec    = 0;
            its.it_interval.tv_sec  = 0;
            its.it_interval.tv_nsec = 0;
            if(timer_settime(adcspi_tmid, 0, &its, NULL) == -1)
				fprintf(stderr, "Timer disarm failed: %s\n", strerror(errno));
        }
        else
        {
            fprintf(stderr, "Failed to start timer for SPI timeout handler: %s\n", strerror(errno));
            fprintf(stderr, "WARNING: Start of the timer for SPI timeout failed and the operation my hang.\n");
            fprintf(stderr, "\t Use Ctrl+C to terminate in such case.\n");
        }
    }

    return res;
}
#endif // !Q_OS_WIN


void RadarDataSource::nextScale() {
  if (_radar_scale->nextScale() == 0) {
    const rli_scale_t* scale = _radar_scale->getCurScale();
    setupScale(scale);
  }
}

void RadarDataSource::prevScale() {
  if(_radar_scale->prevScale() == 0) {
    const rli_scale_t* scale = _radar_scale->getCurScale();
    setupScale(scale);
  }
}

int RadarDataSource::setupScale(const rli_scale_t* pscale) {
  int res = 1;

  #ifndef Q_OS_WIN
  if(pscale == NULL)
    return res;

  res = apctrl_regwr(APCTRL_PKIDPKOD_BASEADDR, pscale->pkidpkod);
  if(res != 0) {
    fprintf(stderr, "%s: PKID and PKOD setup failed (%d)\n", __func__, res);
    return res;
  }

  res = apctrl_adcspi_send(APCTRL_GEN_BASEADDR, pscale->gen_addr, pscale->gen_dat, true);
  if(res != 0) {
    fprintf(stderr, "%s: Frequency setup failed (%d)\n", __func__, res);
    return res;
  }
  #endif // !Q_OS_WIN

  emit scaleChanged(*_radar_scale);

  return res;
}

void RadarDataSource::setGain(int gain) {
  if (static_cast<u_int32_t>(gain) >= max_gain_level)
    gain_level = max_gain_level - 1;
  else
    gain_level = gain;
}

int RadarDataSource::amplify(u_int32_t* brg) {
    u_int32_t v;
    u_int32_t tr;
    u_int32_t ratio;

    if(!gain_level)
        return 0; // Nothing to do

    tr = max_gain_level - gain_level;
    ratio = (255 << 16) / tr;
    for(int i = 3; i < PELENG_SIZE + 3; i++)
    {
        v = brg[i];
        if(v >= tr)
            v = (255 << 16);
        else
            v = v * ratio;
        brg[i] = v >> 16;
    }

    return 0;
}

int RadarDataSource::setupHIP(hip_t hiptype, hip_channel_t hipch)
{
    u_int32_t v;
    u_int32_t regv;
    u_int32_t mask;
    int       res = 0;

    try
    {
        if((hiptype < HIP_FIRST) || (hiptype > HIP_LAST))
            throw 100;
        if((hipch < HIPC_FIRST) || (hipch > HIPC_LAST))
            throw 101;

        switch(hiptype)
        {
        case HIP_NONE:
            v = APCTRL_HIP_NONE;
            break;
        case HIP_WEAK:
            v = APCTRL_HIP_WEAK;
            break;
        case HIP_STRONG:
            v = APCTRL_HIP_STRONG;
            break;
		default:
			return 100;
        }

        mask = APCTRL_HIP_MASK;

        if(hipch == HIPC_MAIN)
        {
            v    <<= APCTRL_HIP_SHIFT;
            mask <<= APCTRL_HIP_SHIFT;
        }
        else // if(hipch == HIPC_SARP)
        {
            v    <<= APCTRL_HIP_SARP_SHIFT;
            mask <<= APCTRL_HIP_SARP_SHIFT;
        }
#ifndef Q_OS_WIN
        //res = ioctl(fd, APCTRL_IOCTL_STOP);
        //if(res != 0)
        //{
        //    fprintf(stderr, "APCTRL_IOCTL_STOP failed.\n");
        //    throw res;
        //}
        //res = apctrl_regrd(APCTRL_HIP_BASEADDR, &regv);
        //if(res != 0)
        //    throw res;
		//
        regv = (hipregv & (~mask)) | v | 0x2000;
        res = apctrl_regwr(APCTRL_HIP_BASEADDR, regv);
        if(res != 0)
            throw res;
		fprintf(stderr, "HIP: %d, 0x%08X\n", hiptype, regv);
		hipregv = regv;
        //res = ioctl(fd, APCTRL_IOCTL_START, 2048);
        //if(res != 0)
        //{
        //    fprintf(stderr, "APCTRL_IOCTL_START failed.\n");
        //    throw res;
        //}
#endif // !Q_OS_WIN
    }
    catch(int e)
    {
        if(e > 0)
            fprintf(stderr, "Failed to setup HIP: %d\n", e);
        else // if(e < 0)
            fprintf(stderr, "HIP setup failed: %s\n", strerror(errno));
        res = e;
    }

    return res;
}

int RadarDataSource::nextHIP(hip_channel_t hipch)
{
    hip_t htype;
    hip_t * ptype;
    int   res;

    switch(hipch)
    {
    case HIPC_MAIN:
        ptype = &hip_main;
        break;
    case HIPC_SARP:
        ptype = &hip_sarp;
        break;
    default:
        return 101;
    }

    htype = *ptype;

    htype = (hip_t)(htype + 1);
    if((htype < HIP_FIRST) || (htype > HIP_LAST))
        htype = HIP_FIRST;

    res = setupHIP(htype, hipch);
    if(res)
        return res;
    *ptype = htype;
    return 0;
}

void RadarDataSource::onSimulationChanged(const QByteArray& str) {
  simulate(str.length() == 3);
}

int RadarDataSource::simulate(bool sim)
{
    int res = 0;
#ifndef Q_OS_WIN
    if(fd == -1)
    {
        simulation = sim;
        return 0;
    }

    u_int32_t regv;

    if(simulation == sim)
        return 0; // Nothing to do
    try
    {
        if(sim == true)
        {
            res = apctrl_regrd(APCTRL_CTRL_BASEADDR, &regv);
            if(res < 0)
                throw res;
            regv &= ~0x0f;
            regv |= APCTRL_CTRL_INTIZIEN | APCTRL_CTRL_MRPEN | APCTRL_CTRL_SIMULEN;
            res = apctrl_regwr(APCTRL_CTRL_BASEADDR, regv);
            if(res < 0)
                throw res;
            res = apctrl_regrd(APCTRL_CTRL_BASEADDR, &regv);
            if(res < 0)
                throw res;

            res = apctrl_regrd(APCTRL_HIP_BASEADDR, &regv);
            regv |= 1 << 13;
            res = apctrl_regwr(APCTRL_HIP_BASEADDR, regv);
            if(res < 0)
                throw res;
        }
        else
        {
            res = apctrl_regrd(APCTRL_CTRL_BASEADDR, &regv);
            if(res < 0)
                throw res;
            regv &= ~0x0f;
            regv |= APCTRL_CTRL_MRPEN;
            res = apctrl_regwr(APCTRL_CTRL_BASEADDR, regv);
            if(res < 0)
                throw res;
            res = apctrl_regrd(APCTRL_CTRL_BASEADDR, &regv);
            if(res < 0)
                throw res;

            res = apctrl_regrd(APCTRL_HIP_BASEADDR, &regv);
            regv &= ~(1 << 13);
            res = apctrl_regwr(APCTRL_HIP_BASEADDR, regv);
            if(res < 0)
                throw res;
        }
        simulation = sim;
    }
    catch(int e)
    {
        if(e > 0)
            fprintf(stderr, "Failed to %s simulation: %d\n", ((sim == true) ? "start" : "stop"), e);
        else // if(e < 0)
            fprintf(stderr, "Failed to %s simulation: %s\n", ((sim == true) ? "start" : "stop"), strerror(errno));
        res = e;
    }

#endif // !Q_OS_WIN
    return res;
}

void RadarDataSource::updateHeading(float hdg)
{
  int res;
  u_int32_t regv;

  regv = (u_int32_t)(hdg * 4096.0 / 360.0);
  if(regv == gyroReg)
    return;
#ifndef Q_OS_WIN
  if(fd == -1)
    return;
  res = apctrl_regwr(APCTRL_GYROREG_BASEADDR, regv);
  if(res < 0)
  {
    fprintf(stderr, "%s: APCTRL_GYROREG_BASEADDR write with value 0x%X failed\n", __func__, regv);
  }
  else
  {
    gyroReg = regv;
    fprintf(stderr, "%s: APCTRL_GYROREG_BASEADDR writen with 0x%X\n", __func__, regv);
  }
  // TODO: check return value of apctrl_regwr and signal error
#endif // !Q_OS_WIN
}
