## Description

This repository is a modification of the VVC Reference Encoder (VTM-12.0) to simulate the GPU acceleration of the Affine Motion Estimation (Affine ME), originally implemented on [VVC-Affine-GPU](https://github.com/iagostorch/VVC-Affine-GPU).

For a guide on compiling and using the baseline parameters of the encoder, refer to the [encoder's repository](https://vcgit.hhi.fraunhofer.de/jvet/VVCSoftware_VTM).

Running the complete encoder, where the Affine ME is performed as in the GPU implementation, takes following form:


```bash
./EncoderAppStatic -c cfg/encoder_lowdelay_P_vtm.cfg --PROF=0 -c cfg/per-sequence/PartyScene.cfg -q 32 -f 2 -i data/PartyScene_832x480_50.yuv --sHEUR_originalTreeHeuristics=1 --sGPU_gpuMe2Cps=1 --sGPU_gpuMe3Cps=1 --sGPU_predict3CpsFrom2Cps=1
```

Where ```sHEUR_originalTreeHeuristics```, ```sGPU_gpuMe2Cps```, ```sGPU_gpuMe3Cps```, and ```sGPU_predict3CpsFrom2Cps``` are new parameters introduced to simulate the GPU behavior.
