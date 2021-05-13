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

typedef enum
{
  EXT_RECONSTRUCTED,
  EXT_ORIGINAL,
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
    static void exportAffineInfo(PredictionUnit pu, Mv mvLT, Mv mvRT, Mv mvLB, int subX, int subY, int mv_x, int mv_y);
    
#if EXAMPLE || EXAMPLE
    static void exampleFunct();
#endif
    // Tracking execution time
    static void startFullSearch();
    static void finishFullSearch();
    
    static void startAffineAMVP();
    static void finishAffineAMVP();

    static void startAffineGradientME();
    static void finishAffineGradientME();
    
    static void startAffineComplete(int params);
    static void finishAffineComplete(int params);
    
    static void startAffineME();
    static void finishAffineME();

    static void startSimpRefAffineME();
    static void finishSimpRefAffineME();

    
    static int extractedFrames[EXT_NUM][500]; // Maks what frame were already extracted   
    
private:   
    static double fsTime, affAMVPTime, affGradTime, aff4pTime, aff6pTime, affMeTime, simpRefAffMeTime;
    static struct timeval fs1,fs2, aamvp1, aamvp2, ag1, ag2, a4p1, a4p2, a6p1, a6p2, affme1, affme2, sraffme1, sraffme2;
    static ofstream affine_file;
    
};

#endif /* STORCHMAIN_H */
