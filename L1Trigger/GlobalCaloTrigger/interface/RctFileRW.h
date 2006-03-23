//-------------------------------------------------
//
/**  \class RCTFileRW
 *
 *   RCT file reader/writer
 * 
 *   $Date: 2006/01/16 17:37:05 $
 *   $Revision: 1.2 $
 *
 *   \author  Maria Hansen, University of Bristol
 */  
//   Description of methods below the class definition.
//--------------------------------------------------
#ifndef RCTFILERW_H
#define RCTFILERW_H

#include <fstream>
#include <vector>
#include "Block.h"
#include "RctCableData.h"

using std::ifstream;
using std::ofstream;

/*RCT Input File Format 
  Line 1: Crossing no as "Crossing x" (2)     
  Line 2: isoe0 isoe1 isoe2 isoe3 nonIsoe0 nonIsoe1 nonIso2 nonIso3 (8) 
  Line 3: RC0mip0 RC0mip1 RC1mip0 RC1mip1 RC2mip0 RC2mip1 RC3mip0 RC3mip1 RC4mip0 RC4mip1 
          RC5mip0 RC5mip1 RC6mip0 RC6mip1 (14)
  Line 4: RC0qt0 RCqt1 RC1qt0 RC1qt1 RC2qt0 RC2qt1 RC3qt0 RC3qt1 RC4qt0 RC4qt1 
          RC5qt0 RC5qt1 RC6qt0 RC6qt1 (14)
  Line 5: RC0reg0 RC0reg1 RC1reg0 RC1reg1 RC2reg0 RC2reg1 RC3reg0 RC3reg1 RC4reg0 RC4reg1
          RC5reg0 RC5reg1 RC6reg0 RC6reg1 (14)
  Line 6: HF0eta0 HF0eta1 HF0eta2 HF0eta3 HF1eta0 HF1eta1 HF1eta2 HF1eta3 (8)
  ...
  ... 
  */ 


class RctFileRW {
 public:
  typedef vector<unsigned> dataVector;
  typedef vector<RctCableData> RctCrateData;

  RctFileRW(char inputFile[256]);
  
  ~RctFileRW();

  dataVector getIsoElectrons(int bx);
  dataVector getNonIsoElectrons(int bx);
  dataVector getMipBits(int bx);
  dataVector getQuietBits(int bx);
 
  void readRctFile(); 
  void writeRctFile();

  RctCrateData getData();
  void setData(RctCrateData data);
 
  Block getCable(int nCable); 
  void setCable(Block data); 
  void writeCableFile(Block dataBlock);

 private:
  ifstream file;
  ofstream ofile;
  string _bx;
  int _run;
  RctCrateData theData;

};

//----------------------------------------------------------------------------
//
/*  Constructor RctFileRW(char inputFile[256]) takes a text file as an input.
 *  When an object is instantiated, the file is opened for reading.
 *
 *  void readRctFile() reads the file into the private vector theData, which 
 *  is a RctCableData vector. The entries in the file is stored as RctCableData
 *  objects.
 *  void writeRctFile() writes what's in the private vector theData to a file 
 *  with the same layout as the input Rct file.
 *
 *  void setData(RctCrateData data) fills up the private vector with the data 
 *  passed to it. The data passed is a vector of RctCableData objects.
 *  RctCrateData getData() gets the data in the private theData vector. 
 *  Note, it returns all bunch crossings!
 *  
 *  Block getCable(int nCable) gets the cable nummer given by nCable 
 *  and returns it as a Block, i.e as a vector of 32bit words.
 *  void setCable(Block data) takes the data passed to it as a Block 
 *  and fills the private variable theData (filled into RctCableData
 *  format).
 *  void writeCableFile(Block dataBlock) writes the Block passed to it
 *  as a list of 32bit words.
 *
 *  Below methods returns iso electrons, non-iso electrons, mip bits
 *  and quiet bits for a given bunch crossing, bx. If bx is set to 66
 *  is will return all 64 bunch crossings:
 *  dataVector getIsoElectrons(int bx);
 *  dataVector getNonIsoElectrons(int bx);
 *  dataVector getMipBits(int bx);
 *  dataVector getQuietBits(int bx);
 */
//
//---------------------------------------------------------------------------
#endif

