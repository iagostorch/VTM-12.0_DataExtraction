/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   storchmain.h
 * Author: iagostorch
 *
 * Created on March 17, 2021
 */

#ifndef STORCHMAIN_H
#define STORCHMAIN_H 1


// My Directives
#define EXTRACT_FRAME 0
#define EXTRACT_BLOCK 0
#define EXTRACT_BLOCK_WIDTH 8
#define EXTRACT_BLOCK_HEIGHT 8

#define EXTRACT_AFFINE_MV 0

// This typedef is used to control what type of samples are being exported from the encoder
typedef enum
{
  EXT_RECONSTRUCTED,
  EXT_ORIGINAL,
  REFERENCE,
  FILTERED_REFERENCE,
  PREDICTED,
  EXT_NUM
} SamplesType;


#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <string>

#include "CommonLib/Unit.h"

using namespace std;

class storch {
    
public:
    static int currPoc;
    static int inheritedCand;
    static int constructedCand;
    static int translationalCand;
    static int temporalCand;
    static int zeroCand;
    static int totalCand;
    
    storch();
    static void printSummary();
    static void exportSamplesFrame(PelBuf samples, int POC, SamplesType type);
    static void exportSamplesBlock(CPelBuf samples, SamplesType type);
    static void exportSamples4x4Block(Pel* samples, int xPos, int yPos, int stride, SamplesType type);
    static void exportAffineInfo(PredictionUnit pu, Mv mvLT, Mv mvRT, Mv mvLB, int subX, int subY, int mv_x, int mv_y);
    
#if EXAMPLE || EXAMPLE
    static void exampleFunct();
#endif
    // Tracking execution time
    static void startFullSearch();
    static void finishFullSearch();
    
    static void startAffineAMVP(EAffineModel param, EAffinePred pred);
    static void finishAffineAMVP(EAffineModel param, EAffinePred pred);

    static void startAffineMEGradient(EAffineModel param, EAffinePred pred);
    static void finishAffineMEGradient(EAffineModel param, EAffinePred pred);
    
    static void startAffineComplete(EAffineModel param);
    static void finishAffineComplete(EAffineModel param);
    
    static void startAffineME(EAffineModel param, EAffinePred pred);
    static void finishAffineME(EAffineModel param, EAffinePred pred);

    static void startAffineMESimpRef(EAffineModel param, EAffinePred pred);
    static void finishAffineMESimpRef(EAffineModel param, EAffinePred pred);
    
    static void startAffineInit(EAffineModel param, EAffinePred pred);
    static void finishAffineInit(EAffineModel param, EAffinePred pred);

    static void startAffineUnipred(EAffineModel param, EAffinePred pred);
    static void finishAffineUnipred(EAffineModel param, EAffinePred pred);
    
    static void startAffineBipred(EAffineModel param, EAffinePred pred);
    static void finishAffineBipred(EAffineModel param, EAffinePred pred);    
    
    static void startAffineMEInit(EAffineModel param, EAffinePred pred);
    static void finishAffineMEInit(EAffineModel param, EAffinePred pred);    
        
    
    static int extractedFrames[EXT_NUM][500]; // Maks what frame were already extracted   
    
private:   
    static double fsTime, aff4pTime, aff6pTime, aff4pAMVPTime, aff6pAMVPTime, affUnip4pTime, affBip4pTime, affUnip6pTime, affBip6pTime, affUnip4pInitTime, affBip4pInitTime, affUnip6pInitTime, affBip6pInitTime, affUnip4pMeTime, affBip4pMeTime, affUnip6pMeTime, affBip6pMeTime, affUnip4pMEGradTime, affBip4pMEGradTime, affUnip6pMEGradTime, affBip6pMEGradTime, affUnip4pMERefTime, affBip4pMERefTime, affUnip6pMERefTime, affBip6pMERefTime, affUnip4pMeInitTime, affBip4pMeInitTime, affUnip6pMeInitTime, affBip6pMeInitTime;
    static struct timeval fs1,fs2, aamvp1, aamvp2, ag1, ag2, a4p1, a4p2, a6p1, a6p2, affme1, affme2, sraffme1, sraffme2, affinit1, affinit2, affunip1, affunip2, affbip1, affbip2, affmeinit1, affmeinit2;
    static ofstream affine_file;
    
};

#endif /* STORCHMAIN_H */
