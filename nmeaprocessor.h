#ifndef NMEAPROCESSOR_H
#define NMEAPROCESSOR_H

#ifndef Q_OS_WIN

#include <QObject>
#include <sys/select.h>
#include <QWaitCondition>
#include "nmeadata.h"
#include "targetengine.h"


#if QT_VERSION >= 0x050000
    #include <QtConcurrent/QtConcurrentRun>
#else
    #include <QtConcurrentRun>
#endif

const int bufLen = 90;
const int strLen = 81;
const int parsNum = 6;
const int portNum = 16;
const int multiSentStrLen = 256;
const int targetN = 100;


enum status{
    SRCH_D,
    SRCH_CR,
    SRCH_LF,
};

enum parsType{
    UNK,
    GP,
    FIRST_PARSER_TYPE = GP,
    GL,
    GN,
    HE,
    HC,
    HN,
    LC,
    VD,
    VM,
    VW,
    SD,
    AI,
    ZA,
    ZC,
    ZQ,
    ZV,
    LAST_PARSER_TYPE = ZV
};

typedef struct{
    struct tm DateTime;
    int ss;
//    char Time[10];
    char Status;
    float Latitude;
    char LaNS;
    float Longitude;
    char LoEW;
    float GrndSpeed;
    float DegrTrue;
//    char Date[7];
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

}RMC;

typedef struct{
    char Status;
    float CrossTrackErr;
    char DirSteer_LR;
    char To_WayPointID[4];
    char From_WayPointID[4];
    float Dest_WayPoint_Latitude;
    char Dest_WayPoint_LaNS;
    float Dest_WayPoint_Longitude;
    char Dest_WayPoint_LoEW;
    float RangeToDest;
    float BearingToDest_DegrTrue;
    float Dest_ClosingVelociti;
    char Arrival_Status;

    union {
        unsigned int Flags;
        struct {
        unsigned int Status_F                   :1;
        unsigned int CrossTrackErr_F            :1;
        unsigned int DirSteer_LR_F              :1;
        unsigned int To_WayPointID_F            :1;
        unsigned int From_WayPointID_F          :1;
        unsigned int Dest_WayPoint_Latitude_F   :1;
        unsigned int Dest_WayPoint_LaNS_F       :1;
        unsigned int Dest_WayPoint_Longitude_F  :1;
        unsigned int Dest_WayPoint_LoEW_F       :1;
        unsigned int RangeToDest_F              :1;
        unsigned int BearingToDest_DegrTrue_F   :1;
        unsigned int Dest_ClosingVelociti_F     :1;
        unsigned int Arrival_Status_F           :1;
        };
    };
}RMB;

typedef struct{
    float DegrTrue;
    char T;
    float DegrMagn;
    char M;
    float Knots;
    char N;
    float Kilometers;
    char K;

    union{
        unsigned int Flags;
        struct {
            unsigned int DegrTrue_F             :1;
            unsigned int T_F                    :1;
            unsigned int DegrMagn_F             :1;
            unsigned int M_F                    :1;
            unsigned int Knots_F                :1;
            unsigned int N_F                    :1;
            unsigned int Kilometers_F           :1;
            unsigned int K_F                    :1;
        };
    };
}VHW;

typedef struct {
    float HeadDegr;
    char T;

    union {
        unsigned int Flags;
        struct{
            unsigned int HeadDegr_F             :1;
            unsigned int T_F                    :1;
        };
    };

}HDT;


typedef struct {
    float MagnSensor;
    float MagnDeviation;
    char MagnDivDir;
    float MagnVarDegrs;
    char MagnVarDir;

    union {
        unsigned int Flags;
        struct{
            unsigned int MagnSensor_F             :1;
            unsigned int MagnDeviation_F          :1;
            unsigned int MagnDivDir_F             :1;
            unsigned int MagnVarDegrs_F           :1;
            unsigned int MagnVarDir_F             :1;
        };
    };

}HDG;


typedef struct {
    float Depth;
    float OffsetFromTransducer;

    union {
        unsigned int Flags;
        struct{
            unsigned int Depth_F             :1;
            unsigned int OffsetFromTransducer_F          :1;
        };
    };

}DPT;

typedef struct {
unsigned char Message_ID;
unsigned char Repeat_Indicator;
unsigned int User_ID;
}encStr_common;


