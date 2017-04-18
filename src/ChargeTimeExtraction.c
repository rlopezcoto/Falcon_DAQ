#include <fstream>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "samples.h"
#include "fcio.h"
#include "timer.h" 
#include "pzpsa.h" 

#include "TH1F.h"
#include "TFile.h"
#include "TF1.h"

#include "ChargeTimeExtraction.h"


void ChargeTimeExtraction(){
  std::cout<<"fine"<<std::endl;

}

//Find the maximum of an event in FADC counts
int GetMaximum(FCIOData *x, int iCh){
  double bl = GetBaseline(x,iCh);
  int es = x->config.eventsamples; 
  double max=0; 
  int imax=0; 
  double tsum=0;
  for(int i1=0;i1<es;i1++) {
    double amp=x->event.trace[iCh][i1]-bl;
    if(amp>max) max=amp, imax=i1; 
    tsum+=amp; // not used for the moment
  }

  return max;
}

//Find the baseline of a trace
float GetBaseline(FCIOData *x,int iCh){
  double bl=1.0*x->event.theader[iCh][0]/x->config.blprecision;

  return bl;
}

//Find the excess of a trace
float GetExcess(FCIOData *x,int iCh){
  double bl = GetBaseline(x,iCh);
  float intsum = 1.0 * x->event.theader[iCh][1]*1.0/x->config.blprecision;
  float excess = (intsum-bl) * x->config.sumlength; 

  return excess;
}

int InitChargeCalibration(const char *file_name,float *single_pe){

  int flag=0;

  std::ifstream ff(file_name);
  if(!ff.is_open()) {
      std::cout << "Error opening calibration file " << file_name << "\n" << std::endl;
      return 0; 
  }

  float dummy;
  std::string line;
  int iCh;
  //float single_pe;
  std::getline(ff, line);
  fprintf(stderr,"Gain per channel taken from the calibration file %s \n", file_name); 
  fprintf(stderr,"%s\n",line.c_str());
  //std::cout<<line<<std::endl;
  while (std::getline(ff, line))
      {
	std::istringstream iss(line);
	if (!(iss >> iCh >> dummy >> flag ))  {
	  fprintf(stderr,"Warning, calibration file %s does not exist or is corrupt\n",file_name);
	  return 0; }
	single_pe[iCh] = dummy;
	fprintf(stderr,"%i \t\t %.2f \t\t %i\n",iCh,single_pe[iCh],flag);
      }
    return flag;
}

float GetPE(FCIOData *x, int iCh, float *single_pe){
  double max=GetMaximum(x,iCh);
  double pe=max/single_pe[iCh];  

  return pe;
}

float GetTime(FCIOData *x, int iCh,float **traces){

  int 	   us=4; 
  float    pz=0.75; 
  int 	   smooth=8; 
  double   mult=1; 
  int      es = x->config.eventsamples;
  float    bl = GetBaseline(x,iCh);
  int 	   sw=0; 
  int 	   ew=100000; 

  float tmax;
  int at;
  PzpsaSmoothUpsample(es,us,x->event.trace[iCh],bl,pz,traces[iCh],&tmax,&at);
  LRAverageFSamplesFilter(es*us,traces[iCh],traces[iCh],2,smooth); 
  at=FindMaximumOfFSamples(es*us,traces[iCh],sw,ew); 
  for(int i1=0; i1<es*us; i1++) if(i1<sw ||i1>ew)  traces[iCh][i1]=0;
  tmax=mult*traces[iCh][at];

  return tmax;
}
