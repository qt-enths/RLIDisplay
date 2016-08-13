#ifndef RLISTRINGS_H
#define RLISTRINGS_H

#include <QByteArray>

enum RLILang { RLI_LANG_ENGLISH = 0, RLI_LANG_RUSSIAN = 1, RLI_LANG_COUNT = 2 };

namespace RLIStrings {
  static char* nEng[2] =     { "ENG",	"АНГЛ" };
  static char* nRus[2] =     { "RUS",	"РУС" };

  static char* nEmsn[2] =    { "EMISSION",	"ИЗЛУЧЕНИЕ" };
  static char* nGrad[2] =    { "°   ",	"°   " };
  static char* nGain[2] =    { "GAIN", "УСИЛЕНИЕ" };
  static char* nRain[2] =    { "RAIN", "ДОЖДЬ" };
  static char* nRain1[2] =   { "RAIN 1", "ДОЖДЬ 1" };
  static char* nWave[2] =    { " SEA ", "ВОЛНЫ" };
  static char* nAfc[2] =     { "AFC", "АПЧ" };
  static char* nTune[2] =    { "TUNE", "РПЧ" };
  static char* nMode[2] =    { "MODE", "РЕЖИМЫ" };
  static char* nHead[2] =    { "HEAD", "КУРС" };
  static char* nNord[2] =    { "NORTH", "СЕВЕР" };
  static char* nCourse[2] =  { "COURSE", "КУРС СТ" };
  static char* nRm[2] =      { "RM", "ОД" };
  static char* nTm[2] =      { "TM", "ИД" };
  static char* nGstab[2] =   { "GROUND", "ГРУНТ" };
  static char* nWstab[2] =   { "WATER", "ВОДА" };
  static char* nEbl[2] =     { "EBL", "ВН" };
  static char* nVN[2] =      { "B ", "П " };
  static char* nTrl[2] =     { "TAILS", "СЛЕДЫ" };
  static char* nDot[2] =     { "POINTS", "ТОЧКИ" };
  static char* nMin[2] =     { "MIN ", "МИН " };
  static char* nVrm[2] =     { "VRM", "ВД" };
  static char* nBlnk[2] =    { "  ", "  " };
  static char* nNM[2] =      { "NM  ", "МИЛЬ" };
  static char* nKM[2] =      { "KM  ", "КМ  " };
  static char* nMETR[2] =    { "M   ", "М   " };
  static char* nGiro[2] =    { "GYRO HDG ", "КУРС ГИРО" };
  static char* nOffGiro[2] = { "GYRO OFF ", "КУРС ОТКЛ" };
  static char* nSspd[2] =    { "LOG SPD(S)", "СК ЛАГ(С) " };
  static char* nLspd[2] =    { "LOG SPD(W)", "СК ЛАГ(В) " };
  static char* nMspd[2] =    { "MAN SPD   ", "СК РУЧ    " };
  static char* nKts[2] =     { "KTS ", "УЗ  " };
  static char* nKms[2] =     { "KM/H", "КМ/Ч" };
  static char* nLat[2] =     { "LAT    ", "ШИРОТА " };
  static char* nNul[2] =     { "", "" };
  static char* nLon[2] =     { "LON    ", "ДОЛГОТА" };
  static char* nACTC[2] =    { "ACT COURSE  ", "КУРС АОЦ  " };
  static char* nGPSC[2] =    { "GPS COURSE  ", "КУРС СНС  " };
  static char* nDLGC[2] =    { "DLG COURSE  ", "КУРС ДЛГ  " };
  static char* nACTS[2] =    { "ACT SPEED", "СКОР АОЦ " };
  static char* nGPSS[2] =    { "GPS SPEED", "СКОР СНС " };
  static char* nDLGS[2] =    { "DLG SPEED", "СКОР ДЛГ " };
  static char* nVec[2] =     { "VECTOR", "ВЕКТОР" };
  static char* nDng[2] =     { "DANGER TRG", "ОПАСНАЯ ЦЕЛЬ" };
  static char* nCPA[2] =     { "CPA LIMIT  ", "ОПАСНАЯ ДКС" };
  static char* nVks[2] =     { "TCPA LIMIT ", "ОПАСНОЕ ВКС" };
  static char* nTrgN[2] =    { "TRG  №    ALL", "ЦЕЛЬ №    ВСЕГО" };
  static char* nTrg[2] =     { "TRG       ALL", "ЦЕЛЬ      ВСЕГО" };
  static char* nBear[2] =    { "BEARING  ", "ПЕЛЕНГ   " };
  static char* nRng[2] =     { "RANGE    ", "ДАЛЬНОСТЬ" };
  static char* nCrsW[2] =    { "COURSE(W)", "КУРС  (В)" };
  static char* nSpdW[2] =    { "SPEED (W)", "СКОР  (В)" };
  static char* nCrsG[2] =    { "COURSE(G)", "КУРС  (Г)" };
  static char* nSpdG[2] =    { "SPEED (G)", "СКОР  (Г)" };
  static char* nTcpa[2] =    { "CPA", "ДКС" };
  static char* nTtcpa[2] =   { "TCPA", "ВКС " };
  static char* nDcc[2] =     { "DCC", "ДПК" };
  static char* nTcc[2] =     { "TCC", "ВПК" };
  static char* nMrk[2] =     { "CURSOR", "КУРСОР" };
  static char* nBrn[2] =     { "HB       ", "КУРС.УГОЛ" };
  static char* nOff[2] =     { "OFF", "ОТКЛ" };
  static char* nOn[2] =      { "ON", " ВКЛ" };
  static char* nAPZ[2] =     { "AUTO A/C", "АПЗ" };
  static char* nBandX[2] =   { "X-BAND", "Х-ДИАП" };
  static char* nBandS[2] =   { "S-BAND", "S-ДИАП" };
  static char* nBandK[2] =   { "K-BAND", "K-ДИАП" };

