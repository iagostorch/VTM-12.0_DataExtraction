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

double storch::fsTime;
struct timeval storch::fs1, storch::fs2;
int storch::extractedFrames[EXT_NUM][500];
int storch::currPoc;


storch::storch() {
    fsTime = 0.0;
    currPoc = 0;
    
    for(int f=0; f<500; f++){
        for(int t=0; t<EXT_NUM; t++){
            extractedFrames[t][f]=0;   // at the start, no frame was extracted
        }
    }
}

// Print extracted encoding information
void storch::printSummary() {
    cout << endl << "---------------------------------------------------------------------" << endl;
    cout << "FS Time =  " << fsTime << endl;
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

#if EXAMPLE || EXAMPLE
void storch::exampleFunct() {
    cout << "Example" << endl;
}
#endif