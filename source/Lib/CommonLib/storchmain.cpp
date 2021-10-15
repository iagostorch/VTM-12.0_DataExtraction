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

double storch::fsTime, storch::aff4pTime, storch::aff6pTime, storch::aff4pAMVPTime, storch::aff6pAMVPTime, storch::affUnip4pTime, storch::affBip4pTime, storch::affUnip6pTime, storch::affBip6pTime, storch::affUnip4pInitTime, storch::affBip4pInitTime, storch::affUnip6pInitTime, storch::affBip6pInitTime, storch::affUnip4pMeTime, storch::affBip4pMeTime, storch::affUnip6pMeTime, storch::affBip6pMeTime, storch::affUnip4pMEGradTime, storch::affBip4pMEGradTime, storch::affUnip6pMEGradTime, storch::affBip6pMEGradTime, storch::affUnip4pMERefTime, storch::affBip4pMERefTime, storch::affUnip6pMERefTime, storch::affBip6pMERefTime, storch::affUnip4pMeInitTime, storch::affBip4pMeInitTime, storch::affUnip6pMeInitTime, storch::affBip6pMeInitTime;
double storch::affUnip4pMEGradTime_pred, storch::affBip4pMEGradTime_pred, storch::affUnip6pMEGradTime_pred, storch::affBip6pMEGradTime_pred, storch::affUnip4pMEGradTime_eq, storch::affBip4pMEGradTime_eq, storch::affUnip6pMEGradTime_eq, storch::affBip6pMEGradTime_eq, storch::affUnip4pMEGradTime_eq_build, storch::affUnip4pMEGradTime_eq_solve, storch::affBip4pMEGradTime_eq_build, storch::affBip4pMEGradTime_eq_solve, storch::affUnip6pMEGradTime_eq_build, storch::affUnip6pMEGradTime_eq_solve, storch::affBip6pMEGradTime_eq_build, storch::affBip6pMEGradTime_eq_solve;
double storch::affAmvpInit4pTime_128x128, storch::gradRefSimp4pTime_128x128, storch::blockPredTime_128x128, storch::affUnip4pTime_128x128, storch::affUnip6pTime_128x128;
struct timeval storch::fs1, storch::fs2, storch::aamvp1, storch::aamvp2, storch::ag1, storch::ag2, storch::a4p1, storch::a4p2, storch::a6p1, storch::a6p2, storch::affme1, storch::affme2, storch::sraffme1, storch::sraffme2, storch::affinit1, storch::affinit2, storch::affunip1, storch::affunip2, storch::affbip1, storch::affbip2, storch::affmeinit1, storch::affmeinit2;
struct timeval storch::amvpInit_128x128_1, storch::amvpInit_128x128_2, storch::gradRefSimp_128x128_1, storch::gradRefSimp_128x128_2, storch::blockPred_128x128_1, storch::blockPred_128x128_2, storch::affunip_128x128_1, storch::affunip_128x128_2;
clock_t storch::clock_agp1, storch::clock_agp2, storch::clock_age1, storch::clock_age2, storch::clock_ageb1, storch::clock_ageb2, storch::clock_ages1, storch::clock_ages2;
int storch::extractedFrames[EXT_NUM][500];
int storch::currPoc;
int storch::inheritedCand, storch::constructedCand, storch::translationalCand, storch::temporalCand, storch::zeroCand, storch::totalCand;
char storch::fillerChar;
std::ofstream storch::affine_file;
std::ofstream storch::affine_me_2cps_file, storch::affine_me_3cps_file;

int storch::prof; // Keep the value of the PROF parameter

storch::storch() {
    fsTime = 0.0;
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
    
    affAmvpInit4pTime_128x128 = 0.0;
    gradRefSimp4pTime_128x128 = 0.0;
        
    currPoc = 0;
    
    prof = 0;
    
    inheritedCand = 0;
    constructedCand = 0;
    translationalCand = 0;
    temporalCand = 0;
    zeroCand = 0;
    totalCand = 0;
    
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
}

void storch::setPROF(int p){
  prof = p;
}

void storch::printParamsSummary(){
  cout << endl << "---------------------------------------------------------------------" << endl;
  cout << "Relevant encoding parameters and macros" << endl;
  cout << "GPU_ME:      " << GPU_ME << endl;
  cout << "SIMD_ENABLE: " << SIMD_ENABLE << endl;
  cout << "PROF:        " << prof << endl;
  

}

