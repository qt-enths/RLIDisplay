#include <QThread>

#ifndef Q_OS_WIN

#include "nmeaprocessor.h"
#include "nmeadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <termios.h>
#include <arpa/inet.h>


using namespace std;

char ASCII6[] =
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[|]^- !_#$%&'()*+,-./0123456789:;<=>?";

char typeTable[LAST_PARSER_TYPE*2+1] = "GPGLGNHEHCHNLCVDVMVWSDAIZAZCZQZV";

const char *typeTableStrs[LAST_PARSER_TYPE+1] = {
    "GP", "GL", "GN", "HE", "HC", "HN", "LC", "VD", "VM", "VW", "SD", "AI",
    "ZA", "ZC", "ZQ", "ZV"
};

sTargetList * pstargs = NULL;
TargetList  * ptargs  = NULL;

RMC rmc;
RMB rmb;
VHW vhw;
HDT hdt;
HDG hdg;
DPT dpt;
VDM vdm;

fd_set readbufs;

RingBuf bufs[portNum];

Parser *parsers[parsNum];

QMutex errno_mtx;
QMutex port_pars_mtx;
//QMutex pars_port_mtx;
//QMutex pars_main_mtx;

QWaitCondition string_ready;
//QWaitCondition string_needed;

/*
pthread_mutex_t errno_mtx;
pthread_mutex_t port_pars_mtx;
pthread_mutex_t pars_port_mtx;
pthread_mutex_t pars_main_mtx;

pthread_cond_t string_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t string_needed = PTHREAD_COND_INITIALIZER;
*/

//sTargetList strgs;


extern void qSleep(int ms);
int fd_rs[portNum];

static int openport(const char * portname){

    int fd;

    //fd = ::open(portname, O_NONBLOCK|O_RDONLY|O_NOCTTY);
    fd = ::open(portname, O_NONBLOCK | O_RDWR | O_NOCTTY);
    if(fd == -1)
    {
        int nerr = errno;
        fprintf(stderr, "Failed to open serial port %s for NMEA-0183: %s\n", portname, strerror(nerr));
        return 1;
    }

    fcntl(fd, F_SETFL, 0);
    struct termios options;

    /*
     * Получение текущих опций для порта...
     */

    tcgetattr(fd, &options);

    /*
     * Установка скорости передачи в 4800...
     */

//    cfsetispeed(&options, B38400);
    cfsetispeed(&options, B4800);
//    cfsetospeed(&options, B115200);

    /*
     * Разрешение приемника и установка локального режима...
     */

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_iflag &= ~(INLCR | ICRNL |
                         IXOFF | IXON);
/*
    if(options.c_iflag & INLCR)
    {
        options.c_iflag &= ~INLCR;
        printf("INLCR was set\n");
    }
    if(options.c_iflag & ICRNL)
    {
        options.c_iflag &= ~ICRNL;
        printf("ICRNL was set\n");
    }

    if(options.c_iflag & IXOFF)
    {
        options.c_iflag &= ~IXOFF;
        printf("IXOFF was set\n");
    }
    if(options.c_iflag & IXON)
    {
        options.c_iflag &= ~IXON;
        printf("IXON was set\n");
    }
*/
    options.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL |
                         ECHOPRT | ECHOKE | ICANON );

    /*
     * Установка новых опций для порта...
     */

    tcsetattr(fd, TCSANOW, &options);

	write(fd, "Hello\r\n", 7); // Just to apply new settings (driver's bug fix)

    return fd;
}

NMEAProcessor::NMEAProcessor(QObject *parent) :
    QObject(parent)
{
	pstargs = new sTargetList;
	ptargs  = new TargetList;
    FD_ZERO(&readbufs);
    maxfd = -1;
    for(int i = 0; i < portNum; i++)
        fd_rs[i] = -1;
    memset(parsers, 0, sizeof(parsers));
    initParsers();
    finish_flag = false;
}

NMEAProcessor::~NMEAProcessor()
{
//    printf("\nDestroying !!!!!!!!!!!!!!!\n");
  finish_flag = true;
  while(readPortsThread.isRunning());
  string_ready.wakeAll();
  while(parsersThread.isRunning());
  for(int i = 0; i < portNum; i++)
  {
      if(fd_rs[i] != -1)
          ::close(fd_rs[i]);
  }

  for(int i = 0; i < parsNum; i++)
  {
      if(parsers[i] != NULL)
      {
          delete parsers[i];
          parsers[i] = NULL;
      }
  }
}


void NMEAProcessor::readPorts(void)
{
    struct timeval tv;
    fd_set readset;

    fprintf(stderr, "\nreadPorts thread has started\n");

    while(!finish_flag)
    {
//        printf("\nreadPorts !!!!!!!!!!!\n");
        tv.tv_sec   = 0;
        tv.tv_usec  = 500000;
        readset = readbufs;
        int r = ::select(maxfd + 1, &readset, NULL, NULL, &tv);
        if (r == 0) continue;
        if (r == -1)
            printf("Error selecting input port -- %s", strerror(errno));


        for (int i = 0; i < portNum; i++){
            if (FD_ISSET(fd_rs[i], &readbufs)){
                if (!bufs[i].isFull()) bufs[i].Act();
            }
        }
    }
    fprintf(stderr, "\nreadPorts thread has finished\n");
}

void NMEAProcessor::readPortBufs(void)
{

    bool emp = true;



    fprintf(stderr, "\nreadPortBufs thread has started\n");

    while (!finish_flag){
        if (emp){

            port_pars_mtx.lock();
            string_ready.wait(&port_pars_mtx);
            port_pars_mtx.unlock();
        }
        emp = true;
        for (int i = 0; i < portNum; i++){
            if (bufs[i].isEmpty()){
                continue;
            }
            else{
                emp = false;
                bufs[i].getStr();
            }
        }

    }
    fprintf(stderr, "\nreadPortBufs thread has finished\n");
}

void NMEAProcessor::targetChanged(unsigned int mmsi)
{
    RadarTarget trg;
    Target * ptrg;

	fprintf(stderr, "%s entered\n", __func__);

    ptrg = ptargs->FindMMSI(mmsi);
    if(!ptrg)
        return;
	fprintf(stderr, "%s: Target found\n", __func__);

    trg.CourseOverGround = ptrg->COG / 10;
    trg.SpeedOverGround  = ptrg->SOG / 10;
    trg.Latitude         = ptrg->Latitude / (10000 * 60);
    trg.Longtitude       = ptrg->Longitude / (10000 * 60);
    trg.Rotation         = ptrg->ROT;
    trg.Lost             = false;
    trg.Heading          = ptrg->True_heading;

    QString tag;
    tag.sprintf("AIS_%d", mmsi);

	fprintf(stderr, "%s: about to emit updateTarget\n", __func__);

    emit updateTarget(tag, trg);

	fprintf(stderr, "%s: updateTarget emitted\n", __func__);
}

void transmit(void){

}


unsigned char maskTab1[8] = {0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7e, 0xfc};
unsigned char maskTab2[5] = {0xf8, 0xf0, 0xe0, 0xc0, 0x80};




void Error(const char *s, int err){
    printf("Error! %s %s\n", s, strerror(err));
}

void SintError(const char *s, char *s1){
    printf("%s expected, but %s found\n", s, s1);
}

void printb(char *b){
    for (int l = 0; l < bufLen; l++)
        if (b[l] == 0)
            printf("%c", 219);
        else if (b[l] == '\r') {
            printf("%c", 220);
        }
        else if (b[l] == '\n') {
            printf("%c", 221);
        }
        else
            printf("%c", b[l]);
    printf("\n");
}

AI_Parser::AI_Parser(){
    Parser();
    memset(tot_num_chan, 0, sizeof(tot_num_chan));
    type = AI;
}



char AI_Parser::str6tochar(int bitSize){
    unsigned short r;
    unsigned char rr[2];
    int B = startByte, b = startBit;

    startByte = B+(b+bitSize)/8;
    startBit = (b+bitSize)%8;

    rr[0] = bitStr[B+1];
    rr[1] = bitStr[B];

    r = *(unsigned short*)rr;

    r <<= b;
    r >>= 16-bitSize;

    r = uint_to_int(r, bitSize);

    return r;
}

short AI_Parser::str6toshort(int bitSize){
    unsigned int r;
    unsigned char rr[4];
    int B = startByte, b = startBit;

    startByte = B+(b+bitSize)/8;
    startBit = (b+bitSize)%8;

    rr[2] = bitStr[B+1];
    rr[3] = bitStr[B];

    r = *(unsigned int*)rr;

    r <<= b;
    r >>= 32-bitSize;

    r = uint_to_int(r, bitSize);

    return r;
}

int AI_Parser::str6toint(int bitSize){
    unsigned long long r;
    unsigned char rr[8];
    int B = startByte, b = startBit;

    startByte = B+(b+bitSize)/8;
    startBit = (b+bitSize)%8;


    rr[4] = bitStr[B+3];
    rr[5] = bitStr[B+2];
    rr[6] = bitStr[B+1];
    rr[7] = bitStr[B];

    r = *(long long *)rr;

    r <<= b;
    r >>= 64-bitSize;

    r = uint_to_int(r, bitSize);

    return r;
}

