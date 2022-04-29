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


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// 
//  MACROS USED TO TRACE AND DEBUG ENCODER DECISIONS
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define TRACE_XCOMPRESSCU 0 // When ebabled: print all calls to xCompressCU, including the block dimension, position, current split series, and encoding modes to be tested

#define TRACE_CTU_COSTS 0 // Trace the best RDCost for each CTU. Works independently from TRACE_XCOMPRESSCU

#define TRACE_CU_COSTS 0 // Trace the best RDCost for each CU. Only works when TRACE_XCOMPRESSCU is enabled

#define TRACE_ENC_MODES 0 // When enabled: print all encoding modes tested by the encoder in each CU (INTRA, INTER, MERGE, SPLITs, ...). Only works when TRACE_XCOMPRESSCU is enabled

#define ONLY_TRACE_AFFINE_SIZES 0 // When enabled: TRACE_ENC_MODES and TRACE_CU_COSTS are used only for block sizes compatible with affine prediction inter_me (16x16 or larger). Only works when TRACE_XCOMPRESSCU is enabled

#define TRACE_INNER_RESULTS_PRED 0 // When enabled: trace the approximate rdcost of the encoding modes. Only works when TRACE_XCOMPRESSCU and TRACE_ENC_MODES is enabled

#define DEBUG_ENABLE_DISABLE_CHILDREN 0 // When enabled: print the modifications on the mechanism used to control when all prediction types are allowed and when only affine prediction is allowed. Only works if CUSTOMIZE_TREE_HEURISTICS and ENFORCE_AFFINE_ON_EXTRA_BLOCKS are enabled

#define TRACE_TRYMODE_DIVERGENCE 0 // When enabled: Shows the divergence between the original and customized tryMode. Only works if TRACE_XCOMPRESSCU and TRACE_ENC_MODES is enabled, while ORIGINAL_TREE_HEURISTICS is disabled


#define EXTRACT_FRAME 0
#define EXTRACT_BLOCK 0
#define EXTRACT_BLOCK_WIDTH 8
#define EXTRACT_BLOCK_HEIGHT 8

#define EXTRACT_AFFINE_MV 0

#define EXTRACT_AME_PROGRESS 0

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// 
//  MACROS THAT MODIFY THE ENCODER DECISIONS
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define CUSTOM_SIZE 16

// One of these three MUST ALWAYS BE TRUE. They control what split heuristics are being used
#define ORIGINAL_TREE_HEURISTICS 0   // When enabled, ALL HEURISTICS OF VTM ARE USED
#define CUSTOMIZE_TREE_HEURISTICS 1  // When enabled, the split heuristics are disabled for blocks with dimensions at least CUSTOM_SIZExCUSTOM_SIZE (i.e., we test all blocks >=CUSTOM_SIZExCUSTOM_SIZE). It ALSO DISABLES some early terminations applied to affine inter prediction. Early terminations for other prediciton modes are not changed.
#define NO_TREE_HEURISTICS 0         // When this is enabled, we AVOID ALL EARLY TERMINATIONS OF BLOCK PARTITIONING. The TERMINATIONS FOR INTER_ME ARE AVOIDED AS WELL. Early terminations for other prediction modes are maintained


// Controls whether the encoder is simulating the corresponding GPU implementation of affine, for 2 and 3CPs independently
// When enabled: Disable AMVP, force predicted MV to 0x0, compute affine distortion using SATD_4x4, and skip the refinement and simplification stages afer Gradient-ME (for both lists L0 and L1)
// When disabled: Conduct affine prediction as the original VTM encoder
#define GPU_ME_2CPs 0 // !!! IMPORTANT !!! IF ANY OF THESE IS TRUE, SIMD_ENABLE MUST BE DISABLED. OTHERWISE THE SATD COMPUTATION IS NOT MODIFIED    
#define GPU_ME_3CPs 0 // !!! IMPORTANT !!! IF ANY OF THESE IS TRUE, SIMD_ENABLE MUST BE DISABLED. OTHERWISE THE SATD COMPUTATION IS NOT MODIFIED    


// This is only valid when CUSTOMIZE_TREE_HEURISTICS==1. Controls what prediciton modes are evailable for extra blocks.
// When enabled: All blocks that the original encoder would not test but we are testing (i.e., we are testing splits that would not be tested, evaluating EXTA BLOCKS) can only be predicted with affine uniprediction
// When disabled: These extra blocks can be predicted with any prediction type (inter, intra, SKIP, ...)
#define ENFORCE_AFFINE_ON_EXTRA_BLOCKS 0


// Controls wheter we are forcing the encoder to test affine with 3 CPs or not
// When enabled: whenever affine with 2 CPs is conducted, we enforce conducting affine with 3 CPs
// When disabled: the original condition for testing 3 CPs is maintained (RD reasonably good when compared to translational ME)
#define ENFORCE_3_CPS 0


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

enum
{
    NOT_FINAL,
    IS_FINAL
};

enum
{
    NOT_FILLER,
    IS_FILLER
};

