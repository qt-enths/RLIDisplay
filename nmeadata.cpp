#include "nmeadata.h"
#include "string.h"
#include <stdio.h>

extern sTargetList stargs;
extern TargetList targs;


sTargetList::sTargetList(){
    memset(&stargs, 0, sizeof(stargs));
    Cnt = 0;
}

sTarget *sTargetList::sFindMMSI(unsigned int mmsi){

    for (int i = 0; i < Cnt; i++) if (starg[i]&&(starg[i]->mmsi == mmsi)){
        stargList_mtx.unlock();
        return starg[i];
    }

    starg[Cnt] = new sTarget;
    starg[Cnt]->mmsi = mmsi;
    Cnt++;

    return starg[Cnt-1];
}

TargetList::TargetList(){
    memset(&targs, 0, sizeof(targs));
    Cnt = 0;
}

Target *TargetList::FindID(unsigned int ID){

    for (int i = 0; i < Cnt; i++) if (targ[i]&&(targ[i]->ID == ID)){
        targList_mtx.unlock();
        return targ[i];
    }

    targ[Cnt] = new Target;
    targ[Cnt]->ID = ID;
    Cnt++;

    return targ[Cnt-1];
}

Target *TargetList::FindMMSI(unsigned int mmsi){

    for (int i = 0; i < Cnt; i++) if (targ[i]&&(targ[i]->mmsi == mmsi)){
        targList_mtx.unlock();
        return targ[i];
    }

    targ[Cnt] = new Target;
    targ[Cnt]->mmsi = mmsi;
    targ[Cnt]->ID = Cnt;
    Cnt++;

    return targ[Cnt-1];
}

Target::Target(){

    ID = 0;
    mmsi = 0;
    AISTarg = false;
    AISTarg_type = Unck;
    SARPTarg = false;
    Latitude = 91*1000*60;
    Longitude = 181*1000*60;

    Navigational_status = 15;
    COG = 360*10;
    True_heading = 511;
    ROT = -128;
    SOG = 1023;
    Position_accuracy = 0;
    Time_stamp = 80;
    Reserv_for_regional_apps = 0;
    Spare1 = 0;
    RAIM_flag = 0;
//    unsigned int Communication_state;

    AIS_version_indicator = 0;
    IMO_number = 0;
    memset(Call_sign, '@', 7); Call_sign[7] = 0;
    memset(Name, '@', 20); Call_sign[20] = 0;
    Ship_cargo_type = 0;
//    Dim_ref_for_position = 0;

     Dim_ref_for_position_A = 0;
     Dim_ref_for_position_B = 0;
     Dim_ref_for_position_C = 0;
     Dim_ref_for_position_D = 0;

    Type_of_electronic_position_fixing_dev = 0;
//    unsigned char ETA;
    ETA_month = 0;
    ETA_day = 0;
    ETA_hour = 24;
    ETA_minute = 60;
    Max_present_static_draught = 0;
    memset(Destination, '@', 20); Destination[20] = 0;
    DTE = 0;

    Type_of_aids_to_navigation = 0;
    Off_position_indicator = 0;

    Flags = 0;
}

void Target::PrintOut(){
    if (AISTarg) printf("Target type: AIS\n");

    printf("ID: %d\n", ID);
    printf("User ID: %u\n", mmsi);
    printf("Navigational Status: ");
    if (Navigational_status_F) printf("%d\n", Navigational_status);
            else printf("not defined\n");
    printf("ROT: ");
    if (ROT_F) printf("%f\n", ROT);
            else printf("not available\n");
    printf("SOG: ");
    if (SOG_F) printf("%f\n", SOG);
            else printf("not available\n");
    printf("Position accuracy: %d\n", Position_accuracy);
    printf("Longitude: ");
    if (Longitude_F) printf("%.20f\n", (float)Longitude/10000);
        else printf("not available\n");
    printf("Latitude: ");
    if (Latitude_F) printf("%.20f\n", (float)Latitude/10000);
        else printf("not available\n");
    printf("COG: ");
    if (COG_F) printf("%f\n", COG);
            else printf("not available\n");
    printf("True heading: ");
    if (True_heading_F) printf("%f\n", True_heading);
            else printf("not available\n");
    printf("Time_stamp: ");
    if (Time_stamp_F) printf("%d\n", Time_stamp);
            else printf("not available\n");
    printf("Reserve for regional apps: %d\n", Reserv_for_regional_apps);
//    printf("Spare: %d\n", trgt->Spare1);
    printf("Raim Flag: %d\n", RAIM_flag);
//    printf("Communication_State: %d\n", trgt->Communication_state);
    printf("AIS version indicator: %d\n", AIS_version_indicator);
    printf("IMO number: ");
    if (IMO_number_F) printf("%d\n", IMO_number);
            else printf("not available\n");
    printf("Call sign: ");
    if (Call_sign_F) printf("%s\n", Call_sign);
            else printf("not available\n");
    printf("Name: ");
    if (Name_F) printf("%s\n", Name);
            else printf("not available\n");
    printf("Type of ship and cargo type: ");
    if (Ship_cargo_type_F != 0) printf("%d\n", Ship_cargo_type);
            else printf("not available\n");
    printf("Ref point: ");
    if (Ref_point_F) printf("A: %d; C: %d\n", Dim_ref_for_position_A, Dim_ref_for_position_C);
        else printf("not available\n");
    printf("Dimensions: ");
    if (Dimensions_F) printf("B: %d; D: %d\n", Dim_ref_for_position_B, Dim_ref_for_position_D);
        else printf("not available\n");
    printf("Type of electronic position fixing device: ");
    if (Type_of_electronic_position_fixing_dev_F) printf("%d\n", Type_of_electronic_position_fixing_dev);
        else printf("not available\n");
    printf("ETA month: ");
    if (ETA_month_F) printf("%d\n", ETA_month);
        else printf("not available\n");
    printf("ETA day: ");
    if (ETA_month_F) printf("%d\n", ETA_day);
        else printf("not available\n");
    printf("ETA hour: ");
    if (ETA_hour_F) printf("%d\n", ETA_hour);
        else printf("not available\n");
    printf("ETA minute: ");
    if (ETA_minute_F) printf("%d\n", ETA_minute);
        else printf("not available\n");
    printf("Max present static draught: ");
    if (Max_present_static_draught_F) printf("%d\n", Max_present_static_draught);
        else printf("not available\n");
    printf("Destination: ");
    if (Destination_F) printf("%s\n", Destination);
            else printf("not available\n");

}