unsigned char AI_Parser::str6to_uchar(int bitSize){
    unsigned short r;
    unsigned char rr[2];
    int B = startByte, b = startBit;

    startByte = B+(b+bitSize)/8;
    startBit = (b+bitSize)%8;

    rr[0] = bitStr[B+1];
    rr[1] = bitStr[B];

    r = *(unsigned short*)rr;

    r <<= b;
    r >>= 16-bitSize;
    return r;
}

unsigned short AI_Parser::str6to_ushort(int bitSize){
    unsigned int r;
    unsigned char rr[4];
    int B = startByte, b = startBit;

    startByte = B+(b+bitSize)/8;
    startBit = (b+bitSize)%8;

    rr[2] = bitStr[B+1];
    rr[3] = bitStr[B];

    r = *(unsigned int*)rr;

    r <<= b;
    r >>= 32-bitSize;
    return r;
}

unsigned int AI_Parser::str6to_uint(int bitSize){
    unsigned long long r;
    unsigned char rr[8];
    int B = startByte, b = startBit;

    startByte = B+(b+bitSize)/8;
    startBit = (b+bitSize)%8;


    rr[4] = bitStr[B+3];
    rr[5] = bitStr[B+2];
    rr[6] = bitStr[B+1];
    rr[7] = bitStr[B];

    r = *(long long *)rr;

    r <<= b;
    r >>= 64-bitSize;
    return r;
}

int AI_Parser::uint_to_int(unsigned int x, int bitSize){
    unsigned int M;
    M = 1 << (bitSize-1);
    if (x & M)
        for (int i = bitSize; i < 32; i++){
            M <<= 1;
            x |= M;
        }
    return x;
}


void AI_Parser::Str6toStr8(int bitSize, char *str8){
    unsigned short r;
    unsigned char rr[2];
    int last;

    for (int i = 0; i < bitSize/6; i++){
        int B = startByte, b = startBit;

        startByte = B+(b+6)/8;
        startBit = (b+6)%8;

        rr[0] = bitStr[B+1];
        rr[1] = bitStr[B];

        r = *(unsigned short*)rr;

        r <<= b;
        r >>= 10;

        str8[i] = ASCII6[r];
        last = i;

/*
        if (r < 0x28)
            str8[i] = r + 0x30;
        else
            str8[i] = r + 0x38;
*/
    }
    str8[last+1] = 0;
}

RingBuf::RingBuf(){
    r = bufLen-1;
    w = 0;
    Dpos = -1;
    CRpos = -1;
    LFpos = -1;
    stat = SRCH_D;
    memset(b, 0, bufLen);
}

int RingBuf::GetNext(int p){
    return (p+1)%bufLen;
}

int RingBuf::GetPred(int p){
    if (p == 0) return bufLen-1;
    else return p-1;
}

int iiii = 0;

void RingBuf::getStr(){

//    printf("^^^^^^^^^getStr");
    while (true){
        switch (stat) {
        case SRCH_D:
            Dpos = Find_D(GetNext(r));
            if (Dpos == -1){
                r = GetPred(w);
                return;
            }
            stat = SRCH_CR;
            continue;
        case SRCH_CR:
            if (isEmpty()) return;
            CRpos = Find('\r', GetNext(Dpos));
            if (CRpos != -1){
                stat = SRCH_LF;
                continue;
            }
            else return;

        case SRCH_LF:
            int np;
            np = GetNext(CRpos);

            if (np == w) return;

//            printf("np: %d, w: %d\n", np, w);

            if (b[np] == '\n'){

//                printf("Dpos: %d; CRpos: %d; LFpos %d\n", Dpos, CRpos, LFpos);

                LFpos = np;

//                printf("r: %d, w: %d\n", r, w);
//                printf("Dpos: %d; CRpos: %d; LFpos %d\n", Dpos, CRpos, LFpos);
//                printb(b);
                char s0 = b[GetNext(Dpos)];
                char s1 = b[GetNext(GetNext(Dpos))];
                parsType t = getParsType(s0, s1);
                if (t == UNK) {
                    r = LFpos;
                    printf("Unknown parser type %c%c\n", s0, s1);
//                    exit(1);
                    //return;
                }
                else {
                    WriteStr(parsers[t-1]);
                    r = LFpos;
                    parsers[t-1]->strBusy = true;
                    if (!(parsers[t-1]->Checksum())){
                        printf("Sentence with invalid checksum\n");
                        parsers[t-1]->str[0] = 0;
                        parsers[t-1]->strBusy = false;
                        return;
                    }
                    if (clock_gettime(CLOCK_MONOTONIC, &timeStamp) != 0){
                        int err = errno;
                        Error("Failed to get time stamp.", err);
                        return;
                    }
                    parsers[t-1]->timeStamp = &timeStamp;
                    parsers[t-1]->Pars();

                }
            }
            else{
                printf("Error: no <LF> after <CR>\n");
                r = LFpos;
                return;
            }

            stat = SRCH_D;

            return;

        }
    }
}


void RingBuf::Act(){
	ssize_t rd;
    int ba, res;

//	fprintf(stderr, "%s entered\n", __func__);
    if (r < w){
//		fprintf(stderr, "%s:1 reading %d bytes from serial (bufLen %d, w %d)\n", __func__, bufLen-w-1, bufLen, w);
/*
	for (int i = 0; i == buflen-w-1; i++){
            rd = read(fd, &b[w++], 1);
	    if (rd == -1) break;
	}	    
	if (i == 1)    
*/
		res = ioctl(fd, FIONREAD, &ba);
//		fprintf(stderr, "%s: res %d, ba %d\n", __func__, res, ba);
		if (ba == 0) return;
        rd = read(fd, &b[w], bufLen-w-1);
		if(rd == -1)
		{
//			fprintf(stderr, "%s:2 read() error: %s\n", __func__, strerror(errno));
			return;
		}
		w += rd;
//		W += i;
//		fprintf(stderr, "%s:3 bytes read. (w %d)\n", __func__, w);

        if (w == bufLen-1 && !isFull()){
//			fprintf(stderr, "%s:4 reading 1 byte\n", __func__);
			res = ioctl(fd, FIONREAD, &ba);
//			fprintf(stderr, "%s: res %d, ba %d\n", __func__, res, ba);
			if (ba == 0) return;
            res = read(fd, &b[w], 1);
            if(res == -1)
			{
//				fprintf(stderr, "%s:5 1 byte read() error: %s\n", __func__, strerror(errno));
				return;
            }
            w = res;
//			fprintf(stderr, "%s:6 1 byte read\n", __func__);
            w = 0;
            if (r != 1)
			{
//				fprintf(stderr, "%s:7 reading %d bytes (r %d)\n", __func__, r - 1, r);
				res = ioctl(fd, FIONREAD, &ba);
//				fprintf(stderr, "%s: res %d, ba %d\n", __func__, res, ba);
				if (ba == 0) return;
                res = read(fd, &b[w], r-1);
                if(res == -1)
				{
//					fprintf(stderr, "%s:8 1 byte read() error: %s\n", __func__, strerror(errno));
					return;
				}
                w = res;
//				fprintf(stderr, "%s:9 %d - 1 bytes read\n", __func__, r);
			}
        }
    }
    else{
//		fprintf(stderr, "%s:10 reading %d bytes (r %d, w %d)\n", __func__, r-w-1, r, w);
		res = ioctl(fd, FIONREAD, &ba);
//		fprintf(stderr, "%s: res %d, ba %d\n", __func__, res, ba);
		if (ba == 0) return;
        rd = read(fd, &b[w], r-w-1);
		if(rd == -1)
		{
//			fprintf(stderr, "%s:11 read() error: %s\n", __func__, strerror(errno));
			return;
		}
		w += rd;
//		fprintf(stderr, "%s:12 %d bytes read\n", __func__, r-w-1);
    }

//    printf("!!!!!!!!!!!  ");
//    printb(b);


    string_ready.wakeOne();

//	fprintf(stderr, "%s finished\n", __func__);
}

bool RingBuf::isEmpty(){
    return w == GetNext(r);
}

bool RingBuf::isFull(){
    return r == GetNext(w);
}

void RingBuf::WriteStr(Parser *prs){
    int nr = GetNext(Dpos); //r
    static int strN = 0;

    prs->eos = 0;

    if (nr < CRpos){
        memcpy(prs->str, &(b[nr]), CRpos-nr);
        prs->eos += CRpos-nr;
    }
    else{
        memcpy(&(prs->str[prs->eos]), &b[nr], bufLen-nr);
        prs->eos += bufLen-nr;
        memcpy(&(prs->str[prs->eos]), &b[0], CRpos);
        prs->eos += CRpos;
    }
    prs->str[prs->eos] = 0;
    strN++;
    printf("%d %s\n", strN, prs->str);
}