// Print extracted encoding information
void storch::printSummary() {

    totalCand = inheritedCand + constructedCand + translationalCand + temporalCand + zeroCand;
    
    cout << endl << "---------------------------------------------------------------------" << endl;
    cout << "Full Search Time = " << fsTime << endl << endl << endl;
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
    
    cout << "[!] Target times CUs 128x128" << endl;
    cout << "  128x128 AMVP+Init 2CPs     " << affAmvpInit4pTime_128x128 << endl;
    cout << "  128x128 Grad+Ref+Simp 2CPs " << gradRefSimp4pTime_128x128 << endl;
    cout << "  128x128 Unipred 2 CPs      " << affUnip4pTime_128x128 << endl;
    cout << "  128x128 Unipred 3 CPs      " << affUnip6pTime_128x128 << endl;
    
    
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
void storch::startFullSearch(){
    gettimeofday(&fs1, NULL);
}
void storch::finishFullSearch(){
    gettimeofday(&fs2, NULL);   
    fsTime += (double) (fs2.tv_usec - fs1.tv_usec)/1000000 + (double) (fs2.tv_sec - fs1.tv_sec);
}
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

void storch::startAffineUnipred_128x128(EAffineModel param, EAffinePred pred){
    gettimeofday(&affunip_128x128_1, NULL);
}

void storch::finishAffineUnipred_128x128(EAffineModel param, EAffinePred pred){
    assert(((param==AFFINEMODEL_4PARAM) || (param==AFFINEMODEL_6PARAM)) 
            && (pred==UNIPRED)); 

    gettimeofday(&affunip_128x128_2, NULL);

    if(param == AFFINEMODEL_4PARAM && pred == UNIPRED){
        affUnip4pTime_128x128 += (double) (affunip_128x128_2.tv_usec - affunip_128x128_1.tv_usec)/1000000 + (double) (affunip_128x128_2.tv_sec - affunip_128x128_1.tv_sec);
    }else if(param == AFFINEMODEL_4PARAM && pred == BIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else if(param == AFFINEMODEL_6PARAM && pred == UNIPRED){
        affUnip6pTime_128x128 += (double) (affunip_128x128_2.tv_usec - affunip_128x128_1.tv_usec)/1000000 + (double) (affunip_128x128_2.tv_sec - affunip_128x128_1.tv_sec);
    }else if(param == AFFINEMODEL_6PARAM && pred == BIPRED){
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }else{
        cout << "ERROR :: Incorrect affine number of parameters or pred type" << endl;
    }     
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
            &&  (pred==BIPRED));     
    
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


void storch::startAffineAmvpInit_128x128(EAffineModel param, EAffinePred pred){
    gettimeofday(&amvpInit_128x128_1, NULL);
}

void storch::finishAffineAmvpInit_128x128(EAffineModel param, EAffinePred pred){
    assert((param==AFFINEMODEL_4PARAM) && (pred==UNIPRED)); 

    gettimeofday(&amvpInit_128x128_2, NULL);
    affAmvpInit4pTime_128x128 += (double) (amvpInit_128x128_2.tv_usec - amvpInit_128x128_1.tv_usec)/1000000 + (double) (amvpInit_128x128_2.tv_sec - amvpInit_128x128_1.tv_sec);
}

void storch::startAffineGradRefSimp_128x128(EAffineModel param, EAffinePred pred){
    gettimeofday(&gradRefSimp_128x128_1, NULL);
}

void storch::finishAffineGradRefSimp_128x128(EAffineModel param, EAffinePred pred){
    assert((param==AFFINEMODEL_4PARAM) && (pred==UNIPRED)); 

    gettimeofday(&gradRefSimp_128x128_2, NULL);
    gradRefSimp4pTime_128x128 += (double) (gradRefSimp_128x128_2.tv_usec - gradRefSimp_128x128_1.tv_usec)/1000000 + (double) (gradRefSimp_128x128_2.tv_sec - gradRefSimp_128x128_1.tv_sec);
}

#if EXAMPLE || EXAMPLE
void storch::exampleFunct() {
    cout << "Example" << endl;
}
#endif