// Allows an easier handling of affine block sizes with less conditionals
typedef enum{
    _128x128 	= 0,
    _128x64     = 1,
    _64x128 	= 2,
    _64x64 	= 3,
    _64x32 	= 4,
    _32x64 	= 5,
    _64x16 	= 6,
    _16x64 	= 7,
    _32x32 	= 8,
    _32x16 	= 9,
    _16x32 	= 10,
    _16x16 	= 11,
    NUM_SIZES	= 12
} CuSize;

#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <string>

#include "CommonLib/Unit.h"
#include "../EncoderLib/EncModeCtrl.h"

using namespace std;

class storch {
    
public:

    static int skipNonAffineUnipred_Current;
    static UnitArea skipNonAffineUnipred_Current_Area;
    static int skipNonAffineUnipred_Children;
    static UnitArea skipNonAffineUnipred_Children_Area;
    static EncTestModeType skipNonAffineUnipred_Children_TriggerMode;
    
    static int currPoc;
    static int inheritedCand;
    static int constructedCand;
    static int translationalCand;
    static int temporalCand;
    static int zeroCand;
    static int totalCand;
    
    // Allows signaling a "target block" when callingxPredAffineBlk
    static int target_xPredAffineBlk;
        
    storch();
    static void printSummary();
    static void printParamsSummary();
    static void exportSamplesFrame(PelBuf samples, int POC, SamplesType type);
    static void exportSamplesBlock(CPelBuf samples, SamplesType type);
    static void exportSamples4x4Block(Pel* samples, int xPos, int yPos, int stride, SamplesType type);
    static void exportAffineInfo(PredictionUnit pu, Mv mvLT, Mv mvRT, Mv mvLB, int subX, int subY, int mv_x, int mv_y);
    
    static void exportAmeProgressFlag(int is3CPs, int flag);
    static void exportAmeProgressMVs(int is3CPs, Mv mvs[3], int isFiller, int isFinal);
    static void exportAmeProgressBlock(int is3CPs, int refList, int refIdx, PredictionUnit& pu);
    
    static bool isAffineSize(SizeType width, SizeType height);
    
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
      
    static void startAffineMEGradientEquations(EAffineModel param, EAffinePred pred);
    static void finishAffineMEGradientEquations(EAffineModel param, EAffinePred pred);
    
    static void startAffineMEGradientEquations_build(EAffineModel param, EAffinePred pred);
    static void finishAffineMEGradientEquations_build(EAffineModel param, EAffinePred pred);
    
    static void startAffineMEGradientEquations_solve(EAffineModel param, EAffinePred pred);
    static void finishAffineMEGradientEquations_solve(EAffineModel param, EAffinePred pred);
    
    static void startAffineMEGradientPred(EAffineModel param, EAffinePred pred);
    static void finishAffineMEGradientPred(EAffineModel param, EAffinePred pred);

    // Functions for measuring the execution time of 128x128 CUs (uniprediction with L0 and L1)
    static void startAffineUnipred_128x128(EAffineModel param, EAffinePred pred);
    static void finishAffineUnipred_128x128(EAffineModel param, EAffinePred pred);
    
    static void startAffineAmvpInit_128x128(EAffineModel param, EAffinePred pred);
    static void finishAffineAmvpInit_128x128(EAffineModel param, EAffinePred pred);
    
    static void startAffineGradRefSimp_128x128(EAffineModel param, EAffinePred pred);
    static void finishAffineGradRefSimp_128x128(EAffineModel param, EAffinePred pred);
    
    // Functions for measuring th execution time of specific CU sizes
    static void startAffineAmvpInit_size(EAffineModel param, EAffinePred pred, CuSize size);
    static void finishAffineAmvpInit_size(EAffineModel param, EAffinePred pred, CuSize size);
    static void startAffineGradRefSimp_size(EAffineModel param, EAffinePred pred, CuSize size);
    static void finishAffineGradRefSimp_size(EAffineModel param, EAffinePred pred, CuSize size);
    
    static void startAffineUnipred_size(EAffineModel param, EAffinePred pred, CuSize size);
    static void finishAffineUnipred_size(EAffineModel param, EAffinePred pred, CuSize size);
    
    // Returns an ENUM based on block size for easier handling
    static CuSize getSizeEnum(PredictionUnit pu);
    
    static void setPROF(int p);
    
    static int extractedFrames[EXT_NUM][500]; // Maks what frame were already extracted   
    
private:   
    //Functions used to verify if the combination of macros is correct
    static void verifyTreeHeuristicsGpuMeMacros();
    static void verifyTraceMacros();
  
  
    static double fsTime, aff4pTime, aff6pTime, aff4pAMVPTime, aff6pAMVPTime, affUnip4pTime, affBip4pTime, affUnip6pTime, affBip6pTime, affUnip4pInitTime, affBip4pInitTime, affUnip6pInitTime, affBip6pInitTime, affUnip4pMeTime, affBip4pMeTime, affUnip6pMeTime, affBip6pMeTime, affUnip4pMEGradTime, affBip4pMEGradTime, affUnip6pMEGradTime, affBip6pMEGradTime, affUnip4pMERefTime, affBip4pMERefTime, affUnip6pMERefTime, affBip6pMERefTime, affUnip4pMeInitTime, affBip4pMeInitTime, affUnip6pMeInitTime, affBip6pMeInitTime;
    static double affUnip4pMEGradTime_pred, affBip4pMEGradTime_pred, affUnip6pMEGradTime_pred, affBip6pMEGradTime_pred, affUnip4pMEGradTime_eq, affBip4pMEGradTime_eq, affUnip6pMEGradTime_eq, affBip6pMEGradTime_eq, affUnip4pMEGradTime_eq_build, affUnip4pMEGradTime_eq_solve, affBip4pMEGradTime_eq_build, affBip4pMEGradTime_eq_solve, affUnip6pMEGradTime_eq_build, affUnip6pMEGradTime_eq_solve, affBip6pMEGradTime_eq_build, affBip6pMEGradTime_eq_solve;
    
