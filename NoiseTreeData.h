//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sat Jun 15 18:13:11 2013 by ROOT version 5.34/05
// from TTree HcalNoiseTree/Hcal noise tree version 1,2134
// found on file: NoiseTree_107.root
//////////////////////////////////////////////////////////

#ifndef NoiseTreeData_h
#define NoiseTreeData_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.

class NoiseTreeData {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   Long64_t        RunNumber;
   Long64_t        EventNumber;
   Long64_t        LumiSection;
   Long64_t        Bunch;
   Long64_t        Orbit;
   Long64_t        Time;

   Int_t           PulseCount;
   Double_t        Gain[5184][10];
   Double_t        Charge[5184][10];
   Double_t        Pedestal[5184][10];
   Double_t        Energy[5184];
   Int_t           IEta[5184];
   Int_t           IPhi[5184];
   Int_t           Depth[5184];

   // List of branches
   TBranch        *b_RunNumber;   //!
   TBranch        *b_EventNumber;   //!
   TBranch        *b_LumiSection;   //!
   TBranch        *b_Bunch;   //!
   TBranch        *b_Orbit;   //!
   TBranch        *b_Time;   //!
   TBranch        *b_PulseCount;
   TBranch        *b_Gain;   //!
   TBranch        *b_Charge;   //!
   TBranch        *b_Pedestal;   //!
   TBranch        *b_Energy;   //!
   TBranch        *b_IEta;   //!
   TBranch        *b_IPhi;   //!
   TBranch        *b_Depth;   //!

   NoiseTreeData(TTree *tree=0);
   ~NoiseTreeData();
   Int_t    Cut(Long64_t entry);
   Int_t    GetEntry(Long64_t entry);
   Long64_t LoadTree(Long64_t entry);
   void     Init(TTree *tree);
   void     Loop();
   Bool_t   Notify();
   void     Show(Long64_t entry = -1);
};

#endif

#ifdef NoiseTreeData_cxx
NoiseTreeData::NoiseTreeData(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("NoiseTree_107.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("NoiseTree_107.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("NoiseTree_107.root:/ExportTree");
      dir->GetObject("HcalNoiseTree",tree);

   }
   Init(tree);
}

NoiseTreeData::~NoiseTreeData()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t NoiseTreeData::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t NoiseTreeData::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void NoiseTreeData::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("RunNumber", &RunNumber, &b_RunNumber);
   fChain->SetBranchAddress("EventNumber", &EventNumber, &b_EventNumber);
   fChain->SetBranchAddress("LumiSection", &LumiSection, &b_LumiSection);
   fChain->SetBranchAddress("Bunch", &Bunch, &b_Bunch);
   fChain->SetBranchAddress("Orbit", &Orbit, &b_Orbit);
   fChain->SetBranchAddress("Time", &Time, &b_Time);
   fChain->SetBranchAddress("Gain", &Gain, &b_Gain);
   fChain->SetBranchAddress("PulseCount", &PulseCount, &b_PulseCount);
   fChain->SetBranchAddress("Charge", Charge, &b_Charge);
   fChain->SetBranchAddress("Pedestal", Pedestal, &b_Pedestal);
   fChain->SetBranchAddress("IEta", IEta, &b_IEta);
   fChain->SetBranchAddress("IPhi", IPhi, &b_IPhi);
   fChain->SetBranchAddress("Depth", Depth, &b_Depth);
   Notify();
}

Bool_t NoiseTreeData::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void NoiseTreeData::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t NoiseTreeData::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef NoiseTreeData_cxx
