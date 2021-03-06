#define Analysis_cxx

#include "Analysis.h"
#include "readparameters/readparameters.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdlib>

using namespace std;

double round_nplaces(double value, int to){
  int places = 1, whole = value;
  for(int i = 0; i < to; i++) places *= 10;
  value -= whole; //leave decimals
  value *= places; //0.1234 -> 123.4
  value = round(value);//123.4 -> 123
  value /= places; //123 -> .123
  value += whole; //bring the whole value back
  return value;
}

string int2string(int i){
  stringstream ss;
  string ret;
  ss<<i;
  ss>>ret;
  return ret;
}

// Ugly hack to apply energy corrections to some HB- cells
double eCorr(int ieta, int iphi, double energy) {
// return energy correction factor for HBM channels 
// iphi=6 ieta=(-1,-15) and iphi=32 ieta=(-1,-7)
// I.Vodopianov 28 Feb. 2011
  static const float low32[7]  = {0.741,0.721,0.730,0.698,0.708,0.751,0.861};
  static const float high32[7] = {0.973,0.925,0.900,0.897,0.950,0.935,1};
  static const float low6[15]  = {0.635,0.623,0.670,0.633,0.644,0.648,0.600,
				  0.570,0.595,0.554,0.505,0.513,0.515,0.561,0.579};
  static const float high6[15] = {0.875,0.937,0.942,0.900,0.922,0.925,0.901,
				  0.850,0.852,0.818,0.731,0.717,0.782,0.853,0.778};
  
  double slope, mid, en;
  double corr = 1.0;

  if (!(iphi==6 && ieta<0 && ieta>-16) && !(iphi==32 && ieta<0 && ieta>-8)) 
    return corr;

  int jeta = -ieta-1;
  double xeta = (double) ieta;
  if (energy > 0.) en=energy;
  else en = 0.;

  if (iphi == 32) {
    slope = 0.2272;
    mid = 17.14 + 0.7147*xeta;
    if (en > 100.) corr = high32[jeta];
    else corr = low32[jeta]+(high32[jeta]-low32[jeta])/(1.0+exp(-(en-mid)*slope));
  }
  else if (iphi == 6) {
    slope = 0.1956;
    mid = 15.96 + 0.3075*xeta;
    if (en > 100.0) corr = high6[jeta];
    else corr = low6[jeta]+(high6[jeta]-low6[jeta])/(1.0+exp(-(en-mid)*slope));
  }

  return corr;
}

int main(int argc, char **argv)
{
  int ret=0;
  if (argc!=2) {
    cerr<<"Usage: ./Analysis <paramfile>"<<endl;
    ret=1;
  } else {

    readparameters rp(argv[1]);
    //TChain* ch = new TChain("HcalNoiseTree");
    TChain* ch = new TChain("ExportTree/HcalTree");

    string filelistname;

    filelistname=rp.get<string>((string("in_filelist")).c_str());
    string line;
    ifstream filelist(filelistname.c_str());
    if (filelist.fail()) { //catch
      cerr << "\nERROR: Could not open " << filelistname << endl;
      exit(1);
    }
    while (getline(filelist,line)) {
      ch->Add(line.c_str());
    }

  Analysis Ana25ns((TTree*) ch);

  Ana25ns.Init(argv[1]);
  Ana25ns.DefineHistograms();
  Ana25ns.Process();
  Ana25ns.Finish();
  }
 return ret;
}

Analysis::~Analysis() {
}
Analysis::Analysis(TTree *tree):analysistree(tree){};

void Analysis::Init(char* paramfile)
{
  try {
    readparameters rp(paramfile);
    try {Output_File=rp.get<string>("Output_File");}
    catch (exception& e) {cerr<<e.what()<<endl;} 
  } 
  catch (exception& e) {cerr<<e.what()<<endl;} 
  return; 
}

