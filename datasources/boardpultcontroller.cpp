#include "boardpultcontroller.h"

#ifndef Q_OS_WIN

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/input.h>

#define ABS_AXIS_MUL 1

#endif // !Q_OS_WIN

BoardPultController::BoardPultController(QObject *parent) : QObject(parent) {
#ifndef Q_OS_WIN
  evdevFd    = -1;
  stopEvdev  = 0;

  fprintf(stderr, "Trying /dev/input/event1\n");
  if(setupEvdev("/dev/input/event1") != 0) {
    fprintf(stderr, "Trying /dev/input/event2\n");

    if( setupEvdev("/dev/input/event2") != 0) {
      fprintf(stderr, "Trying /dev/input/event0\n");

      if( setupEvdev("/dev/input/event0") != 0)
        fprintf(stderr, "ERROR: no input event device 'Board Pult'. Givinig up!\n");
      else
        fprintf(stderr, "Input device: /dev/input/event0\n");
    } else
      fprintf(stderr, "Input device: /dev/input/event2\n");
  } else
    fprintf(stderr, "Input device: /dev/input/event1\n");
#endif // !Q_OS_WIN
}

BoardPultController::~BoardPultController() {
#ifndef Q_OS_WIN
  printf("Waiting for the input event device thread to terminate.\n");
  stopEvdev = 1;
  while(evdevThread.isRunning());
  ::close(evdevFd);
  printf("Input event device thread terminated.\n");
  evdevFd = -1;
}
#endif // !Q_OS_WIN

#ifndef Q_OS_WIN

int BoardPultController::setupEvdev(char *evdevfn) {
  int     res;
  int     fd, ver;
  struct  input_id iid;
  char    edname[256];

  if(evdevFd != -1) {
    fprintf(stderr, "WARNING: Input event device file is already opened.\n");
    return 1;
  }

  if (evdevThread.isRunning())
    return 2;

  if(evdevfn == NULL) {
    fprintf(stderr, "WARNING: Name of an input event device file is given.\n\tGain, Rain and Waves controls are unavailable\n");
    return -1000;
  }

  fd = ::open(evdevfn, O_RDWR);
  if(fd == -1) {
    res = errno;
    fprintf(stderr, "WARNING: Failed to open input event device file.\n\tGain, Rain and Waves controls are unavailable\n");
    fprintf(stderr, "Error: %s\n", strerror(res));
    return res;
  }

  res = ::ioctl(fd, EVIOCGVERSION, &ver);
  if(res == -1) {
    res = errno;
    fprintf(stderr, "WARNING: Failed to get version of input event device.\n");
    fprintf(stderr, "Error: %s\n", strerror(res));
  } else
    printf("Version of input device driver: %d\n", ver);

  res = ::ioctl(fd, EVIOCGID, &iid);
  if(res == -1) {
    res = errno;
    fprintf(stderr, "WARNING: Failed to get ID of input event device.\n");
    fprintf(stderr, "Error: %s\n", strerror(res));
  } else
    printf("ID of input event device:\n\tbus %04x, vendor %04x, product %04x, version %04x\n", iid.bustype, iid.vendor, iid.product, iid.version);

  res = ::ioctl(fd, EVIOCGNAME(sizeof(edname)), edname);
  if(res == -1) {
    res = errno;
    fprintf(stderr, "ERROR: Failed to get the name of the input event device.\n\tGain, Rain and Waves controls are unavailable\n");
    fprintf(stderr, "Error: %s\n", strerror(res));
    ::close(fd);
    return res;
  }
  printf("Input event device name: %s\n", edname);

  QString sname(edname);
  res = sname.indexOf("Pult", 0, Qt::CaseInsensitive);
  if(res == -1) {
    res = errno;
    fprintf(stderr, "ERROR: Wrong input event device name. Must be \'Board Pult\'.\n\tGain, Rain and Waves controls are unavailable\n");
    fprintf(stderr, "Error: %s\n", strerror(res));
    ::close(fd);
    return res;
  }

  evdevFd = fd;

  stopEvdev   = 0;
  evdevThread = QtConcurrent::run(this, &BoardPultController::evdevHandleThread);
  return 0;
}

void BoardPultController::evdevHandleThread() {
  struct input_event ie;
  int    value;

  if(evdevFd == -1) {
    fprintf(stderr, "WARNING: Input event device file is not opened.\n\tGain, Rain and Waves controls are unavailable\n");
    return;
  }

  while(!stopEvdev) {
    int res = read(evdevFd, &ie, sizeof(ie));

    if(res != sizeof(ie)) {
      printf("read returned: %d\n", res);
      continue;
    }

    if(ie.type != EV_ABS) {
      printf("type != EV_ABS: %d\n", ie.type);
      continue;
    }

    //----- now the e.code field contains the axis ID, value should be in range [0; 255]
    //
    switch(ie.code) {
    case ABS_THROTTLE:
      value = ie.value * ABS_AXIS_MUL;
      value = 255 - value;
      emit gain_changed(value);
      break;
    case ABS_GAS:
      value = ie.value * ABS_AXIS_MUL;
      value = 255 - value;
      emit wave_changed(value);
      break;
    case ABS_BRAKE:
      value = ie.value * ABS_AXIS_MUL;
      value = 255 - value;
      emit rain_changed(value);
      break;
    default:
      printf("EV_ABS: code: %u, value %d\n", (unsigned int) ie.code, ie.value);
    }
  }
#endif // !Q_OS_WIN
}
