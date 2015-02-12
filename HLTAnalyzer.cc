#include <iostream>
#include <cmath>
#include <climits>
#include "HLTAnalyzer.h"
//#include "isFinite.h"


// Here I have created a mostly blank class where the HLT code can be added
// If you do not alter the arguments for the apply function it 
// is already implemented correctly

HLTAnalyzer::HLTAnalyzer() {
}

HLTAnalyzer::~HLTAnalyzer() { 
}

void HLTAnalyzer::apply(const std::vector<double> & inputCharge, const std::vector<double> & inputPedestal, const std::vector<double> & inputGain, std::vector<double> & HLTOutput, double & RatioTS54, double & TimeSlew, double & Pulse, TF1 *slewFit) const
{

   //const unsigned int cssize = cs.size(); // Removed for standalone code
   
   // Initialize the Arrays
   // We later pass these to the function which handles the fitting
   double chargeArr[10]={}, pedArr[10]={}, gainArr[10]={};
   double energyArr[10]={}, pedenArr[10]={};
   double tsTOT = 0, tstrig = 0; // in fC
   double tsTOTen = 0; // in GeV

        int i = 0;
        int Counter = 0;
        double TotalPedestal, AveragePedestal = 0.;
	double TS[10] = {0.};
        double slewpar[4] = {1.38492, -0.00762413, 0., 0.};//pol1
//        double slewpar[4] = {8.02411, -0.0680858, 0.000114372, 0.};//pol2
//        double slewpar[4] = {8.24058, -0.0830289, 0.000253934, -2.62945e-07};//pol3
//        double slewpar[4] = {3.89838e+01, -6.93560e+00, 8.52052e+01, 0.};//log
//        double slewpar[4] = {-2.69330e+00, 1.09162e+01, -7.60722e-03, 0.};//exp


	//slewFit = new TF1("slewFit","pol4*expo(5)",-10.,14.);
	//slewFit->SetParameters(1.07618e-02,-4.19145e-06,2.70310e-05,-8.71584e-08,1.86597e-07,3.59216e+00,-1.02057e-01);
   
   // Loop through all TS and fill arrays
   for(unsigned int ip=0; ip<10; ++ip){
      if( ip >= (unsigned) 10 ) continue; // Too many samples than what we wanna fit (10 is enough...) -> skip them
//       const int capid = capidvec[ip];          // For CMSSW Version, Removed for Standalone  Testing
//       double charge = cs[ip];                  // For CMSSW Version, Removed for Standalone  Testing
//       double ped = calibs.pedestal(capid);     // For CMSSW Version, Removed for Standalone  Testing
//       double gain = calibs.respcorrgain(capid);// For CMSSW Version, Removed for Standalone  Testing
      double gain = inputGain[ip];
      double charge = inputCharge[ip];
      double ped = inputPedestal[ip];

      double energy = charge*gain;
      double peden = ped*gain;

      // Make sure you know if your charge is pedestal subtracted or not
      chargeArr[ip] = charge; pedArr[ip] = ped; gainArr[ip] = gain;
      energyArr[ip] = energy; pedenArr[ip] = peden;
   }

	
	for(int iTS = 0; iTS < 10; iTS++){
		i = iTS;
		TS[i] = chargeArr[i];
		if(i != 4 && i != 5 && TS[i] < 4.0){
			TotalPedestal += TS[i];
			Counter++;
		}
	}

	if(TotalPedestal > 0.){
		AveragePedestal = TotalPedestal / Counter;
	}else{
		AveragePedestal = 0.;
	}

        for(int iTS = 0; iTS < 10; iTS++){
                i = iTS;
                TS[i] = TS[i] - AveragePedestal;
	}

	RatioTS54 = TS[5] / TS[4];
	TimeSlew = -slewFit->GetX(RatioTS54);
	Pulse = TS[5] + TS[4];// Maybe TS[4] Only?

	double amp[3]= {0., 0., 0.};
	SolveEquations(TS, slewpar, amp);

	HLTOutput.clear();
	HLTOutput.push_back(amp[0]);
        HLTOutput.push_back(amp[1]);
        HLTOutput.push_back(amp[2]);

}