void Analysis::Process() {
  if (fChain == 0) return;

  int nentries = fChain->GetEntries();

  fitfail_hb=0;
  chi2fail_hb=0;
  singlepulse_hb=0;
  doublepulse_hb=0;
  doublepulse_hb_TwoInTime=0;
  doublepulse_hb_TwoOutOfTime=0;
  doublepulse_hb_OneInTime_OneOutOfTime=0;
  fitfail_he=0;
  chi2fail_he=0;
  singlepulse_he=0;
  doublepulse_he=0;
  doublepulse_he_TwoInTime=0;
  doublepulse_he_TwoOutOfTime=0;
  doublepulse_he_OneInTime_OneOutOfTime=0; 

  nevents=0;
  nevents_sel=0;
  hits_sel=0;
  hits_sel_b11=0;
  hits_sel_b12=0;
  hits_sel_b13=0;
  hits_sel_badch=0;
  hits_sel_hb=0;
  hits_sel_he=0;


  cout<<"Number of Entries to Process: "<<nentries<<endl<<endl;

  int nbytes = 0, nb = 0;

  for (int jentry=0; jentry<nentries;jentry++) {
 // for (int jentry=0; jentry<100000;jentry++) {
    int ientry =(int) LoadTree(jentry);
    
    if(nevents%1000==0) cout<<" "<<nevents<<"\t out of  "<<nentries<<"\t have already been processed ("<<round_nplaces((double)nevents/nentries*100,1)<<"%/100%)"<<endl;

    nb = fChain->GetEntry(jentry);   nbytes += nb;

    nevents++;
    MakeCutflow();
    FillHistograms();
  }

/*
  TCanvas *c_tstot_hb_fit = new TCanvas("c_tstot_hb_fit");
  CHARGE_TSTOT_HB_FIT->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HB_FIT->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HB_FIT->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT->Draw();
  c_tstot_hb_fit->SetLogy();
  c_tstot_hb_fit->SaveAs("CHARGE_TSTOT_HB_FIT.png");

  TCanvas *c_tstot_hb_fit_failure = new TCanvas("c_tstot_hb_fit_failure");
  CHARGE_TSTOT_HB_FIT_FAILURE->GetXaxis()->SetTitle("Total Charge [fC]");
  CHARGE_TSTOT_HB_FIT_FAILURE->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT_FAILURE->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HB_FIT_FAILURE->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT_FAILURE->Draw();
  c_tstot_hb_fit_failure->SetLogy();
  c_tstot_hb_fit_failure->SaveAs("CHARGE_TSTOT_HB_FIT_FAILURE.png");

  TCanvas *c_tstot_hb_fit_pulse1 = new TCanvas("c_tstot_hb_fit_pulse1");
  CHARGE_TSTOT_HB_FIT_PULSE1->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HB_FIT_PULSE1->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT_PULSE1->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HB_FIT_PULSE1->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT_PULSE1->Draw();
  c_tstot_hb_fit_pulse1->SetLogy();
  c_tstot_hb_fit_pulse1->SaveAs("CHARGE_TSTOT_HB_FIT_PULSE1.png");

  TCanvas *c_tstot_hb_fit_pulse2 = new TCanvas("c_tstot_hb_fit_pulse2");
  CHARGE_TSTOT_HB_FIT_PULSE2->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HB_FIT_PULSE2->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT_PULSE2->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HB_FIT_PULSE2->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_FIT_PULSE2->Draw();
  c_tstot_hb_fit_pulse2->SetLogy();
  c_tstot_hb_fit_pulse2->SaveAs("CHARGE_TSTOT_HB_FIT_PULSE2.png");

  TCanvas *c_tstot_he_fit = new TCanvas("c_tstot_he_fit");
  CHARGE_TSTOT_HE_FIT->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HE_FIT->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HE_FIT->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT->Draw();
  c_tstot_he_fit->SetLogy();
  c_tstot_he_fit->SaveAs("CHARGE_TSTOT_HE_FIT.png");

  TCanvas *c_tstot_he_fit_failure = new TCanvas("c_tstot_he_fit_failure");
  CHARGE_TSTOT_HE_FIT_FAILURE->GetXaxis()->SetTitle("Total Charge [fC]");
  CHARGE_TSTOT_HE_FIT_FAILURE->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT_FAILURE->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HE_FIT_FAILURE->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT_FAILURE->Draw();
  c_tstot_he_fit_failure->SetLogy();
  c_tstot_he_fit_failure->SaveAs("CHARGE_TSTOT_HE_FIT_FAILURE.png");

  TCanvas *c_tstot_he_fit_pulse1 = new TCanvas("c_tstot_he_fit_pulse1");
  CHARGE_TSTOT_HE_FIT_PULSE1->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HE_FIT_PULSE1->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT_PULSE1->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HE_FIT_PULSE1->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT_PULSE1->Draw();
  c_tstot_he_fit_pulse1->SetLogy();
  c_tstot_he_fit_pulse1->SaveAs("CHARGE_TSTOT_HE_FIT_PULSE1.png");

  TCanvas *c_tstot_he_fit_pulse2 = new TCanvas("c_tstot_he_fit_pulse2");
  CHARGE_TSTOT_HE_FIT_PULSE2->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HE_FIT_PULSE2->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT_PULSE2->GetYaxis()->SetTitle("Hits");
  CHARGE_TSTOT_HE_FIT_PULSE2->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_FIT_PULSE2->Draw();
  c_tstot_he_fit_pulse2->SetLogy();
  c_tstot_he_fit_pulse2->SaveAs("CHARGE_TSTOT_HE_FIT_PULSE2.png");

  
  TCanvas *c_tstot_hb_vs_tstotfit = new TCanvas("c_tstot_hb_vs_tstotfit");
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->GetXaxis()->SetTitle("Total Charge [fC]");
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->GetXaxis()->SetRangeUser(0,1000);
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->GetYaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->GetYaxis()->SetRangeUser(0,1000);
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->GetYaxis()->SetTitleSize(0.05);
  //gStyle->SetPalette(1);
  CHARGE_TSTOT_HB_VS_TSTOT_FIT->Draw("COLZ");
  c_tstot_hb_vs_tstotfit->SaveAs("CHARGE_TSTOT_HB_VS_TSTOT_FIT.png");

  TCanvas *c_tstot_he_vs_tstotfit = new TCanvas("c_tstot_he_vs_tstotfit");
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->GetXaxis()->SetTitle("Total Charge [fC]");
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->GetXaxis()->SetRangeUser(0,1000);
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->GetYaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->GetYaxis()->SetRangeUser(0,1000);
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->GetYaxis()->SetTitleSize(0.05);
  //gStyle->SetPalette(1);
  CHARGE_TSTOT_HE_VS_TSTOT_FIT->Draw("COLZ");
  c_tstot_he_vs_tstotfit->SaveAs("CHARGE_TSTOT_HE_VS_TSTOT_FIT.png");

  TCanvas *c_t_hb_fit = new TCanvas("c_t_hb_fit");
  PULSE_ARRIVAL_HB_FIT->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HB_FIT->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HB_FIT->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT->Draw();
  //c_t_hb_fit->SetLogy();
  c_t_hb_fit->SaveAs("PULSE_ARRIVAL_HB_FIT.png");

  TCanvas *c_t_hb_fit_pulse1 = new TCanvas("c_t_hb_fit_pulse1");
  PULSE_ARRIVAL_HB_FIT_PULSE1->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HB_FIT_PULSE1->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_PULSE1->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HB_FIT_PULSE1->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_PULSE1->Draw();
  c_t_hb_fit_pulse1->SetLogy();
  c_t_hb_fit_pulse1->SaveAs("PULSE_ARRIVAL_HB_FIT_PULSE1.png");

  TCanvas *c_t_hb_fit_pulse2 = new TCanvas("c_t_hb_fit_pulse2");
  PULSE_ARRIVAL_HB_FIT_PULSE2->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HB_FIT_PULSE2->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_PULSE2->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HB_FIT_PULSE2->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_PULSE2->Draw();
  c_t_hb_fit_pulse2->SetLogy();
  c_t_hb_fit_pulse2->SaveAs("PULSE_ARRIVAL_HB_FIT_PULSE2.png");

  TCanvas *c_t_hb_fit_time_diff = new TCanvas("c_t_hb_fit_time_diff");
  PULSE_ARRIVAL_HB_FIT_TIME_DIFF->GetXaxis()->SetTitle("Arrival Time Difference from Fit [ns]");
  PULSE_ARRIVAL_HB_FIT_TIME_DIFF->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_TIME_DIFF->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HB_FIT_TIME_DIFF->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_TIME_DIFF->Draw();
  c_t_hb_fit_time_diff->SetLogy();
  c_t_hb_fit_time_diff->SaveAs("PULSE_ARRIVAL_HB_FIT_TIME_DIFF.png");

  TCanvas *c_t_hb_fit_vs_tstot_fit_pulse1 = new TCanvas("c_t_hb_fit_vs_tstot_fit_pulse1");
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1->GetYaxis()->SetTitle("Total Charge from Fit [fC]");
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1->Draw("COLZ");
  c_t_hb_fit_vs_tstot_fit_pulse1->SaveAs("PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1.png");

  TCanvas *c_t_hb_fit_vs_tstot_fit_pulse2 = new TCanvas("c_t_hb_fit_vs_tstot_fit_pulse2");
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2->GetYaxis()->SetTitle("Total Charge from Fit [fC]");
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2->Draw("COLZ");
  c_t_hb_fit_vs_tstot_fit_pulse2->SaveAs("PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2.png");

  TCanvas *c_t_he_fit = new TCanvas("c_t_he_fit");
  PULSE_ARRIVAL_HE_FIT->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HE_FIT->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HE_FIT->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT->Draw();
 // c_t_he_fit->SetLogy();
  c_t_he_fit->SaveAs("PULSE_ARRIVAL_HE_FIT.png");

  TCanvas *c_t_he_fit_pulse1 = new TCanvas("c_t_he_fit_pulse1");
  PULSE_ARRIVAL_HE_FIT_PULSE1->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HE_FIT_PULSE1->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_PULSE1->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HE_FIT_PULSE1->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_PULSE1->Draw();
  c_t_he_fit_pulse1->SetLogy();
  c_t_he_fit_pulse1->SaveAs("PULSE_ARRIVAL_HE_FIT_PULSE1.png");

  TCanvas *c_t_he_fit_pulse2 = new TCanvas("c_t_he_fit_pulse2");
  PULSE_ARRIVAL_HE_FIT_PULSE2->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HE_FIT_PULSE2->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_PULSE2->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HE_FIT_PULSE2->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_PULSE2->Draw();
  c_t_he_fit_pulse2->SetLogy();
  c_t_he_fit_pulse2->SaveAs("PULSE_ARRIVAL_HE_FIT_PULSE2.png");

  TCanvas *c_t_he_fit_time_diff = new TCanvas("c_t_he_fit_time_diff");
  PULSE_ARRIVAL_HE_FIT_TIME_DIFF->GetXaxis()->SetTitle("Arrival Time Difference from Fit [ns]");
  PULSE_ARRIVAL_HE_FIT_TIME_DIFF->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_TIME_DIFF->GetYaxis()->SetTitle("Hits");
  PULSE_ARRIVAL_HE_FIT_TIME_DIFF->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_TIME_DIFF->Draw();
  c_t_he_fit_time_diff->SetLogy();
  c_t_he_fit_time_diff->SaveAs("PULSE_ARRIVAL_HE_FIT_TIME_DIFF.png");

  TCanvas *c_t_he_fit_vs_tstot_fit_pulse1 = new TCanvas("c_t_he_fit_vs_tstot_fit_pulse1");
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1->GetYaxis()->SetTitle("Total Charge from Fit [fC]");
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1->Draw("COLZ");
  c_t_he_fit_vs_tstot_fit_pulse1->SaveAs("PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1.png");

  TCanvas *c_t_he_fit_vs_tstot_fit_pulse2 = new TCanvas("c_t_he_fit_vs_tstot_fit_pulse2");
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2->GetYaxis()->SetTitle("Total Charge from Fit [fC]");
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2->Draw("COLZ");
  c_t_he_fit_vs_tstot_fit_pulse2->SaveAs("PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2.png");

  TCanvas *c_x_hb_fit = new TCanvas("c_x_hb_fit");
  PULSE_CHI2VAL_HB_FIT->GetXaxis()->SetTitle("Chi2 Value from Fit");
  PULSE_CHI2VAL_HB_FIT->GetXaxis()->SetTitleSize(0.05);
  PULSE_CHI2VAL_HB_FIT->GetYaxis()->SetTitle("Hits");
  PULSE_CHI2VAL_HB_FIT->GetYaxis()->SetTitleSize(0.05);
  PULSE_CHI2VAL_HB_FIT->Draw();
  c_x_hb_fit->SetLogy();
  c_x_hb_fit->SaveAs("PULSE_CHI2VAL_HB_FIT.png");

  TCanvas *c_t_vs_x_hb_fit = new TCanvas("c_t_vs_x_hb_fit");
  PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT->GetYaxis()->SetTitle("Chi2 Value from Fit [ns]");
  PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT->Draw("COLZ");
  c_t_vs_x_hb_fit->SaveAs("PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT.png");

  TCanvas *c_tstot_vs_x_hb_fit = new TCanvas("c_tstot_vs_x_hb_fit");
  CHARGE_TSTOT_HB_VS_CHI2VAL_FIT->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HB_VS_CHI2VAL_FIT->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_VS_CHI2VAL_FIT->GetYaxis()->SetTitle("Chi2 Value from Fit [ns]");
  CHARGE_TSTOT_HB_VS_CHI2VAL_FIT->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HB_VS_CHI2VAL_FIT->Draw("COLZ");
  c_tstot_vs_x_hb_fit->SaveAs("CHARGE_TSTOT_HB_VS_CHI2VAL_FIT.png");

  TCanvas *c_x_he_fit = new TCanvas("c_x_he_fit");
  PULSE_CHI2VAL_HE_FIT->GetXaxis()->SetTitle("Chi2 Value from Fit");
  PULSE_CHI2VAL_HE_FIT->GetXaxis()->SetTitleSize(0.05);
  PULSE_CHI2VAL_HE_FIT->GetYaxis()->SetTitle("Hits");
  PULSE_CHI2VAL_HE_FIT->GetYaxis()->SetTitleSize(0.05);
  PULSE_CHI2VAL_HE_FIT->Draw();
  c_x_he_fit->SetLogy();
  c_x_he_fit->SaveAs("PULSE_CHI2VAL_HE_FIT.png");

  TCanvas *c_t_vs_x_he_fit = new TCanvas("c_t_vs_x_he_fit");
  PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT->GetXaxis()->SetTitle("Arrival Time from Fit [ns]");
  PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT->GetXaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT->GetYaxis()->SetTitle("Chi2 Value from Fit [ns]");
  PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT->GetYaxis()->SetTitleSize(0.05);
  PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT->Draw("COLZ");
  c_t_vs_x_he_fit->SaveAs("PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT.png");

  TCanvas *c_tstot_vs_x_he_fit = new TCanvas("c_tstot_vs_x_he_fit");
  CHARGE_TSTOT_HE_VS_CHI2VAL_FIT->GetXaxis()->SetTitle("Total Charge from Fit [fC]");
  CHARGE_TSTOT_HE_VS_CHI2VAL_FIT->GetXaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_VS_CHI2VAL_FIT->GetYaxis()->SetTitle("Chi2 Value from Fit [ns]");
  CHARGE_TSTOT_HE_VS_CHI2VAL_FIT->GetYaxis()->SetTitleSize(0.05);
  CHARGE_TSTOT_HE_VS_CHI2VAL_FIT->Draw("COLZ");
  c_tstot_vs_x_he_fit->SaveAs("CHARGE_TSTOT_HE_VS_CHI2VAL_FIT.png");

  cout<<"Total Events: "<<nevents<<endl;
  cout<<"Selected Events: "<<nevents_sel<<endl;
  cout<<"Selected Hits: "<<hits_sel<<endl;
  cout<<"Selected Hits Bit11: "<<hits_sel_b11<<endl;
  cout<<"Selected Hits Bit12: "<<hits_sel_b12<<endl;
  cout<<"Selected Hits Bit13: "<<hits_sel_b13<<endl;
  cout<<"Bad Channels: "<<hits_sel_badch<<endl;
  cout<<"Selected Hits HB: "<<hits_sel_hb<<endl;
  cout<<"Selected Hits HE: "<<hits_sel_he<<endl<<endl<<endl;
 
  cout<<"Fit Failure Rate HB: "<<(double)fitfail_hb/hits_sel_hb*100<<endl;
  cout<<"Chi2 Failure Rate HB: "<<(double)chi2fail_hb/(hits_sel_hb-fitfail_hb)*100<<endl;
  cout<<"Single Pulse Fits HB: "<<(double)singlepulse_hb/(hits_sel_hb-fitfail_hb)*100<<endl;
  cout<<"Double Pulse Fits HB: "<<(double)doublepulse_hb/(hits_sel_hb-fitfail_hb)*100<<endl;
  cout<<"Double Pulse Fits HB (Two In-Time Pulses): "<<(double)doublepulse_hb_TwoInTime/doublepulse_hb*100<<endl;
  cout<<"Double Pulse Fits HB (Two Out-of-Time Pulses): "<<(double)doublepulse_hb_TwoOutOfTime/doublepulse_hb*100<<endl;
  cout<<"Double Pulse Fits HB (One In-Time Pulse + One Out-of_Time Pulse): "<<(double)doublepulse_hb_OneInTime_OneOutOfTime/doublepulse_hb*100<<endl;

  cout<<"Fit Failure Rate HE: "<<(double)fitfail_he/hits_sel_he*100<<endl;
  cout<<"Chi2 Failure Rate HE: "<<(double)chi2fail_he/(hits_sel_he-fitfail_he)*100<<endl;
  cout<<"Single Pulse Fits HE: "<<(double)singlepulse_he/(hits_sel_he-fitfail_he)*100<<endl;
  cout<<"Double Pulse Fits HE: "<<(double)doublepulse_he/(hits_sel_he-fitfail_he)*100<<endl;
  cout<<"Double Pulse Fits HE (Two In-Time Pulses): "<<(double)doublepulse_he_TwoInTime/doublepulse_he*100<<endl;
  cout<<"Double Pulse Fits HE (Two Out-of-Time Pulses): "<<(double)doublepulse_he_TwoOutOfTime/doublepulse_he*100<<endl;
  cout<<"Double Pulse Fits HE (One In-Time Pulse + One Out-of_Time Pulse): "<<(double)doublepulse_he_OneInTime_OneOutOfTime/doublepulse_he*100<<endl;
*/
 }
 
