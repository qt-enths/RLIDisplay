#include "rlicontrolevent.h"

RLIControlEvent::RLIControlEvent( RLIControlButtons b
                                , RLIControlSpinners s
                                , float v)
  : QEvent((Type) (QEvent::User + 111)) {
  _button = b;
  _spinner = s;
  _spinnerVal = v;
}