int RingBuf::Find_D(int p){
    if (r < p && p < w){
        for (int i = p; i <= w-1; i++)
            if (b[i] == '$'||b[i] == '!') return i;
    }
    else if (w < r && r < p){
        for (int i = p; i < bufLen; i++){
            if (b[i] == '$'||b[i] == '!') return i;
        }
        for (int i = 0; i < w; i++){
            if (b[i] == '$'||b[i] == '!') return i;
        }
    }
    else if (p < w && w < r){
            for (int i = p; i <= w-1; i++)
                if (b[i] == '$'||b[i] == '!') return i;
        }
    else {
//        printf("Internal error: incorrect parameter p in function Find: p: %d is in write zone: w: %d; r: %d\n", p, w, r);
    }
    return -1;

    if (p < w){
        for (int i = p; i <= w-1; i++)
            if (b[i] == '$'||b[i] == '!')
                return i;
    }
    else{

        for (int i = p; i < bufLen; i++){
            if (b[i] == '$'||b[i] == '!') return i;
        }
        for (int i = 0; i < w; i++){
            if (b[i] == '$'||b[i] == '!') return i;
        }

    }
    return -1;
}

int RingBuf::Find(char c, int p){
    if (r < p && p < w){
        for (int i = p; i <= w-1; i++)
            if (c == b[i])
                return i;
    }
    else if (w < r && r < p){
        for (int i = p; i < bufLen; i++){
            if (c == b[i]) return i;
        }
        for (int i = 0; i < w; i++){
            if (c == b[i]) return i;
        }
    }
    else if (p < w && w < r){
            for (int i = p; i <= w-1; i++)
                if (c == b[i])
                    return i;
        }
    else {
//        printf("Internal error: incorrect parameter p in function Find: p: %d is in write zone: w: %d; r: %d\n", p, w, r);
    }
    return -1;
}

parsType RingBuf::getParsType(char s0, char s1){

    for (int i = 0; i < LAST_PARSER_TYPE*2; i+=2){
        if (s0 == typeTable[i] && s1 == typeTable[i+1])
            return (parsType)(i/2+1);
    }

    return UNK;
}


bool Parser::Checksum(){
    if (str[eos-3] != '*'){
        if (str[eos-1] == '*') eos--;
        str[eos] = 0;
        return true;
    }

    const char *table = "0123456789ABCDEF";

    unsigned char sum = 0, checksum;
    int i;

    for (i = 0; i < 16; i++)
        if (str[eos-2] == table[i]) break;
    if (i == 16) {
        printf("Unknown hex number %c\n", str[eos-2]);
        return false;
    }

    checksum = i*16;

    for (i = 0; i < 16; i++)
        if (str[eos-1] == table[i]) break;

    if (i == 16) {
        printf("Unknown hex number %c\n", str[eos-1]);
        return false;
    }

    checksum += i;

    for (int i = 0; i < eos-3; i++)
        sum ^= str[i];

    str[eos-3] = '\0';

//    printf("sum: %x, checksum: %x\n", sum, checksum);
    return sum == checksum;
}




void NMEAProcessor::open_fds(QStringList & ports){
    for(QStringList::size_type i = 0; i < ports.size(); i++)
    {
        fd_rs[i] = openport(ports.at(i).toLocal8Bit().data());
        FD_SET(fd_rs[i], &readbufs);
        bufs[i].fd = fd_rs[i];
    }
    maxfd = getMaxFd();
}

int NMEAProcessor::getMaxFd(void)
{
    int mfd = -1;

    for(int i = 0; i < portNum; i++)
    {
        if(fd_rs[i] > mfd)
            mfd = fd_rs[i];
    }

    return mfd;
}

int NMEAProcessor::start(void)
{
    finish_flag = false;

    readPortsThread = QtConcurrent::run(this, &NMEAProcessor::readPorts);
    parsersThread = QtConcurrent::run(this, &NMEAProcessor::readPortBufs);
    return 0;
}

/*
void analize(char s[msglen]){

}
*/


void NMEAProcessor::initParsers(){

    parsers[GP-1] = new GP_Parser();
    parsers[HE-1] = new HE_Parser();
    parsers[HC-1] = new HC_Parser();
    parsers[VW-1] = new VW_Parser();
    parsers[SD-1] = new SD_Parser();
    parsers[AI-1] = new AI_Parser();
    parsers[AI-1]->_prc = this;
}

int strfind(char c, char * str, int pos, int l){
    for (int i = pos; i < l; i++)
        if (c == str[i]) return i;
    return -1;
}


Parser::Parser(NMEAProcessor *prc){
    _prc = prc;
    readStr = -1;
    writeStr = 0;
}

void Parser::Pars(){

    printf("-------------------\n");

    predComaPos = -1; currComaPos = 5;


}

void Parser::ParsEnd(){
    strBusy = false;
}


int Parser::FindComa(int Pos){

    int i;
    for (i = Pos; i < strLen; i++)
        if (str[i] == ',' || str[i] == '\0') return i;

    return i-1;

}

int Parser::ComaEOLCheck(int p){

    if (str[p] == '\0'){
        printf("Unexpected EOL\n");
        return -1;
    }

    if (str[p] != ','){
        SintError(",", &str[p]);
        return -1;
    }

    return 0;

}


int Parser::GetC(unsigned int *field_F, void *field){

    if (ComaEOLCheck(currComaPos) == -1) return -1;

    predComaPos = currComaPos;
    currComaPos++;

    if (str[currComaPos] == ','){
        *field_F = 0;
        return 0;
    }

    currComaPos = FindComa(currComaPos);

    if (currComaPos-predComaPos > 2){
        SintError("Character", &str[currComaPos+1]);
        return -1;
    }

    *field_F = 1;
    *(char *)field = str[predComaPos+1];

    return 0;
}

int Parser::GetS(unsigned int *field_F, void *field){


    if (ComaEOLCheck(currComaPos) == -1) return -1;


    predComaPos = currComaPos;
    currComaPos++;

    int i = FindComa(currComaPos);

    if (currComaPos == i){
        *field_F = 0;
        return 0;
    }

    currComaPos = i;

    *field_F = 1;
    memcpy(field, &str[predComaPos+1], currComaPos-predComaPos-1);

    return 0;
}

int Parser::GetI(unsigned int *field_F, void *field){

    if (ComaEOLCheck(currComaPos) == -1) return -1;

    predComaPos = currComaPos;
    currComaPos++;

    int i = FindComa(currComaPos);
    if (currComaPos == i){
        *field_F = 0;
        return 0;
    }

    currComaPos = i;

    int en;
    char **end;
    end = 0;    //????????????????????
    *field_F = 1;

    errno_mtx.lock();

    errno = 0;
    int iv = strtol(&str[predComaPos+1], end, 10);
    en = errno;

    errno_mtx.unlock();

    if (en){
        SintError("invalid int", &str[predComaPos+1]);
        return -1;
    }
    if (**end != ','){
        SintError(",", *end);
        return -1;
    }

    *field_F = 1;
    *(int *)field = iv;

    return 0;

}



int Parser::GetF(unsigned int *field_F, void *field){
    if (ComaEOLCheck(currComaPos) == -1) return -1;

    predComaPos = currComaPos;
    currComaPos++;

    int i = FindComa(currComaPos);
    if (currComaPos == i){
        *field_F = 0;
        return 0;
    }

    currComaPos = i;

    int en;
    char *end;
    *field_F = 1;
    errno_mtx.lock();
    errno = 0;
    float f = strtof(&str[predComaPos+1], &end);
    en = errno;
    errno_mtx.unlock();

    if (en){
        SintError("invalid int", &str[predComaPos+1]);
        return -1;
    }

    if (*end != ',' && *end != '\0' && *end != '.'){

        SintError(",", end);
        return -1;
    }

    *field_F = 1;
    *(float *)field = f;

    return 0;

}

GP_Parser::GP_Parser(){
    Parser();
    type = GP;
}

void GP_Parser::printRMB(){
    printf("Status: "); if (rmb.Status_F) printf("%c\n", rmb.Status); else printf("\n");
    printf("Cross Track Error: "); if (rmb.CrossTrackErr_F) printf("%f\n", rmb.CrossTrackErr); else printf("\n");
    printf("Direction to Steer (L/R): "); if (rmb.DirSteer_LR_F) printf("%c\n", rmb.DirSteer_LR); else printf("\n");
    printf("TO Waypoint: "); if (rmb.To_WayPointID_F) printf("%s\n", rmb.To_WayPointID); else printf("\n");
    printf("FROM Waypoint: "); if (rmb.From_WayPointID_F) printf("%s\n", rmb.From_WayPointID); else printf("\n");
    printf("Destination Waypoint Latitude: "); if (rmb.Dest_WayPoint_Latitude_F) printf("%f\n", rmb.Dest_WayPoint_Latitude); else printf("\n");
    printf("N or S: "); if (rmb.Dest_WayPoint_LaNS_F) printf("%c\n", rmb.Dest_WayPoint_LaNS); else printf("\n");
    printf("Destination Waypoint Longitude: "); if (rmb.Dest_WayPoint_Longitude_F) printf("%f\n", rmb.Dest_WayPoint_Longitude); else printf("\n");
    printf("E or W: "); if (rmb.Dest_WayPoint_LoEW_F) printf("%c\n", rmb.Dest_WayPoint_LoEW); else printf("\n");
    printf("Range to destination: "); if (rmb.RangeToDest_F) printf("%f\n", rmb.RangeToDest); else printf("\n");
    printf("Bearing to destination: "); if (rmb.BearingToDest_DegrTrue_F) printf("%f\n", rmb.BearingToDest_DegrTrue); else printf("\n");
    printf("Destination closing velocity: "); if (rmb.Dest_ClosingVelociti_F) printf("%f\n", rmb.Dest_ClosingVelociti); else printf("\n");
    printf("Arrival status: "); if (rmb.Arrival_Status_F) printf("%c\n", rmb.Arrival_Status); else printf("\n");

}