void Analysis::DefineHistograms()
{
  fout = new TFile(Output_File.c_str(), "RECREATE");

    RatioPulse = new TH2F("RatioPulse","TS5/TS4 vs TS45",20,0.,2.,100,0.,500.);
    TimeSlewPulse = new TH2F("TimeSlewPulse","Time Slew vs TS45",25,-14.5,10.5,100,0.,500.);

    Norm0 = new TH1F("fC0","Amplitude in Pulse ealier [fC]",100,0.,500.);
    Norm1 = new TH1F("fC1","Amplitude in in-time Pulse [fC]",100,0.,500.);
    Norm2 = new TH1F("fC2","Amplitude in Pulse later [fC]",100,0.,500.);

//    Ped = new TH1F("ped","Pedestal [fC]",100,0.,10.);
//    Time = new TH1F("time","Time [ns]",100,10.,25.);
//    Chi2 = new TH1F("chisq","Chi2",100,0.,100.);

    slewFit = new TF1("slewFit","pol4*expo(5)",-10.,14.);
    slewFit->SetParameters(1.07618e-02,-4.19145e-06,2.70310e-05,-8.71584e-08,1.86597e-07,3.59216e+00,-1.02057e-01);

    logtimeslewFit = new TF1("logtimeslewFit", "[0]+[1]*TMath::Log(x+[2])",0.,500.);
    logtimeslewFit->SetParameters(3.89838e+01, -6.93560e+00, 8.52052e+01);

    exptimeslewFit = new TF1("exptimeslewFit", "[0]+[1]*TMath::Exp([2]*x)",0.,500.);
    exptimeslewFit->SetParameters(-2.69330e+00, 1.09162e+01, -7.60722e-03);


  NUMBER_TS_ABOVE_THR_HB=new TH1D("NUMBER_TS_ABOVE_THR_HB","",11,-0.5,10.5);
  NUMBER_TS_ABOVE_THR_HE=new TH1D("NUMBER_TS_ABOVE_THR_HE","",11,-0.5,10.5);

  ENERGY_FRACTION_VS_TS_HB=new TProfile("ENERGY_FRACTION_VS_TS_HB","",10,-0.5,9.5,0.,1.0,"s");
  ENERGY_FRACTION_VS_TS_HE=new TProfile("ENERGY_FRACTION_VS_TS_HE","",10,-0.5,9.5,0.,1.0,"s");
   

  //PULSE_ARRIVAL_HB_FIT=new TH1D("PULSE_ARRIVAL_HB_FIT","",200,-139.5,60.5);
  PULSE_ARRIVAL_HB_FIT=new TH1D("PULSE_ARRIVAL_HB_FIT","",50,-30.0,30.0);
  PULSE_ARRIVAL_HB_FIT_PULSE1=new TH1D("PULSE_ARRIVAL_HB_FIT_PULSE1","",200,-139.5,60.5);
  PULSE_ARRIVAL_HB_FIT_PULSE2=new TH1D("PULSE_ARRIVAL_HB_FIT_PULSE2","",200,-139.5,60.5);
  PULSE_ARRIVAL_HB_FIT_TIME_DIFF=new TH1D("PULSE_ARRIVAL_HB_FIT_TIME_DIFF","",200,-100,100);
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1=new TH2D("PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE1","",200,-139.5,60.5,150,0,1500);
  PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2=new TH2D("PULSE_ARRIVAL_HB_FIT_VS_TSTOT_FIT_PULSE2","",200,-139.5,60.5,150,0,1500);

  //PULSE_ARRIVAL_HE_FIT=new TH1D("PULSE_ARRIVAL_HE_FIT","",200,-139.5,60.5);
  PULSE_ARRIVAL_HE_FIT=new TH1D("PULSE_ARRIVAL_HE_FIT","",50,-30.0,30.0);
  PULSE_ARRIVAL_HE_FIT_PULSE1=new TH1D("PULSE_ARRIVAL_HE_FIT_PULSE1","",200,-139.5,60.5);
  PULSE_ARRIVAL_HE_FIT_PULSE2=new TH1D("PULSE_ARRIVAL_HE_FIT_PULSE2","",200,-139.5,60.5);
  PULSE_ARRIVAL_HE_FIT_TIME_DIFF=new TH1D("PULSE_ARRIVAL_HE_FIT_TIME_DIFF","",200,-100,100);
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1=new TH2D("PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE1","",200,-139.5,60.5,150,0,1500);
  PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2=new TH2D("PULSE_ARRIVAL_HE_FIT_VS_TSTOT_FIT_PULSE2","",200,-139.5,60.5,150,0,1500);

 
  PULSE_CHI2VAL_HB_FIT=new TH1D("PULSE_CHI2VAL_HB_FIT","",1000,0,100);
  PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT=new TH2D("PULSE_ARRIVAL_HB_VS_CHI2VAL_FIT","",200,-139.5,60.5,1000,0,100);
  CHARGE_TSTOT_HB_VS_CHI2VAL_FIT=new TH2D("CHARGE_TSTOT_HB_VS_CHI2VAL_FIT","",150,0,1500,1000,0,100);
  PULSE_CHI2VAL_HE_FIT=new TH1D("PULSE_CHI2VAL_HE_FIT","",1000,0,100);
  PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT=new TH2D("PULSE_ARRIVAL_HE_VS_CHI2VAL_FIT","",200,-139.5,60.5,1000,0,100);
  CHARGE_TSTOT_HE_VS_CHI2VAL_FIT=new TH2D("CHARGE_TSTOT_HE_VS_CHI2VAL_FIT","",150,0,1500,1000,0,100);

  
  CHARGE_TSTOT_HB_FIT=new TH1D("CHARGE_TSTOT_HB_FIT","",150,0,1500);
  CHARGE_TSTOT_HB_FIT_FAILURE=new TH1D("CHARGE_TSTOT_HB_FIT_FAILURE","",150,0,1500);
  CHARGE_TSTOT_HB_FIT_PULSE1=new TH1D("CHARGE_TSTOT_HB_FIT_PULSE1","",150,0,1500);
  CHARGE_TSTOT_HB_FIT_PULSE2=new TH1D("CHARGE_TSTOT_HB_FIT_PULSE2","",150,0,1500);

  CHARGE_TSTOT_HE_FIT=new TH1D("CHARGE_TSTOT_HE_FIT","",150,0,1500);
  CHARGE_TSTOT_HE_FIT_FAILURE=new TH1D("CHARGE_TSTOT_HE_FIT_FAILURE","",150,0,1500);
  CHARGE_TSTOT_HE_FIT_PULSE1=new TH1D("CHARGE_TSTOT_HE_FIT_PULSE1","",150,0,1500);
  CHARGE_TSTOT_HE_FIT_PULSE2=new TH1D("CHARGE_TSTOT_HE_FIT_PULSE2","",150,0,1500);


  CHARGE_TSTOT_HB_VS_TSTOT_FIT=new TH2D("CHARGE_TSTOT_HB_VS_TSTOT_FIT","",150,0,1500,150,0,1500);
  CHARGE_TSTOT_HE_VS_TSTOT_FIT=new TH2D("CHARGE_TSTOT_HE_VS_TSTOT_FIT","",150,0,1500,150,0,1500);
}

