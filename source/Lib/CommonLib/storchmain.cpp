/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   storchmain.cpp
 * Author: iagostorch
 * 
 * Created on March 17, 2021
 */

#include "storchmain.h"
// These are used to access methods of object parameters
#include "CodingStructure.h"
#include "Picture.h"

using namespace std;

double storch::aff4pTime, storch::aff6pTime, storch::aff4pAMVPTime, storch::aff6pAMVPTime, storch::affUnip4pTime, storch::affBip4pTime, storch::affUnip6pTime, storch::affBip6pTime, storch::affUnip4pInitTime, storch::affBip4pInitTime, storch::affUnip6pInitTime, storch::affBip6pInitTime, storch::affUnip4pMeTime, storch::affBip4pMeTime, storch::affUnip6pMeTime, storch::affBip6pMeTime, storch::affUnip4pMEGradTime, storch::affBip4pMEGradTime, storch::affUnip6pMEGradTime, storch::affBip6pMEGradTime, storch::affUnip4pMERefTime, storch::affBip4pMERefTime, storch::affUnip6pMERefTime, storch::affBip6pMERefTime, storch::affUnip4pMeInitTime, storch::affBip4pMeInitTime, storch::affUnip6pMeInitTime, storch::affBip6pMeInitTime;
double storch::affUnip4pMEGradTime_pred, storch::affBip4pMEGradTime_pred, storch::affUnip6pMEGradTime_pred, storch::affBip6pMEGradTime_pred, storch::affUnip4pMEGradTime_eq, storch::affBip4pMEGradTime_eq, storch::affUnip6pMEGradTime_eq, storch::affBip6pMEGradTime_eq, storch::affUnip4pMEGradTime_eq_build, storch::affUnip4pMEGradTime_eq_solve, storch::affBip4pMEGradTime_eq_build, storch::affBip4pMEGradTime_eq_solve, storch::affUnip6pMEGradTime_eq_build, storch::affUnip6pMEGradTime_eq_solve, storch::affBip6pMEGradTime_eq_build, storch::affBip6pMEGradTime_eq_solve;
int storch::ET_aligned2CPs, storch::ET_aligned3CPs, storch::ET_half2CPs, storch::ET_half3CPs;

// These are used to track if affine was conducted for the current block, and to track what is the current split series (i.e., sequence of splits) when it is not directly available
bool storch::testedAffine2CP, storch::testedAffine3CP;
SplitSeries storch::currSplitSeries;

int storch::calls_unipred[2][NUM_SIZES][NUM_ALIGNMENTS]; // Number of times affine unipred is conducted for each CP and CU Size (multiple INTER_ME on the same CU count only once)
int storch::numberUniqBlocks[2][NUM_SIZES][NUM_ALIGNMENTS]; // Number of times affine unipred is conducted for each CP and CU Size with unique split series(multiple INTER_ME on the same CU count only once, if the same split series is aplied a second time it is not computed here)
double storch::unipredTimeUniqueBlocks[2][NUM_SIZES][NUM_ALIGNMENTS]; // Affine time for each CP and cu size considering only unique blocks

// positionAndSplitseries is a struct that holds the block position and splitSeries
set<positionAndSplitseries> storch::cusTestedWithAffine[2][NUM_SIZES][NUM_ALIGNMENTS]; // Used to track if a block with SIZE, at position @(x,y), was already encoded with a SPECIFIC split series


// Used to track the execution time of affine prediction

// Affine time for each CP and cu size
double storch::affAmvpInitTime[2][NUM_SIZES][NUM_ALIGNMENTS], storch::gradRefSimpTime[2][NUM_SIZES][NUM_ALIGNMENTS], storch::affUnipTime[2][NUM_SIZES][NUM_ALIGNMENTS];
double storch::gradientMeTime[2][NUM_SIZES][NUM_ALIGNMENTS], storch::refSimpMeTime[2][NUM_SIZES][NUM_ALIGNMENTS];

// Allows signaling a "target block" when calling the xPredAffineBlk
int storch::target_xPredAffineBlk;

int storch::targetAffine; // This can be used to debug the encoding for specific CUs

// When this is true, we are saying that "the original encoder would not evaluate this block, but we are evaluating this block considering only the affine prediction"
int storch::skipNonAffineUnipred_Current, storch::skipNonAffineUnipred_Children;
UnitArea storch::skipNonAffineUnipred_Current_Area, storch::skipNonAffineUnipred_Children_Area;
EncTestModeType storch::skipNonAffineUnipred_Children_TriggerMode;

struct timeval storch::aamvp1, storch::aamvp2, storch::ag1, storch::ag2, storch::a4p1, storch::a4p2, storch::a6p1, storch::a6p2, storch::affme1, storch::affme2, storch::sraffme1, storch::sraffme2, storch::affinit1, storch::affinit2, storch::affunip1, storch::affunip2, storch::affbip1, storch::affbip2, storch::affmeinit1, storch::affmeinit2;
struct timeval storch::amvpInit_128x128_1, storch::amvpInit_128x128_2, storch::gradRefSimp_128x128_1, storch::gradRefSimp_128x128_2, storch::blockPred_128x128_1, storch::blockPred_128x128_2, storch::affunip_128x128_1, storch::affunip_128x128_2;

// Used to probe the begin/finish times of affine predicitn in each block size
struct timeval storch::amvpInit_target_1, storch::amvpInit_target_2, storch::gradRefSimp_target_1, storch::gradRefSimp_target_2, storch::affunip_target_1, storch::affunip_target_2;
struct timeval storch::gradProbe1, storch::gradProbe2, storch::refSimpProbeProbe1, storch::refSimpProbeProbe2;

clock_t storch::clock_agp1, storch::clock_agp2, storch::clock_age1, storch::clock_age2, storch::clock_ageb1, storch::clock_ageb2, storch::clock_ages1, storch::clock_ages2;
int storch::extractedFrames[EXT_NUM][500];
int storch::currPoc;
int storch::inheritedCand, storch::constructedCand, storch::translationalCand, storch::temporalCand, storch::zeroCand, storch::totalCand;
char storch::fillerChar;
std::ofstream storch::affine_file;
std::ofstream storch::affine_me_2cps_file, storch::affine_me_3cps_file;

int storch::prof; // Keep the value of the PROF parameter