void GP_Parser::printRMC(sTarget *currTarg){
    char Date[9], Time[12];

    printf("Time: ");
    if (currTarg->Time_F) {
        strftime(Time, 11, "%H:%M:%S.", &currTarg->DateTime);
        printf("%s%.2d\n", Time, currTarg->ss);
    }
    else printf("\n");
    printf("Status: "); if (currTarg->Status_F) printf("%c\n", currTarg->Status);
    printf("Latitude: "); if (currTarg->Longitude_F) printf("%f\n", currTarg->Latitude); else printf("\n");
    printf("N or S: "); if (currTarg->LaNS_F) printf("%c\n", currTarg->LaNS); else printf("\n");
    printf("Longitude: "); if (currTarg->Longitude_F) printf("%f\n", currTarg->Longitude); else printf("\n");
    printf("E or W: "); if (currTarg->LoEW_F) printf("%c\n", currTarg->LoEW); else printf("\n");
    printf("Speed over ground: "); if (currTarg->GrndSpeed_F) printf("%f\n", currTarg->GrndSpeed); else printf("\n");
    printf("Track made good: "); if (currTarg->DegrTrue_F) printf("%f\n", currTarg->DegrTrue); else printf("\n");
    printf("Date: ");
    if (currTarg->Date_F) {
        strftime(Date, 9, "%d.%m.%y", &currTarg->DateTime);
        printf("%s\n", Date);
    }
    else printf("\n");
    printf("Magnetic Variation: "); if (currTarg->MagnVar_F) printf("%f\n", currTarg->MagnVar); else printf("\n");
    printf("E or W: "); if (currTarg->MV_EW_F) printf("%c\n", currTarg->MV_EW); else printf("\n");
}

void GP_Parser::Pars(){

    Parser::Pars();

//	fprintf(stderr, "%s entered\n", __func__);

    if (strncmp(&str[2], "RMB", 3) == 0){

//		fprintf(stderr, "%s RMB\n", __func__);

        if (GetC(&Flag, &rmb.Status) == -1) return;
        else rmb.Status_F = Flag;
        if (GetF(&Flag, &rmb.CrossTrackErr) == -1) return;
        else rmb.CrossTrackErr_F = Flag;
        if (GetC(&Flag, &rmb.DirSteer_LR) == -1) return;
        else rmb.DirSteer_LR_F = Flag;
        if (GetS(&Flag, &rmb.To_WayPointID) == -1) return;
        else rmb.To_WayPointID_F = Flag;
        if (GetS(&Flag, &rmb.From_WayPointID) == -1) return;
        else rmb.From_WayPointID_F = Flag;
        if (GetF(&Flag, &rmb.Dest_WayPoint_Latitude) == -1) return;
        else rmb.Dest_WayPoint_Latitude_F = Flag;
        if (GetC(&Flag, &rmb.Dest_WayPoint_LaNS) == -1) return;
        else rmb.Dest_WayPoint_LaNS_F = Flag;
        if (GetF(&Flag, &rmb.Dest_WayPoint_Longitude) == -1) return;
        else rmb.Dest_WayPoint_Longitude_F = Flag;
        if (GetC(&Flag, &rmb.Dest_WayPoint_LoEW) == -1) return;
        else rmb.Dest_WayPoint_LoEW_F = Flag;
        if (GetF(&Flag, &rmb.RangeToDest) == -1) return;
        else rmb.RangeToDest_F = Flag;
        if (GetF(&Flag, &rmb.BearingToDest_DegrTrue) == -1) return;
        else rmb.BearingToDest_DegrTrue_F = Flag;
        if (GetF(&Flag, &rmb.Dest_ClosingVelociti) == -1) return;
        else rmb.Dest_ClosingVelociti_F = Flag;
        if (GetC(&Flag, &rmb.Arrival_Status) == -1) return;
        else rmb.Arrival_Status_F = Flag;

        printRMB();

    }

    else if (strncmp(&str[2], "RMC", 3) == 0){
        char TimeStr[9], ss_str[3], DateStr[7];


//        pstargs->stargList_mtx.lock();
//		fprintf(stderr, "%s locked\n", __func__);
        sTarget *currTarg = pstargs->sFindMMSI(0);
//		fprintf(stderr, "%s found %p\n", __func__, currTarg);


        if (GetS(&Flag, TimeStr) == -1) return;
        else currTarg->Time_F = Flag;
        if (Flag){
            memcpy(ss_str, &TimeStr[7], 2);
            currTarg->ss = atoi(ss_str);
            strptime(TimeStr, "%H%M%S", &currTarg->DateTime);
        }
        if (GetC(&Flag, &currTarg->Status) == -1) return;
        else currTarg->Status_F = Flag;
        if (GetF(&Flag, &currTarg->Latitude) == -1) return;
        else currTarg->Latitude_F = Flag;
        if (GetC(&Flag, &currTarg->LaNS) == -1) return;
        else currTarg->LaNS_F = Flag;
        if (GetF(&Flag, &currTarg->Longitude) == -1) return;
        else currTarg->Longitude_F = Flag;
        if (GetC(&Flag, &currTarg->LoEW) == -1) return;
        else currTarg->LoEW_F = Flag;
        if (GetF(&Flag, &currTarg->GrndSpeed) == -1) return;
        else currTarg->GrndSpeed_F = Flag;
        if (GetF(&Flag, &currTarg->DegrTrue) == -1) return;
        else currTarg->DegrTrue_F = Flag;
        if (GetS(&Flag, DateStr) == -1) return;
        else currTarg->Date_F = Flag;
        if (Flag) strptime(DateStr, "%d%m%y", &currTarg->DateTime);
        if (GetF(&Flag, &currTarg->MagnVar) == -1) return;
        else currTarg->MagnVar_F = Flag;
        if (GetC(&Flag, &currTarg->MV_EW) == -1) return;
        else currTarg->MV_EW_F = Flag;

        printRMC(currTarg);

//        pstargs->stargList_mtx.unlock();
//        stargs.stargList_mtx.unlock();
    }
    else
        printf("RMB or RMC expected, but %s found\n", &str[2]);
//    i = FindComa(currComaPos+1);
//        pstargs->stargList_mtx.unlock();

//    structReady = true;
//    pars_main_mtx.unlock();
//    pthread_mutex_unlock(&pars_main_mtx);
//	fprintf(stderr, "%s finished\n", __func__);

}

HE_Parser::HE_Parser(){
    Parser();
    type = HE;
}

void HE_Parser::Pars(){

    Parser::Pars();

    if (GetF(&Flag, &hdt.HeadDegr) == -1) return;
    else hdt.HeadDegr_F = Flag;
    if (GetC(&Flag, &hdt.T) == -1) return;
    else hdt.T_F = Flag;

    printHDT();

}

void HE_Parser::printHDT(){

    printf("Heading Degrees: "); if (hdt.HeadDegr_F) printf("%f\n", hdt.HeadDegr); else printf("\n");
    printf("T: "); if (hdt.T_F) printf("%c\n", hdt.T); else printf("\n");

}

HC_Parser::HC_Parser(){
    Parser();
    type = HC;
}

void HC_Parser::Pars(){

    Parser::Pars();

    if (GetF(&Flag, &hdg.MagnSensor) == -1) return;
    else hdg.MagnSensor_F = Flag;
    if (GetF(&Flag, &hdg.MagnDeviation) == -1) return;
    else hdg.MagnDeviation_F = Flag;
    if (GetC(&Flag, &hdg.MagnDivDir) == -1) return;
    else hdg.MagnDivDir_F = Flag;
    if (GetF(&Flag, &hdg.MagnVarDegrs) == -1) return;
    else hdg.MagnVarDegrs_F = Flag;
    if (GetC(&Flag, &hdg.MagnVarDir) == -1) return;
    else hdg.MagnVarDir_F = Flag;

    printHDG();

}

void HC_Parser::printHDG(){

    printf("Magnetic Sensor heading: "); if (hdg.MagnSensor_F) printf("%f\n", hdg.MagnSensor); else printf("\n");
    printf("Magnetic Deviation: "); if (hdg.MagnDeviation_F) printf("%f\n", hdg.MagnDeviation); else printf("\n");
    printf("Magnetic Deviation direction: "); if (hdg.MagnDivDir_F) printf("%c\n", hdg.MagnDivDir); else printf("\n");
    printf("Magnetic Variation: "); if (hdg.MagnVarDegrs_F) printf("%f\n", hdg.MagnVarDegrs); else printf("\n");
    printf("Magnetic Variation direction: "); if (hdg.MagnVarDir_F) printf("%c\n", hdg.MagnVarDir); else printf("\n");

}

SD_Parser::SD_Parser(){
    Parser();
    type = SD;
}

void SD_Parser::Pars(){

    Parser::Pars();

    if (GetF(&Flag, &dpt.Depth) == -1) return;
    else dpt.Depth_F = Flag;
    if (GetF(&Flag, &dpt.OffsetFromTransducer) == -1) return;
    else dpt.OffsetFromTransducer_F = Flag;

    printDPT();

}

