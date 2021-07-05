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
struct timeval storch::fs1, storch::fs2, storch::aamvp1, storch::aamvp2, storch::ag1, storch::ag2, storch::a4p1, storch::a4p2, storch::a6p1, storch::a6p2, storch::affme1, storch::affme2, storch::sraffme1, storch::sraffme2, storch::affinit1, storch::affinit2, storch::affunip1, storch::affunip2, storch::affbip1, storch::affbip2, storch::affmeinit1, storch::affmeinit2;
int storch::extractedFrames[EXT_NUM][500];
int storch::currPoc;
int storch::inheritedCand, storch::constructedCand, storch::translationalCand, storch::temporalCand, storch::zeroCand, storch::totalCand;
std::ofstream storch::affine_file;

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

    
    currPoc = 0;
    
    inheritedCand = 0;
    constructedCand = 0;
    translationalCand = 0;
    temporalCand = 0;
    zeroCand = 0;
    totalCand = 0;
    
    // Print the header of the file
    if(EXTRACT_AFFINE_MV){
        string affineFileName = (string) "affine_mv.csv";
        affine_file.open(affineFileName);    
        
        affine_file << "POC,frameWidth,frameHeight,";
        affine_file << "puX,puY,puWidth,puHeight,biPred,";
        affine_file << "nCP,LT_X,LT_Y,RT_X,RT_Y,LB_X,LB_Y,";
        affine_file << "subX,subY,mv_X,mv_Y" << endl;
        
    }
       
    for(int f=0; f<500; f++){
        for(int t=0; t<EXT_NUM; t++){
            extractedFrames[t][f]=0;   // at the start, no frame was extracted
        }
    }
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
    cout << "    ME Simp/Refinement  " << affUnip4pMERefTime << endl;
    
    cout << "Unipred 6 params        " << affUnip6pTime << endl;
    cout << "  Initialization        " << affUnip6pInitTime << endl;
    cout << "  AMVP                  " << aff6pAMVPTime << endl;
    cout << "  ME                    " << affUnip6pMeTime << endl;
    cout << "    ME Init             " << affUnip6pMeInitTime << endl;    
    cout << "    ME Gradient         " << affUnip6pMEGradTime << endl;
    cout << "    ME Simp/Refinement  " << affUnip6pMERefTime << endl;
    
    cout << "Bipred 4 params         " << affBip4pTime << endl;
    cout << "  Initialization        " << affBip4pInitTime << endl;
    cout << "  ME                    " << affBip4pMeTime << endl;
    cout << "    ME Init             " << affBip4pMeInitTime << endl;    
    cout << "    ME Gradient         " << affBip4pMEGradTime << endl;
    cout << "    ME Simp/Refinement  " << affBip4pMERefTime << endl;
    
    cout << "Bipred 6 params         " << affBip6pTime << endl;
    cout << "  Initialization        " << affBip6pInitTime << endl;
    cout << "  ME                    " << affBip6pMeTime << endl;
    cout << "    ME Init             " << affBip6pMeInitTime << endl;    
    cout << "    ME Gradient         " << affBip6pMEGradTime << endl;
    cout << "    ME Simp/Refinement  " << affBip6pMERefTime << endl;    
    
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


#if EXAMPLE || EXAMPLE
void storch::exampleFunct() {
    cout << "Example" << endl;
}
#endif