storch::storch() {
    aff4pTime = 0.0;
    aff6pTime = 0.0;
    aff4pAMVPTime = 0.0;
    aff6pAMVPTime = 0.0;
    affUnip4pTime = 0.0;
    affBip4pTime = 0.0;
    affUnip6pTime = 0.0;
    affBip6pTime = 0.0;
    affUnip4pInitTime = 0.0;
    affBip4pInitTime = 0.0;
    affUnip6pInitTime = 0.0;
    affBip6pInitTime = 0.0;
    affUnip4pMeTime = 0.0;
    affBip4pMeTime = 0.0;
    affUnip6pMeTime = 0.0;
    affBip6pMeTime = 0.0;
    affUnip4pMEGradTime = 0.0;
    affBip4pMEGradTime = 0.0;
    affUnip6pMEGradTime = 0.0;
    affBip6pMEGradTime = 0.0;
    affUnip4pMERefTime = 0.0;
    affBip4pMERefTime = 0.0;
    affUnip6pMERefTime = 0.0;
    affBip6pMERefTime = 0.0;
    
    affUnip4pMEGradTime_pred = 0.0;
    affBip4pMEGradTime_pred = 0.0;
    affUnip6pMEGradTime_pred = 0.0;
    affBip6pMEGradTime_pred = 0.0;
    affUnip4pMEGradTime_eq = 0.0;
    affBip4pMEGradTime_eq = 0.0;
    affUnip6pMEGradTime_eq = 0.0;
    affBip6pMEGradTime_eq = 0.0;
    affUnip4pMEGradTime_eq_build = 0.0;
    affUnip4pMEGradTime_eq_solve = 0.0;
    affBip4pMEGradTime_eq_build = 0.0;
    affBip4pMEGradTime_eq_solve = 0.0;
    affUnip6pMEGradTime_eq_build = 0.0;
    affUnip6pMEGradTime_eq_solve = 0.0;
    affBip6pMEGradTime_eq_build = 0.0;
    affBip6pMEGradTime_eq_solve = 0.0;
    
    targetAffine = 0;
    testedAffine2CP = false;
    testedAffine3CP = false;
    for(int i=0; i<NUM_SIZES; i++){ 
      for(int align=0; align<NUM_ALIGNMENTS; align++){
        affAmvpInitTime[AFFINEMODEL_4PARAM][i][align] = 0.0;
        affAmvpInitTime[AFFINEMODEL_6PARAM][i][align] = 0.0;
        gradRefSimpTime[AFFINEMODEL_4PARAM][i][align] = 0.0;
        gradRefSimpTime[AFFINEMODEL_6PARAM][i][align] = 0.0;
        gradientMeTime[AFFINEMODEL_4PARAM][i][align] = 0.0;
        gradientMeTime[AFFINEMODEL_6PARAM][i][align] = 0.0;
        refSimpMeTime[AFFINEMODEL_4PARAM][i][align] = 0.0;
        refSimpMeTime[AFFINEMODEL_6PARAM][i][align] = 0.0;
        affUnipTime[AFFINEMODEL_4PARAM][i][align] = 0.0;
        affUnipTime[AFFINEMODEL_6PARAM][i][align] = 0.0;
        unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][i][align] = 0.0;
        unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][i][align] = 0.0;
        calls_unipred[AFFINEMODEL_4PARAM][i][align] = 0;
        calls_unipred[AFFINEMODEL_6PARAM][i][align] = 0;
      }
      
      // Initializes an empty set
      for(int align=0; align<NUM_ALIGNMENTS; align++){
        cusTestedWithAffine[AFFINEMODEL_4PARAM][i][align].clear();
        cusTestedWithAffine[AFFINEMODEL_6PARAM][i][align].clear();
      }
    }

    target_xPredAffineBlk = 0;
       
    skipNonAffineUnipred_Current = 0;
    skipNonAffineUnipred_Children = 0;
    
    skipNonAffineUnipred_Current_Area = UnitArea();
    skipNonAffineUnipred_Children_Area = UnitArea();
    
    currPoc = 0;
    
    prof = 0;
    
    inheritedCand = 0;
    constructedCand = 0;
    translationalCand = 0;
    temporalCand = 0;
    zeroCand = 0;
    totalCand = 0;
    
    
    ET_aligned2CPs = 0;
    ET_aligned3CPs = 0;
    ET_half2CPs = 0;
    ET_half3CPs = 0;
    
    
    fillerChar = 'x';
    
    // Print the header of the file
    if(EXTRACT_AFFINE_MV){
        string affineFileName = (string) "affine_mv.csv";
        affine_file.open(affineFileName);    
        
        affine_file << "POC,frameWidth,frameHeight,";
        affine_file << "puX,puY,puWidth,puHeight,biPred,";
        affine_file << "nCP,LT_X,LT_Y,RT_X,RT_Y,LB_X,LB_Y,";
        affine_file << "subX,subY,mv_X,mv_Y" << endl;
        
    }
    
    if(EXTRACT_AME_PROGRESS){
        affine_me_2cps_file.open("affine_progress_2CPs.csv");
        affine_me_3cps_file.open("affine_progress_3CPs.csv");
        
        
        affine_me_2cps_file << "POC,List,Ref,X,Y,W,H,";
        affine_me_2cps_file << "AMVP_0_X,AMVP_0_Y,AMVP_1_X,AMVP_1_Y,AMVP_2_X,AMVP_2_Y,";
        affine_me_2cps_file << "INIT_0_X,INIT_0_Y,INIT_1_X,INIT_1_Y,INIT_2_X,INIT_2_Y,";
        affine_me_2cps_file << "isGradient,";
        affine_me_2cps_file << " GRAD_0_X,GRAD_0_Y,GRAD_1_X,GRAD_1_Y,GRAD_2_X,GRAD_2_Y,";
        affine_me_2cps_file << "isRefinement,";
        affine_me_2cps_file << "FINAL_0_X,FINAL_0_Y,FINAL_1_X,FINAL_1_Y,FINAL_2_X,FINAL_2_Y" << endl;
        
        affine_me_3cps_file << "POC,List,Ref,X,Y,W,H,";
        affine_me_3cps_file << "AMVP_0_X,AMVP_0_Y,AMVP_1_X,AMVP_1_Y,AMVP_2_X,AMVP_2_Y,";
        affine_me_3cps_file << "INIT_0_X,INIT_0_Y,INIT_1_X,INIT_1_Y,INIT_2_X,INIT_2_Y,";
        affine_me_3cps_file << "isGradient,";
        affine_me_3cps_file << " GRAD_0_X,GRAD_0_Y,GRAD_1_X,GRAD_1_Y,GRAD_2_X,GRAD_2_Y,";
        affine_me_3cps_file << "isRefinement,";
        affine_me_3cps_file << "FINAL_0_X,FINAL_0_Y,FINAL_1_X,FINAL_1_Y,FINAL_2_X,FINAL_2_Y" << endl;
        
    }
       
    for(int f=0; f<500; f++){
        for(int t=0; t<EXT_NUM; t++){
            extractedFrames[t][f]=0;   // at the start, no frame was extracted
        }
    }
    
    // Verify if the combination of macros is allowed
    verifyTreeHeuristicsGpuMeMacros();
    verifyTraceMacros();
}

void storch::setPROF(int p){
  prof = p;
}

void storch::printParamsSummary(){
  cout << endl << "---------------------------------------------------------------------" << endl;
  cout << "Relevant encoding parameters and macros" << endl;
  cout << "GPU_ME_2CPs:              " << GPU_ME_2CPs << endl;
  cout << "GPU_ME_3CPs:              " << GPU_ME_3CPs << endl;
  cout << "PREDICT_3CPs_FROM_2CPs    " << PREDICT_3CPs_FROM_2CPs << endl;
  cout << "SIMD_ENABLE:              " << SIMD_ENABLE << endl;
  cout << "PROF:                     " << prof << endl;
  cout << "ORIGINAL TREE HEURISTICS: " << ORIGINAL_TREE_HEURISTICS << endl;
  cout << "CUSTOM HEURISTICS:        " << CUSTOMIZE_TREE_HEURISTICS << endl;
  cout << "NO TREE HEURISTICS:       " << NO_TREE_HEURISTICS << endl;
  cout << "ENFORCE_AFFINE_EXTRA:     " << ENFORCE_AFFINE_ON_EXTRA_BLOCKS << endl;
  cout << "ENFORCE_3_CPS:            " << ENFORCE_3_CPS << endl;
  cout << "REUSE_CACHE:              " << REUSE_CU_RESULTS << endl;

}