    // Used to track the affine uniprediction in specific block sizes
    static double affAmvpInit4pTime_128x128, gradRefSimp4pTime_128x128, affUnip4pTime_128x128;
    static double affAmvpInit4pTime_128x64, gradRefSimp4pTime_128x64, affUnip4pTime_128x64;
    static double affAmvpInit4pTime_64x128, gradRefSimp4pTime_64x128, affUnip4pTime_64x128;
    static double affAmvpInit4pTime_64x64, gradRefSimp4pTime_64x64, affUnip4pTime_64x64;
    static double affAmvpInit4pTime_64x32, gradRefSimp4pTime_64x32, affUnip4pTime_64x32;
    static double affAmvpInit4pTime_32x64, gradRefSimp4pTime_32x64, affUnip4pTime_32x64;
    static double affAmvpInit4pTime_64x16, gradRefSimp4pTime_64x16, affUnip4pTime_64x16;
    static double affAmvpInit4pTime_16x64, gradRefSimp4pTime_16x64, affUnip4pTime_16x64;
    static double affAmvpInit4pTime_32x32, gradRefSimp4pTime_32x32, affUnip4pTime_32x32;
    static double affAmvpInit4pTime_32x16, gradRefSimp4pTime_32x16, affUnip4pTime_32x16;
    static double affAmvpInit4pTime_16x32, gradRefSimp4pTime_16x32, affUnip4pTime_16x32;
    static double affAmvpInit4pTime_16x16, gradRefSimp4pTime_16x16, affUnip4pTime_16x16;
    
    static double affAmvpInit6pTime_128x128, gradRefSimp6pTime_128x128, affUnip6pTime_128x128;
    static double affAmvpInit6pTime_128x64, gradRefSimp6pTime_128x64, affUnip6pTime_128x64;
    static double affAmvpInit6pTime_64x128, gradRefSimp6pTime_64x128, affUnip6pTime_64x128;
    static double affAmvpInit6pTime_64x64, gradRefSimp6pTime_64x64, affUnip6pTime_64x64;
    static double affAmvpInit6pTime_64x32, gradRefSimp6pTime_64x32, affUnip6pTime_64x32;
    static double affAmvpInit6pTime_32x64, gradRefSimp6pTime_32x64, affUnip6pTime_32x64;
    static double affAmvpInit6pTime_64x16, gradRefSimp6pTime_64x16, affUnip6pTime_64x16;
    static double affAmvpInit6pTime_16x64, gradRefSimp6pTime_16x64, affUnip6pTime_16x64;
    static double affAmvpInit6pTime_32x32, gradRefSimp6pTime_32x32, affUnip6pTime_32x32;
    static double affAmvpInit6pTime_32x16, gradRefSimp6pTime_32x16, affUnip6pTime_32x16;
    static double affAmvpInit6pTime_16x32, gradRefSimp6pTime_16x32, affUnip6pTime_16x32;
    static double affAmvpInit6pTime_16x16, gradRefSimp6pTime_16x16, affUnip6pTime_16x16;
    
    static struct timeval fs1,fs2, aamvp1, aamvp2, ag1, ag2, a4p1, a4p2, a6p1, a6p2, affme1, affme2, sraffme1, sraffme2, affinit1, affinit2, affunip1, affunip2, affbip1, affbip2, affmeinit1, affmeinit2;   
    static clock_t clock_agp1, clock_agp2, clock_age1, clock_age2, clock_ageb1, clock_ageb2, clock_ages1, clock_ages2;
    
    static struct timeval amvpInit_128x128_1, amvpInit_128x128_2, gradRefSimp_128x128_1, gradRefSimp_128x128_2, blockPred_128x128_1, blockPred_128x128_2, affunip_128x128_1, affunip_128x128_2;
    
    // Used to probe the start/finish time of affine predicitn in specific block sizes
    static struct timeval amvpInit_target_1, amvpInit_target_2, gradRefSimp_target_1, gradRefSimp_target_2, blockPred_target_1, blockPred_target_2, affunip_target_1, affunip_target_2;
    
    static ofstream affine_file;
    static ofstream affine_me_2cps_file, affine_me_3cps_file;
    static char fillerChar;
    
    static int prof; // Keep the value of PROF parameter
            
};

#endif /* STORCHMAIN_H */