  // для маршрута
  static char* nGO[2] =      { "GO TO RP", "ИДЕМ НА МТ" };
  static char* nPar[2] =     { "PARAM OF A ROUTE", "ПАРАМЕТРЫ МАРШРУТА" };
  static char* nFar[2] =     { "FAIRWAY m", "ФАРВАТЕР м" };
  static char* nDfl[2] =     { "DEFLECT   ", "ОТКЛОНЕН  " };
  static char* nDist[2] =    { "RANGE RP", "ДАЛЬН МТ" };
  static char* nTime[2] =    { "TIME  RP", "ВРЕМЯ МТ" };
  static char* nPU[2] =      { "TA ON RP", "ПУ НА МТ" };
  static char* nNK[2] =      { "NEW COURSE", "НОВЫЙ КУРС" };
  static char* nEcho[2] =    { "DEPTH", "ГЛУБИНА" };
  static char* nMetrs[2] =   { "METRS", "МЕТРЫ" };

  static char* nMenu0[2] =   { "MAIN MENU", "ГЛАВНОЕ МЕНЮ" };

  static char* nMenu00[2] =  { "BRIGHTNESS", "ЯРКОСТЬ" };
  static char* nMenu01[2] =  { "AUTO PLOTTING", "АВТОПРОКЛАДКА" };
  static char* nMenu02[2] =  { "SETTINGS", "УСТАНОВКИ" };
  static char* nMenu03[2] =  { "RADAR TEST", "КОНТРОЛЬ" };
  static char* nMenu04[2] =  { "MAP", "КАРТА" };
  static char* nMenu05[2] =  { "RECOGNITION", "ОПОЗНАВАНИЕ" };

  static char* nMenu000[2] = { "RADAR PICTURE", "РЛ ВИДЕО " };
  static char* nMenu001[2] = { "MEASURES", "ИЗМЕРИТ СРЕДСТВА" };
  static char* nMenu002[2] = { "FCC", "МД" };
  static char* nMenu003[2] = { "PLOT SYMBOLS", "СИМВОЛЫ АРП" };
  static char* nMenu004[2] = { "TAILS", "СЛЕДЫ" };
  static char* nMenu005[2] = { "SCREEN PANEL", "ПАНЕЛЬ" };
  static char* nMenu006[2] = { "CONTROL PANEL", "ПУЛЬТ" };
  static char* nMenu007[2] = { "MAP", "КАРТА" };
  static char* nMenu008[2] = { "DAY/NIGHT", "ДЕНЬ/НОЧЬ" };

  static char* nMenu010[2] = { "CPA  LIMIT nm", "ОПАСН ДКС миль" };
  static char* nMenu011[2] = { "TCPA LIMITmin", "ОПАСН ВКС  мин" };
  static char* nMenu012[2] = { "VECTORS    min", "ВЕКТОРЫ    мин" };
  static char* nMenu013[2] = { "TAILS/PNT min", "СЛЕДЫ/ТЧК  мин" };
  static char* nMenu014[2] = { "NUMBER TARGET", "НОМЕРА ЦЕЛЕЙ" };
  static char* nMenu015[2] = { "AZ DELETE", "СБРОС ЗОНЫ" };
  static char* nMenu016[2] = { "AZ SETTING", "УСТАН ЗОНЫ" };
  static char* nMenu017[2] = { "TRUE VECTOR", "ЛИД" };

  static char* nMenu020[2] = { "TUNE", "РПЧ" };
  static char* nMenu021[2] = { "CHANGE VRM", "СМЕНА ВД" };
  static char* nMenu022[2] = { "SPEED DEVICE", "ДАТЧИК СКОР" };
  static char* nMenu023[2] = { "MAN SPD kts ", "СК  РУЧ  уз " };
  static char* nMenu024[2] = { "STAB SYSTEM", "ДАТЧИК СТАБ" };
  static char* nMenu025[2] = { "TIMER min", "ТАЙМЕР мин" };
  static char* nMenu026[2] = { "LANGUAGE", "ЯЗЫК" };
  static char* nMenu027[2] = { "GYRO ALIGN", "СОГЛАС ГИРО" };
  static char* nMenu028[2] = { "DANGER DPT m","ОПАС ГЛУБ м" };
  static char* nMenu029[2] = { "ANCHORAGE m", "ЯКОРНАЯ СТ м" };
  static char* nMenu02A[2] = { "STAB NOISE", "ШУМ СТАБ" };