void Analysis::MakeCutflow() 
{
  
  // No longer needed... remove later
  cut[0]=true;
  cut[1]=true;

  if(cut[0]&&cut[1]){
    nevents_sel++;

    for (int j = 0; j < (int)PulseCount; j++) {
      //int Pass_Continue=false;
            
      // This should already have been set and I don't have an input method set up now
      // but just arbitrarily put in things for the constraints at the moment
      
      // move these later if I need to
      bool iPedestalConstraint = true;
      bool iTimeConstraint = true;
      bool iAddPulseJitter = false;
      bool iUnConstrainedFit = false;
      bool iApplyTimeSlew = true;
      double iTS4Min = 5.;
      double iTS4Max = 500.;
      double iPulseJitter = 1.;
      double iTimeMean = -5.5;
      double iTimeSig = 5.;
      double iPedMean = 0.;
      double iPedSig = 0.5;
      double iNoise = 1.;
      double iTMin = -18.;
      double iTMax = 7.;
      double its3Chi2 = 5.;
      double its4Chi2 = 15.;
      double its345Chi2 = 100.;
      double iChargeThreshold = 6.;
      int iFitTimes = 1;


      psFitOOTpuCorr_->setPUParams(iPedestalConstraint,iTimeConstraint,iAddPulseJitter,iUnConstrainedFit,iApplyTimeSlew,
                    iTS4Min, iTS4Max, iPulseJitter,iTimeMean,iTimeSig,iPedMean,iPedSig,iNoise,iTMin,iTMax,its3Chi2,its4Chi2,its345Chi2,
                    iChargeThreshold,HcalTimeSlew::Medium, iFitTimes);

                   
      // Now set the Pulse shape type
      psFitOOTpuCorr_->setPulseShapeTemplate(theHcalPulseShapes_.getShape(105));
      // void PulseShapeFitOOTPileupCorrection::apply(const CaloSamples & cs, const std::vector<int> & capidvec, const HcalCalibrations & calibs, std::vector<double> & correctedOutput)
      // Changing to take the inputs (vectors) Charge and Pedestal and correctedOutput vector for the moment
      // and will remain this way unless we change the ntuple

      std::vector<double> correctedOutput, HLTOutput;
      std::vector<double> inputCaloSample, inputPedestal;
      std::vector<double> inputGain;

     // std::cout << "Fill the input vectors" << std::endl;//DEBUG
      for(int i = 0; i < 10; ++i) {
        // Note: In CMSSW the "Charge" vector is not already pedestal subtracted, unlike here
        // so I add the pedestal back to the charge so we can keep the same CMSSW implementation
        inputCaloSample.push_back(Charge[j][i]+Pedestal[j][i]);
        inputPedestal.push_back(Pedestal[j][i]);
        inputGain.push_back(Gain[j][i]);
      }
      
      //std::cout << "Begin to apply the method" << std::endl;//DEBUG
      psFitOOTpuCorr_->apply(inputCaloSample,inputPedestal,inputGain,correctedOutput);
      //std::cout << "Finish applying the method" << std::endl;//DEBUG
      //if(correctedOutput.size() > 1){std::cout << "Results are: energy = " << correctedOutput.at(0) << " time = " << correctedOutput.at(1) << std::endl;}

	double RatioTS54, TimeSlew, Pulse = 0.;
	hltThing_->apply(inputCaloSample,inputPedestal,inputGain,HLTOutput, RatioTS54, TimeSlew, Pulse, slewFit);
	RatioPulse->Fill(RatioTS54, Pulse);
	TimeSlewPulse->Fill(TimeSlew, Pulse);
	Norm0->Fill(HLTOutput[0]);
	Norm1->Fill(HLTOutput[1]);
	Norm2->Fill(HLTOutput[2]);

      
      // Should do something with the correctedOutput vector here, such as fill a histogram...
      if(IEta[j] < 16 && correctedOutput.size() > 1) {
        if(correctedOutput.at(1) > -99.){
        CHARGE_TSTOT_HB_FIT->Fill(correctedOutput.at(0));
        PULSE_ARRIVAL_HB_FIT->Fill(correctedOutput.at(1));
        }
      } else if(IEta[j] >= 16 && correctedOutput.size() > 1){
       CHARGE_TSTOT_HE_FIT->Fill(correctedOutput.at(0));
       PULSE_ARRIVAL_HE_FIT->Fill(correctedOutput.at(1));
      }
    }//PulseCount
  }//cut[0]
}// End MakeCutflow Function

void Analysis::FillHistograms()
{
}

void Analysis::Finish()
{
    gStyle->SetOptFit(1);
    TimeSlewPulse->Draw("BOX");
    TimeSlewPulse->ProfileY("Y Profile",1,-1,"do")->Fit("pol1");
    TimeSlewPulse->ProfileY("X Profile",1,-1,"do")->Fit("pol1");

  fout->cd();
  fout->Write();
  fout->Close();
}
