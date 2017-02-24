#ifndef RLICONTROLWIDGET_H
#define RLICONTROLWIDGET_H

#include <QWidget>

namespace Ui {
  class RLIControlWidget;
}

class RLIControlWidget : public QWidget
{
  Q_OBJECT

public:
  explicit RLIControlWidget(QWidget *parent = 0);
  ~RLIControlWidget();

  inline void setReciever(QWidget* w) { _reciever = w; }

private slots:
  void on_sldVN_sliderReleased();
  void on_sldVD_sliderReleased();

  void on_sldVN_sliderMoved(int position);
  void on_sldVD_sliderMoved(int position);

  void on_btnMode3_clicked();
  void on_btnModeDec_clicked();
  void on_btnModeInc_clicked();

  void on_sldGain_valueChanged(int value);
  void on_sldWater_valueChanged(int value);
  void on_sldRain_valueChanged(int value);

  void on_btnOnOff3_clicked();

  void on_btnMenu_clicked();

  void on_btnTrace1_clicked();
  void on_btnTrace4_clicked();
  void on_btnTrace5_clicked();
  void on_btnTrace6_clicked();


  void on_btnConfigMenu_clicked();

signals:
  void gainChanged(u_int32_t val);
  void waterChanged(int val);
  void rainChanged(int val);

private:
  int _vn_pos;
  int _vd_pos;

  QWidget* _reciever;
  Ui::RLIControlWidget *ui;
};

#endif // RLICONTROLWIDGET_H