  static char* nMenu030[2] = { "STROBE AT", "СТРОБ АС" };
  static char* nMenu031[2] = { "TARGETS SIMUL", "ИМИТАЦИЯ ЦЕЛИ" };
  static char* nMenu032[2] = { "PERFORM MON", "ЭП" };
  static char* nMenu033[2] = { "PEAK POWER", "КИМ" };
  static char* nMenu034[2] = { "MAGNETRON", "ТОК МАГНЕТР" };
  static char* nMenu035[2] = { "DISP TRIG", "ЗАПУСК И" };
  static char* nMenu036[2] = { "RADAR VIDEO", "РЛ ВИДЕО" };
  static char* nMenu037[2] = { "AZ PULSE", "КУА" };
  static char* nMenu038[2] = { "HEAD PULSE", "ОК" };

  static char* nMenu040[2] = { "CALL ROUTE №", "ВЫЗОВ КАРТЫ №" };
  static char* nMenu041[2] = { "TAILS/PNT", "СЛЕДЫ/ТЧК" };
  static char* nMenu042[2] = { "-------------------------", "-------------------------" };
  static char* nMenu043[2] = { "CALL ROUTE №", "ВЫЗОВ КАРТЫ №" };
  static char* nMenu044[2] = { "LINE OF A ROUTE", "ЛИНИЯ МАРШРУТА" };
  static char* nMenu045[2] = { "POINT NUMBER", "НОМЕР ТОЧКИ" };
  static char* nMenu046[2] = { "SYMBOLS", "СИМВОЛЫ" };
  static char* nMenu047[2] = { "RECORDING MAP №", "ЗАПИСЬ КАРТЫ №" };

  static char* nMenu050[2] = { "SIGN", "ПРИЗНАК" };
  static char* nMenu051[2] = { "REQUEST", "ЗАПРОС" };
  static char* nMenu052[2] = { "ABORT EMIS", "ЗАПРЕТ ИЗЛУЧ" };

  // выбор день/ночь
  static char* dayArray[2][2] = { { "NIGHT", "НОЧЬ" }
                                , { "DAY", " ДЕНЬ" } };

  // значения следов/точек для меню
  static char *trackArray[5][2] = { { "1", "1" }
                                  , { "2", "2" }
                                  , { "3", "3" }
                                  , { "6", "6" }
                                  , { "12", "12" } };

  // выбор ОТКЛ/ВКЛ
  static char *OffOnArray[2][2] = { { "OFF", "ОТКЛ" }
                                  , { "ON", "ВКЛ" } };

  // выбор ДА/НЕТ
  static char *YesNoArray[2][2] = { { "YES", "ДА" }
                                  , { "NO", "НЕТ" } };

  // выбор ЛИД для АС
  static char *tvecApArray[2][2] = { { "WATER", "ВОДА" }
                                   , { "GRND", "ГРУНТ" } };

  // значения типа скорости для меню
  static char *speedArray[2][2] = { { "MAN", "РУЧ" }
                                  , { "LOG", "ЛАГ" } };

  // выбор языка системы
  static char *langArray[2][2] = { { "ENGL", "АНГЛ" }
                                 , { "RUS", "РУС" } };

  // выбор датчика стабилизации
  static char *devStabArray[4][2] = { { "ATER", "АОЦ" }
                                    , { "GPS", "СНС" }
                                    , { "DLG", "ДЛГ" }
                                    , { "L-G(W)", "ГК-Л(В)" } };

  // выбор единиц измерения ВД
  static char *vdArray[2][2] = { { "KM", "КМ" }
                               , { "NM", "МИЛИ" } };

  // символы маршрута
  static char *nameSymb[5][2] = { { "╚", "╚" } // буй                  0xc8 в cp866
                                , { "╔", "╔" } // веха                 0xc9 в cp866
                                , { "╩", "╩" } // подводная опасность  0xca в cp866
                                , { "╦", "╦" } // якорная стоянка      0xcb в cp866
                                , { "╠", "╠" } // береговой ориентир   0xcc в cp866
                                };



  // опознавание
  static char *nameRecog[3][2] = { { "OFF", "НЕТ" }
                                 , { "ROUND", "КРУГ" }
                                 , { "SECT", "СЕКТ" } };

  static char *nameSign[3][2] = { { "UNINDENT", "НЕТ" }
                                , { "FRIENDLY", "СВОЙ" }
                                , { "ENEMY", "ЧУЖОЙ" } };
}

#endif // RLISTRINGS_H