typedef struct {
    unsigned char Navigational_Status;
    int ROT;
    int SOG;
    unsigned char Position_accuracy;
    int Longitude;
    int Latitude;
    unsigned int COG;
    unsigned int True_Heading;
    unsigned char Time_stamp;
    unsigned char Reserve_for_regional_apps;
    unsigned char Spare;
    unsigned char RAIM_Flag;
    unsigned int Communication_State;
}encStr_123;

typedef struct {
    unsigned short UTC_year;
    unsigned char UTC_month;
    unsigned char UTC_day;
    unsigned char UTC_hour;
    unsigned char UTC_minute;
    unsigned char UTC_second;
    unsigned char Position_accuracy;
    unsigned int Longitude;
    unsigned int Latitude;
    unsigned char Type_of_pos_fixing_device;
    unsigned char RAIM_Flag;
    unsigned int Communication_State;
}encStr_411;

typedef struct {
    unsigned char AIS_version_indicator;
    unsigned int IMO_number;
    char Call_sign[8];
    char Name[21];
    unsigned char Type_of_ship_and_cargo;
    unsigned int Dim_ref_for_position;
    unsigned char Type_of_pos_fixing_device;
    unsigned int ETA;
    unsigned char Max_static_draught;
    char Destination[21];
    unsigned char DTE;
}encStr_5;

typedef struct {
    unsigned char Sequence_number;
    unsigned int Destination_ID;
    unsigned char Retransmit_flag;
    unsigned short Applicatoin_identifier_DAC;
    unsigned char Applicatoin_identifier_function_ID;
    unsigned char Binary_data[115];
}encStr_6;

typedef struct {
    unsigned int Longitude;
    unsigned int Latitude;
    unsigned char Message_type;
    unsigned short Station_ID;
    unsigned short Z_count;
    unsigned char Sequence_number;
    unsigned char N;
    unsigned char Health;
    unsigned int DGNSS_data_words[29];
}encStr_17;

typedef struct {
    unsigned char Reserve_for_reg_loc_apps;
    unsigned short SOG;
    unsigned char Position_accuracy;
    unsigned int Longitude;
    unsigned int Latitude;
    unsigned short COG;
    unsigned short True_Heading;
    unsigned char Time_stamp;
    unsigned char Reserve_for_regional_apps;
    unsigned char RAIM_Flag;
    unsigned char Comm_state_selector_flag;
    unsigned int Communication_State;
}encStr_18;

typedef struct {
    unsigned short Offset_number_1;
    unsigned char Number_of_slots_1;
    unsigned short Time_out_1;
    unsigned short Increment_1;
    unsigned short Offset_number_2;
    unsigned char Number_of_slots_2;
    unsigned short Time_out_2;
    unsigned short Increment_2;
    unsigned short Offset_number_3;
    unsigned char Number_of_slots_3;
    unsigned short Time_out_3;
    unsigned short Increment_3;
    unsigned short Offset_number_4;
    unsigned char Number_of_slots_4;
    unsigned short Time_out_4;
    unsigned short Increment_4;
}encStr_20;

typedef struct {
    unsigned char Types_of_aids_to_navigation;
    char Name_of_aids_to_navigation[21];
    unsigned char Position_accuracy;
    unsigned int Longitude;
    unsigned int Latitude;
    unsigned int Dim_ref_for_position;
    unsigned char Type_of_pos_fixing_device;
    unsigned char Time_stamp;
    unsigned char Off_position_indicator;
    unsigned char Reserve_for_regional_apps;
    unsigned char RAIM_Flag;
}encStr_21;

typedef struct {


}encStr_24;


typedef struct {
    int totN, sentenceN, SN;
    char AIS_Channel;
    union {
        unsigned int Flags;
        struct{
            unsigned int AIS_Channel_F             :1;
        };
    };
    encStr_common eS_common;
    union {
    encStr_123 eS123;
    encStr_411 eS411;
    encStr_5 eS5;
    encStr_6 eS6;
    encStr_17 eS17;
    encStr_18 eS18;
    encStr_20 eS20;
    encStr_21 eS21;
    };

}VDM;

typedef struct {
    int totN, sentenceN, SN;
    char AIS_Channel;
    int EOS;
}VDM_mult;

//VDM *vdms[10];

class RingBuf;
class NMEAProcessor;