void SD_Parser::printDPT(){

    printf("Depth: "); if (dpt.Depth_F) printf("%f\n", dpt.Depth); else printf("\n");
    printf("Offset from transducer: "); if (dpt.OffsetFromTransducer_F) printf("%f\n", dpt.OffsetFromTransducer); else printf("\n");

}


unsigned char AI_Parser::byte_to_6bit(unsigned char byte){
    byte += 0x28;
    if (byte > 0x80)
        byte += 0x20;
    else
        byte += 0x28;

    return byte & 0x3f;
}


void AI_Parser::Str8toStr6(char *str){

    memset(bitStr, 0, sizeof(bitStr));
    int faze = 1;

    int bitStrPos = 0;
    unsigned char curr6bit = 0;

//    pos = 2;

    while (str[pos] != ','){

        switch (faze) {
        case 1:
            bitStr[bitStrPos] = byte_to_6bit(str[pos]) << 2;
            faze = 2;
            break;
        case 2:
            curr6bit = byte_to_6bit(str[pos]);
            bitStr[bitStrPos] |=  (curr6bit & 0x30) >> 4;
            bitStrPos++;
            bitStr[bitStrPos] = (curr6bit & 0x0f) << 4;
            faze = 3;
            break;
        case 3:
            curr6bit = byte_to_6bit(str[pos]);
            bitStr[bitStrPos] |=  (curr6bit & 0x3c) >> 2;
            bitStrPos++;
            bitStr[bitStrPos] = (curr6bit & 0x03) << 6;
            faze = 4;
            break;
        case 4:
            bitStr[bitStrPos] |= byte_to_6bit(str[pos]);
            bitStrPos++;
            faze = 1;
            break;
        default:
            break;
        }
        pos++;

    }
//    printb((char *)bitStr);
}


void AI_Parser::sS_123(sTarget *strgt){


    strgt->Navigational_status = str6tochar(4);
    strgt->ROT = str6tochar(8);
    strgt->SOG = str6toshort(10);
    strgt->Position_accuracy = str6tochar(1);
    strgt->Longitude = str6toint(28);
    strgt->Latitude =  str6toint(27);
    strgt->COG = str6toshort(12);
    strgt->True_heading = str6toshort(9);
    strgt->Time_stamp = str6tochar(6);

    strgt->Reserv_for_regional_apps = str6tochar(4);
    strgt->Spare1 = str6tochar(1);

    strgt->RAIM_flag = str6tochar(1);
    strgt->Communication_state = str6toint(19);

    sprintVDM_123(strgt);
}

void AI_Parser::sprintVDM_123(sTarget *strgt){

//    if (vdm.AIS_Channel_F) printf("AIS channel: %c\n", vdm.AIS_Channel); else printf("\n");
//    printf("Message ID: %d\n", trgt->Message_ID);
//    printf("Repeat Indicator: %d\n", vdm.eS_common.Repeat_Indicator);
    printf("User ID: %u\n", strgt->mmsi);
    printf("Navigational Status: %d\n", strgt->Navigational_status);
    printf("ROT: %f\n", strgt->ROT);
    printf("SOG: %f\n", (float)strgt->SOG/10);
    printf("Position accuracy: %d\n", strgt->Position_accuracy);
    printf("Longitude: %.20f\n", (float)strgt->Longitude/10000);
    printf("Latitude: %.20f\n", (float)strgt->Latitude/10000);
    printf("COG: %f\n", strgt->COG);
    printf("True_Heading: %f\n", strgt->True_heading);
    printf("Time_stamp: %d\n", strgt->Time_stamp);
    printf("Reserve for regional apps: %d\n", strgt->Reserv_for_regional_apps);
    printf("Spare: %d\n", vdm.eS123.Spare);
    printf("Raim Flag: %d\n", strgt->RAIM_flag);
    printf("Communication_State: %d\n", strgt->Communication_state);
}

void AI_Parser::S_123(Target *targ){

    targ->AISTarg_type = Ship_A;

    targ->Navigational_status = str6to_uchar(4);
    targ->ROT = str6tochar(8);
    targ->SOG = str6to_ushort(10);
    targ->Position_accuracy = str6to_uchar(1);
//    unsigned int L = str6toint(28);
    targ->Longitude = str6toint(28);
    targ->Latitude = str6toint(27);
    targ->COG = str6to_ushort(12);
    targ->True_heading = str6to_ushort(9);
    targ->Time_stamp = str6to_uchar(6);
    targ->Reserv_for_regional_apps = str6to_uchar(4);
    targ->Spare1 = str6to_uchar(1);
    targ->RAIM_flag = str6to_uchar(1);

    targ->Flags &= ~0xff;

    if (targ->Latitude != 91*10000*60) targ->Latitude_F = 1;
    if (targ->Longitude != 181*10000*60) targ->Longitude_F = 1;
    if (targ->COG != 360*10) targ->COG_F = 1;
    if (targ->SOG != 1023) targ->SOG_F = 1;
    if (targ->True_heading != 511) targ->True_heading_F = 1;
    if (targ->Navigational_status != 15) targ->Navigational_status_F = 1;
    if (targ->ROT != -128) targ->ROT_F = 1;
    if (targ->Time_stamp != 60) targ->Time_stamp_F = 1;

//    printVDM_123(targ);
    targ->PrintOut();
}

void AI_Parser::sS_411(){

    vdm.eS411.UTC_year = str6toshort(14);
    vdm.eS411.UTC_month = str6tochar(4);
    vdm.eS411.UTC_day = str6tochar(5);
    vdm.eS411.UTC_hour = str6tochar(5);
    vdm.eS411.UTC_minute = str6tochar(6);
    vdm.eS411.UTC_second = str6tochar(6);
    vdm.eS411.Position_accuracy = str6tochar(1);
    vdm.eS411.Longitude = str6toint(28);
    vdm.eS411.Latitude = str6toint(27);
    vdm.eS411.Type_of_pos_fixing_device = str6tochar(4);
    str6toshort(10);
    vdm.eS411.RAIM_Flag = str6toshort(1);
    vdm.eS411.Communication_State = str6toint(19);

    sprintVDM_411();
}

void AI_Parser::sprintVDM_411(){

    printf("Message ID: %d\n", vdm.eS_common.Message_ID);
    printf("Repeat Indicator: %d\n", vdm.eS_common.Repeat_Indicator);
    printf("User ID: %d\n", vdm.eS_common.User_ID);
    printf("UTC year: %d\n", vdm.eS411.UTC_year);
    printf("UTC month: %d\n", vdm.eS411.UTC_month);
    printf("UTC day: %d\n", vdm.eS411.UTC_day);
    printf("UTC hour: %d\n", vdm.eS411.UTC_hour);
    printf("UTC minute: %d\n", vdm.eS411.UTC_minute);
    printf("UTC second: %d\n", vdm.eS411.UTC_second);
    printf("Position accuracy: %d\n", vdm.eS411.Position_accuracy);
    printf("Longitude: %d\n", vdm.eS411.Longitude);
    printf("Latitude: %d\n", vdm.eS411.Latitude);
    printf("Type electronic position fixing device: %d\n", vdm.eS411.Type_of_pos_fixing_device);
    printf("Raim Flag: %d\n", vdm.eS411.RAIM_Flag);
    printf("Communication_State: %d\n", vdm.eS411.Communication_State);

}

void AI_Parser::S_411(Target *targ){

    targ->AISTarg_type = Base_station;

    /*.UTC_year = */str6to_ushort(14);
    /*.UTC_month = */str6to_uchar(4);
    /*.UTC_day = */str6to_uchar(5);
    /*.UTC_hour = */str6to_uchar(5);
    /*.UTC_minute = */str6to_uchar(6);
    /*.UTC_second = */str6to_uchar(6);
    targ->Position_accuracy = str6to_uchar(1);
    targ->Longitude = str6toint(28);
    targ->Latitude = str6toint(27);
    targ->Type_of_electronic_position_fixing_dev = str6to_uchar(4);
    str6to_ushort(10);
    targ->RAIM_flag = str6to_ushort(1);
    /*.Communication_State = */str6to_uint(19);

    targ->Flags &= ~0x4003;

    if (targ->Latitude != 91*10000*60) targ->Latitude_F = 1;
    if (targ->Longitude != 181*10000*60) targ->Latitude_F = 1;
    if (targ->Type_of_electronic_position_fixing_dev != 0)
        targ->Type_of_electronic_position_fixing_dev_F = 1;

    targ->PrintOut();

}


void AI_Parser::sS_5(sTarget *strgt){


    strgt->AIS_version_indicator = str6to_uchar(2);
    strgt->IMO_number = str6to_uint(30);
    Str6toStr8(42, vdm.eS5.Call_sign);
    Str6toStr8(120, vdm.eS5.Name);
    strgt->Ship_cargo_type =  str6to_uchar(8);
    strgt->Dim_ref_for_position = str6toint(30);
    strgt->Electronic_position_fixing_dev_type = str6to_uchar(4);
    strgt->ETA = str6to_uint(20);
    strgt->Max_present_static_draught =  str6to_uchar(8);
    Str6toStr8(120, vdm.eS5.Destination);
    strgt->DTE = str6to_uchar(1);
    str6tochar(1);

    sprintVDM_5(strgt);

}