// Print extracted encoding information
void storch::printSummary() {

    totalCand = inheritedCand + constructedCand + translationalCand + temporalCand + zeroCand;
    
    cout << endl << "---------------------------------------------------------------------" << endl;
    
    printf("###      Early Terminations of Gradient Search\n");
    printf("ET_FULL_2CPs %d\n", storch::ET_aligned2CPs);
    printf("ET_FULL_3CPs %d\n", storch::ET_aligned3CPs);
    printf("ET_HALF_2CPs %d\n", storch::ET_half2CPs);
    printf("ET_HALF_3CPs %d\n", storch::ET_half3CPs);
    cout << endl << endl;
    
    cout << "###      Affine encoding time:" << endl;
    
    cout << "Complete 4 parameters:  " << (affUnip4pTime + affBip4pTime) << endl;
    cout << "Complete 6 parameters:  " << (affUnip6pTime + affBip6pTime) << endl;
    cout << "Complete uniprediction: " << (affUnip4pTime + affUnip6pTime) << endl;
    cout << "Complete biprediction:  " << (affBip4pTime + affBip6pTime) << endl;
       
    cout << "Unipred 4 params        " << affUnip4pTime << endl;
    cout << "  Initialization        " << affUnip4pInitTime << endl;
    cout << "  AMVP                  " << aff4pAMVPTime << endl;
    cout << "  ME                    " << affUnip4pMeTime << endl;   
    cout << "    ME Init             " << affUnip4pMeInitTime << endl;    
    cout << "    ME Gradient         " << affUnip4pMEGradTime << endl;
    cout << "        Equations:      " << affUnip4pMEGradTime_eq << endl;
    cout << "          Build:        " << affUnip4pMEGradTime_eq_build << endl;
    cout << "          Solve:        " << affUnip4pMEGradTime_eq_solve << endl;
    cout << "        Pred+RD:        " << affUnip4pMEGradTime_pred << endl;
    cout << "    ME Simp/Refinement  " << affUnip4pMERefTime << endl;
    
    cout << "Unipred 6 params        " << affUnip6pTime << endl;
    cout << "  Initialization        " << affUnip6pInitTime << endl;
    cout << "  AMVP                  " << aff6pAMVPTime << endl;
    cout << "  ME                    " << affUnip6pMeTime << endl;
    cout << "    ME Init             " << affUnip6pMeInitTime << endl;    
    cout << "    ME Gradient         " << affUnip6pMEGradTime << endl;
    cout << "        Equations:      " << affUnip6pMEGradTime_eq << endl;
    cout << "          Build:        " << affUnip6pMEGradTime_eq_build << endl;
    cout << "          Solve:        " << affUnip6pMEGradTime_eq_solve << endl;    
    cout << "        Pred+RD:        " << affUnip6pMEGradTime_pred << endl;    
    cout << "    ME Simp/Refinement  " << affUnip6pMERefTime << endl;
    
    cout << "Bipred 4 params         " << affBip4pTime << endl;
    cout << "  Initialization        " << affBip4pInitTime << endl;
    cout << "  ME                    " << affBip4pMeTime << endl;
    cout << "    ME Init             " << affBip4pMeInitTime << endl;    
    cout << "    ME Gradient         " << affBip4pMEGradTime << endl;
    cout << "        Equations:      " << affBip4pMEGradTime_eq << endl;
    cout << "          Build:        " << affBip4pMEGradTime_eq_build << endl;
    cout << "          Solve:        " << affBip4pMEGradTime_eq_solve << endl;    
    cout << "        Pred+RD:        " << affBip4pMEGradTime_pred << endl;        
    cout << "    ME Simp/Refinement  " << affBip4pMERefTime << endl;
    
    cout << "Bipred 6 params         " << affBip6pTime << endl;
    cout << "  Initialization        " << affBip6pInitTime << endl;
    cout << "  ME                    " << affBip6pMeTime << endl;
    cout << "    ME Init             " << affBip6pMeInitTime << endl;    
    cout << "    ME Gradient         " << affBip6pMEGradTime << endl;
    cout << "        Equations:      " << affBip6pMEGradTime_eq << endl;
    cout << "          Build:        " << affBip6pMEGradTime_eq_build << endl;
    cout << "          Solve:        " << affBip6pMEGradTime_eq_solve << endl;    
    cout << "        Pred+RD:        " << affBip6pMEGradTime_pred << endl;    
    cout << "    ME Simp/Refinement  " << affBip6pMERefTime << endl;    
    
    cout << endl;
    
    
    cout << endl << endl;
        
    cout << "###      Affine AMVP Candidates:" << endl;
    cout << "    Inherited     " << inheritedCand <<     "\t -> " << (float)inheritedCand/totalCand << endl;
    cout << "    Constructed   " << constructedCand <<   "\t -> " << (float)constructedCand/totalCand << endl;
    cout << "    Translational " << translationalCand << "\t -> " << (float)translationalCand/totalCand << endl;
    cout << "    Temporal      " << temporalCand <<      "\t -> " << (float)temporalCand/totalCand << endl;
    cout << "    Zero MV       " << zeroCand <<          "\t -> " << (float)zeroCand/totalCand << endl;
    cout << "    Total         " << totalCand <<         "\t -> " << (float)totalCand/totalCand << endl;
    
    cout << "---------------------------------------------------------------------" << endl;
}

