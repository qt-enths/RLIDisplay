#ifndef RLICONTROLEVENT_H
#define RLICONTROLEVENT_H

#include <QString>
#include <QEvent>


class RLIControlEvent : public QEvent  {

public:
  enum RLIControlSpinners { NoSpinner
                          , Gain
                          , Water
                          , Rain
                          , VN
                          , VD };

  enum RLIControlButtons { NoButton
                         , ButtonPlus
                         , ButtonMinus
                         , CenterShift
                         , ParallelLines
                         , Menu
                         , Up
                         , Down
                         , Enter
                         , Back };

  explicit RLIControlEvent(RLIControlButtons b = NoButton
                         , RLIControlSpinners s = NoSpinner
                         , float v = 0);

  inline RLIControlSpinners spinner() { return _spinner; }
  inline RLIControlButtons button() { return _button; }
  inline float spinnerVal() { return _spinnerVal; }

private:
  float _spinnerVal;

  RLIControlButtons _button;
  RLIControlSpinners _spinner;
};

#endif // RLICONTROLEVENT_H