void AI_Parser::sprintVDM_5(sTarget *strgt){
//    printf("Message ID: %d\n", vdm.eS_common.Message_ID);
//    printf("Repeat Indicator: %d\n", vdm.eS_common.Repeat_Indicator);
    printf("User ID: %u\n", strgt->mmsi);
    printf("AIS version indicator: %d\n", strgt->AIS_version_indicator);
    printf("IMO number: %d\n", strgt->IMO_number);
    printf("Call sign: %s\n", *(strgt->Call_sign));
    printf("Name: %s\n", *(strgt->Name));
    printf("Type of ship and cargo type: %d\n", strgt->Ship_cargo_type);
    printf("Dimension/refernce for position: A: %d, B: %d, C: %d, D: %d\n",
           ((strgt->Dim_ref_for_position & 0x3fe00000) >> 21),
           ((strgt->Dim_ref_for_position & 0x1ff000) >> 12),
           ((strgt->Dim_ref_for_position & 0xfc0) >> 6),
           (strgt->Dim_ref_for_position & 0x3f));
    printf("Type electronic position fixing device: %d\n", vdm.eS5.Type_of_pos_fixing_device);
    printf("ETA: month: %d, day: %d, hour: %d, minute: %d\n",
           (strgt->ETA & 0xf0000) >> 16,
           (strgt->ETA & 0xf800) >> 11,
           (strgt->ETA & 0x7c0) >> 6,
            strgt->ETA & 0x3f);
    printf("Maximum present static draught: %d\n", strgt->Max_present_static_draught);
    printf("Destination: %s\n", *(strgt->Destination));
    printf("DTE: %d\n", strgt->DTE);
}

void AI_Parser::S_5(Target *targ){

    targ->AISTarg_type = Ship_A;

    targ->AIS_version_indicator = str6to_uchar(2);
    targ->IMO_number = str6to_uint(30);
    Str6toStr8(42, targ->Call_sign);
    Str6toStr8(120, targ->Name);
    targ->Ship_cargo_type =  str6to_uchar(8);
    unsigned int Dim_ref_for_position = str6to_uint(30);
    targ->Dim_ref_for_position_A = ((Dim_ref_for_position & 0x3fe00000) >> 21);
    targ->Dim_ref_for_position_B = ((Dim_ref_for_position & 0x1ff000) >> 12),
    targ->Dim_ref_for_position_C = ((Dim_ref_for_position & 0xfc0) >> 6),
    targ->Dim_ref_for_position_D = (Dim_ref_for_position & 0x3f);

    targ->Type_of_electronic_position_fixing_dev = str6to_uchar(4);
    unsigned int ETA = str6to_uint(20);
    targ->ETA_month = (ETA & 0xf0000) >> 16;
    targ->ETA_day = (ETA & 0xf800) >> 11;
    targ->ETA_hour = (ETA & 0x7c0) >> 6;
    targ->ETA_minute = (ETA & 0x3f) >> 16;
    targ->Max_present_static_draught =  str6to_uchar(8);
    Str6toStr8(120, targ->Destination);
    targ->DTE = str6to_uchar(1);
    str6tochar(1);

    targ->Flags &= ~0xfff00;

    if (targ->IMO_number != 0) targ->IMO_number_F = 1;
    if (strcmp(targ->Call_sign, "@@@@@@@") != 0) targ->Call_sign_F = 1;
    if (strcmp(targ->Name, "@@@@@@@@@@@@@@@@@@@@") != 0) targ->Name_F = 1;
    if (targ->Ship_cargo_type != 0) targ->Ship_cargo_type_F = 1;

    if ((targ->Dim_ref_for_position_A && targ->Dim_ref_for_position_C) != 0)
        targ->Ref_point_F = 1;
    if ((targ->Dim_ref_for_position_B && targ->Dim_ref_for_position_D) != 0)
        targ->Dimensions_F = 1;

    if (targ->Type_of_electronic_position_fixing_dev != 0)
        targ->Type_of_electronic_position_fixing_dev_F = 1;
    if (targ->ETA_month != 0) targ->ETA_month_F = 1;
    if (targ->ETA_day != 0) targ->ETA_day_F = 1;
    if (targ->ETA_hour != 24) targ->ETA_hour_F = 1;
    if (targ->ETA_minute != 60) targ->ETA_minute_F = 1;
    if (targ->Max_present_static_draught != 0)
        targ->Max_present_static_draught_F = 1;
    if (strcmp(targ->Destination, "@@@@@@@@@@@@@@@@@@@@") != 0)
        targ->Destination_F = 1;

    targ->PrintOut();

}

void AI_Parser::S_6(){

    vdm.eS6.Sequence_number = str6tochar(2);
    vdm.eS6.Destination_ID = str6toint(30);
    vdm.eS6.Retransmit_flag = str6tochar(1);
    str6tochar(1);
    vdm.eS6.Applicatoin_identifier_DAC = str6toshort(10);
    vdm.eS6.Applicatoin_identifier_function_ID = str6tochar(6);
    memcpy(&vdm.eS6.Binary_data, &bitStr[startByte], bitPos-startByte);

    printVDM_6();

}

void AI_Parser::printVDM_6(){
    printf("Sequence_number: %d\n", vdm.eS6.Sequence_number);
    printf("Destination ID: %d\n", vdm.eS6.Destination_ID);
    printf("Retransmit flag: %d\n", vdm.eS6.Retransmit_flag);
    printf("Applicatoin identifier DAC: %d\n", vdm.eS6.Applicatoin_identifier_DAC);
    printf("Applicatoin identifier function ID: %d\n", vdm.eS6.Applicatoin_identifier_function_ID);

}

void AI_Parser::S_17(){
    str6tochar(2);
    vdm.eS17.Longitude = str6toint(18);
    vdm.eS17.Latitude = str6toint(17);
    str6tochar(5);
    vdm.eS17.Message_type = str6tochar(6);
    if (vdm.eS17.Message_type){
        vdm.eS17.Station_ID = str6toshort(10);
        vdm.eS17.Z_count = str6toshort(13);
        vdm.eS17.Sequence_number = str6tochar(3);
        vdm.eS17.N = str6tochar(5);
        vdm.eS17.Health = str6tochar(3);
        for (int i = 0; i < vdm.eS17.N; i++)
            vdm.eS17.DGNSS_data_words[i] = str6toint(24);

    }




    printVDM_17();

}

void AI_Parser::printVDM_17(){
    printf("Message ID: %d\n", vdm.eS_common.Message_ID);
    printf("Repeat Indicator: %d\n", vdm.eS_common.Repeat_Indicator);
    printf("User ID: %d\n", vdm.eS_common.User_ID);
    printf("Longitude: %d\n", vdm.eS17.Longitude);
    printf("Longitude: %d\n", vdm.eS17.Latitude);
    if (vdm.eS17.Message_type){
        printf("Message type: %d\n", vdm.eS17.Message_type);
        printf("Station ID: %d\n", vdm.eS17.Station_ID);
        printf("Z count: %d\n", vdm.eS17.Z_count);
        printf("Sequence number: %d\n", vdm.eS17.Sequence_number);
        printf("N: %d\n", vdm.eS17.N);
        printf("Health: %d\n", vdm.eS17.Health);
        for (int i = 0; i < vdm.eS17.N; i++)
            printf("DGNSS data word %d: %d\n", i, vdm.eS17.DGNSS_data_words[i]);
    }
}

void AI_Parser::sS_18(){
    vdm.eS18.Reserve_for_reg_loc_apps = str6tochar(8);
    vdm.eS18.SOG = str6toshort(10);
    vdm.eS18.Position_accuracy = str6tochar(1);
    vdm.eS18.Longitude = str6toint(28);
    vdm.eS18.Latitude = str6toint(27);
    vdm.eS18.COG = str6toshort(12);
    vdm.eS18.True_Heading = str6toshort(9);
    vdm.eS18.Time_stamp = str6tochar(6);
    vdm.eS18.Reserve_for_regional_apps = str6tochar(4);
    str6tochar(4);
    vdm.eS18.RAIM_Flag = str6tochar(1);
    vdm.eS18.Comm_state_selector_flag = str6tochar(1);
    vdm.eS18.Communication_State = str6toint(19);

    sprintVDM_18();
}

void AI_Parser::sprintVDM_18(){

    printf("Reserve for regional/local applications: %d\n", vdm.eS18.Reserve_for_reg_loc_apps);
    printf("SOG: %d\n", vdm.eS18.SOG);
    printf("Position accuracy: %d\n", vdm.eS18.Position_accuracy);
    printf("Longitude: %d\n", vdm.eS18.Longitude);
    printf("Latitude: %d\n", vdm.eS18.Latitude);
    printf("COG: %d\n", vdm.eS18.COG);
    printf("True Heading: %d\n", vdm.eS18.True_Heading);
    printf("Time stamp: %d\n", vdm.eS18.Time_stamp);
    printf("Reserve for regional applications: %d\n", vdm.eS18.Reserve_for_regional_apps);
    printf("RAIM_Flag: %d\n", vdm.eS18.RAIM_Flag);
    printf("Communication state selector flag: %d\n", vdm.eS18.Comm_state_selector_flag);
    printf("Communication State: %d\n", vdm.eS18.Communication_State);
}

