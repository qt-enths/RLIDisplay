#ifndef NMEADATA_H
#define NMEADATA_H

#include <QObject>
#include <qmutex.h>
#include <time.h>
typedef struct {

    unsigned int mmsi;

    //--------- RMC -------
    struct tm DateTime;
    int ss;
    char Status;
    float Latitude;
    char LaNS;
    float Longitude;
    char LoEW;
    float GrndSpeed;
    float DegrTrue;
    float MagnVar;
    char MV_EW;

    union {
        unsigned int Flags;
        struct {
            unsigned int Time_F            :1;
            unsigned int Status_F          :1;
            unsigned int Latitude_F        :1;
            unsigned int LaNS_F            :1;
            unsigned int Longitude_F       :1;
            unsigned int LoEW_F            :1;
            unsigned int GrndSpeed_F       :1;
            unsigned int DegrTrue_F        :1;
            unsigned int Date_F            :1;
            unsigned int MagnVar_F         :1;
            unsigned int MV_EW_F           :1;

        };
    };

    //--------- 1, 2, 3 -------
    unsigned char Navigational_status;
    float ROT;
    float SOG;
    unsigned char Position_accuracy;
    float COG;
    float True_heading;
    unsigned char Reserv_for_regional_apps;
    unsigned char Time_stamp;
    unsigned char Spare1;
    unsigned char RAIM_flag;
    unsigned int Communication_state;
    //---------- 5 ------------
    unsigned char AIS_version_indicator;
    unsigned int IMO_number;
    char *Call_sign[7];
    char *Name[20];
    unsigned char Ship_cargo_type;
    unsigned short Dim_ref_for_position;
    //unsigned short Dim_ref_for_position_A;
    //unsigned short Dim_ref_for_position_B;
    //unsigned char Dim_ref_for_position_C;
    //unsigned char Dim_ref_for_position_D;
    unsigned char Electronic_position_fixing_dev_type;
    unsigned int ETA;
//    unsigned char ETA_month, ETA_day, ETA_hour, ETA_minute;
    unsigned char Max_present_static_draught;
    char *Destination[20];
    unsigned char DTE;
    unsigned char Spare2;
}sTarget;

enum AISTarget_type {
    Unck,
    Ship_A,
    Ship_B,
    Base_station,
    SAR,
    Aid_to_navigation_fixed,
    Aid_to_navigation_float
};

class Target{
public:
    unsigned int ID;
    unsigned int mmsi;
    bool AISTarg;
    AISTarget_type AISTarg_type;
    bool SARPTarg;
    float Latitude;
    float Longitude;

    unsigned char Navigational_status;
    float COG;
    float True_heading;
    float ROT;
    float SOG;
    unsigned char Position_accuracy;
    unsigned char Time_stamp;
    unsigned char Reserv_for_regional_apps;
    unsigned char Spare1;
    unsigned char RAIM_flag;
//    unsigned int Communication_state;

    unsigned char AIS_version_indicator;
    unsigned int IMO_number;
    char Call_sign[8];
    char Name[21];
    unsigned char Ship_cargo_type;
//    unsigned short Dim_ref_for_position;

    unsigned short Dim_ref_for_position_A;
    unsigned short Dim_ref_for_position_B;
    unsigned char Dim_ref_for_position_C;
    unsigned char Dim_ref_for_position_D;

    unsigned char Type_of_electronic_position_fixing_dev;
//    unsigned char ETA;
    unsigned char ETA_month, ETA_day, ETA_hour, ETA_minute;
    unsigned char Max_present_static_draught;
    char Destination[21];
    unsigned char DTE;

    unsigned char Type_of_aids_to_navigation;
    unsigned char Off_position_indicator;


    union {
        unsigned int Flags;
        struct {
            unsigned int Latitude_F                         :1;
            unsigned int Longitude_F                        :1;
            unsigned int COG_F                              :1;
            unsigned int True_heading_F                     :1;
            unsigned int Navigational_status_F              :1;
            unsigned int ROT_F                              :1;
            unsigned int SOG_F                              :1;
            unsigned int Time_stamp_F                       :1;
            unsigned int IMO_number_F                       :1;
            unsigned int Call_sign_F                        :1;
            unsigned int Name_F                             :1;
            unsigned int Ship_cargo_type_F                  :1;
            unsigned int Ref_point_F                        :1;
            unsigned int Dimensions_F                       :1;
            unsigned int Type_of_electronic_position_fixing_dev_F :1;
            unsigned int ETA_month_F                        :1;
            unsigned int ETA_day_F                          :1;
            unsigned int ETA_hour_F                         :1;
            unsigned int ETA_minute_F                       :1;
            unsigned int Max_present_static_draught_F       :1;
            unsigned int Destination_F                      :1;
            unsigned int Type_of_aids_to_navigation_F       :1;
        };
    };

    Target();

    void PrintOut();
};


class sTargetList : public QObject{
  Q_OBJECT
public:
    int Cnt;
    sTarget *starg[101];
    QMutex stargList_mtx;
    explicit sTargetList();
//private:
    sTarget *sFindMMSI(unsigned int mmsi);
};


extern sTargetList * pstargs;

class TargetList : public QObject{
  Q_OBJECT
public:
    int Cnt;
    Target *targ[101];
    QMutex targList_mtx;
    TargetList();
//private:
    Target *FindMMSI(unsigned int mmsi);
    Target *FindID(unsigned int ID);
};


extern TargetList * ptargs;

#endif // NMEADATA_H
