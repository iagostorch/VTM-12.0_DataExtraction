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
#define EXTRACT_FRAME 1
#define EXTRACT_BLOCK 0
#define EXTRACT_BLOCK_WIDTH 8
#define EXTRACT_BLOCK_HEIGHT 8

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
#include "CommonLib/Buffer.h"

using namespace std;

class storch {
    
public:
    static int currPoc;
    
    storch();
    static void printSummary();
    static void exportSamplesFrame(PelBuf samples, int POC, SamplesType type);
#if EXAMPLE || EXAMPLE
    static void exampleFunct();
#endif
    // Tracking execution time
    static void startFullSearch();
    static void finishFullSearch();

    static int extractedFrames[EXT_NUM][500]; // Maks what frame were already extracted   
    
private:   
    static double fsTime;
    static struct timeval fs1,fs2;
    
};

#endif /* STORCHMAIN_H */