void AI_Parser::S_18(Target *targ){

    targ->AISTarg_type = Ship_B;

    str6tochar(8);
    targ->SOG = str6toshort(10);
    targ->Position_accuracy = str6tochar(1);
    targ->Longitude = str6toint(28);
    targ->Latitude = str6toint(27);
    targ->COG = str6toshort(12);
    targ->True_heading = str6toshort(9);
    targ->Time_stamp = str6tochar(6);
    /*Reserve_for_regional_apps = */str6tochar(4);
    str6tochar(4);
    targ->RAIM_flag = str6tochar(1);
    /*Comm_state_selector_flag = */str6tochar(1);
    /*Communication_State = */str6toint(19);

    targ->Flags &= 0xcf;

    if (targ->SOG != 1023) targ->SOG_F = 1;
    if (targ->Longitude != 181*10000*60) targ->Longitude_F = 1;
    if (targ->Latitude != 91*10000*60) targ->Latitude_F = 1;
    if (targ->COG != 360*10) targ->COG_F = 1;
    if (targ->True_heading != 511) targ->True_heading_F = 1;
    if (targ->Time_stamp != 60) targ->Time_stamp_F = 1;


    targ->PrintOut();

}

void AI_Parser::S_19(Target *targ){

    targ->AISTarg_type = Ship_B;

    /*Reserve_for_regional_or_local_apps = */str6tochar(8);
    targ->SOG = str6toshort(10);
    targ->Position_accuracy = str6tochar(1);
    targ->Longitude = str6toint(28);
    targ->Latitude = str6toint(27);
    targ->COG = str6toshort(12);
    targ->True_heading = str6toshort(9);
    targ->Time_stamp = str6tochar(6);
    /*Reserve_for_regional_apps = */str6tochar(4);
    Str6toStr8(120, targ->Name);
    targ->Ship_cargo_type =  str6tochar(8);

    unsigned int Dim_ref_for_position = str6toint(30);
    targ->Dim_ref_for_position_A = ((Dim_ref_for_position & 0x3fe00000) >> 21);
    targ->Dim_ref_for_position_B = ((Dim_ref_for_position & 0x1ff000) >> 12),
    targ->Dim_ref_for_position_C = ((Dim_ref_for_position & 0xfc0) >> 6),
    targ->Dim_ref_for_position_D = (Dim_ref_for_position & 0x3f);

    targ->Type_of_electronic_position_fixing_dev = str6tochar(4);
    /*RAIM_fglag = */str6tochar(1);
    targ->DTE = str6tochar(1);
    str6tochar(5);

    targ->Flags &= 0x78cf;

    if (targ->SOG != 1023) targ->SOG_F = 1;
    if (targ->Longitude != 181*10000*60) targ->Longitude_F = 1;
    if (targ->Latitude != 91*10000*60) targ->Latitude_F = 1;
    if (targ->COG != 360*10) targ->COG_F = 1;
    if (targ->True_heading != 511) targ->True_heading_F = 1;
    if (targ->Time_stamp != 60) targ->Time_stamp_F = 1;
    if (targ->Ship_cargo_type != 0) targ->Ship_cargo_type_F = 1;

    if ((targ->Dim_ref_for_position_A && targ->Dim_ref_for_position_C) != 0)
        targ->Ref_point_F = 1;
    if ((targ->Dim_ref_for_position_B && targ->Dim_ref_for_position_D) != 0)
        targ->Dimensions_F = 1;

    if (targ->Type_of_electronic_position_fixing_dev != 0)
        targ->Type_of_electronic_position_fixing_dev_F = 1;

    targ->PrintOut();

}

void AI_Parser::S_20(){
    str6tochar(2);
    vdm.eS20.Offset_number_1 = str6toshort(12);
    if (vdm.eS20.Offset_number_1){
        vdm.eS20.Number_of_slots_1 = str6tochar(4);
        vdm.eS20.Time_out_1 = str6tochar(3);
        vdm.eS20.Increment_1 = str6tochar(11);
    }
    else
        return;
    vdm.eS20.Offset_number_2 = str6toshort(12);
    if (vdm.eS20.Offset_number_2){
        vdm.eS20.Number_of_slots_2 = str6tochar(4);
        vdm.eS20.Time_out_2 = str6tochar(3);
        vdm.eS20.Increment_2 = str6tochar(11);
    }
    else
        return;
    vdm.eS20.Offset_number_3 = str6toshort(12);
    if (vdm.eS20.Offset_number_3){
        vdm.eS20.Number_of_slots_3 = str6tochar(4);
        vdm.eS20.Time_out_3 = str6tochar(3);
        vdm.eS20.Increment_3 = str6tochar(11);
    }
    else
        return;
    vdm.eS20.Offset_number_4 = str6toshort(12);
    if (vdm.eS20.Offset_number_4){
        vdm.eS20.Number_of_slots_4 = str6tochar(4);
        vdm.eS20.Time_out_4 = str6tochar(3);
        vdm.eS20.Increment_4 = str6tochar(11);
    }

    printVDM_20();

}

void AI_Parser::printVDM_20(){
    printf("Message ID: %d\n", vdm.eS_common.Message_ID);
    printf("Repeat Indicator: %d\n", vdm.eS_common.Repeat_Indicator);
    printf("Source station ID: %d\n", vdm.eS_common.User_ID);
    if (vdm.eS20.Offset_number_1){
        printf("Offset number 1: %d\n", vdm.eS20.Offset_number_1);
        printf("Number of slots 1: %d\n", vdm.eS20.Number_of_slots_1);
        printf("Time-out 1: %d\n", vdm.eS20.Time_out_1);
        printf("Increment 1: %d\n", vdm.eS20.Increment_1);
    }
    else
        return;
    if (vdm.eS20.Offset_number_2){
        printf("Offset number 2: %d\n", vdm.eS20.Offset_number_2);
        printf("Number of slots 2: %d\n", vdm.eS20.Number_of_slots_2);
        printf("Time-out 2: %d\n", vdm.eS20.Time_out_2);
        printf("Increment 2: %d\n", vdm.eS20.Increment_2);
    }
    else
        return;
    if (vdm.eS20.Offset_number_3){
        printf("Offset number 3: %d\n", vdm.eS20.Offset_number_3);
        printf("Number of slots 3: %d\n", vdm.eS20.Number_of_slots_3);
        printf("Time-out 3: %d\n", vdm.eS20.Time_out_3);
        printf("Increment 3: %d\n", vdm.eS20.Increment_3);
    }
    else
        return;
    if (vdm.eS20.Offset_number_4){
        printf("Offset number 4: %d\n", vdm.eS20.Offset_number_4);
        printf("Number of slots 4: %d\n", vdm.eS20.Number_of_slots_4);
        printf("Time-out 4: %d\n", vdm.eS20.Time_out_4);
        printf("Increment 4: %d\n", vdm.eS20.Increment_4);
    }
    else
        return;
}

void AI_Parser::sS_21(){
    vdm.eS21.Types_of_aids_to_navigation = str6tochar(5);
    Str6toStr8(120, vdm.eS21.Name_of_aids_to_navigation);
    vdm.eS21.Position_accuracy = str6tochar(1);
    vdm.eS21.Longitude = str6toint(28);
    vdm.eS21.Latitude = str6toint(27);
    vdm.eS21.Dim_ref_for_position = str6toint(30);
    vdm.eS21.Type_of_pos_fixing_device = str6tochar(4);
    vdm.eS21.Time_stamp = str6tochar(6);
    vdm.eS21.Off_position_indicator = str6tochar(1);
    vdm.eS21.Reserve_for_regional_apps = str6tochar(8);
    vdm.eS21.RAIM_Flag = str6tochar(1);
    str6tochar(3);

    sprintVDM_21();

}

void AI_Parser::sprintVDM_21(){
        printf("Types of aids to navigation: %d\n", vdm.eS21.Types_of_aids_to_navigation);
        printf("Name of aids to navigation: %s\n", vdm.eS21.Name_of_aids_to_navigation);
        printf("Position accuracy: %d\n", vdm.eS21.Position_accuracy);
        printf("Longitude: %d\n", vdm.eS21.Longitude);
        printf("Latitude: %d\n", vdm.eS21.Latitude);
        printf("Dimension/refernce for position: %d\n", vdm.eS21.Dim_ref_for_position);
        printf("Type of pos fixing device: %d\n", vdm.eS21.Type_of_pos_fixing_device);
        printf("Time stamp: %d\n", vdm.eS21.Time_stamp);
        printf("Off-position indicator: %d\n", vdm.eS21.Off_position_indicator);
        printf("Reserve for regional apps: %d\n", vdm.eS21.Reserve_for_regional_apps);
        printf("RAIM Flag: %d\n", vdm.eS21.RAIM_Flag);
}

