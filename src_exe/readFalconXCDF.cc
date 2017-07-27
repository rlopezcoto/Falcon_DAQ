#include <iostream>
#include <xcdf/XCDF.h>

using namespace std;


int main(int argc, char** argv){
  cout << "XCDF reading of FALCON data " << endl;
  
  if (argc != 2) {
    cout << "usage: ./readFalconXCDF <file.xcdf>" << endl;
    return 0;
  }
  
  XCDFFile fXCDF(argv[1], "r");
  
  XCDFSignedIntegerField fTimeOffsetMasterUnixSec = fXCDF.GetSignedIntegerField("timeOffsetMasterUnixSec");
  XCDFSignedIntegerField fTimeOffsetMasterUnixMuSec = fXCDF.GetSignedIntegerField("timeOffsetMasterUnixMuSec");
  XCDFSignedIntegerField fCalcSecAddToMaster = fXCDF.GetSignedIntegerField("calcSecAddToMaster");
  XCDFSignedIntegerField fDeltaTimeMasterUnixMuSec = fXCDF.GetSignedIntegerField("deltaTimeMasterUnixMuSec");
  XCDFSignedIntegerField fAbsTimeMasterUnixMuSec = fXCDF.GetSignedIntegerField("absTimeMasterUnixMuSec");
  //From fcio.h [0] Event no., [1] PPS, [2] ticks, [3] max. ticks
  XCDFSignedIntegerField fEventNumber = fXCDF.GetSignedIntegerField("eventNumber");
  XCDFSignedIntegerField fPPS = fXCDF.GetSignedIntegerField("pps");
  XCDFSignedIntegerField fTicks = fXCDF.GetSignedIntegerField("ticks");
  XCDFSignedIntegerField fMaxTicks = fXCDF.GetSignedIntegerField("maxTicks");
  //waveforms
  XCDFSignedIntegerField fTrigType = fXCDF.GetSignedIntegerField("trigType");
  XCDFUnsignedIntegerField fNSelectChan = fXCDF.GetUnsignedIntegerField("nSelectedChannels");
  XCDFUnsignedIntegerField fNBinsPerChannel = fXCDF.GetUnsignedIntegerField("nBinsPerChannel");
  XCDFUnsignedIntegerField fChannelId = fXCDF.GetUnsignedIntegerField("channelId");
  XCDFUnsignedIntegerField fWaveforms = fXCDF.GetUnsignedIntegerField("waveforms");
  //extracted values
  XCDFUnsignedIntegerField fNChannels = fXCDF.GetUnsignedIntegerField("nChannels");
  XCDFFloatingPointField fSignalTime = fXCDF.GetFloatingPointField("fSignalTime");
  XCDFFloatingPointField fCharge = fXCDF.GetFloatingPointField("fCharge");
  
  
  while (fXCDF.Read()) {
    if (*fEventNumber %5000 ==0) {
      
      cout << "event number: " << *fEventNumber << endl;
      cout << "number of selected channels " << *fNSelectChan << endl;
      cout << "Channel Ids: ";
      
      int ch = 0;
      int iStart = 0;
      int iStop = 0;
      for (XCDFUnsignedIntegerField::ConstIterator it = fChannelId.Begin();
           it != fChannelId.End(); ++it) {
        cout << "Channel " << *it << ", Samples " << fNBinsPerChannel[ch] <<  ") \n";
        
        cout << "Waveform: ";
        
        iStop += fNBinsPerChannel[ch];
        for (int i = iStart; i < iStop; i++)
          cout << fWaveforms[i] << ", ";
        cout << endl;
        iStart = iStop;
        ch++;
      }
      cout << endl;
    }

  }

  cout << "Read " <<  *fEventNumber << " events" << endl;
  fXCDF.Close();
  
  return 0;
}
