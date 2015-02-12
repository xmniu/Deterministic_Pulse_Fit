#define EXAMPLE1_CPP

#include <iostream>
#include <exception>
#include <vector>
#include <string>
#include "readparameters.hpp"

using namespace std;

void init(char* paramfile) {
    try {
	readparameters rp(paramfile);
	try {
	    //read parameters:
	    unsigned number1 = rp.get<unsigned>("number1");
	    double mW = rp.get<double>("mW");
	    vector<string> specialstrings=rp.get_vector<string>("specialstrings");
	    //write parameters:
	    cout<<"example1\n--------"<<endl;
	    cout<<"number1: "<<number1<<endl;
	    cout<<"mW: "<<mW<<endl; 
	    cout<<"specialstrings: | ";
	    for (vector<string>::const_iterator it=specialstrings.begin();it!=specialstrings.end();++it) cout<<*it<<" | ";
	    cout<<endl;
	}
	catch (exception& e) {	    
	    cerr<<e.what()<<endl;
	}
    }
    catch (exception& e) {
	cerr<<e.what()<<endl;
    }

    return;
}



int main(int argc,char **argv) {
    int ret=0;

    if (argc!=2) {
	cerr<<"Usage: example1 <paramfile>"<<endl;
	ret=1;
    }
    else {
	init(argv[1]);
    }

    return ret;
}
