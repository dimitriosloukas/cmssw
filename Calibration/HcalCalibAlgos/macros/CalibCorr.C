#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

class CalibCorr {
public :
  CalibCorr(const std::string& infile, bool debug=false);
  ~CalibCorr() {}

  float getCorr(int run, unsigned int id);
private:
  void                     readCorr(const std::string& infile);
  std::vector<std::string> splitString(const std::string&);
  unsigned int getDetIdHE(int ieta, int iphi, int depth);
  unsigned int getDetId(int subdet, int ieta, int iphi, int depth);
  unsigned int correctDetId(const unsigned int& detId);

  static const     unsigned int nmax_=10;
  bool                          debug_;
  std::map<unsigned int,float>  corrFac_[nmax_];
  std::vector<int>              runlow_;
};

CalibCorr::CalibCorr(const std::string& infile, bool debug) : debug_(debug) {
  readCorr(infile);
}

float CalibCorr::getCorr(int run, unsigned int id) {
  float cfac(1.0);
  int ip(-1);
  for (unsigned int k=0; k<runlow_.size(); ++k) {
    unsigned int i = runlow_.size()-k-1;
    if (run >= runlow_[i]) {
      ip = (int)(i); break;
    }
  }
  if (debug_) std::cout << "Run " << run << " Perdiod " << ip << std::endl;
  unsigned idx = correctDetId(id);
  if (ip >= 0) {
    std::map<unsigned int,float>::iterator itr = corrFac_[ip].find(idx);
    if (itr != corrFac_[ip].end()) cfac = itr->second;
  }
  if (debug_) {
    // The maskings are defined in DataFormats/DetId/interface/DetId.h
    //                      and in DataFormats/HcalDetId/interface/HcalDetId.h
    // The macro does not invoke the classes there and use them
    int subdet = (idx >> 25) & (0x7);
    int depth  = (idx >> 20) & (0xF);
    int zside  = (idx&0x80000)?(1):(-1);
    int ieta   = (idx >> 10) & (0x1FF);
    int iphi   = (idx) & (0x3FF);
    std::cout << "ID " << std::hex << id << std::dec << " (Sub " << subdet 
	      << " eta " << zside*ieta << " phi " << iphi << " depth " << depth
	      << ")  Factor " << cfac << std::endl;
  }
  return cfac;
}

void CalibCorr::readCorr(const std::string& infile) {

  std::ifstream fInput(infile);
  unsigned int ncorr(0);
  if (!fInput.good()) {
    std::cout << "Cannot open file " << infile << std::endl;
  } else {
    char buffer [1024];
    unsigned int all(0), good(0);
    while (fInput.getline(buffer, 1024)) {
      ++all;
      std::string bufferString(buffer);
      if (bufferString.substr(0,5) == "#IOVs") {
	std::vector<std::string> items = splitString(bufferString.substr(6));
	ncorr = items.size() - 1;
	for (unsigned int n=0; n<ncorr; ++n) {
	  int run  = std::atoi (items[n].c_str());
	  runlow_.push_back(run);
	}
	std::cout << ncorr << ":" << runlow_.size() << " Run ranges\n";
	for (unsigned int n=0; n<runlow_.size(); ++n) 
	  std::cout << " [" << n << "] " << runlow_[n];
	std::cout << std::endl;
      } else if (buffer [0] == '#') {
	continue; //ignore other comments
      } else {
	std::vector<std::string> items = splitString(bufferString);
	if (items.size () != ncorr+3) {
	  std::cout << "Ignore  line: " << buffer << std::endl;
	} else {
	  ++good;
	  int   ieta  = std::atoi (items[0].c_str());
	  int   iphi  = std::atoi (items[1].c_str());
	  int   depth = std::atoi (items[2].c_str());
	  unsigned int id = getDetIdHE(ieta,iphi,depth);
	  for (unsigned int n=0; n<ncorr; ++n) {
	    float corrf = std::atof (items[n+3].c_str());
	    if (n<nmax_) corrFac_[n][id] = corrf;
	  }
	  if (debug_) {
	    std::cout << "ID " << std::hex << id << std::dec << ":" << id
		      << " (eta " << ieta << " phi " << iphi << " depth " 
		      << depth << ")";
	    for (unsigned int n=0; n<ncorr; ++n) 
	      std::cout << " " << corrFac_[n][id];
	    std::cout << std::endl;
	  }
	}
      }
    }
    fInput.close();
    std::cout << "Reads total of " << all << " and " << good << " good records"
	      << std::endl;
  }
}

std::vector<std::string> CalibCorr::splitString (const std::string& fLine) {
  std::vector <std::string> result;
  int start = 0;
  bool empty = true;
  for (unsigned i = 0; i <= fLine.size (); i++) {
    if (fLine [i] == ' ' || i == fLine.size ()) {
      if (!empty) {
	std::string item (fLine, start, i-start);
	result.push_back (item);
	empty = true;
      }
      start = i+1;
    } else {
      if (empty) empty = false;
    }
  }
  return result;
}

unsigned int CalibCorr::getDetIdHE(int ieta, int iphi, int depth) {
  return getDetId(2,ieta,iphi,depth);
}

unsigned int CalibCorr::getDetId(int subdet, int ieta, int iphi, int depth) {
  // All numbers used here are described as masks/offsets in 
  // DataFormats/HcalDetId/interface/HcalDetId.h
  unsigned int id_ = ((4<<28)|((subdet&0x7)<<25));
  id_ |= ((0x1000000) | ((depth&0xF)<<20) |
	  ((ieta>0)?(0x80000|(ieta<<10)):((-ieta)<<10)) |
	  (iphi&0x3FF));
  return id_;
}

unsigned int CalibCorr::correctDetId(const unsigned int & detId) {
  // All numbers used here are described as masks/offsets in 
  // DataFormats/HcalDetId/interface/HcalDetId.h
  int subdet = ((detId >> 25) & (0x7));
  int ieta, zside, depth, iphi;
  if ((detId&0x1000000) == 0) {
    ieta   = ((detId >> 7) & 0x3F);
    zside  = (detId&0x2000)?(1):(-1);
    depth  = ((detId >> 14) & 0x1F);
    iphi   = (detId & 0x3F);
  } else {
    ieta   = ((detId >> 10) & 0x1FF);
    zside  = (detId&0x80000)?(1):(-1);
    depth  = ((detId >> 20) & 0xF);
    iphi   = (detId & 0x3FF);
  }
  if (subdet == 0) {
    if (ieta > 16)                    subdet = 2;
    else if (ieta == 16 && depth > 2) subdet = 2;
    else                              subdet = 1;
  }
  unsigned int id = getDetId(subdet,ieta*zside,iphi,depth);
  if ((id != detId) && debug_) 
    std::cout << "Correct Id " << std::hex << detId << " to " << id << std::dec
	      << "(Sub " << subdet << " eta " << ieta*zside << " phi " << iphi
	      << " depth " << depth << ")" << std::endl;
  return id;
}