void HLTAnalyzer::SolveEquations(Double_t *TS, Double_t *par, Double_t *amp) const{

        double A3, A4, A5;
	double TS24[3] = {0.,0.,0.};
        double TS35[3];
        double TS46[3];
	double TS57[3];

        PulseFraction(TS[3], TS35, par);
        PulseFraction(TS[4], TS46, par);
        PulseFraction(TS[5], TS57, par);

        double a3[3] = {TS35[0], TS35[1], TS35[2]};
        double b3[3] = {0., TS46[0], TS46[1]};
        double c3[3] = {0., 0., TS57[0]}; 
        double d3[3] = {TS[3], TS[4], TS[5]};

	double deno3 = Det3(a3, b3, c3);

        A3 = Det3(d3, b3, c3) / deno3;
        A4 = Det3(a3, d3, c3) / deno3;
        A5 = Det3(a3, b3, d3) / deno3;

	if(A3 < 0){
		double b2[2] = {TS46[0], TS46[1]};
		double c2[2] = {0., TS57[0]};
		double d2[2] = {TS[4], TS[5]};

		double deno2 = Det2(b2, c2);

		A3 = 0.;
		A4 = Det2(d2, c2) / deno2;
		A5 = Det2(b2, d2) / deno2;

		if(A5 < 0){
			A3 = 0.;
			A4 = (TS[4] + TS[5]) / (TS46[0] + TS46[1]);
			A5 = 0.;
		
			if(A4 < 0){
				A3 = 0.;
				A4 = 0.;
				A5 = 0.;
			}
		}
	}else if(A5 < 0){
                double a2[2] = {TS35[0], TS35[1]};
                double b2[2] = {0., TS46[0]};
                double d2[2] = {TS[3], TS[4]};

		double deno2 = Det2(a2, b2);

		A3 = Det2(d2, b2) / deno2;
		A4 = Det2(a2, d2) / deno2;
		A5 = 0.;

		if(A3 < 0){
			A3 = 0.;
			A4 = TS[4] / TS46[0];
			A5 = 0.;

			if(A4 < 0){
                                A3 = 0.;
                                A4 = 0.;
                                A5 = 0.;
			}
		}
	}else if(A4 < 0){
                                A3 = 0.;
                                A4 = 0.;
                                A5 = 0.;
	}

        amp[0] = A3;
        amp[1] = A4;
        amp[2] = A5;
}

double HLTAnalyzer::Det2(double *b, double *c) const{
        return b[1]*c[2]-b[2]*c[1];
}

double HLTAnalyzer::Det3(double *a, double *b, double *c) const{
        return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0]);
}

void HLTAnalyzer::PulseFraction(Double_t fC, Double_t *TS46, Double_t *par) const{

//	static Double_t TS3par[3] = {0.44, -18.6, 5.136}; //Gaussian parameters: norm, mean, sigma for the TS3 fraction

	static Double_t TS4par[3] = {0.71, -5.17, 12.23}; //Gaussian parameters: norm, mean, sigma for the TS4 fraction
	static Double_t TS5par[3] = {0.258, 0.0178, 4.786e-4}; // pol2 parameters for the TS5 fraction
	static Double_t TS6par[4] = {0.06391, 0.002737, 8.396e-05, 1.475e-06};// pol3 parameters for the TS6 fraction

	Double_t tslew = par[0] + par[1]*fC;//pol1
//	Double_t tslew = par[0] + par[1]*fC + par[2]*fC*fC;//pol2
//	Double_t tslew = par[0] + par[1]*fC + par[2]*fC*fC + par[3]*fC*fC*fC;//pol3
//	Double_t tslew = par[0] + par[1]*TMath::Log(fC + par[2]);//log
//	Double_t tslew = par[0] + par[1]*TMath::Exp(par[2]*fC);//exp


//	TS35[0] = TS3par[0]*TMath::Gaus(tslew,TS3par[1],TS3par[2]); // fraction of pulse in the TS3

	TS46[0] = TS4par[0] * TMath::Gaus(tslew,TS4par[1],TS4par[2]); // fraction of pulse in the TS4
	TS46[1] = TS5par[0] + TS5par[1]*tslew + TS5par[2]*tslew*tslew; // fraction of pulse in the T5S
	TS46[2] = TS6par[0] + TS6par[1]*tslew + TS6par[2]*tslew*tslew + TS6par[3]*tslew*tslew*tslew; //fraction of pulse in the TS6

	return;
}
   

