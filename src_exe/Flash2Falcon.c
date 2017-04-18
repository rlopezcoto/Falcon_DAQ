#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//flashcam
#include "fcio.h"
#include "samples.h"
#include "pzpsa.h"
#include "timer.h" 
//xcdf
#include <xcdf/XCDF.h>
//DAQ
#include "ChargeTimeExtraction.h" 


//global parameters from the command line
int debug = 0;
int reduction_level = 0;
int cal_flag = 0;
const char 		*ofile=0;
const char 		*ifile=0;
const char 		*calfile=0;

//global parameter to extract the single pe
float single_pe[24]={0};

//global parameters for the upsampling
float   **uptraces=0; // for upsampling
int      us=4;


typedef struct {//hold the data derived from one FlashADC card
  int type;//calib, ext trig,
  unsigned long channelBitMask;// 24 lsb are used to indicate which channels are used
  unsigned short nSelectedChannels;//number of channels in ADC
  unsigned short nSamples;//number of samples per trace
  int timeoffset[10];//time offset information
  int timestamp[10];
  float charges[24];//for each channel, -1 if no charge found
  float times[24];// time of signal within trace, -1 if no signal found
  unsigned short** traces;//baseline subtracted traces
} CardData;


//more fancy shit should go here in the future
void AnalyseTraces(FCIOData *x, CardData& c) {
  c.channelBitMask = 0;
  c.nSelectedChannels = 0;
  /*  fprintf(stderr,"samples in waveform %d\n",x->config.eventsamples);
   fprintf(stderr,"samples in sum %d\n",x->config.sumlength);
   */
  for (int iCh = 0; iCh < x->config.adcs;iCh++) {
    // Time calculation
    float time = GetTime(x,iCh,uptraces);
    c.times[iCh] = time;
    if (debug>2){ fprintf(stderr,"Time %.2f\n",time);}
    // Charge calculation
    float pe=0;
    if (cal_flag>0){
      pe = GetPE(x,iCh,single_pe);
      if (debug>2){ fprintf(stderr,"gain single_pe %.2f; pe %.2f; calibration flag %i\n",single_pe[iCh],pe,cal_flag);}
      c.charges[iCh] = pe;     
    }
    else {
      c.charges[iCh] = -1;
    }
    

    if (reduction_level == 0) {
      c.channelBitMask |= 1 << iCh;
      c.nSelectedChannels++;
    } else  if (reduction_level == 1) {
      if (pe<0.5){continue;} // We do not store data with pe<1
      c.channelBitMask |= 1 << iCh;
      c.nSelectedChannels++;
    } else {
      fprintf(stderr,"Error: Invalid reduction level %d",reduction_level);
      exit(-1);
    }
    

  }
  
  c.traces = (unsigned short**) malloc(sizeof(unsigned short*) * c.nSelectedChannels);

  /*int iTr = 0;
  for (int i = 23; i >= 0;--i) {
    unsigned int num = 1 << i;
    if (num&c.channelBitMask) {
      c.traces[iTr] = (unsigned short*)malloc(sizeof(unsigned short*) * c.nSamples);
      for (int iSamp = 0; iSamp < c.nSamples; iSamp++)
        c.traces[iTr][iSamp] = x->event.trace[i][iSamp];
      iTr++;
    }
  }*/
  int iTr = 0;
  for (int i = 0; i < 24; i++) {
    unsigned int num = 1 << i;
    if (num&c.channelBitMask) {
      c.traces[iTr] = (unsigned short*)malloc(sizeof(unsigned short*) * c.nSamples);
      for (int iSamp = 0; iSamp < c.nSamples; iSamp++)
        c.traces[iTr][iSamp] = x->event.trace[i][iSamp];
      iTr++;
    }
  }

 
}


void ClearEvent(CardData& c) {
  
  for (int i = 0; i < c.nSelectedChannels; i++) {
    free(c.traces[i]);
  }
  free(c.traces);
}

void EventToCardData(FCIOData* x,CardData& c){
  //copying over some information that is needed
  c.type = x->event.type;
  memcpy(c.timeoffset,x->event.timeoffset,sizeof(x->event.timeoffset));
  memcpy(c.timestamp,x->event.timestamp,sizeof(x->event.timestamp));
  //fprintf(stderr,"time offset copy\n");
  //  for (int i =0; i < 10; i++) fprintf(stderr,"%d\t%d\n",c.timeoffset[i],x->event.timeoffset[i]);
  
  c.nSamples = x->config.eventsamples;
  AnalyseTraces(x,c);
}


void printOptions(){
  fprintf(stderr,"Options:\n");
  fprintf(stderr,"-d debug level 0, 1, 2 \n");
  fprintf(stderr,"-i input FCIO-file \n");
  fprintf(stderr,"-o output XCDF-file \n");
  fprintf(stderr,"-r reduction 0 = no reduction, 1 = data reduction \n");
  fprintf(stderr,"-c calibration txt-file\n\n");
}