class Parser{
public:
    Parser(NMEAProcessor * prc = NULL);
    virtual ~Parser() {};
    NMEAProcessor *_prc;
    char str[strLen];
    bool strBusy;
    unsigned char eos;
//    char strs[parsStrN][strLen];
    int readStr, writeStr;
    int currComaPos, predComaPos;
    timespec *timeStamp;
//    bool strReady;
    bool structReady;
    parsType type;
    unsigned int Flag;
    RingBuf *RB;
    virtual void Pars();
    bool Checksum();
    void ParsEnd();
    int FindComa(int Pos);
    int ComaEOLCheck(int p);
    int GetC(unsigned int *field_F, void *field);
    int GetS(unsigned int *field_F, void *field);
    int GetI(unsigned int *field_F, void *field);
    int GetF(unsigned int *field_F, void *field);
};

class GP_Parser: public Parser{
public:
    GP_Parser();
    virtual void Pars();
    void printRMB();
    void printRMC(sTarget *currTarg);

};

class HE_Parser: public Parser{
public:
    HE_Parser();
    virtual void Pars();
    void printHDT();

};

class HC_Parser: public Parser{
public:
    HC_Parser();
    virtual void Pars();
    void printHDG();

};

class VW_Parser: public Parser{
public:
    VW_Parser();
    virtual void Pars();
    void printVHW();

};

class SD_Parser: public Parser{
public:
    SD_Parser();
    virtual void Pars();
    void printDPT();

};

class AI_Parser: public Parser{
public:

    unsigned char bitStr[53];
    char multiSentStrArr[targetN][multiSentStrLen];
    VDM_mult *tot_num_chan[targetN];
    int pos, bitPos;
    int startByte;
    int startBit;

    unsigned char byte_to_6bit(unsigned char byte);
    AI_Parser();

    int uint_to_int(unsigned int x, int bitSize);
    char str6tochar(int bitSize);
    short str6toshort(int bitSize);
    int str6toint(int bitSize);

    unsigned char str6to_uchar(int bitSize);
    unsigned short str6to_ushort(int bitSize);
    unsigned int str6to_uint(int bitSize);

    void Str8toStr6(char *str);
    void Str6toStr8(int bitSize, char *str8);
    virtual void Pars();

    void sS_123(sTarget *strgt);
    void sprintVDM_123(sTarget *strgt);

    void S_123(Target *trgt);
//    void printVDM_123(Target *trgt);

    void sS_411();
    void sprintVDM_411();

    void S_411(Target *targ);

    void sS_5(sTarget *strgt);
    void sprintVDM_5(sTarget *strgt);

    void S_5(Target *targ);
//    void printVDM_5(Target *trgt);

    void S_6();
    void printVDM_6();
    void S_17();
    void printVDM_17();
    void sS_18();
    void S_18(Target *targ);
    void sprintVDM_18();
    void S_19(Target *targ);
    void S_20();
    void printVDM_20();
    void sS_21();
    void sprintVDM_21();
    void S_21(Target *targ);

};

class RingBuf{
public:
    RingBuf();
    unsigned short r;
    unsigned short w;
    char b[bufLen];
    int fd;
//    bool strBusy;
    status stat;
    short Dpos;
    short CRpos;
    short LFpos;
    void Act();
    bool isFull();
    bool isEmpty();
    int GetNext(int p);
    int GetPred(int p);
    int Find_D(int p);
    int Find(char c, int p);
    parsType getParsType(char s0, char s1);
    void WriteStr(Parser *prs);
    void getStr();
    struct timespec timeStamp;
};

void Error(char *s, int err);

class NMEAProcessor : public QObject
{
    Q_OBJECT
public:
    explicit NMEAProcessor(QObject *parent = 0);
    ~NMEAProcessor();



    QFuture<void> readPortsThread;
    QFuture<void> parsersThread;

    void open_fds(QStringList & ports);
    void initParsers();
    void targetChanged(unsigned int mmsi);
    int start(void);

signals:
    void updateTarget(QString tag, RadarTarget target);

public slots:

private:
    void readPorts(void);
    void readPortBufs(void);
    void transmit(void);
    int getMaxFd(void);

    bool finish_flag;

    int maxfd;

};
#endif // !Q_OS_WIN

#endif // !NMEAPROCESSOR_H