// This presents a detailed report with statistics for each affine block size
void storch::printDetailedTimeSummary(bool isCommaSep, Alignment alignment){
    
    storch::resetUnipredUniqControl(); // used to sum the uniq blocks from the last frame. This is necessary here because the block count is updated at the start of the next frame, and the last frame has no next frame
  
    printf("{");
    printf("%s,", isCommaSep? "ComputerReadable" : "HumanReadable");
    printf("%s\n", alignment==ALIGNED ? "ALIGNED" : (alignment==HALF_ALIGNED ? "HALF_ALIGNED" : (alignment==UNALIGNED ? "UNALIGNED" : (alignment==COMBINED ? "COMBINED" : "ERROR"))));
    
cout << "  [!] Target times CUs 128x128" << endl;
    printf("    128x128 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_128x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_128x128][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_128x128][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_128x128][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_128x128][alignment]);
    
    printf("    128x128 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_128x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_128x128][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_128x128][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_128x128][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_128x128][alignment]);
    printf("    128x128 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_128x128][alignment]);
    
    cout << endl;
    
    printf("  [!] Target times CUs 128x64\n");
    printf("    128x64 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_128x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_128x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_128x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_128x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_128x64][alignment]);
    
    printf("    128x64 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_128x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_128x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_128x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_128x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_128x64][alignment]);
    printf("    128x64 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_128x64][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 64x128\n";
    printf("    64x128 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_64x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_64x128][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_64x128][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_64x128][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x128][alignment]);
    
    printf("    64x128 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_64x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_64x128][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_64x128][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_64x128][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x128][alignment]);
    printf("    64x128 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x128][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 64x64" << endl;
    printf("    64x64 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_64x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_64x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_64x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_64x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x64][alignment]);
    
    printf("    64x64 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_64x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_64x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_64x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_64x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x64][alignment]);
    printf("    64x64 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x64][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 64x32" << endl;
    printf("    64x32 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_64x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_64x32][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_64x32][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_64x32][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x32][alignment]);
    
    printf("    64x32 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_64x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_64x32][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_64x32][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_64x32][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x32][alignment]);
    printf("    64x32 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x32][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 32x64" << endl;
    printf("    32x64 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_32x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_32x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_32x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_32x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_32x64][alignment]);
    
    printf("    32x64 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_32x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_32x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_32x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_32x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_32x64][alignment]);
    printf("    32x64 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_32x64][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 64x16" << endl;
    printf("    64x16 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_64x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_64x16][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_64x16][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_64x16][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_64x16][alignment]);
    
    printf("    64x16 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_64x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_64x16][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_64x16][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_64x16][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_64x16][alignment]);
    printf("    64x16 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_64x16][alignment]);

    cout << endl;
    
    cout << "  [!] Target times CUs 16x64" << endl;
    printf("    16x64 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_16x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_16x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_16x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_16x64][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_16x64][alignment]);
    
    printf("    16x64 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_16x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_16x64][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_16x64][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_16x64][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_16x64][alignment]);
    printf("    16x64 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_16x64][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 32x32" << endl;
    printf("    32x32 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_32x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_32x32][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_32x32][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_32x32][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_32x32][alignment]);
    
    printf("    32x32 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_32x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_32x32][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_32x32][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_32x32][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_32x32][alignment]);
    printf("    32x32 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_32x32][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 32x16" << endl;
    printf("    32x16 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_32x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_32x16][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_32x16][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_32x16][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_32x16][alignment]);
    
    printf("    32x16 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_32x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_32x16][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_32x16][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_32x16][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_32x16][alignment]);
    printf("    32x16 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_32x16][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 16x32" << endl;
    printf("    16x32 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_16x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_16x32][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_16x32][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_16x32][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_16x32][alignment]);
    
    printf("    16x32 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_16x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_16x32][alignment]==0?-1.0:(float) numberUniqBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_16x32][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_6PARAM][_16x32][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_16x32][alignment]);
    printf("    16x32 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_16x32][alignment]);
    
    cout << endl;
    
    cout << "  [!] Target times CUs 16x16" << endl;
    printf("    16x16 AMVP+Init 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "       ",affAmvpInitTime[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Grad+Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Only Grad 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Only Ref+Simp 2 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 2 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 2 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Ratio Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_4PARAM][_16x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]/affUnipTime[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 2 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 2 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Ratio #Uniq/All Unipred 2 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_4PARAM][_16x16][alignment]==0?-1:(float) numberUniqBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 2 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_4PARAM][_16x16][alignment]==0?-1.0:(float) affUnipTime[AFFINEMODEL_4PARAM][_16x16][alignment]/calls_unipred[AFFINEMODEL_4PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 2 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]==0?-1.0:(float) unipredTimeUniqueBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]/storch::numberUniqBlocks[AFFINEMODEL_4PARAM][_16x16][alignment]);
    
    printf("    16x16 AMVP+Init 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",affAmvpInitTime[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Grad+Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",gradRefSimpTime[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Only Grad 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "      ",gradientMeTime[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Only Ref+Simp 3 CPs AllBlocks %s%f\n", isCommaSep ? "," : "  ",refSimpMeTime[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 3 CPs AllBlocks%s%f\n", isCommaSep ? "," : "         ",affUnipTime[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 3 CPs UniqBlocks %s%f\n", isCommaSep ? "," : "       ",unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Ratio Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "   ",affUnipTime[AFFINEMODEL_6PARAM][_16x16][alignment]==0?-1.0:(float)unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]/affUnipTime[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 3 CPs #AllBlocks%s%d\n", isCommaSep ? "," : "        ",calls_unipred[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 3 CPs #UniqBlocks%s%d\n", isCommaSep ? "," : "       ",numberUniqBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Ratio #Uniq/All Unipred 3 CPs %s%f\n", isCommaSep ? "," : "  ",calls_unipred[AFFINEMODEL_6PARAM][_16x16][alignment]==0?-1:(float)numberUniqBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 3 CPs time/AllBlocks %s%f\n", isCommaSep ? "," : "   ",calls_unipred[AFFINEMODEL_6PARAM][_16x16][alignment]==0?-1.0:(float)affUnipTime[AFFINEMODEL_6PARAM][_16x16][alignment]/calls_unipred[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("    16x16 Unipred 3 CPs time/UniqBlocks %s%f\n", isCommaSep ? "," : "  ",storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]==0?-1.0:(float)unipredTimeUniqueBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]/storch::numberUniqBlocks[AFFINEMODEL_6PARAM][_16x16][alignment]);
    printf("}\n");
}


bool storch::isAffineSize(SizeType width, SizeType height){
  if((width>=32) && (height>=32))
    return true;
  else if((width==32) && (height==16))
    return true;
  else if((width==16) && (height==32))
    return true;
  else if((width==64) && (height==16))
    return true;
  else if((width==16) && (height==64))
    return true;
  else if((width==16) && (height==16))
    return true;
  else
    return false;
}

bool storch::isAligned(UnitArea area){
  int width, height, x, y;
  
  width  = area.lwidth();
  height = area.lheight();
  x      = area.lx();
  y      = area.ly();
  
  int count = 0;
  
  count += x%width==0 ? 0 : 1; // If remainder is different than zero the block is unaligned in one axis
  count += y%height==0 ? 0 : 1;
  
  // If remainder is zero in both axis the block is aligned, otherwise it is halfAligned (count==1) or unaligned (count==2)
  return count==0;
}

bool storch::isHalfAligned(UnitArea area){
  int width, height, x, y;
  
  width  = area.lwidth();
  height = area.lheight();
  x      = area.lx();
  y      = area.ly();
  
  int count = 0;
  
  count += x%width==0 ? 0 : 1; // If remainder is different than zero the block is unaligned in one axis
  count += y%height==0 ? 0 : 1;
  
  // If remainder is zero in a single axis the block is halfAligned, otherwise it is aligned (count==0) or unaligned (count==2)
  return count==1;
}

bool storch::isUnaligned(UnitArea area){
  int width, height, x, y;
  
  width  = area.lwidth();
  height = area.lheight();
  x      = area.lx();
  y      = area.ly();
  
  int count = 0;
  
  count += x%width==0 ? 0 : 1; // If remainder is different than zero the block is unaligned in one axis
  count += y%height==0 ? 0 : 1;
  
  // If remainder is zero in none axis the block is unaligned, otherwise it is aligned (count==0) or halfAligned (count==1)
  return count==2;
}

Alignment storch::getAlignment(UnitArea area){
  
  if(storch::isAligned(area))
    return ALIGNED;
  if(storch::isHalfAligned(area))
    return HALF_ALIGNED;
  if(storch::isUnaligned(area))
    return UNALIGNED;
  
  printf("ERROR GETTING THE ALIGNMENT OF [%dx%d] @(%dx%d)\n", area.lwidth(), area.lheight(), area.lx(), area.ly());
  exit(0);
}

bool storch::isAligned(PredictionUnit& pu){
  // Creates a UnitArea to use the main function
  Area tempArea(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea tempUnit(CHROMA_420, tempArea);
  
  return storch::isAligned( tempUnit );
}

bool storch::isHalfAligned(PredictionUnit& pu){
  // Creates a UnitArea to use the main function
  Area tempArea(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea tempUnit(CHROMA_420, tempArea);
  
  return storch::isHalfAligned( tempUnit );
}

bool storch::isUnaligned(PredictionUnit& pu){
  // Creates a UnitArea to use the main function
  Area tempArea(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea tempUnit(CHROMA_420, tempArea);
  
  return storch::isUnaligned( tempUnit );
}

//static Alignment getAlignment(PredictionUnit& pu){
//  Area tempArea(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
//  UnitArea tempUnit(CHROMA_420, tempArea);
//  
//  if(storch::isAligned(tempUnit))
//    return ALIGNED;
//  if(storch::isHalfAligned(tempUnit))
//    return HALF_ALIGNED;
//  if(storch::isUnaligned(tempUnit))
//    return UNALIGNED;
//  
//  printf("ERROR GETTING THE ALIGNMENT OF [%dx%d] @(%dx%d)\n", tempUnit.lwidth(), tempUnit.lheight(), tempUnit.lx(), tempUnit.ly());
//  exit(0);
//}


// Export the samples of a 4x4 sub-block during affine ME into a CSV file
void storch::exportSamples4x4Block(Pel* samples, int xPos, int yPos, int stride, SamplesType type){
    int h,w;

    std::ofstream fileHandler;
    string name;
    
    if(type == REFERENCE){
        name = (string) "block_reference_" + std::to_string(xPos) + "_" + std::to_string(yPos);
    }
    else if(type == FILTERED_REFERENCE){
        name = (string) "block_filtered_" + std::to_string(xPos) + "_" + std::to_string(yPos);
    }
    else{
        printf("ERROR -- Incorrect samples type when exporting samples from 4x4 sub-block");
    }    
    
    fileHandler.open(name + ".csv");

    int blockWidth = 4;
    int blockHeight = 4;

    for (h=0; h<blockHeight; h++){
        for(w=0; w<blockWidth-1; w++){
            fileHandler << samples[h*stride + w] << ",";
        }
        fileHandler << samples[h*stride + w];
        fileHandler << endl;
    }
    fileHandler.close();
    
}

// Export the samples of a PU into a CSV file
void storch::exportSamplesBlock(CPelBuf samples, SamplesType type){
    int h,w;

    std::ofstream fileHandler;
    string name;
    
    if(type == REFERENCE){
        name = (string) "block_reference";
    }
    else if(type == FILTERED_REFERENCE){
        name = (string) "block_filtered";
    }
    else if(type == PREDICTED){
        name = (string) "block_predicted";
    }
    else{
        printf("ERROR -- Incorrect samples type when exporting block samples");
    }

    fileHandler.open(name + ".csv");

    int blockWidth = samples.width;
    int blockHeight = samples.height;

    for (h=0; h<blockHeight; h++){
        for(w=0; w<blockWidth-1; w++){
            fileHandler << samples.at(w,h) << ",";
        }
        fileHandler << samples.at(w,h);
        fileHandler << endl;
    }
    fileHandler.close();
}

// Export the samples of a frame into a CSV file
void storch::exportSamplesFrame(PelBuf samples, int POC, SamplesType type){
    int h,w;
    if(type == EXT_ORIGINAL){
        if(extractedFrames[EXT_ORIGINAL][POC] == 0){ // If the original frame was not extracted yet...
//            cout << "original " << POC << endl;
            std::ofstream fileHandler;

            string name = (string) "original_" + to_string(POC);

            fileHandler.open(name + ".csv");

            int frameWidth = samples.width;
            int frameHeight = samples.height;

            for (h=0; h<frameHeight; h++){
                for(w=0; w<frameWidth-1; w++){
                    fileHandler << samples.at(w,h) << ",";
                }
                fileHandler << samples.at(w,h);
                fileHandler << endl;
            }
            fileHandler.close();
            extractedFrames[EXT_ORIGINAL][POC] = 1; // Mark the current frame as extracted
        }
        else{
            cout << "ERROR - EXTRACTING THE SAME ORIGINAL FRAME TWICE" << endl;
        }        
    }
    else if(type == EXT_RECONSTRUCTED){
        if(extractedFrames[EXT_RECONSTRUCTED][POC] == 0){ // If the reconstructed frame was not extracted yet...
//            cout << "reconstructed " << POC << endl;
            std::ofstream fileHandler;

            string name = (string) "reconstructed_" + to_string(POC);

            fileHandler.open(name + ".csv");

            int frameWidth = samples.width;
            int frameHeight = samples.height;

            for (h=0; h<frameHeight; h++){
                for(w=0; w<frameWidth-1; w++){
                    fileHandler << samples.at(w,h) << ",";
                }
                fileHandler << samples.at(w,h);
                fileHandler << endl;
            }
            fileHandler.close();
            extractedFrames[EXT_RECONSTRUCTED][POC] = 1; // Mark the current frame as extracted
        }
    }
    else{
        cout << "ERROR - Incorrect type passes when exporting frame samples" << endl;
    }
}

// Export information from the affine ME into a file
void storch::exportAffineInfo(const PredictionUnit pu, Mv mvLT, Mv mvRT, Mv mvLB, int subX, int subY, int mv_x, int mv_y){
    int POC = pu.cu->cs->picture->getPOC();
    int frameWidth = pu.cu->cs->picture->getPicWidthInLumaSamples();
    int frameHeight = pu.cu->cs->picture->getPicHeightInLumaSamples();
    
    int puWidth = pu.Y().width;
    int puHeight = pu.Y().height;
    int puX = pu.lx();
    int puY = pu.ly();
    int biPred = (pu.interDir == 3) ? 1 : 0;
    
    int nCP = (pu.cu->affineType == AFFINEMODEL_4PARAM)? 2 : 3;
    
    affine_file << POC << "," << frameWidth << "," << frameHeight << ",";
    affine_file << puX << "," << puY << "," << puWidth << "," << puHeight << "," << biPred << ",";
    affine_file << nCP << "," << mvLT.hor << "," << mvLT.ver << "," << mvRT.hor << "," << mvRT.ver << "," << mvLB.hor << "," << mvLB.ver << ",";
    affine_file << subX << "," << subY << "," << mv_x << "," << mv_y << endl;
//    affine_file.close();
    
}

void storch::exportAmeProgressFlag(int is3CPs, int flag){
    if(!is3CPs){ // 2 CPs
        storch::affine_me_2cps_file << flag << ",";
    }
    else{
        storch::affine_me_3cps_file << flag << ",";
    }
}

void storch::exportAmeProgressMVs(int is3CPs, Mv mvs[3], int isFiller, int isFinal){
    if(!is3CPs){ // 2 CPs
        if(isFiller)
            storch::affine_me_2cps_file << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar;
        else
            storch::affine_me_2cps_file << mvs[0].hor << "," << mvs[0].ver << "," << mvs[1].hor << "," << mvs[1].ver << "," << mvs[2].hor << "," << mvs[2].ver;
        
        if(isFinal)
            storch::affine_me_2cps_file << endl;
        else
            storch::affine_me_2cps_file << ",";
    }
    else{
        if(isFiller)
            storch::affine_me_3cps_file << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar << "," << storch::fillerChar;
        else
            storch::affine_me_3cps_file << mvs[0].hor << "," << mvs[0].ver << "," << mvs[1].hor << "," << mvs[1].ver << "," << mvs[2].hor << "," << mvs[2].ver;
        
        if(isFinal)
            storch::affine_me_3cps_file << endl;
        else
            storch::affine_me_3cps_file << ",";
    }
}

void storch::exportAmeProgressBlock(int is3CPs, int refList, int refIdx, PredictionUnit& pu){
    if(!is3CPs){ // 2CPs
        storch::affine_me_2cps_file << pu.cs->picture->getPOC() << "," << refList << "," << refIdx << "," << pu.lx() << "," << pu.ly() << "," << pu.lwidth() << "," << pu.lheight() << ",";
    }
    else{
        storch::affine_me_3cps_file << pu.cs->picture->getPOC() << "," << refList << "," << refIdx << "," << pu.lx() << "," << pu.ly() << "," << pu.lwidth() << "," << pu.lheight() << ",";
    }
}

// Used to track the execution time
void storch::startAffineMEGradient(EAffineModel param, EAffinePred pred){
    gettimeofday(&ag1, NULL);
}

void storch::finishAffineMEGradient(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED)));

    gettimeofday(&ag2, NULL);   
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMEGradTime += (double) (ag2.tv_usec - ag1.tv_usec)/1000000 + (double) (ag2.tv_sec - ag1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMEGradTime += (double) (ag2.tv_usec - ag1.tv_usec)/1000000 + (double) (ag2.tv_sec - ag1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMEGradTime += (double) (ag2.tv_usec - ag1.tv_usec)/1000000 + (double) (ag2.tv_sec - ag1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMEGradTime += (double) (ag2.tv_usec - ag1.tv_usec)/1000000 + (double) (ag2.tv_sec - ag1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }
}

void storch::startAffineMEGradientEquations(EAffineModel param, EAffinePred pred){
    clock_age1 = clock();
}

void storch::finishAffineMEGradientEquations(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED)));

    clock_age2 = clock();
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMEGradTime_eq += (double) (clock_age2 - clock_age1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMEGradTime_eq += (double) (clock_age2 - clock_age1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMEGradTime_eq += (double) (clock_age2 - clock_age1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMEGradTime_eq += (double) (clock_age2 - clock_age1)/(double) (CLOCKS_PER_SEC);        
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }
}


void storch::startAffineMEGradientEquations_build(EAffineModel param, EAffinePred pred){
    clock_ageb1 = clock();
}

void storch::finishAffineMEGradientEquations_build(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED)));

    clock_ageb2 = clock();
       
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMEGradTime_eq_build += (double) (clock_ageb2 - clock_ageb1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMEGradTime_eq_build += (double) (clock_ageb2 - clock_ageb1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMEGradTime_eq_build += (double) (clock_ageb2 - clock_ageb1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMEGradTime_eq_build += (double) (clock_ageb2 - clock_ageb1)/(double) (CLOCKS_PER_SEC);        
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }   
}

void storch::startAffineMEGradientEquations_solve(EAffineModel param, EAffinePred pred){
    clock_ages1 = clock();
}

void storch::finishAffineMEGradientEquations_solve(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED)));

    clock_ages2 = clock();
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMEGradTime_eq_solve += (double) (clock_ages2 - clock_ages1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMEGradTime_eq_solve += (double) (clock_ages2 - clock_ages1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMEGradTime_eq_solve += (double) (clock_ages2 - clock_ages1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMEGradTime_eq_solve += (double) (clock_ages2 - clock_ages1)/(double) (CLOCKS_PER_SEC);        
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }  
    
}

void storch::startAffineMEGradientPred(EAffineModel param, EAffinePred pred){
    clock_agp1 = clock();
}

void storch::finishAffineMEGradientPred(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED)));

    clock_agp2 = clock();
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMEGradTime_pred += (double) (clock_agp2 - clock_agp1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMEGradTime_pred += (double) (clock_agp2 - clock_agp1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMEGradTime_pred += (double) (clock_agp2 - clock_agp1)/(double) (CLOCKS_PER_SEC);        
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMEGradTime_pred += (double) (clock_agp2 - clock_agp1)/(double) (CLOCKS_PER_SEC);        
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }
}


void storch::startAffineAMVP(EAffineModel param, EAffinePred pred){
    gettimeofday(&aamvp1, NULL);
}

void storch::finishAffineAMVP(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
        && (pred==UNIPRED));

    gettimeofday(&aamvp2, NULL);  
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        aff4pAMVPTime += (double) (aamvp2.tv_usec - aamvp1.tv_usec)/1000000 + (double) (aamvp2.tv_sec - aamvp1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        aff6pAMVPTime += (double) (aamvp2.tv_usec - aamvp1.tv_usec)/1000000 + (double) (aamvp2.tv_sec - aamvp1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }
}

void storch::startAffineComplete(EAffineModel param){
    assert((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)); 
    
    if(param == AFFINEMODEL_4PARAM){
        gettimeofday(&a4p1, NULL);
    }else if(param == AFFINEMODEL_6PARAM){
        gettimeofday(&a6p1, NULL);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }          
}

void storch::finishAffineComplete(EAffineModel param){
    assert((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM));
    
    if(param == AFFINEMODEL_4PARAM){
        gettimeofday(&a4p2, NULL);
        aff4pTime += (double) (a4p2.tv_usec - a4p1.tv_usec)/1000000 + (double) (a4p2.tv_sec - a4p1.tv_sec);
    
    }else if(param == AFFINEMODEL_6PARAM){
        gettimeofday(&a6p2, NULL);
        aff6pTime += (double) (a6p2.tv_usec - a6p1.tv_usec)/1000000 + (double) (a6p2.tv_sec - a6p1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    } 
}

void storch::startAffineME(EAffineModel param, EAffinePred pred){
    gettimeofday(&affme1, NULL);
}

void storch::finishAffineME(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED))); 
    
    gettimeofday(&affme2, NULL);   
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMeTime += (double) (affme2.tv_usec - affme1.tv_usec)/1000000 + (double) (affme2.tv_sec - affme1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMeTime += (double) (affme2.tv_usec - affme1.tv_usec)/1000000 + (double) (affme2.tv_sec - affme1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMeTime += (double) (affme2.tv_usec - affme1.tv_usec)/1000000 + (double) (affme2.tv_sec - affme1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMeTime += (double) (affme2.tv_usec - affme1.tv_usec)/1000000 + (double) (affme2.tv_sec - affme1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }    
}

void storch::startAffineMESimpRef(EAffineModel param, EAffinePred pred){
    gettimeofday(&sraffme1, NULL);
}

void storch::finishAffineMESimpRef(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED))); 

    gettimeofday(&sraffme2, NULL); 
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMERefTime += (double) (sraffme2.tv_usec - sraffme1.tv_usec)/1000000 + (double) (sraffme2.tv_sec - sraffme1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMERefTime += (double) (sraffme2.tv_usec - sraffme1.tv_usec)/1000000 + (double) (sraffme2.tv_sec - sraffme1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMERefTime += (double) (sraffme2.tv_usec - sraffme1.tv_usec)/1000000 + (double) (sraffme2.tv_sec - sraffme1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMERefTime += (double) (sraffme2.tv_usec - sraffme1.tv_usec)/1000000 + (double) (sraffme2.tv_sec - sraffme1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }     
}

void storch::startAffineInit(EAffineModel param, EAffinePred pred){
    gettimeofday(&affinit1, NULL);
}

void storch::finishAffineInit(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && ((pred==UNIPRED) || (pred==BIPRED))); 

    gettimeofday(&affinit2, NULL);
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pInitTime += (double) (affinit2.tv_usec - affinit1.tv_usec)/1000000 + (double) (affinit2.tv_sec - affinit1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pInitTime += (double) (affinit2.tv_usec - affinit1.tv_usec)/1000000 + (double) (affinit2.tv_sec - affinit1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pInitTime += (double) (affinit2.tv_usec - affinit1.tv_usec)/1000000 + (double) (affinit2.tv_sec - affinit1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pInitTime += (double) (affinit2.tv_usec - affinit1.tv_usec)/1000000 + (double) (affinit2.tv_sec - affinit1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    } 
}

void storch::startAffineUnipred(EAffineModel param, EAffinePred pred){
    gettimeofday(&affunip1, NULL);
}

void storch::finishAffineUnipred(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && (pred==UNIPRED)); 

    gettimeofday(&affunip2, NULL);

    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pTime += (double) (affunip2.tv_usec - affunip1.tv_usec)/1000000 + (double) (affunip2.tv_sec - affunip1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pTime += (double) (affunip2.tv_usec - affunip1.tv_usec)/1000000 + (double) (affunip2.tv_sec - affunip1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }     
}

// Probe start and finish time of complete affine uniprediction in a specific block size
void storch::startAffineUnipred_size(EAffineModel param, EAffinePred pred, CuSize size){
  gettimeofday(&affunip_target_1, NULL);
}

void storch::finishAffineUnipred_size(EAffineModel param, EAffinePred pred, CuSize size, PredictionUnit& pu){
  assert((param==AFFINEMODEL_4PARAM || param==AFFINEMODEL_6PARAM) 
            && (pred==UNIPRED));
  
  gettimeofday(&affunip_target_2, NULL);
   
  
  // Create a temp struct to hold the properties of the current block (XY position and splitSeries)
  positionAndSplitseries tempStruct;
  tempStruct.x = pu.lx();
  tempStruct.y = pu.ly();
  tempStruct.split = storch::currSplitSeries; // Split series of current CU. This value is initialized before the do-while which tests all encoding modes for each CU
  // Alignment of current CU
  Area currArea = Area(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea currUnitArea = UnitArea(CHROMA_420, currArea);
  Alignment align = storch::getAlignment(currUnitArea);
    
  if(storch::cusTestedWithAffine[param][size][COMBINED].count(tempStruct)==0){ // We have not conducted affine for this splitseries yet
    unipredTimeUniqueBlocks[param][size][COMBINED] += (double) (affunip_target_2.tv_usec - affunip_target_1.tv_usec)/1000000 + (double) (affunip_target_2.tv_sec - affunip_target_1.tv_sec);
    unipredTimeUniqueBlocks[param][size][align] += (double) (affunip_target_2.tv_usec - affunip_target_1.tv_usec)/1000000 + (double) (affunip_target_2.tv_sec - affunip_target_1.tv_sec);
  }

  affUnipTime[param][size][COMBINED] += (double) (affunip_target_2.tv_usec - affunip_target_1.tv_usec)/1000000 + (double) (affunip_target_2.tv_sec - affunip_target_1.tv_sec);
  affUnipTime[param][size][align] += (double) (affunip_target_2.tv_usec - affunip_target_1.tv_usec)/1000000 + (double) (affunip_target_2.tv_sec - affunip_target_1.tv_sec);
}

void storch::startAffineBipred(EAffineModel param, EAffinePred pred){
    gettimeofday(&affbip1, NULL);
}

void storch::finishAffineBipred(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            &&  (pred==BIPRED)); 

    gettimeofday(&affbip2, NULL);
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pTime += (double) (affbip2.tv_usec - affbip1.tv_usec)/1000000 + (double) (affbip2.tv_sec - affbip1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pTime += (double) (affbip2.tv_usec - affbip1.tv_usec)/1000000 + (double) (affbip2.tv_sec - affbip1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    } 
}

void storch::startAffineMEInit(EAffineModel param, EAffinePred pred){
    gettimeofday(&affmeinit1, NULL);
}
    
void storch::finishAffineMEInit(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            &&  (pred==BIPRED || pred==UNIPRED));     
    
    gettimeofday(&affmeinit2, NULL);
    
    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pMeInitTime += (double) (affmeinit2.tv_usec - affmeinit1.tv_usec)/1000000 + (double) (affmeinit2.tv_sec - affmeinit1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        affBip4pMeInitTime += (double) (affmeinit2.tv_usec - affmeinit1.tv_usec)/1000000 + (double) (affmeinit2.tv_sec - affmeinit1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pMeInitTime += (double) (affmeinit2.tv_usec - affmeinit1.tv_usec)/1000000 + (double) (affmeinit2.tv_sec - affmeinit1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        affBip6pMeInitTime += (double) (affmeinit2.tv_usec - affmeinit1.tv_usec)/1000000 + (double) (affmeinit2.tv_sec - affmeinit1.tv_sec);
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }     
    
    
}

// Probe start and finish time of Affine AMVP and initialization of affine uniprediction in a specific block size
void storch::startAffineAmvpInit_size(EAffineModel param, EAffinePred pred, CuSize size){
  gettimeofday(&amvpInit_target_1, NULL);
}

void storch::finishAffineAmvpInit_size(EAffineModel param, EAffinePred pred, CuSize size, PredictionUnit& pu){
  assert((param==AFFINEMODEL_4PARAM || param==AFFINEMODEL_6PARAM) && (pred==UNIPRED)); 
  
  gettimeofday(&amvpInit_target_2, NULL);

  // Alignment of current CU
  Area currArea = Area(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea currUnitArea = UnitArea(CHROMA_420, currArea);
  Alignment align = storch::getAlignment(currUnitArea); 
  
  affAmvpInitTime[param][size][COMBINED] += (double) (amvpInit_target_2.tv_usec - amvpInit_target_1.tv_usec)/1000000 + (double) (amvpInit_target_2.tv_sec - amvpInit_target_1.tv_sec);
  affAmvpInitTime[param][size][align] += (double) (amvpInit_target_2.tv_usec - amvpInit_target_1.tv_usec)/1000000 + (double) (amvpInit_target_2.tv_sec - amvpInit_target_1.tv_sec);
  
}


// Probe start and finish time of Gradient-ME, Simplification and Refinement stages of affine uniprediction in a specific block size
void storch::startAffineGradRefSimp_size(EAffineModel param, EAffinePred pred, CuSize size){
    gettimeofday(&gradRefSimp_target_1, NULL);
}

void storch::finishAffineGradRefSimp_size(EAffineModel param, EAffinePred pred, CuSize size, PredictionUnit& pu){
  assert((param==AFFINEMODEL_4PARAM || param==AFFINEMODEL_6PARAM) && (pred==UNIPRED)); 
  
  gettimeofday(&gradRefSimp_target_2, NULL);
  
  // Alignment of current CU
  Area currArea = Area(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea currUnitArea = UnitArea(CHROMA_420, currArea);
  Alignment align = storch::getAlignment(currUnitArea); 
  
  gradRefSimpTime[param][size][COMBINED] += (double) (gradRefSimp_target_2.tv_usec - gradRefSimp_target_1.tv_usec)/1000000 + (double) (gradRefSimp_target_2.tv_sec - gradRefSimp_target_1.tv_sec);
  gradRefSimpTime[param][size][align] += (double) (gradRefSimp_target_2.tv_usec - gradRefSimp_target_1.tv_usec)/1000000 + (double) (gradRefSimp_target_2.tv_sec - gradRefSimp_target_1.tv_sec);
}

// Probe start and finish time of Gradient-ME alone inside affine uniprediction in a specific block size
void storch::startAffineGradME_size(EAffineModel param, EAffinePred pred){
  gettimeofday(&gradProbe1, NULL);
}

void storch::finishAffineGradME_size(EAffineModel param, EAffinePred pred, CuSize size, PredictionUnit& pu){
  assert((param==AFFINEMODEL_4PARAM || param==AFFINEMODEL_6PARAM) && (pred==UNIPRED)); 
    
  // For now, we are not intereseted in tracking the bipred time
  if(pred==BIPRED)
    return;
  
  gettimeofday(&gradProbe2, NULL);
  
  // Alignment of current CU
  Area currArea = Area(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea currUnitArea = UnitArea(CHROMA_420, currArea);
  Alignment align = storch::getAlignment(currUnitArea); 
  
  gradientMeTime[param][size][COMBINED] += (double) (gradProbe2.tv_usec - gradProbe1.tv_usec)/1000000 + (double) (gradProbe2.tv_sec - gradProbe1.tv_sec);
  gradientMeTime[param][size][align] += (double) (gradProbe2.tv_usec - gradProbe1.tv_usec)/1000000 + (double) (gradProbe2.tv_sec - gradProbe1.tv_sec);
}

// Probe start and finish time of refinement and simplification stages of affine uniprediction in a specific block size
void storch::startAffineRefSimp_size(EAffineModel param, EAffinePred pred){
  gettimeofday(&refSimpProbeProbe1, NULL);
}

void storch::finishAffineRefSimp_size(EAffineModel param, EAffinePred pred, CuSize size, PredictionUnit& pu){
  assert((param==AFFINEMODEL_4PARAM || param==AFFINEMODEL_6PARAM) && (pred==UNIPRED)); 
  
  // For now, we are not intereseted in tracking the bipred time
  if(pred==BIPRED)
    return;
  
  gettimeofday(&refSimpProbeProbe2, NULL);
  
  // Alignment of current CU
  Area currArea = Area(pu.lx(), pu.ly(), pu.lwidth(), pu.lheight());
  UnitArea currUnitArea = UnitArea(CHROMA_420, currArea);
  Alignment align = storch::getAlignment(currUnitArea); 
  
  refSimpMeTime[param][size][COMBINED] += (double) (refSimpProbeProbe2.tv_usec - refSimpProbeProbe1.tv_usec)/1000000 + (double) (refSimpProbeProbe2.tv_sec - refSimpProbeProbe1.tv_sec);
  refSimpMeTime[param][size][align] += (double) (refSimpProbeProbe2.tv_usec - refSimpProbeProbe1.tv_usec)/1000000 + (double) (refSimpProbeProbe2.tv_sec - refSimpProbeProbe1.tv_sec);
}

// Allows an easier handling of block sizes with meaninful names and less conditionals
// There are two of these with different input parameters
CuSize storch::getSizeEnum(UnitArea area){ 
  if(area.lwidth()==128 && area.lheight()==128)
    return _128x128;
  else if (area.lwidth()==128 && area.lheight()==64)
    return _128x64;
  else if (area.lwidth()==64 && area.lheight()==128)
    return _64x128;
  else if (area.lwidth()==64 && area.lheight()==64)
    return _64x64;
  else if (area.lwidth()==64 && area.lheight()==32)
    return _64x32;
  else if (area.lwidth()==32 && area.lheight()==64)
    return _32x64;
  else if (area.lwidth()==64 && area.lheight()==16)
    return _64x16;
  else if (area.lwidth()==16 && area.lheight()==64)
    return _16x64;
  else if (area.lwidth()==32 && area.lheight()==32)
    return _32x32;
  else if (area.lwidth()==32 && area.lheight()==16)
    return _32x16;
  else if (area.lwidth()==16 && area.lheight()==32)
    return _16x32;
  else if (area.lwidth()==16 && area.lheight()==16)
    return _16x16;
  else{
    printf("ERROR - Wrong PU size in getSizeEnum\n");
    exit(0);
  }
}

// Allows an easier handling of block sizes with meaninful names and less conditionals
CuSize storch::getSizeEnum(PredictionUnit pu){
  if(pu.lwidth()==128 && pu.lheight()==128)
    return _128x128;
  else if (pu.lwidth()==128 && pu.lheight()==64)
    return _128x64;
  else if (pu.lwidth()==64 && pu.lheight()==128)
    return _64x128;
  else if (pu.lwidth()==64 && pu.lheight()==64)
    return _64x64;
  else if (pu.lwidth()==64 && pu.lheight()==32)
    return _64x32;
  else if (pu.lwidth()==32 && pu.lheight()==64)
    return _32x64;
  else if (pu.lwidth()==64 && pu.lheight()==16)
    return _64x16;
  else if (pu.lwidth()==16 && pu.lheight()==64)
    return _16x64;
  else if (pu.lwidth()==32 && pu.lheight()==32)
    return _32x32;
  else if (pu.lwidth()==32 && pu.lheight()==16)
    return _32x16;
  else if (pu.lwidth()==16 && pu.lheight()==32)
    return _16x32;
  else if (pu.lwidth()==16 && pu.lheight()==16)
    return _16x16;
  else{
    printf("ERROR - Wrong PU size in getSizeEnum\n");
    exit(0);
  }
}

void storch::verifyTreeHeuristicsGpuMeMacros(){
  int errors = 0;
  
  if(ORIGINAL_TREE_HEURISTICS + CUSTOMIZE_TREE_HEURISTICS + NO_TREE_HEURISTICS != 1){
    printf("MACRO ERROR: EXACTLY ONE TREE HEURISTIC MACRO MUST BE ENABLED:\n");
    printf("  ORIGINAL_TREE_HEURISTICS=%d\n", ORIGINAL_TREE_HEURISTICS);
    printf("  CUSTOMIZE_TREE_HEURISTICS=%d\n", CUSTOMIZE_TREE_HEURISTICS);
    printf("  NO_TREE_HEURISTICS=%d\n", NO_TREE_HEURISTICS);
    errors++;
  }
  
  if(CUSTOMIZE_TREE_HEURISTICS==0 && ENFORCE_AFFINE_ON_EXTRA_BLOCKS){
    printf("MACRO ERROR: IT IS NOT POSSIBLE TO ENFORCE AFFINE ON EXTRA BLOCKS WHEN CUSTOMIZE_TREE_HEURISTICS IS DISABLED\n");
    printf("  CUSTOMIZE_TREE_HEURISTICS: %d\n", CUSTOMIZE_TREE_HEURISTICS);
    printf("  ENFORCE_AFFINE_ON_EXTRA_BLOCKS: %d\n", ENFORCE_AFFINE_ON_EXTRA_BLOCKS);
    errors++;
  }
  
  if(SIMD_ENABLE && (GPU_ME_2CPs || GPU_ME_3CPs)){
    printf("MACRO ERROR: GPU_ME ONLY WORKS WHEN SIMD OPTIMIZATIONS ARE DISABLED\n");
    printf("  SIMD_ENABLE=%d\n", SIMD_ENABLE);
    printf("  GPU_ME_2CPs=%d\n", GPU_ME_2CPs);
    printf("  GPU_ME_3CPs=%d\n", GPU_ME_3CPs);
    errors++;
  }
  if(errors)
    exit(0);
  
}

void storch::verifyTraceMacros(){
  int errors = 0;
  if(TRACE_XCOMPRESSCU==0){
    if(TRACE_CU_COSTS){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE CU COSTS WHILE TRACING xCompressCU\n");
      printf("  TRACE_XCOMPRESSCU=%d\n", TRACE_XCOMPRESSCU);
      printf("  TRACE_CU_COSTS=%d\n", TRACE_CU_COSTS);
      errors++;
    }
    if(TRACE_ENC_MODES){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE ENCODING MODES WHILE TRACING xCompressCU\n");
      printf("  TRACE_XCOMPRESSCU=%d\n", TRACE_XCOMPRESSCU);
      printf("  TRACE_ENC_MODES=%d\n", TRACE_CU_COSTS);
      errors++;
    }
    if(ONLY_TRACE_AFFINE_SIZES){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO REDUCE TRACE TO AFFINE SIZES WHILE TRACING xCompressCU\n");
      printf("  TRACE_XCOMPRESSCU=%d\n", TRACE_XCOMPRESSCU);
      printf("  ONLY_TRACE_AFFINE_SIZES=%d\n", TRACE_CU_COSTS);
      errors++;
    }
    if(TRACE_INNER_RESULTS_PRED){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE INNER PREDICTION RESULTS WHILE TRACING xCompressCU AND TRACING ENCODING MODES\n");
      printf("  TRACE_XCOMPRESSCU=%d\n", TRACE_XCOMPRESSCU);
      printf("  TRACE_ENC_MODES=%d\n", TRACE_ENC_MODES);
      printf("  TRACE_INNER_RESULTS_PRED=%d\n", TRACE_CU_COSTS);
      errors++;
    }
    if(errors)
      exit(0);
  }
  
  if(TRACE_ENC_MODES==0 && TRACE_INNER_RESULTS_PRED){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE INNER PREDICTION RESULTS WHILE TRACING xCompressCU AND TRACING ENCODING MODES\n");
      printf("  TRACE_XCOMPRESSCU=%d\n", TRACE_XCOMPRESSCU);
      printf("  TRACE_ENC_MODES=%d\n", TRACE_ENC_MODES);
      printf("  TRACE_INNER_RESULTS_PRED=%d\n", TRACE_CU_COSTS);
      exit(0);
  }
  
  if(DEBUG_ENABLE_DISABLE_CHILDREN){
    if(CUSTOMIZE_TREE_HEURISTICS==0 || ENFORCE_AFFINE_ON_EXTRA_BLOCKS==0){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE ENABLE/DISABLE CHILDREN BLOCKS WHEN CUSTOMIZE_TREE_HEURISTICS AND ENFORCE_AFFINE_ON_EXTRA_BLOCKS ARE ENABLED\n");
      printf("  DEBUG_ENABLE_DISABLE_CHILDREN=%d\n", DEBUG_ENABLE_DISABLE_CHILDREN);
      printf("  CUSTOMIZE_TREE_HEURISTICS=%d\n", CUSTOMIZE_TREE_HEURISTICS);
      printf("  ENFORCE_AFFINE_ON_EXTRA_BLOCKS=%d\n", ENFORCE_AFFINE_ON_EXTRA_BLOCKS);
      exit(0);
    }
  }
  
  if(TRACE_TRYMODE_DIVERGENCE){
    if(TRACE_XCOMPRESSCU==0 || TRACE_ENC_MODES==0){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE THE DIVERGENCES BETWEEN ORIGINAL AND CUSTOMIZED TRYMODE WHEN TRACING xCompressCU AND TRACE_ENC_MODES IS ENABLED\n");
      printf("  TRACE_TRYMODE_DIVERGENCE=%d\n", TRACE_TRYMODE_DIVERGENCE);
      printf("  TRACE_XCOMPRESSCU=%d\n", TRACE_XCOMPRESSCU);
      printf("  TRACE_ENC_MODES=%d\n", TRACE_ENC_MODES);
      exit(0);
    }
    if(ORIGINAL_TREE_HEURISTICS){
      printf("MACRO ERROR: IT IS ONLY POSSIBLE TO TRACE THE DIVERGENCES BETWEEN ORIGINAL AND CUSTOMIZED TRYMODE WHEN ORIGINAL_TREE_HEURISTICS IS DISABLED\n");
      printf("  TRACE_TRYMODE_DIVERGENCE=%d\n", TRACE_TRYMODE_DIVERGENCE);
      printf("  ORIGINAL_TREE_HEURISTICS=%d\n", ORIGINAL_TREE_HEURISTICS);
      exit(0);
    }
  }  
}

// This resets the control of whether affine prediciton was already applied to a block or not
// And updates the "global" counter based on the affine of the last frame
// This is used before starting the encoding of a new frame to allow an adaptive control
void storch::resetUnipredUniqControl(){
  
  for(int size=0; size<NUM_SIZES; size++){     
    for(int align=0; align<NUM_ALIGNMENTS; align++){
      // Add number of elements from the set into the matrix storing the total number of uniqBlocks
      numberUniqBlocks[AFFINEMODEL_4PARAM][size][align] += cusTestedWithAffine[AFFINEMODEL_4PARAM][size][align].size();
      numberUniqBlocks[AFFINEMODEL_6PARAM][size][align] += cusTestedWithAffine[AFFINEMODEL_6PARAM][size][align].size();

      // Clear the set so we can start the new frame with an empty set
      cusTestedWithAffine[AFFINEMODEL_4PARAM][size][align].clear();
      cusTestedWithAffine[AFFINEMODEL_6PARAM][size][align].clear();
    }
  }
}

#if EXAMPLE || EXAMPLE
void storch::exampleFunct() {
    cout << "Example" << endl;
}
#endif