void AI_Parser::S_21(Target *targ){

    targ->Type_of_aids_to_navigation = str6tochar(5);
    Str6toStr8(120, targ->Name);
    targ->Position_accuracy = str6tochar(1);
    targ->Longitude = str6toint(28);
    targ->Latitude = str6toint(27);
    targ->Type_of_electronic_position_fixing_dev = str6tochar(4);
    targ->Time_stamp = str6tochar(6);
    targ->Off_position_indicator = str6tochar(1);
    /*targ->Reserve_for_regional_apps = */str6tochar(8);
    /*targ->RAIM_Flag = */str6tochar(1);
    str6tochar(3);

    targ->Flags &= ~0x204083;

    if (targ->Type_of_aids_to_navigation != 0) {
        targ->Type_of_aids_to_navigation_F = 1;
        if (targ->Type_of_aids_to_navigation < 16)
            targ->AISTarg_type = Aid_to_navigation_fixed;
        else
            targ->AISTarg_type = Aid_to_navigation_float;
    }
    if (targ->Longitude != 181*10000*60) targ->Longitude_F = 1;
    if (targ->Latitude != 91*10000*60) targ->Latitude_F = 1;
    if (targ->Type_of_electronic_position_fixing_dev != 0)
        targ->Type_of_electronic_position_fixing_dev_F = 1;
    if (targ->Time_stamp != 60) targ->Time_stamp_F = 1;

    targ->PrintOut();

}
void AI_Parser::Pars(){

    Parser::Pars();
    //unsigned char bitStr[53];
    memset(bitStr, 0, sizeof(bitStr));


    char s[4];

    unsigned char chan;
    int start_of_enc_str;

    pos = 2;

    s[3] = 0;

    memcpy(s, &str[pos], 3);

    if (strcmp(s, "VDM") != 0){
        printf("Unknown formatter %s\n", s);
        return;
    }

    pos += 3; // ,

    if (str[pos] != ','){
        printf("',' expected, but %c found\n", str[pos]);
        return;
    }
    pos++; //4 totN

    if (!isdigit(str[pos])){
        printf("Total number of sentences must be 0-9, but %d found\n", str[pos]);
        return;
    }

    int totN = atoi(&str[pos]);

    pos++; //5 ,
    if (str[pos] != ','){
        printf("',' expected, but %c found\n", str[pos]);
        return;
    }

    pos++; //6 sentenceN
    if (!isdigit(str[pos])){
        printf("Sentence number expected (1-9), but %c found\n", str[pos]);
        return;
    }

    int sentenceN = atoi(&str[pos]);

    if (sentenceN == 0){
        printf("Sentence number can not be zero\n");
        return;
    }
    pos++; //7 ,

    if (str[pos] != ','){
        printf("',' expected, but %c found\n", str[pos]);
        return;
    }
    pos++; //8 SN

    if (str[pos] == ',' && totN != 1){
        printf("Total number of sentences != 1, but Sequential message identifier field is empty\n");
        return;
    }
    if (totN == 1 && isdigit(str[pos])){
        printf("Total number of sentences == 1, but Sequential message identifier field is not empty\n");
        return;
    }

    if (sentenceN > totN){
        printf("Sentence number %d exceeds total number of sentences %d\n", sentenceN, totN);
        return;
    }

    int SN = -1;

    if (totN != 1) {
        SN = atoi(&str[pos]);
        if (SN < 0 || SN > 9){
            printf("Sequential message identifier is out of range: must be 0..9, but equals %d\n", SN);
            return;
        }
        pos++;
        if (str[pos] != ','){
            printf("',' expected, but %c found\n", str[pos]);
            return;
        }
    }

    pos++; //9 chan

    vdm.AIS_Channel_F = 1;
    if (str[pos] == ',') vdm.AIS_Channel_F = 0;
    else {
        chan = str[pos];
        if (chan != 'A' && chan != 'B'){
            printf("AIS Channel must be 'A' or 'B', but it is %c\n", chan);
            return;
        }

        pos++;
    }

    if (str[pos] != ','){
        printf("',' expected, but %c found\n", str[pos]);
        return;
    }

    pos++;

    if ((!isalnum(str[pos]) && (str[pos] != '@') && (str[pos] != '>'))){
        printf("Encapsulated string expected, but %c found\n", str[pos]);
        return;
    }

    start_of_enc_str = pos;
    startBit = 0;
    startByte = 0;

/* //--struct vdm ver:
    vdm.AIS_Channel = chan;
*/
    if (totN == 1){
        if (sentenceN != 1){
            printf("Total number of sentences == 1, but sentenceN == %d\n", sentenceN);
            return;
        }

        Str8toStr6(str);
    }
    else
    {
        if (tot_num_chan[SN] == 0){
            tot_num_chan[SN] = new(VDM_mult);
            tot_num_chan[SN]->totN = totN;
            tot_num_chan[SN]->sentenceN = 0;
            tot_num_chan[SN]->SN = SN;
            tot_num_chan[SN]->AIS_Channel = chan;
            tot_num_chan[SN]->EOS = 0;
        }
        else if (tot_num_chan[SN]->sentenceN > totN){
            printf("Sentence num is out of range: must be < %d, but it is %d\n", totN, tot_num_chan[SN]->sentenceN);
            return;
        }

        if (tot_num_chan[SN]->sentenceN+1 != sentenceN){
            printf("Invalid sentence order: sentence num %d expected, but %d found\n", tot_num_chan[SN]->sentenceN+1, sentenceN);
            return;
        }

        tot_num_chan[SN]->sentenceN = sentenceN;

        for (; pos < eos; pos++)
            if (str[pos] == ',') break;

        if (pos == eos){
            printf("Missing ',' at the end of encapsulated string");
            return;
        }

        memcpy(&(multiSentStrArr[SN][tot_num_chan[SN]->EOS]), &str[start_of_enc_str], pos-start_of_enc_str);

        tot_num_chan[SN]->EOS += pos-start_of_enc_str;

        if (sentenceN == totN){
            multiSentStrArr[SN][tot_num_chan[SN]->EOS] = ',';
            bitPos = tot_num_chan[SN]->EOS;
            pos = 0;
            Str8toStr6(multiSentStrArr[SN]);
            delete(tot_num_chan[SN]);
            tot_num_chan[SN] = NULL;
        }
        else return;

    }

    unsigned char Message_ID = str6tochar(6);
    str6tochar(2);

    unsigned int User_ID = str6toint(30);


//    if (ptargs->targ[0]) fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    ptargs->targList_mtx.lock();

    Target *currTarg = ptargs->FindMMSI(User_ID);

//    ptargs->targList_mtx.unlock();

    currTarg->AISTarg = true;

    switch (Message_ID){
    case 1: case 2: case 3:
        S_123(currTarg);
        if(_prc)
            _prc->targetChanged(currTarg->mmsi);
        break;
    case 4: case 11:
        S_411(currTarg);
        break;
    case 5:
        S_5(currTarg);
        break;
    case 6:
        S_6();
        break;
    case 17:
        S_17();
        break;
    case 18:
        S_18(currTarg);
        break;
    case 20:
        S_20();
        break;
    case 21:
        S_21(currTarg);
        break;

     default:
        printf("Unknown message type %d\n", vdm.eS_common.Message_ID);
//        exit(1); //!!!!!!!!!!
    }

    ptargs->targList_mtx.unlock();
//    stargs.stargList_mtx.unlock();


}

void Process_GP(){

}

void Process_HE(){

}

void Process_HC(){

}

void Process_VW(){

}

void Process_SD(){

}

void Process_AI(){

}


VW_Parser::VW_Parser(){
    Parser();
    type = VW;
}

void VW_Parser::Pars(){

    Parser::Pars();

    if (GetF(&Flag, &vhw.DegrTrue) == -1) return;
    else vhw.DegrTrue_F = Flag;
    if (GetC(&Flag, &vhw.T) == -1) return;
    else vhw.T_F = Flag;
    if (GetF(&Flag, &vhw.DegrMagn) == -1) return;
    else vhw.DegrMagn_F = Flag;
    if (GetC(&Flag, &vhw.M) == -1) return;
    else vhw.M_F = Flag;
    if (GetF(&Flag, &vhw.Knots) == -1) return;
    else vhw.Knots_F = Flag;
    if (GetC(&Flag, &vhw.N) == -1) return;
    else vhw.N_F = Flag;
    if (GetF(&Flag, &vhw.Kilometers) == -1) return;
    else vhw.Kilometers_F = Flag;
    if (GetC(&Flag, &vhw.K) == -1) return;
    else vhw.K_F = Flag;

    printVHW();
}


void VW_Parser::printVHW(){

    printf("Degrees True: "); if (vhw.DegrTrue_F) printf("%f\n", vhw.DegrTrue); else printf("\n");
    printf("T: "); if (vhw.T_F) printf("%c\n", vhw.T); else printf("\n");
    printf("Degrees Magnetic: "); if (vhw.DegrMagn_F) printf("%f\n", vhw.DegrMagn); else printf("\n");
    printf("M: "); if (vhw.M_F) printf("%c\n", vhw.M); else printf("\n");
    printf("Knots: "); if (vhw.Knots_F) printf("%f\n", vhw.Knots); else printf("\n");
    printf("T: "); if (vhw.T_F) printf("%c\n", vhw.T); else printf("\n");
    printf("Kilometers: "); if (vhw.Kilometers_F) printf("%f\n", vhw.Kilometers); else printf("\n");
    printf("K: "); if (vhw.K_F) printf("%c\n", vhw.K); else printf("\n");

}

#endif // !Q_OS_WIN
