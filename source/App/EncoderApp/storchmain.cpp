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

double storch::fsTime, storch::affAMVPTime, storch::affGradTime, storch::aff4pTime, storch::aff6pTime, storch::affMeTime, storch::simpRefAffMeTime;
struct timeval storch::fs1, storch::fs2, storch::aamvp1, storch::aamvp2, storch::ag1, storch::ag2, storch::a4p1, storch::a4p2, storch::a6p1, storch::a6p2, storch::affme1, storch::affme2, storch::sraffme1, storch::sraffme2;
int storch::extractedFrames[EXT_NUM][500];
int storch::currPoc;
int storch::inheritedCand, storch::constructedCand, storch::translationalCand, storch::temporalCand, storch::zeroCand, storch::totalCand;

storch::storch() {
    fsTime = 0.0;
    affAMVPTime = 0.0;
    affGradTime = 0.0;
    aff4pTime = 0.0;
    aff6pTime = 0.0;
    affMeTime = 0.0;
    
    currPoc = 0;
    
    inheritedCand = 0;
    constructedCand = 0;
    translationalCand = 0;
    temporalCand = 0;
    zeroCand = 0;
    totalCand = 0;
    
    
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
    cout << "Comp Affine 4 and 6 Params = " << aff4pTime+aff6pTime << endl;
    cout << "  4 Params = " << aff4pTime << endl;
    cout << "  6 Params = " << aff6pTime << endl;
    cout << "  Affine AMVP = " << affAMVPTime << endl;
    cout << "  Affine ME = " << affMeTime << endl;
    cout << "    Gradient = " << affGradTime << endl;
    cout << "    Simp/Ref = " << simpRefAffMeTime << endl;
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

// Export the input samples into a CSV file
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

// Used to track the execution time
void storch::startFullSearch(){
    gettimeofday(&fs1, NULL);
}
void storch::finishFullSearch(){
    gettimeofday(&fs2, NULL);   
    fsTime += (double) (fs2.tv_usec - fs1.tv_usec)/1000000 + (double) (fs2.tv_sec - fs1.tv_sec);
}
void storch::startAffineGradientME(){
    gettimeofday(&ag1, NULL);
}
void storch::finishAffineGradientME(){
    gettimeofday(&ag2, NULL);   
    affGradTime += (double) (ag2.tv_usec - ag1.tv_usec)/1000000 + (double) (ag2.tv_sec - ag1.tv_sec);
}
void storch::startAffineAMVP(){
    gettimeofday(&aamvp1, NULL);
}
void storch::finishAffineAMVP(){
    gettimeofday(&aamvp2, NULL);   
    affAMVPTime += (double) (aamvp2.tv_usec - aamvp1.tv_usec)/1000000 + (double) (aamvp2.tv_sec - aamvp1.tv_sec);
}
void storch::startAffineComplete(int params){
    assert(params==4 || params==6);
    if(params==4){
        gettimeofday(&a4p1, NULL);
    }
    else{ // 6 params
        gettimeofday(&a6p1, NULL);
    }      
}
void storch::finishAffineComplete(int params){
    assert(params==4 || params==6);
    if(params==4){
        gettimeofday(&a4p2, NULL);
        aff4pTime += (double) (a4p2.tv_usec - a4p1.tv_usec)/1000000 + (double) (a4p2.tv_sec - a4p1.tv_sec);
    }
    else{ // 6 params
        gettimeofday(&a6p2, NULL);
        aff6pTime += (double) (a6p2.tv_usec - a6p1.tv_usec)/1000000 + (double) (a6p2.tv_sec - a6p1.tv_sec);
    }    
}
void storch::startAffineME(){
    gettimeofday(&affme1, NULL);
}
void storch::finishAffineME(){
    gettimeofday(&affme2, NULL);   
    affMeTime += (double) (affme2.tv_usec - affme1.tv_usec)/1000000 + (double) (affme2.tv_sec - affme1.tv_sec);
}
void storch::startSimpRefAffineME(){
    gettimeofday(&sraffme1, NULL);
}
void storch::finishSimpRefAffineME(){
    gettimeofday(&sraffme2, NULL);   
    simpRefAffMeTime += (double) (sraffme2.tv_usec - sraffme1.tv_usec)/1000000 + (double) (sraffme2.tv_sec - sraffme1.tv_sec);
}
#if EXAMPLE || EXAMPLE
void storch::exampleFunct() {
    cout << "Example" << endl;
}
#endif