int main(int argc, char** argv) {
  fprintf(stderr,"hello flash cam world...\n");
  printOptions();

  // read params for command line
  int i=0;
  while(++i<argc) {
    char *opt=argv[i];
    if(strcmp(opt,"-d")==0) sscanf(argv[++i],"%d",&debug); 		// set debug value 0-10 (0=none,2=warn,3=info,>3=debug)
    else if(strcmp(opt,"-o")==0) 		ofile=argv[++i];			// set output FCIO file (defualt none)
    else if(strcmp(opt,"-i")==0) 		ifile=argv[++i];			// set output FCIO file (defualt none)
    else if(strcmp(opt,"-c")==0) 		calfile=argv[++i];			// set calibration FCIO file (defualt none)
    if(strcmp(opt,"-r")==0) sscanf(argv[++i],"%d",&reduction_level); 		// set reduction level
  }
  //
  if (ifile == 0) {
    fprintf(stderr,"Error, no input file specified\n");
    return 0;
  }
  if (ofile == 0) {
    fprintf(stderr,"Error, no output file specified\n");
    ofile = "out.xcdf";
    fprintf(stderr,"output will be written in out.xcdf \n");
  }
  if (calfile == 0) {
    fprintf(stderr,"Warning, no calibration file specified\n");
    fprintf(stderr,"reduction level will be set 0 \n");
    reduction_level=0;
    cal_flag=-1;
  }
  else cal_flag=InitChargeCalibration(calfile,single_pe);
  
  if(cal_flag==0) {
    fprintf(stderr,"Warning, calibration file %s does not exist or is corrupt\n",calfile);
    return 0;
  }
  
  FCIODebug(debug);
  FCIOData *x = FCIOOpen(ifile,10000,0);
  if (!x) {
    fprintf(stderr,"Failed to open file\n");
    return -1;
  }
  
  XCDFFile fXCDF(ofile,"w");
  //From fcio.h
  // [0] the offset in sec between the master and unix
  // [1] the offset in usec between master and unix
  // [2] the calculated sec which must be added to the master
  // [3] the delta time between master and unix in usec
  // [4] the abs(time) between master and unix in usec
  // [5-9] reserved for future use
  XCDFSignedIntegerField fTimeOffsetMasterUnixSec = fXCDF.AllocateSignedIntegerField("timeOffsetMasterUnixSec",1);
  XCDFSignedIntegerField fTimeOffsetMasterUnixMuSec = fXCDF.AllocateSignedIntegerField("timeOffsetMasterUnixMuSec",1);
  XCDFSignedIntegerField fCalcSecAddToMaster = fXCDF.AllocateSignedIntegerField("calcSecAddToMaster",1);
  XCDFSignedIntegerField fDeltaTimeMasterUnixMuSec = fXCDF.AllocateSignedIntegerField("deltaTimeMasterUnixMuSec",1);
  XCDFSignedIntegerField fAbsTimeMasterUnixMuSec = fXCDF.AllocateSignedIntegerField("absTimeMasterUnixMuSec",1);
  //From fcio.h [0] Event no., [1] PPS, [2] ticks, [3] max. ticks
  XCDFSignedIntegerField fEventNumber = fXCDF.AllocateSignedIntegerField("eventNumber",1);
  XCDFSignedIntegerField fPPS = fXCDF.AllocateSignedIntegerField("pps",1);
  XCDFSignedIntegerField fTicks = fXCDF.AllocateSignedIntegerField("ticks",1);
  XCDFSignedIntegerField fMaxTicks = fXCDF.AllocateSignedIntegerField("maxTicks",1);
  //waveforms
  XCDFSignedIntegerField fTrigType = fXCDF.AllocateSignedIntegerField("trigType",2);
  XCDFUnsignedIntegerField fNSelectChan = fXCDF.AllocateUnsignedIntegerField("nSelectedChannels",1);
  XCDFUnsignedIntegerField fNBinsPerChannel = fXCDF.AllocateUnsignedIntegerField("nBinsPerChannel",1,"nSelectedChannels");
  XCDFUnsignedIntegerField fChannelId = fXCDF.AllocateUnsignedIntegerField("channelId",1,"nSelectedChannels");
  XCDFUnsignedIntegerField fWaveforms = fXCDF.AllocateUnsignedIntegerField("waveforms",1,"nBinsPerChannel");
  //extracted values
  XCDFUnsignedIntegerField fNChannels = fXCDF.AllocateUnsignedIntegerField("nChannels",1);
  XCDFFloatingPointField fSignalTime = fXCDF.AllocateFloatingPointField("fSignalTime", 0.1,"nChannels");//ns
  XCDFFloatingPointField fCharge = fXCDF.AllocateFloatingPointField("fCharge", 0.05,"nChannels");//in pe
  //
  int iotag;
  int nEvent = 0;
  while ((iotag = FCIOGetRecord(x)) > 0) {
    //only processing events
    if (iotag == FCIOConfig) {
      if (debug > 0) {
        fprintf(stderr,"x->config.telid %d \n",x->config.telid);
        fprintf(stderr,"number of ADC's: %d \n",x->config.adcs);
        fprintf(stderr,"number of triggers: %d \n",x->config.triggers);
        fprintf(stderr,"number of event samples: %d \n",x->config.eventsamples);
        fprintf(stderr,"number of adc bits: %d \n",x->config.adcbits);
        fprintf(stderr,"sum length: %d \n",x->config.sumlength);
        fprintf(stderr,"bl precision : %d \n",x->config.blprecision);
        fprintf(stderr,"master cards: %d \n",x->config.mastercards);
        fprintf(stderr,"trigger cards: %d \n",x->config.triggercards);
        fprintf(stderr,"adc cards: %d \n",x->config.adccards);
        fprintf(stderr,"gps flag: %d \n",x->config.gps);
      }
      Free2DFSamples(uptraces);
      uptraces=Allocate2DFSamples(x->config.adcs,x->config.eventsamples*us);

    } else if (iotag == FCIOEvent) {
      CardData c;
      EventToCardData(x,c);
      if (debug > 0) {
        fprintf(stderr, "\nEvent: %d\n",nEvent);
        fprintf(stderr,"nSamples %d\n" ,c.nSamples);
        fprintf(stderr,"selectedChannels: %d\n",c.nSelectedChannels);
      }
      
      fTrigType << c.type;
      fNSelectChan << c.nSelectedChannels;
      for (int iCh = 0; iCh < 24; iCh++) {
        if ( (1<<iCh) & c.channelBitMask)
          fChannelId << iCh;
      }
      
      for (int iCh = 0; iCh < c.nSelectedChannels; iCh++) {
        fNBinsPerChannel << c.nSamples;
        for (int i = 0; i < c.nSamples; i++)
          fWaveforms << c.traces[iCh][i];
      }
      fTimeOffsetMasterUnixSec << c.timeoffset[0];
      fTimeOffsetMasterUnixMuSec << c.timeoffset[1];
      fCalcSecAddToMaster << c.timeoffset[2];
      fDeltaTimeMasterUnixMuSec << c.timeoffset[3];
      fAbsTimeMasterUnixMuSec << c.timeoffset[4];
      
      fEventNumber <<c.timestamp[0] ;
      fPPS <<c.timestamp[1] ;
      fTicks << c.timestamp[2] ;
      fMaxTicks << c.timestamp[3] ;
      fNChannels << x->config.adcs;
      for (int iCh = 0; iCh <x->config.adcs; iCh++) {
        fSignalTime << c.times[iCh];
        fCharge << c.charges[iCh];
      }
      
      fXCDF.Write();
      ClearEvent(c);
      nEvent++;
    } else if (iotag == FCIOStatus) {
      if (debug > 0) {
        fprintf(stderr,"x->status.status %d \n", x->status.status);
        fprintf(stderr,"x->status.statustime[0] %d \n", x->status.statustime[0]);
        fprintf(stderr,"x->status.statustime[1] %d \n", x->status.statustime[1]);
        fprintf(stderr,"x->status.statustime[2] %d \n", x->status.statustime[2]);
        fprintf(stderr,"x->status.statustime[3] %d \n", x->status.statustime[3]);
        fprintf(stderr,"x->status.statustime[4] %d \n", x->status.statustime[4]);
        fprintf(stderr,"x->status.cards %d \n", x->status.cards);
        fprintf(stderr,"x->status.size %d \n", x->status.size);
        
        for (int ic = 0; ic <x->status.cards; ic++) {
          fprintf(stderr,"\t ic %d: x->status.data[ic].reqid %d\n",ic,x->status.data[ic].reqid);
          fprintf(stderr,"\t ic %d: x->status.data[ic].status %d \n",ic,x->status.data[ic].status);
          fprintf(stderr,"\t ic %d: x->status.data[ic].eventno %d \n",ic,x->status.data[ic].eventno);
          fprintf(stderr,"\t ic %d: x->status.data[ic].pps %d \n",ic,x->status.data[ic].pps);
          fprintf(stderr,"\t ic %d: x->status.data[ic].ticks %d \n",ic,x->status.data[ic].ticks);
          fprintf(stderr,"\t ic %d: x->status.data[ic].maxticks %d \n",ic,x->status.data[ic].maxticks);
          fprintf(stderr,"\t ic %d: x->status.data[ic].numenv %d \n",ic,x->status.data[ic].numenv);
          fprintf(stderr,"\t ic %d: x->status.data[ic].numctilinks %d \n",ic,x->status.data[ic].numctilinks);
          fprintf(stderr,"\t ic %d: x->status.data[ic].numlinks %d \n",ic,x->status.data[ic].numlinks);
        }
        fprintf(stderr,"\n");
      }
    }
    else {
      fprintf(stderr,"iotag %d",iotag);
    }
  }

  fprintf(stderr,"N Events %d\n",nEvent);
  
  fXCDF.AddComment("Falcon data format");
  fXCDF.Close();
  return 0;
}
