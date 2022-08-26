/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2021, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     EncModeCtrl.cpp
    \brief    Encoder controller for trying out specific modes
*/

#include "../CommonLib/storchmain.h"

#include "EncModeCtrl.h"

#include "AQp.h"
#include "RateCtrl.h"

#include "CommonLib/RdCost.h"
#include "CommonLib/CodingStructure.h"
#include "CommonLib/Picture.h"
#include "CommonLib/UnitTools.h"

#include "CommonLib/dtrace_next.h"

#include <cmath>

void EncModeCtrl::init( EncCfg *pCfg, RateCtrl *pRateCtrl, RdCost* pRdCost )
{
  m_pcEncCfg      = pCfg;
  m_pcRateCtrl    = pRateCtrl;
  m_pcRdCost      = pRdCost;
  m_fastDeltaQP   = false;
#if SHARP_LUMA_DELTA_QP
  m_lumaQPOffset  = 0;

  initLumaDeltaQpLUT();
#endif
}

// TODO: Verify if there is another function like this in another file (without_BAK)
// Used for debugging purposes. Print the name of the input encoding mode
void translateEncTestModeType_BAK(EncTestModeType t){
  
  switch(t){
    case(ETM_HASH_INTER):
      printf("ETM_HASH_INTER,");
      break;
    case(ETM_MERGE_SKIP):
      printf("ETM_MERGE_SKIP,");
      break;
    case(ETM_INTER_ME):
      printf("ETM_INTER_ME,");
      break;
    case(ETM_AFFINE):
      printf("ETM_AFFINE,");
      break;
    case(ETM_MERGE_GEO):
      printf("ETM_MERGE_GEO,");
      break;
    case(ETM_INTRA):
      printf("ETM_INTRA,");
      break;
    case(ETM_PALETTE):
      printf("ETM_PALETTE,");
      break;
    case(ETM_SPLIT_QT):
      printf("ETM_SPLIT_QT,");
      break;
    case(ETM_SPLIT_BT_H):
      printf("ETM_SPLIT_BT_H,");
      break;
    case(ETM_SPLIT_BT_V):
      printf("ETM_SPLIT_BT_V,");
      break;
    case(ETM_SPLIT_TT_H):
      printf("ETM_SPLIT_TT_H,");
      break;
    case(ETM_SPLIT_TT_V):
      printf("ETM_SPLIT_TT_V,");
      break;      
    case(ETM_POST_DONT_SPLIT):
      printf("ETM_POST_DONT_SPLIT,");
      break; 
    #if REUSE_CU_RESULTS
    case(ETM_RECO_CACHED):
      printf("ETM_RECO_CACHED,");
      break; 
    #endif  
    case(ETM_TRIGGER_IMV_LIST):
      printf("ETM_TRIGGER_IMV_LIST,");
      break; 
    case(ETM_IBC):
      printf("ETM_IBC,");
      break;  
    case(ETM_IBC_MERGE):
      printf("ETM_IBC_MERGE,");
      break;   
    case(ETM_INVALID):
      printf("ETM_INVALID,");
      break;         
  }
  
  return;
}

// Tests if the current CS, partitioned with a given split mode, will produce children-CS that are supported by affine prediction
bool isChildrenAffineCompatible(const CodingStructure& cs, EncTestModeType type){
  
  if(
         cs.area.lwidth()<CUSTOM_SIZE || cs.area.lheight()<CUSTOM_SIZE        // Current block has at least one dimension smaller than 16 and is not supported by affine
      || (cs.area.lwidth()==CUSTOM_SIZE    && cs.area.lheight()==CUSTOM_SIZE)     // Current block is the smallest size supported by affine, smaller blocks are unsupported
      || (cs.area.lwidth()<=CUSTOM_SIZE    && ((type==ETM_SPLIT_QT) || (type==ETM_SPLIT_BT_V) || (type==ETM_SPLIT_TT_V) ) ) // Smallest width and any vertical partitioning produces unsupported
      || (cs.area.lheight()<=CUSTOM_SIZE   && ((type==ETM_SPLIT_QT) || (type==ETM_SPLIT_BT_H) || (type==ETM_SPLIT_TT_H) ) ) // Smallest height and any vertical partitioning produces unsupported
      || (cs.area.lwidth()<=2*CUSTOM_SIZE  && (type==ETM_SPLIT_TT_V) ) // Width 32 and ternary partition produces CUs with width=8
      || (cs.area.lheight()<=2*CUSTOM_SIZE && (type==ETM_SPLIT_TT_H) ) // Height 32 and ternary partition produces CUs with height=8
    )
    return false;
  else
    return true;
  
}

// This function was heavily modified to allow controling what encoding modes (prediction and split) will be tested for the current CU based on the customized encoding macros
bool EncModeCtrl::tryModeMaster( const EncTestMode& encTestmode, const CodingStructure &cs, Partitioner& partitioner,  bool onlyAllowAffine)
{ 
  
  // There are four cases here
  // (1) We are conducting the original mode selection. Whether for the I-Slice or when the original heuristics are enabled
  // (2) We are avoiding all SPLIT and INTER_ME early terminations, and allowing all prediction modes on the extra blocks (FULL encoding)
  // (3) We are enforcing the encoder to test all block sizes allowed by affine prediction, but we allow all prediciton modes on these extra blocks
  // (4) We are enforcing the encoder to test all block sizes allowed by affine prediction, but the extra blocks can only be encoded with affine prediction unipred
  
  // Case (1) occurs for the first frame in LowDelay and when ORIGINAL_TREE_HEURISTICS is enabled
  // Case (2) occurs when NO_TREE_HEURISTICS is enabled
  // Case (3) occurs when CUSTOMIZE_TREE_HEURISTICS is enabled and ENFORCE_AFFINE_ON_EXTRA BLOCKS is disabled
  // Case (4) occurs when both CUSTOMIZE_TREE_HEURISTICS and ENFORCE_AFFINE_ON_EXTRA BLOCKS are enabled
  // The value of input parameter onlyAllowAffine depends on ENFORCE_AFFINE_ON_EXTRA_BLOCKS and on the context in which tryModeMaster is called (in CU initialization we allow all modes, in CU encoding we only allow affine
  // onlyAllowAffine CANNOT BE TRUE while CUSTOMIZE_TREE_HEURISTICS is false
  
 
  // This "original" represents the original tryMode result, with all heuristics enabled
  bool tryMode_orig = tryMode_original( encTestmode, cs, partitioner );
  
  // This "custom" represents the customized tryMode result, the modifications are
  // (1) We enforce the encoder to test all block sizes or all block sizes compatible with affine, depending on CUSTOMIZE_TREE_HEURISTICS AND NO_TREE_HEURISTICS
  // (2) We disable all early terminations for inter prediction (merge/skip are still early terminated).
  bool tryMode_custom = tryMode( encTestmode, cs, partitioner );
  
  // Shows when the original and customized tryMode are producing divergent results
  if(storch::sHEUR_originalTreeHeuristics==0 && storch::sTRACE_xCompressCU && storch::sTRACE_encodingModes && storch::sTRACE_tryModeDivergence && (storch::sTRACE_onlyAffineSizes==0 || storch::isAffineSize(cs.area.lwidth(), cs.area.lheight()))){
    if(cs.area.lx()==0 && cs.area.ly()==64 && cs.area.lwidth()==64 && cs.area.lheight()==64){
      printf("  tryMode ");
      translateEncTestModeType_BAK(encTestmode.type);
      printf("  Orig/Custom %d/%d\n", tryMode_orig, tryMode_custom);
    }
  }  

  // In the first frame (I-Slice) we do not interfere. Only intraframe prediction is used
  if(cs.picture->poc==0)
    return tryMode_orig;
    
  // If both versions of tryMode agree, then return the agreed result and do not interfere on the partitioning/testing of other blocks
  if(tryMode_orig == tryMode_custom)
    return tryMode_orig;
  
  if(storch::sHEUR_noTreeHeuristics) // When NO_TREE_HEURISTICS is enabled, we always return the custom tryMode (disable all early terminations for SPLIT and INTER_ME)
    return tryMode_custom;
  
  // At this point we know that NO_TREE_HEURISTICS is disabled. If CUSTOMIZE_TREE_HEURISTICS is false as well, then we are conducting the original encoding
  // This is the "original" case
  if (storch::sHEUR_customizeTreeHeuristics==false && onlyAllowAffine==false)
      return tryMode_orig;
   
  
  // From now on we are interfering on the encoder decisions, and there are two paths:
  // (1) We are testing all affine block sizes and allowing ANY PREDICTION MODE    on these extra blocks (CUSTOMIZE_TREE_HEURISTICS==1 and onlyAllowAffine=0)
  // (2) We are testing all affine block sizes and allowing ONLY AFFINE PREDICTION on these extra blocks (CUSTOMIZE_TREE_HEURISTICS==1 and onlyAllowAffine=1)
  
  // In case the current mode is a SPLIT, then the decision is based on CUSTOMIZE_TREE_HEURISTICS and onlyAllowAffine
  // CUSTOMIZE_TREE_HEURISTICS will control if we will test the current partition or not, whereas onlyAllowAffine will control what encoding modes are available for children blocks
    
  if( isModeSplit(encTestmode) == true ){
    if(storch::sHEUR_customizeTreeHeuristics){ // This is guaranteed to be true. If it was false the encoder would not get to this point
      if(onlyAllowAffine==1){ // If onlyAllowAffine is true, the children blocks will support ONLY AFFINE PREDICTION
        if(storch::skipNonAffineUnipred_Children==0) // If children is already enabled we should not overwrite it
        {
          storch::skipNonAffineUnipred_Children = 1;
          storch::skipNonAffineUnipred_Children_Area = cs.area; // Trace block are to know what CUs are its children and to know when to disable the skipping
          storch::skipNonAffineUnipred_Children_TriggerMode = encTestmode.type; // Store the split that triggered the condition. We may not interfere in other splits for the same root block
          if(storch::sTRACE_debugEnableDisableChildren)
            printf(">> ENABLED CHILDREN AFFINE (tryModeMaster @(%dx%d [%dx%d])\n", cs.area.lx(), cs.area.ly(), cs.area.lwidth(), cs.area.lheight());          
        }
      }
      return tryMode_custom;
    }
    printf("ERROR @ tryModeMaster: Should not get here\n");
  }

  // Voltar aqui e colocar uns print
  
  // In case the current mode is NOT A SPLIT it must be a PREDICTION. This decision is based solely on onlyAllowAffine
  if( isModeNoSplit(encTestmode) == true ){
    if(onlyAllowAffine == false){ // If onlyAllowAffine is false, then we can test ANY PREDICTIN MODE. This is the case where we enforce all affine block sizes but allow all prediction modes
      printf("ENTROU AQUI\n");
      return tryMode_orig; 
    }
    else if(onlyAllowAffine == true){ // If onlyAllowAffine is true, then we can test ONLY AFFINE for the current block
      storch::skipNonAffineUnipred_Current = 1;             // In case tryMode_custom has an intra/merge/skip mode, this assignment makes the encoder skip the prediction mode on the next iteration of xCompressCU
      storch::skipNonAffineUnipred_Current_Area = cs.area;  // Also, if the current mode is INTER, skipNonAffineUnipred_Current is used to choose between conducting only affine or all inter inside PredInterSearch
      if(storch::sTRACE_debugEnableDisableChildren)
        printf(">> ENABLED CURRENT AFFINE  (EncModeCtrl)\n");
      return tryMode_custom;
    }
  }

  printf("ERROR @ tryModeMaster: Should not get to the end without returning anything\n");
  return tryMode_orig;
}

void EncModeCtrl::setEarlySkipDetected()
{
  m_ComprCUCtxList.back().earlySkip = true;
}

void EncModeCtrl::xExtractFeatures( const EncTestMode encTestmode, CodingStructure& cs )
{
  CHECK( cs.features.size() < NUM_ENC_FEATURES, "Features vector is not initialized" );

  cs.features[ENC_FT_DISTORTION     ] = double( cs.dist              );
  cs.features[ENC_FT_FRAC_BITS      ] = double( cs.fracBits          );
  cs.features[ENC_FT_RD_COST        ] = double( cs.cost              );
  cs.features[ENC_FT_ENC_MODE_TYPE  ] = double( encTestmode.type     );
  cs.features[ENC_FT_ENC_MODE_OPTS  ] = double( encTestmode.opts     );
}

// This function was modified to allow forcing the encoder to test more block sizes, disabling the early terminations adaptively
bool EncModeCtrl::nextMode( const CodingStructure &cs, Partitioner &partitioner, bool onlyAllowAffine )
{
  
  // Used for debugging purposes. Lists all encoding modes in the list of "next modes" before they are actualy tested or discarded
  if(0 && cs.picture->poc>0){
    printf("CS,X=%d,Y=%d,W=%d,H=%d\n", cs.area.lx(), cs.area.ly(), cs.area.lwidth(), cs.area.lheight());
    EncTestMode *all = m_ComprCUCtxList.back().testModes.data();
    for(int el=0; el<m_ComprCUCtxList.back().testModes.size(); el++){
      EncTestModeType curr = all[el].type;
      EncTestModeOpts opts = all[el].opts;
      
      switch(curr){
        case ETM_HASH_INTER:
          printf("ETM_HASH_INTER,%d\n",opts);
          break;
        case ETM_MERGE_SKIP:
          printf("ETM_MERGE_SKIP,%d\n",opts);
          break;
        case ETM_INTER_ME:
          printf("ETM_INTER_ME,%d\n",opts);
          break;
        case ETM_AFFINE:
          printf("ETM_AFFINE,%d\n",opts);
          break;
        case ETM_MERGE_GEO:
          printf("ETM_MERGE_GEO\n");
          break;          
        case ETM_INTRA:
          printf("ETM_INTRA\n");
          break;
        case ETM_PALETTE:
          printf("ETM_PALETTE\n");
          break;
          
        case ETM_SPLIT_QT:
          printf("ETM_SPLIT_QT\n");
          break;         
        case ETM_SPLIT_BT_H:
          printf("ETM_SPLIT_BT_H\n");
          break;
        case ETM_SPLIT_BT_V:
          printf("ETM_SPLIT_BT_V\n");
          break;
        case ETM_SPLIT_TT_H:
          printf("ETM_SPLIT_TT_H\n");
          break;
        case ETM_SPLIT_TT_V:
          printf("ETM_SPLIT_TT_V\n");
          break;
          
        case ETM_POST_DONT_SPLIT:
          printf("ETM_POST_DONT_SPLIT\n");
          break;
          
        default:
          printf("Others\n");
          break;
      }
    } 
  }
  
  // Copy the current mode for "last" position, which is tested first
  m_ComprCUCtxList.back().lastTestMode = m_ComprCUCtxList.back().testModes.back();
  
  // Removes the current mode
  m_ComprCUCtxList.back().testModes.pop_back();

  // Conducts some manipulations in the list of modes.
  // In summary, it goes over the list checking if each mode should be tested or not (tryModeMaster)
  // When a mode SHOULD NOT be tested, it is "popped" out of the list and the next mode is checked
  // When a mode SHOULD be tested, it exits the loop and this mode is kept in the ".back() position"
  // After the while, it returns whether the list is empty or not: if it is not empty, a valid mode is in the ".back() position"
  // The "onlyAllowAffine" parameter is used to define if non-affine predictin modes can be tested or not (based on ENFORCE_AFFINE_ON_EXTRA_BLOCKS and if we are in the encoding or initialization stage)
  while( !m_ComprCUCtxList.back().testModes.empty() && !tryModeMaster( currTestMode(), cs, partitioner, onlyAllowAffine ) ) 
  {
    m_ComprCUCtxList.back().testModes.pop_back();
  }

  // If the list is not empty, the mode in ".back() position" will be tested in xCompressCU
  return !m_ComprCUCtxList.back().testModes.empty(); 
}

EncTestMode EncModeCtrl::currTestMode() const
{
  return m_ComprCUCtxList.back().testModes.back();
}

EncTestMode EncModeCtrl::lastTestMode() const
{
  return m_ComprCUCtxList.back().lastTestMode;
}

bool EncModeCtrl::anyMode() const
{
  return !m_ComprCUCtxList.back().testModes.empty();
}

void EncModeCtrl::setBest( CodingStructure& cs )
{
  if( cs.cost != MAX_DOUBLE && !cs.cus.empty() )
  {
    m_ComprCUCtxList.back().bestCS = &cs;
    m_ComprCUCtxList.back().bestCU = cs.cus[0];
    m_ComprCUCtxList.back().bestTU = cs.cus[0]->firstTU;
    m_ComprCUCtxList.back().lastTestMode = getCSEncMode( cs );
  }
}

void EncModeCtrl::xGetMinMaxQP( int& minQP, int& maxQP, const CodingStructure& cs, const Partitioner &partitioner, const int baseQP, const SPS& sps, const PPS& pps, const PartSplit splitMode )
{
  if( m_pcEncCfg->getUseRateCtrl() )
  {
    minQP = m_pcRateCtrl->getRCQP();
    maxQP = m_pcRateCtrl->getRCQP();
    return;
  }

  const unsigned subdivIncr = (splitMode == CU_QUAD_SPLIT) ? 2 : (splitMode == CU_BT_SPLIT) ? 1 : 0;
  const bool qgEnable = partitioner.currQgEnable(); // QG possible at current level
  const bool qgEnableChildren = qgEnable && ((partitioner.currSubdiv + subdivIncr) <= cs.slice->getCuQpDeltaSubdiv()) && (subdivIncr > 0); // QG possible at next level
  const bool isLeafQG = (qgEnable && !qgEnableChildren);

  if( isLeafQG ) // QG at deepest level
  {
    int deltaQP = m_pcEncCfg->getMaxDeltaQP();
    minQP = Clip3( -sps.getQpBDOffset( CHANNEL_TYPE_LUMA ), MAX_QP, baseQP - deltaQP );
    maxQP = Clip3( -sps.getQpBDOffset( CHANNEL_TYPE_LUMA ), MAX_QP, baseQP + deltaQP );
  }
  else if( qgEnableChildren ) // more splits and not the deepest QG level
  {
    minQP = baseQP;
    maxQP = baseQP;
  }
  else // deeper than QG
  {
    minQP = cs.currQP[partitioner.chType];
    maxQP = cs.currQP[partitioner.chType];
  }
}


int EncModeCtrl::xComputeDQP( const CodingStructure &cs, const Partitioner &partitioner )
{
  Picture* picture    = cs.picture;
  unsigned uiAQDepth  = std::min( partitioner.currSubdiv/2, ( uint32_t ) picture->aqlayer.size() - 1 );
  AQpLayer* pcAQLayer = picture->aqlayer[uiAQDepth];

  double dMaxQScale   = pow( 2.0, m_pcEncCfg->getQPAdaptationRange() / 6.0 );
  double dAvgAct      = pcAQLayer->getAvgActivity();
  double dCUAct       = pcAQLayer->getActivity( cs.area.Y().topLeft() );
  double dNormAct     = ( dMaxQScale*dCUAct + dAvgAct ) / ( dCUAct + dMaxQScale*dAvgAct );
  double dQpOffset    = log( dNormAct ) / log( 2.0 ) * 6.0;
  int    iQpOffset    = int( floor( dQpOffset + 0.49999 ) );
  return iQpOffset;
}


#if SHARP_LUMA_DELTA_QP
void EncModeCtrl::initLumaDeltaQpLUT()
{
  const LumaLevelToDeltaQPMapping &mapping = m_pcEncCfg->getLumaLevelToDeltaQPMapping();

  if( !mapping.isEnabled() )
  {
    return;
  }

  // map the sparse LumaLevelToDeltaQPMapping.mapping to a fully populated linear table.

  int         lastDeltaQPValue = 0;
  std::size_t nextSparseIndex = 0;
  for( int index = 0; index < LUMA_LEVEL_TO_DQP_LUT_MAXSIZE; index++ )
  {
    while( nextSparseIndex < mapping.mapping.size() && index >= mapping.mapping[nextSparseIndex].first )
    {
      lastDeltaQPValue = mapping.mapping[nextSparseIndex].second;
      nextSparseIndex++;
    }
    m_lumaLevelToDeltaQPLUT[index] = lastDeltaQPValue;
  }
}

int EncModeCtrl::calculateLumaDQP( const CPelBuf& rcOrg )
{
  double avg = 0;

  // Get QP offset derived from Luma level
#if !WCG_EXT
  if( m_pcEncCfg->getLumaLevelToDeltaQPMapping().mode == LUMALVL_TO_DQP_AVG_METHOD )
#else
  CHECK( m_pcEncCfg->getLumaLevelToDeltaQPMapping().mode != LUMALVL_TO_DQP_AVG_METHOD, "invalid delta qp mode" );
#endif
  {
    // Use average luma value
    avg = (double) rcOrg.computeAvg();
  }
#if !WCG_EXT
  else
  {
    // Use maximum luma value
    int maxVal = 0;
    for( uint32_t y = 0; y < rcOrg.height; y++ )
    {
      for( uint32_t x = 0; x < rcOrg.width; x++ )
      {
        const Pel& v = rcOrg.at( x, y );
        if( v > maxVal )
        {
          maxVal = v;
        }
      }
    }
    // use a percentage of the maxVal
    avg = ( double ) maxVal * m_pcEncCfg->getLumaLevelToDeltaQPMapping().maxMethodWeight;
  }
#endif
  int lumaBD = m_pcEncCfg->getBitDepth(CHANNEL_TYPE_LUMA);
  int lumaIdxOrg = Clip3<int>(0, int(1 << lumaBD) - 1, int(avg + 0.5));
  int lumaIdx = lumaBD < 10 ? lumaIdxOrg << (10 - lumaBD) : lumaBD > 10 ? lumaIdxOrg >> (lumaBD - 10) : lumaIdxOrg;
  int QP = m_lumaLevelToDeltaQPLUT[lumaIdx];
  return QP;
}
#endif

void CacheBlkInfoCtrl::create()
{
  const unsigned numPos = MAX_CU_SIZE >> MIN_CU_LOG2;

  m_numWidths  = gp_sizeIdxInfo->numWidths();
  m_numHeights = gp_sizeIdxInfo->numHeights();

  bool isLog2MttPartitioning = !!dynamic_cast<SizeIndexInfoLog2*>( gp_sizeIdxInfo );

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      m_codedCUInfo[x][y] = new CodedCUInfo**[m_numWidths];

      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( !( gp_sizeIdxInfo->isCuSize( gp_sizeIdxInfo->sizeFrom( wIdx ) ) && x + ( gp_sizeIdxInfo->sizeFrom( wIdx ) >> MIN_CU_LOG2 ) <= ( MAX_CU_SIZE >> MIN_CU_LOG2 ) ) )
        {
          m_codedCUInfo[x][y][wIdx] = nullptr;
          continue;
        }

        const int wLog2 = floorLog2( gp_sizeIdxInfo->sizeFrom( wIdx ) );

        if( isLog2MttPartitioning && ( ( x << MIN_CU_LOG2 ) & ( ( 1 << ( wLog2 - 1 ) ) - 1 ) ) != 0 )
        {
          m_codedCUInfo[x][y][wIdx] = nullptr;
          continue;
        }

        m_codedCUInfo[x][y][wIdx] = new CodedCUInfo*[gp_sizeIdxInfo->numHeights()];

        for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
        {
          if( !( gp_sizeIdxInfo->isCuSize( gp_sizeIdxInfo->sizeFrom( hIdx ) ) && y + ( gp_sizeIdxInfo->sizeFrom( hIdx ) >> MIN_CU_LOG2 ) <= ( MAX_CU_SIZE >> MIN_CU_LOG2 ) ) )
          {
            m_codedCUInfo[x][y][wIdx][hIdx] = nullptr;
            continue;
          }
          
          const int hLog2 = floorLog2( gp_sizeIdxInfo->sizeFrom( hIdx ) );

          if( isLog2MttPartitioning && ( ( ( y << MIN_CU_LOG2 ) & ( ( 1 << ( hLog2 - 1 ) ) - 1 ) ) != 0 ) )
          {
            m_codedCUInfo[x][y][wIdx][hIdx] = nullptr;
            continue;
          }

          m_codedCUInfo[x][y][wIdx][hIdx] = new CodedCUInfo;
        }
      }
    }
  }
}

void CacheBlkInfoCtrl::destroy()
{
  const unsigned numPos = MAX_CU_SIZE >> MIN_CU_LOG2;

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( m_codedCUInfo[x][y][wIdx] )
        {
          for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
          {
            if( m_codedCUInfo[x][y][wIdx][hIdx] )
            {
              delete m_codedCUInfo[x][y][wIdx][hIdx];
            }
          }

          delete[] m_codedCUInfo[x][y][wIdx];
        }
      }

      delete[] m_codedCUInfo[x][y];
    }
  }
}

void CacheBlkInfoCtrl::init( const Slice &slice )
{
  const unsigned numPos = MAX_CU_SIZE >> MIN_CU_LOG2;

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( m_codedCUInfo[x][y][wIdx] )
        {
          for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
          {
            if( m_codedCUInfo[x][y][wIdx][hIdx] )
            {
              memset( m_codedCUInfo[x][y][wIdx][hIdx], 0, sizeof( CodedCUInfo ) );
            }
          }
        }
      }
    }
  }

  m_slice_chblk = &slice;
}

CodedCUInfo& CacheBlkInfoCtrl::getBlkInfo( const UnitArea& area )
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  return *m_codedCUInfo[idx1][idx2][idx3][idx4];
}

bool CacheBlkInfoCtrl::isSkip( const UnitArea& area )
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  return m_codedCUInfo[idx1][idx2][idx3][idx4]->isSkip;
}

char CacheBlkInfoCtrl::getSelectColorSpaceOption(const UnitArea& area)
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx(area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4);

  return m_codedCUInfo[idx1][idx2][idx3][idx4]->selectColorSpaceOption;
}

bool CacheBlkInfoCtrl::isMMVDSkip(const UnitArea& area)
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx(area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4);

  return m_codedCUInfo[idx1][idx2][idx3][idx4]->isMMVDSkip;
}

void CacheBlkInfoCtrl::setMv( const UnitArea& area, const RefPicList refPicList, const int iRefIdx, const Mv& rMv )
{
  if( iRefIdx >= MAX_STORED_CU_INFO_REFS ) return;

  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  m_codedCUInfo[idx1][idx2][idx3][idx4]->saveMv [refPicList][iRefIdx] = rMv;
  m_codedCUInfo[idx1][idx2][idx3][idx4]->validMv[refPicList][iRefIdx] = true;
}

bool CacheBlkInfoCtrl::getMv( const UnitArea& area, const RefPicList refPicList, const int iRefIdx, Mv& rMv ) const
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  if( iRefIdx >= MAX_STORED_CU_INFO_REFS )
  {
    rMv = m_codedCUInfo[idx1][idx2][idx3][idx4]->saveMv[refPicList][0];
    return false;
  }

  rMv = m_codedCUInfo[idx1][idx2][idx3][idx4]->saveMv[refPicList][iRefIdx];
  return m_codedCUInfo[idx1][idx2][idx3][idx4]->validMv[refPicList][iRefIdx];
}

void SaveLoadEncInfoSbt::init( const Slice &slice )
{
  m_sliceSbt = &slice;
}

void SaveLoadEncInfoSbt::create()
{
  int numSizeIdx = gp_sizeIdxInfo->idxFrom( SBT_MAX_SIZE ) - MIN_CU_LOG2 + 1;
  int numPosIdx = MAX_CU_SIZE >> MIN_CU_LOG2;

  m_saveLoadSbt = new SaveLoadStructSbt***[numPosIdx];

  for( int xIdx = 0; xIdx < numPosIdx; xIdx++ )
  {
    m_saveLoadSbt[xIdx] = new SaveLoadStructSbt**[numPosIdx];
    for( int yIdx = 0; yIdx < numPosIdx; yIdx++ )
    {
      m_saveLoadSbt[xIdx][yIdx] = new SaveLoadStructSbt*[numSizeIdx];
      for( int wIdx = 0; wIdx < numSizeIdx; wIdx++ )
      {
        m_saveLoadSbt[xIdx][yIdx][wIdx] = new SaveLoadStructSbt[numSizeIdx];
      }
    }
  }
}

void SaveLoadEncInfoSbt::destroy()
{
  int numSizeIdx = gp_sizeIdxInfo->idxFrom( SBT_MAX_SIZE ) - MIN_CU_LOG2 + 1;
  int numPosIdx = MAX_CU_SIZE >> MIN_CU_LOG2;

  for( int xIdx = 0; xIdx < numPosIdx; xIdx++ )
  {
    for( int yIdx = 0; yIdx < numPosIdx; yIdx++ )
    {
      for( int wIdx = 0; wIdx < numSizeIdx; wIdx++ )
      {
        delete[] m_saveLoadSbt[xIdx][yIdx][wIdx];
      }
      delete[] m_saveLoadSbt[xIdx][yIdx];
    }
    delete[] m_saveLoadSbt[xIdx];
  }
  delete[] m_saveLoadSbt;
}

uint16_t SaveLoadEncInfoSbt::findBestSbt( const UnitArea& area, const uint32_t curPuSse )
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( area.Y(), *m_sliceSbt->getPPS()->pcv, idx1, idx2, idx3, idx4 );
  SaveLoadStructSbt* pSbtSave = &m_saveLoadSbt[idx1][idx2][idx3 - MIN_CU_LOG2][idx4 - MIN_CU_LOG2];

  for( int i = 0; i < pSbtSave->numPuInfoStored; i++ )
  {
    if( curPuSse == pSbtSave->puSse[i] )
    {
      return pSbtSave->puSbt[i] + ( pSbtSave->puTrs[i] << 8 );
    }
  }

  return MAX_UCHAR + ( MAX_UCHAR << 8 );
}

bool SaveLoadEncInfoSbt::saveBestSbt( const UnitArea& area, const uint32_t curPuSse, const uint8_t curPuSbt, const uint8_t curPuTrs )
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( area.Y(), *m_sliceSbt->getPPS()->pcv, idx1, idx2, idx3, idx4 );
  SaveLoadStructSbt* pSbtSave = &m_saveLoadSbt[idx1][idx2][idx3 - MIN_CU_LOG2][idx4 - MIN_CU_LOG2];

  if( pSbtSave->numPuInfoStored == SBT_NUM_SL )
  {
    return false;
  }

  pSbtSave->puSse[pSbtSave->numPuInfoStored] = curPuSse;
  pSbtSave->puSbt[pSbtSave->numPuInfoStored] = curPuSbt;
  pSbtSave->puTrs[pSbtSave->numPuInfoStored] = curPuTrs;
  pSbtSave->numPuInfoStored++;
  return true;
}

void SaveLoadEncInfoSbt::resetSaveloadSbt( int maxSbtSize )
{
  int numSizeIdx = gp_sizeIdxInfo->idxFrom( maxSbtSize ) - MIN_CU_LOG2 + 1;
  int numPosIdx = MAX_CU_SIZE >> MIN_CU_LOG2;

  for( int xIdx = 0; xIdx < numPosIdx; xIdx++ )
  {
    for( int yIdx = 0; yIdx < numPosIdx; yIdx++ )
    {
      for( int wIdx = 0; wIdx < numSizeIdx; wIdx++ )
      {
        memset( m_saveLoadSbt[xIdx][yIdx][wIdx], 0, numSizeIdx * sizeof( SaveLoadStructSbt ) );
      }
    }
  }
}

bool CacheBlkInfoCtrl::getInter(const UnitArea& area)
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx(area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4);

  return m_codedCUInfo[idx1][idx2][idx3][idx4]->isInter;
}
void CacheBlkInfoCtrl::setBcwIdx(const UnitArea& area, uint8_t gBiIdx)
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx(area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4);

  m_codedCUInfo[idx1][idx2][idx3][idx4]->BcwIdx = gBiIdx;
}
uint8_t CacheBlkInfoCtrl::getBcwIdx(const UnitArea& area)
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx(area.Y(), *m_slice_chblk->getPPS()->pcv, idx1, idx2, idx3, idx4);

  return m_codedCUInfo[idx1][idx2][idx3][idx4]->BcwIdx;
}

#if REUSE_CU_RESULTS
static bool isTheSameNbHood( const CodingUnit &cu, const CodingStructure& cs, const Partitioner &partitioner
                            , const PredictionUnit &pu, int picW, int picH
                           )
{
  if( cu.chType != partitioner.chType )
  {
    return false;
  }

  const PartitioningStack &ps = partitioner.getPartStack();

  int i = 1;

  for( ; i < ps.size(); i++ )
  {
    if( ps[i].split != CU::getSplitAtDepth( cu, i - 1 ) )
    {
      break;
    }
  }

  const UnitArea &cmnAnc = ps[i - 1].parts[ps[i - 1].idx];
  const UnitArea cuArea  = CS::getArea( cs, cu, partitioner.chType );
//#endif

  for( int i = 0; i < cmnAnc.blocks.size(); i++ )
  {
    if( i < cuArea.blocks.size() && cuArea.blocks[i].valid() && cuArea.blocks[i].pos() != cmnAnc.blocks[i].pos() )
    {
      return false;
    }
  }

  return true;
}

void BestEncInfoCache::create( const ChromaFormat chFmt )
{
  const unsigned numPos = MAX_CU_SIZE >> MIN_CU_LOG2;

  m_numWidths  = gp_sizeIdxInfo->numWidths();
  m_numHeights = gp_sizeIdxInfo->numHeights();

  bool isLog2MttPartitioning = !!dynamic_cast<SizeIndexInfoLog2*>( gp_sizeIdxInfo );

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      m_bestEncInfo[x][y] = new BestEncodingInfo**[m_numWidths];

      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( !( gp_sizeIdxInfo->isCuSize( gp_sizeIdxInfo->sizeFrom( wIdx ) ) && x + ( gp_sizeIdxInfo->sizeFrom( wIdx ) >> MIN_CU_LOG2 ) <= ( MAX_CU_SIZE >> MIN_CU_LOG2 ) ) )
        {
          m_bestEncInfo[x][y][wIdx] = nullptr;
          continue;
        }

        const int wLog2 = floorLog2( gp_sizeIdxInfo->sizeFrom( wIdx ) );

        if( isLog2MttPartitioning && ( ( x << MIN_CU_LOG2 ) & ( ( 1 << ( wLog2 - 1 ) ) - 1 ) ) != 0 )
        {
          m_bestEncInfo[x][y][wIdx] = nullptr;
          continue;
        }

        m_bestEncInfo[x][y][wIdx] = new BestEncodingInfo*[gp_sizeIdxInfo->numHeights()];

        for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
        {
          if( !( gp_sizeIdxInfo->isCuSize( gp_sizeIdxInfo->sizeFrom( hIdx ) ) && y + ( gp_sizeIdxInfo->sizeFrom( hIdx ) >> MIN_CU_LOG2 ) <= ( MAX_CU_SIZE >> MIN_CU_LOG2 ) ) )
          {
            m_bestEncInfo[x][y][wIdx][hIdx] = nullptr;
            continue;
          }
          
          const int hLog2 = floorLog2( gp_sizeIdxInfo->sizeFrom( hIdx ) );

          if( isLog2MttPartitioning && ( ( ( y << MIN_CU_LOG2 ) & ( ( 1 << ( hLog2 - 1 ) ) - 1 ) ) != 0 ) )
          {
            m_bestEncInfo[x][y][wIdx][hIdx] = nullptr;
            continue;
          }

          m_bestEncInfo[x][y][wIdx][hIdx] = new BestEncodingInfo;

          int w = gp_sizeIdxInfo->sizeFrom( wIdx );
          int h = gp_sizeIdxInfo->sizeFrom( hIdx );

          const UnitArea area( chFmt, Area( 0, 0, w, h ) );

          new ( &m_bestEncInfo[x][y][wIdx][hIdx]->cu ) CodingUnit    ( area );
          new ( &m_bestEncInfo[x][y][wIdx][hIdx]->pu ) PredictionUnit( area );
#if REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
          m_bestEncInfo[x][y][wIdx][hIdx]->numTus = 0;
          for( int i = 0; i < MAX_NUM_TUS; i++ )
          {
            new ( &m_bestEncInfo[x][y][wIdx][hIdx]->tus[i] ) TransformUnit( area );
          }
#else
          new ( &m_bestEncInfo[x][y][wIdx][hIdx]->tu ) TransformUnit( area );
#endif

          m_bestEncInfo[x][y][wIdx][hIdx]->poc      = -1;
          m_bestEncInfo[x][y][wIdx][hIdx]->testMode = EncTestMode();
        }
      }
    }
  }
}

void BestEncInfoCache::destroy()
{
  const unsigned numPos = MAX_CU_SIZE >> MIN_CU_LOG2;

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( m_bestEncInfo[x][y][wIdx] )
        {
          for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
          {
            if( m_bestEncInfo[x][y][wIdx][hIdx] )
            {
              delete m_bestEncInfo[x][y][wIdx][hIdx];
            }
          }

          delete[] m_bestEncInfo[x][y][wIdx];
        }
      }

      delete[] m_bestEncInfo[x][y];
    }
  }

  delete[] m_pCoeff;
  delete[] m_pPcmBuf;

  if (m_runType != nullptr)
  {
    delete[] m_runType;
    m_runType = nullptr;
  }
}

void BestEncInfoCache::init( const Slice &slice )
{
  bool isInitialized = m_slice_bencinf;

  m_slice_bencinf = &slice;

  if( isInitialized ) return;

  const unsigned numPos = MAX_CU_SIZE >> MIN_CU_LOG2;

  m_numWidths  = gp_sizeIdxInfo->numWidths();
  m_numHeights = gp_sizeIdxInfo->numHeights();

  size_t numCoeff = 0;

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( m_bestEncInfo[x][y][wIdx] ) for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
        {
          if( m_bestEncInfo[x][y][wIdx][hIdx] )
          {
            for( const CompArea& blk : m_bestEncInfo[x][y][wIdx][hIdx]->cu.blocks )
            {
              numCoeff += blk.area();
            }
          }
        }
      }
    }
  }

#if REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  m_pCoeff  = new TCoeff[numCoeff*MAX_NUM_TUS];
  m_pPcmBuf = new Pel   [numCoeff*MAX_NUM_TUS];
  if (slice.getSPS()->getPLTMode())
  {
    m_runType   = new bool[numCoeff*MAX_NUM_TUS];
  }
#else
  m_pCoeff  = new TCoeff[numCoeff];
  m_pPcmBuf = new Pel   [numCoeff];
  if (slice.getSPS()->getPLTMode())
  {
    m_runType   = new bool[numCoeff];
  }
#endif

  TCoeff *coeffPtr = m_pCoeff;
  Pel    *pcmPtr   = m_pPcmBuf;
  bool   *runTypePtr   = m_runType;
  m_dummyCS.pcv = m_slice_bencinf->getPPS()->pcv;

  for( unsigned x = 0; x < numPos; x++ )
  {
    for( unsigned y = 0; y < numPos; y++ )
    {
      for( int wIdx = 0; wIdx < gp_sizeIdxInfo->numWidths(); wIdx++ )
      {
        if( m_bestEncInfo[x][y][wIdx] ) for( int hIdx = 0; hIdx < gp_sizeIdxInfo->numHeights(); hIdx++ )
        {
          if( m_bestEncInfo[x][y][wIdx][hIdx] )
          {
            TCoeff *coeff[MAX_NUM_TBLOCKS] = { 0, };
            Pel    *pcmbf[MAX_NUM_TBLOCKS] = { 0, };
            bool   *runType[MAX_NUM_TBLOCKS - 1] = { 0, };

#if REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
            for( int i = 0; i < MAX_NUM_TUS; i++ )
            {
              TransformUnit &tu = m_bestEncInfo[x][y][wIdx][hIdx]->tus[i];
              const UnitArea &area = tu;

              for( int i = 0; i < area.blocks.size(); i++ )
              {
                coeff[i] = coeffPtr; coeffPtr += area.blocks[i].area();
                pcmbf[i] = pcmPtr;   pcmPtr += area.blocks[i].area();
                if (i < 2)
                {
                  runType[i]   = runTypePtr;   runTypePtr   += area.blocks[i].area();
                }
              }

              tu.cs = &m_dummyCS;
              tu.init(coeff, pcmbf, runType);
            }
#else
            const UnitArea &area = m_bestEncInfo[x][y][wIdx][hIdx]->tu;

            for( int i = 0; i < area.blocks.size(); i++ )
            {
              coeff[i] = coeffPtr; coeffPtr += area.blocks[i].area();
              pcmbf[i] =   pcmPtr;   pcmPtr += area.blocks[i].area();
              runType[i] = runTypePtr;     runTypePtr += area.blocks[i].area();
              runLength[i] = runLengthPtr; runLengthPtr += area.blocks[i].area();
            }

            m_bestEncInfo[x][y][wIdx][hIdx]->tu.cs = &m_dummyCS;
            m_bestEncInfo[x][y][wIdx][hIdx]->tu.init(coeff, pcmbf, runLength, runType);
#endif
          }
        }
      }
    }
  }
}

bool BestEncInfoCache::setFromCs( const CodingStructure& cs, const Partitioner& partitioner )
{
#if REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  if( cs.cus.size() != 1 || cs.pus.size() != 1 )
#else
  if( cs.cus.size() != 1 || cs.tus.size() != 1 || cs.pus.size() != 1 )
#endif
  {
    return false;
  }

  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( cs.area.Y(), *m_slice_bencinf->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  BestEncodingInfo& encInfo = *m_bestEncInfo[idx1][idx2][idx3][idx4];

  encInfo.poc            =  cs.picture->poc;
  encInfo.cu.repositionTo( *cs.cus.front() );
  encInfo.pu.repositionTo( *cs.pus.front() );
#if !REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  encInfo.tu.repositionTo( *cs.tus.front() );
#endif
  encInfo.cu             = *cs.cus.front();
  encInfo.pu             = *cs.pus.front();
#if REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  int tuIdx = 0;
  for( auto tu : cs.tus )
  {
    encInfo.tus[tuIdx].repositionTo( *tu );
    encInfo.tus[tuIdx].resizeTo( *tu );
    for( auto &blk : tu->blocks )
    {
      if( blk.valid() )
        encInfo.tus[tuIdx].copyComponentFrom( *tu, blk.compID );
    }
    tuIdx++;
  }
  CHECKD( cs.tus.size() > MAX_NUM_TUS, "Exceeding tus array boundaries" );
  encInfo.numTus = cs.tus.size();
#else
  for( auto &blk : cs.tus.front()->blocks )
  {
    if( blk.valid() ) encInfo.tu.copyComponentFrom( *cs.tus.front(), blk.compID );
  }
#endif
  encInfo.testMode       = getCSEncMode( cs );

  return true;
}

bool BestEncInfoCache::isValid( const CodingStructure& cs, const Partitioner& partitioner, int qp )
{
  if( partitioner.treeType == TREE_C )
  {
    return false; //if save & load is allowed for chroma CUs, we should check whether luma info (pred, recon, etc) is the same, which is quite complex
  }
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( cs.area.Y(), *m_slice_bencinf->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  BestEncodingInfo& encInfo = *m_bestEncInfo[idx1][idx2][idx3][idx4];

  if( encInfo.cu.treeType != partitioner.treeType || encInfo.cu.modeType != partitioner.modeType )
  {
    return false;
  }
  if( encInfo.cu.qp != qp || cs.slice->getUseChromaQpAdj())
    return false;
  if( cs.picture->poc != encInfo.poc || CS::getArea( cs, cs.area, partitioner.chType ) != CS::getArea( cs, encInfo.cu, partitioner.chType ) || !isTheSameNbHood( encInfo.cu, cs, partitioner
    , encInfo.pu, (cs.picture->Y().width), (cs.picture->Y().height)
)
    || CU::isIBC(encInfo.cu)
    || partitioner.currQgEnable() || cs.currQP[partitioner.chType] != encInfo.cu.qp
    )
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool BestEncInfoCache::setCsFrom( CodingStructure& cs, EncTestMode& testMode, const Partitioner& partitioner ) const
{
  unsigned idx1, idx2, idx3, idx4;
  getAreaIdx( cs.area.Y(), *m_slice_bencinf->getPPS()->pcv, idx1, idx2, idx3, idx4 );

  BestEncodingInfo& encInfo = *m_bestEncInfo[idx1][idx2][idx3][idx4];

  if( cs.picture->poc != encInfo.poc || CS::getArea( cs, cs.area, partitioner.chType ) != CS::getArea( cs, encInfo.cu, partitioner.chType ) || !isTheSameNbHood( encInfo.cu, cs, partitioner
    , encInfo.pu, (cs.picture->Y().width), (cs.picture->Y().height)
    )
    || partitioner.currQgEnable() || cs.currQP[partitioner.chType] != encInfo.cu.qp
    )
  {
    return false;
  }

  CodingUnit     &cu = cs.addCU( CS::getArea( cs, cs.area, partitioner.chType ), partitioner.chType );
  PredictionUnit &pu = cs.addPU( CS::getArea( cs, cs.area, partitioner.chType ), partitioner.chType );
#if !REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  TransformUnit  &tu = cs.addTU( CS::getArea( cs, cs.area, partitioner.chType ), partitioner.chType );
#endif

  cu          .repositionTo( encInfo.cu );
  pu          .repositionTo( encInfo.pu );
#if !REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  tu          .repositionTo( encInfo.tu );
#endif

  cu          = encInfo.cu;
  pu          = encInfo.pu;
#if REUSE_CU_RESULTS_WITH_MULTIPLE_TUS
  CHECKD( !( encInfo.numTus > 0 ), "Empty tus array" );
  for( int i = 0; i < encInfo.numTus; i++ )
  {
    TransformUnit  &tu = cs.addTU( encInfo.tus[i], partitioner.chType );

    for( auto &blk : tu.blocks )
    {
      if( blk.valid() ) tu.copyComponentFrom( encInfo.tus[i], blk.compID );
    }
  }
#else
  for( auto &blk : tu.blocks )
  {
    if( blk.valid() ) tu.copyComponentFrom( encInfo.tu, blk.compID );
  }
#endif

  testMode    = encInfo.testMode;

  return true;
}

#endif

static bool interHadActive( const ComprCUCtx& ctx )
{
  return ctx.interHad != 0;
}

//////////////////////////////////////////////////////////////////////////
// EncModeCtrlQTBT
//////////////////////////////////////////////////////////////////////////

void EncModeCtrlMTnoRQT::create( const EncCfg& cfg )
{
  CacheBlkInfoCtrl::create();
#if REUSE_CU_RESULTS
  BestEncInfoCache::create( cfg.getChromaFormatIdc() );
#endif
  SaveLoadEncInfoSbt::create();
}

void EncModeCtrlMTnoRQT::destroy()
{
  CacheBlkInfoCtrl::destroy();
#if REUSE_CU_RESULTS
  BestEncInfoCache::destroy();
#endif
  SaveLoadEncInfoSbt::destroy();
}

void EncModeCtrlMTnoRQT::initCTUEncoding( const Slice &slice )
{
  CacheBlkInfoCtrl::init( slice );
#if REUSE_CU_RESULTS
  BestEncInfoCache::init( slice );
#endif
  SaveLoadEncInfoSbt::init( slice );

  CHECK( !m_ComprCUCtxList.empty(), "Mode list is not empty at the beginning of a CTU" );

  m_slice             = &slice;

  if( m_pcEncCfg->getUseE0023FastEnc() )
  {
    if (m_pcEncCfg->getUseCompositeRef())
      m_skipThreshold = ( ( slice.getMinPictureDistance() <= PICTURE_DISTANCE_TH * 2 ) ? FAST_SKIP_DEPTH : SKIP_DEPTH );
    else
      m_skipThreshold = ((slice.getMinPictureDistance() <= PICTURE_DISTANCE_TH) ? FAST_SKIP_DEPTH : SKIP_DEPTH);

  }
  else
  {
    m_skipThreshold = SKIP_DEPTH;
  }
}

void EncModeCtrlMTnoRQT::initCULevel( Partitioner &partitioner, const CodingStructure& cs )
{
  // Min/max depth
  unsigned minDepth = 0;
  unsigned maxDepth = floorLog2(cs.sps->getCTUSize()) - floorLog2(cs.sps->getMinQTSize( m_slice->getSliceType(), partitioner.chType ));
  if( m_pcEncCfg->getUseFastLCTU() )
  {
    if( auto adPartitioner = dynamic_cast<AdaptiveDepthPartitioner*>( &partitioner ) )
    {
      // LARGE CTU
      adPartitioner->setMaxMinDepth( minDepth, maxDepth, cs );
    }
  }

  m_ComprCUCtxList.push_back( ComprCUCtx( cs, minDepth, maxDepth, NUM_EXTRA_FEATURES ) );

  const CodingUnit* cuLeft  = cs.getCU( cs.area.blocks[partitioner.chType].pos().offset( -1, 0 ), partitioner.chType );
  const CodingUnit* cuAbove = cs.getCU( cs.area.blocks[partitioner.chType].pos().offset( 0, -1 ), partitioner.chType );

  // Defines if the quaternary tree should be tested before the binary tree
  const bool qtBeforeBt = ( (  cuLeft  &&  cuAbove  && cuLeft ->qtDepth > partitioner.currQtDepth && cuAbove->qtDepth > partitioner.currQtDepth )
                         || (  cuLeft  && !cuAbove  && cuLeft ->qtDepth > partitioner.currQtDepth )
                         || ( !cuLeft  &&  cuAbove  && cuAbove->qtDepth > partitioner.currQtDepth )
                         || ( !cuAbove && !cuLeft   && cs.area.lwidth() >= ( 32 << cs.slice->getDepth() ) ) )
                         && ( cs.area.lwidth() > ( cs.pcv->getMinQtSize( *cs.slice, partitioner.chType ) << 1 ) );

  // set features
  ComprCUCtx &cuECtx  = m_ComprCUCtxList.back();
  cuECtx.set( BEST_NON_SPLIT_COST,  MAX_DOUBLE );
  cuECtx.set( BEST_VERT_SPLIT_COST, MAX_DOUBLE );
  cuECtx.set( BEST_HORZ_SPLIT_COST, MAX_DOUBLE );
  cuECtx.set( BEST_TRIH_SPLIT_COST, MAX_DOUBLE );
  cuECtx.set( BEST_TRIV_SPLIT_COST, MAX_DOUBLE );
  cuECtx.set( DO_TRIH_SPLIT,        1 );
  cuECtx.set( DO_TRIV_SPLIT,        1 );
  cuECtx.set( BEST_IMV_COST,        MAX_DOUBLE * .5 );
  cuECtx.set( BEST_NO_IMV_COST,     MAX_DOUBLE * .5 );
  cuECtx.set( QT_BEFORE_BT,         qtBeforeBt );
  cuECtx.set( DID_QUAD_SPLIT,       false );
  cuECtx.set( IS_BEST_NOSPLIT_SKIP, false );
  cuECtx.set( MAX_QT_SUB_DEPTH,     0 );

  // QP
  int baseQP = cs.baseQP;
  if (!partitioner.isSepTree(cs) || isLuma(partitioner.chType))
  {
    if (m_pcEncCfg->getUseAdaptiveQP())
    {
      baseQP = Clip3(-cs.sps->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, baseQP + xComputeDQP(cs, partitioner));
    }
#if ENABLE_QPA_SUB_CTU
    else if (m_pcEncCfg->getUsePerceptQPA() && !m_pcEncCfg->getUseRateCtrl() && cs.pps->getUseDQP() && cs.slice->getCuQpDeltaSubdiv() > 0)
    {
      const PreCalcValues &pcv = *cs.pcv;

      if ((partitioner.currArea().lwidth() < pcv.maxCUWidth) && (partitioner.currArea().lheight() < pcv.maxCUHeight) && cs.picture)
      {
        const Position    &pos = partitioner.currQgPos;
        const unsigned mtsLog2 = (unsigned)floorLog2(std::min (cs.sps->getMaxTbSize(), pcv.maxCUWidth));
        const unsigned  stride = pcv.maxCUWidth >> mtsLog2;

        baseQP = cs.picture->m_subCtuQP[((pos.x & pcv.maxCUWidthMask) >> mtsLog2) + stride * ((pos.y & pcv.maxCUHeightMask) >> mtsLog2)];
      }
    }
#endif
#if SHARP_LUMA_DELTA_QP
    if (m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled())
    {
      if (partitioner.currQgEnable())
      {
        m_lumaQPOffset = calculateLumaDQP (cs.getOrgBuf (clipArea (cs.area.Y(), cs.picture->Y())));
      }
      baseQP = Clip3 (-cs.sps->getQpBDOffset (CHANNEL_TYPE_LUMA), MAX_QP, baseQP - m_lumaQPOffset);
    }
#endif
  }
  int minQP = baseQP;
  int maxQP = baseQP;

  xGetMinMaxQP( minQP, maxQP, cs, partitioner, baseQP, *cs.sps, *cs.pps, CU_QUAD_SPLIT );
  bool checkIbc = true;
  if (partitioner.chType == CHANNEL_TYPE_CHROMA)
  {
    checkIbc = false;
  }
  // Add coding modes here
  // NOTE: Working back to front, as a stack, which is more efficient with the container
  // NOTE: First added modes will be processed at the end.

  //////////////////////////////////////////////////////////////////////////
  // Add unit split modes

  // iagostorch debug
  // Used for debugging purposes. Print what partitionings are allowed for a specific block
  int debugCondition = cs.area.lwidth()==0 && cs.area.lheight()==0 && cs.area.lx()==0 && cs.area.ly()==0;
  if( debugCondition){
        printf("initCULevel - POC %d, CU [%dx%d] @ (%d,%d)\n", cs.picture->poc, cs.area.lwidth(), cs.area.lheight(), cs.area.lx(), cs.area.ly());
        printf("    can(TRI_V) - %d\n", partitioner.canSplit( CU_TRIV_SPLIT, cs ));
        printf("    can(TRI_H) - %d\n", partitioner.canSplit( CU_TRIH_SPLIT, cs ));
        printf("    can(BI_V)  - %d\n", partitioner.canSplit( CU_VERT_SPLIT, cs ));
        printf("    can(BI_H)  - %d\n", partitioner.canSplit( CU_HORZ_SPLIT, cs ));  
  }
  
  if( !cuECtx.get<bool>( QT_BEFORE_BT ) )
  {
    for( int qp = maxQP; qp >= minQP; qp-- )
    {
      m_ComprCUCtxList.back().testModes.push_back( { ETM_SPLIT_QT, ETO_STANDARD, qp } );
    }
  }

  if( partitioner.canSplit( CU_TRIV_SPLIT, cs ) )
  {
    // add split modes
    for( int qp = maxQP; qp >= minQP; qp-- )
    {
      m_ComprCUCtxList.back().testModes.push_back( { ETM_SPLIT_TT_V, ETO_STANDARD, qp } );
    }
  }

  if( partitioner.canSplit( CU_TRIH_SPLIT, cs ) )
  {
    // add split modes
    for( int qp = maxQP; qp >= minQP; qp-- )
    {
      m_ComprCUCtxList.back().testModes.push_back( { ETM_SPLIT_TT_H, ETO_STANDARD, qp } );
    }
  }
    
  int minQPq = minQP;
  int maxQPq = maxQP;
  xGetMinMaxQP( minQP, maxQP, cs, partitioner, baseQP, *cs.sps, *cs.pps, CU_BT_SPLIT );
  if( partitioner.canSplit( CU_VERT_SPLIT, cs ) )
  {
    // add split modes
    for( int qp = maxQP; qp >= minQP; qp-- )
    {
      m_ComprCUCtxList.back().testModes.push_back( { ETM_SPLIT_BT_V, ETO_STANDARD, qp } );
    }
    m_ComprCUCtxList.back().set( DID_VERT_SPLIT, true );
  }
  else
  {
    m_ComprCUCtxList.back().set( DID_VERT_SPLIT, false );
  }

  if( partitioner.canSplit( CU_HORZ_SPLIT, cs ) )
  {
    // add split modes
    for( int qp = maxQP; qp >= minQP; qp-- )
    {
      m_ComprCUCtxList.back().testModes.push_back( { ETM_SPLIT_BT_H, ETO_STANDARD, qp } );
    }
    m_ComprCUCtxList.back().set( DID_HORZ_SPLIT, true );
  }
  else
  {
    m_ComprCUCtxList.back().set( DID_HORZ_SPLIT, false );
  }

  if( cuECtx.get<bool>( QT_BEFORE_BT ) )
  {
    for( int qp = maxQPq; qp >= minQPq; qp-- )
    {
      m_ComprCUCtxList.back().testModes.push_back( { ETM_SPLIT_QT, ETO_STANDARD, qp } );
    }
  }

  m_ComprCUCtxList.back().testModes.push_back( { ETM_POST_DONT_SPLIT } );

  xGetMinMaxQP( minQP, maxQP, cs, partitioner, baseQP, *cs.sps, *cs.pps, CU_DONT_SPLIT );

  int  lowestQP = minQP;

  //////////////////////////////////////////////////////////////////////////
  // Add unit coding modes: Intra, InterME, InterMerge ...
  bool tryIntraRdo = true;
  bool tryInterRdo = true;
  bool tryIBCRdo   = true;
  if( partitioner.isConsIntra() )
  {
    tryInterRdo = false;
  }
  else if( partitioner.isConsInter() )
  {
    tryIntraRdo = tryIBCRdo = false;
  }
  checkIbc &= tryIBCRdo;

  for( int qpLoop = maxQP; qpLoop >= minQP; qpLoop-- )
  {
    const int  qp       = std::max( qpLoop, lowestQP );
#if REUSE_CU_RESULTS
    const bool isReusingCu = isValid( cs, partitioner, qp );
    cuECtx.set( IS_REUSING_CU, isReusingCu );
    if( isReusingCu )
    {
      m_ComprCUCtxList.back().testModes.push_back( {ETM_RECO_CACHED, ETO_STANDARD, qp} );
    }
#endif
    // add intra modes
    if( tryIntraRdo )
    {
    if (cs.slice->getSPS()->getPLTMode() && (partitioner.treeType != TREE_D || cs.slice->isIntra() || (cs.area.lwidth() == 4 && cs.area.lheight() == 4)) && getPltEnc())
    {
      m_ComprCUCtxList.back().testModes.push_back({ ETM_PALETTE, ETO_STANDARD, qp });
    }
    m_ComprCUCtxList.back().testModes.push_back( { ETM_INTRA, ETO_STANDARD, qp } );
    if (cs.slice->getSPS()->getPLTMode() && partitioner.treeType == TREE_D && !cs.slice->isIntra() && !(cs.area.lwidth() == 4 && cs.area.lheight() == 4) && getPltEnc())
    {
      m_ComprCUCtxList.back().testModes.push_back({ ETM_PALETTE,  ETO_STANDARD, qp });
    }
    }
    // add ibc mode to intra path
    if (cs.sps->getIBCFlag() && checkIbc)
    {
      m_ComprCUCtxList.back().testModes.push_back({ ETM_IBC,         ETO_STANDARD,  qp });
      if (partitioner.chType == CHANNEL_TYPE_LUMA)
      {
        m_ComprCUCtxList.back().testModes.push_back({ ETM_IBC_MERGE,   ETO_STANDARD,  qp });
      }
    }
  }

  // add first pass modes
  if ( !m_slice->isIntra() && !( cs.area.lwidth() == 4 && cs.area.lheight() == 4 ) && tryInterRdo )
  {
    for( int qpLoop = maxQP; qpLoop >= minQP; qpLoop-- )
    {
      const int  qp       = std::max( qpLoop, lowestQP );
      if (m_pcEncCfg->getIMV())
      {
        m_ComprCUCtxList.back().testModes.push_back({ ETM_INTER_ME,  EncTestModeOpts( 4 << ETO_IMV_SHIFT ), qp });
      }
      if( m_pcEncCfg->getIMV() || m_pcEncCfg->getUseAffineAmvr() )
      {
        int imv = m_pcEncCfg->getIMV4PelFast() ? 3 : 2;
        m_ComprCUCtxList.back().testModes.push_back( { ETM_INTER_ME, EncTestModeOpts( imv << ETO_IMV_SHIFT ), qp } );
        m_ComprCUCtxList.back().testModes.push_back( { ETM_INTER_ME, EncTestModeOpts( 1 << ETO_IMV_SHIFT ), qp } );
      }
      // add inter modes
      if( m_pcEncCfg->getUseEarlySkipDetection() )
      {
        if( cs.sps->getUseGeo() && cs.slice->isInterB() )
        {
          m_ComprCUCtxList.back().testModes.push_back( { ETM_MERGE_GEO, ETO_STANDARD, qp } );
        }
        m_ComprCUCtxList.back().testModes.push_back( { ETM_MERGE_SKIP,  ETO_STANDARD, qp } );
        if (cs.sps->getUseAffine() || cs.sps->getSbTMVPEnabledFlag())
        {
          m_ComprCUCtxList.back().testModes.push_back( { ETM_AFFINE,    ETO_STANDARD, qp } );
        }
        m_ComprCUCtxList.back().testModes.push_back( { ETM_INTER_ME,    ETO_STANDARD, qp } );
      }
      else
      {
        m_ComprCUCtxList.back().testModes.push_back( { ETM_INTER_ME,    ETO_STANDARD, qp } );
        if( cs.sps->getUseGeo() && cs.slice->isInterB() )
        {
          m_ComprCUCtxList.back().testModes.push_back( { ETM_MERGE_GEO, ETO_STANDARD, qp } );
        }
        m_ComprCUCtxList.back().testModes.push_back( { ETM_MERGE_SKIP,  ETO_STANDARD, qp } );
        if (cs.sps->getUseAffine() || cs.sps->getSbTMVPEnabledFlag())
        {
          m_ComprCUCtxList.back().testModes.push_back( { ETM_AFFINE,    ETO_STANDARD, qp } );
        }
      }
      if (m_pcEncCfg->getUseHashME())
      {
        int minSize = min(cs.area.lwidth(), cs.area.lheight());
        if (minSize < 128 && minSize >= 4)
        {
          m_ComprCUCtxList.back().testModes.push_back({ ETM_HASH_INTER, ETO_STANDARD, qp });
        }
      }
    }
  }

  // This if tests if the current mode in the ".back() position" should be tested. If not, the nextMode() function will remove as many modes as necessary
  // until finding a mode that should be tested
  // The false in tryModeMaster and nextMode represents onlyAllowAffine=false. In the initialization of the CU we always add all encoding modes, and during the encoding of the CU the unwanted modes are discarded by xCheckModeSplit and by tryModeMaster as well
  if( !tryModeMaster( m_ComprCUCtxList.back().testModes.back(), cs, partitioner, false ) )
  {
    // The false in tryModeMaster and nextMode represents onlyAllowAffine=false. In the initialization of the CU we always add all encoding modes, and during the encoding of the CU the unwanted modes are discarded by xCheckModeSplit and by tryModeMaster as well
    nextMode( cs, partitioner, false );
  }

  m_ComprCUCtxList.back().lastTestMode = EncTestMode();
}

void EncModeCtrlMTnoRQT::finishCULevel( Partitioner &partitioner )
{
  m_ComprCUCtxList.pop_back();
}

// Override of tryMode
// This function decides if the current "mode" will be tested or not, where "mode" can be a prediction or splitting mode
// Several heuristics are used to skip the evaluation of some modes
// This function is modified to allow enabling/disabling the heuristics depending if the encoding mode and block size
// These modifications are intended to enforce the AFFINE PREDICTION OF ALL BLOCKS
bool EncModeCtrlMTnoRQT::tryMode( const EncTestMode& encTestmode, const CodingStructure &cs, Partitioner& partitioner )
{        
  int debug = 0;
  // iagostorch debug
//  if(cs.area.lx()==0 && cs.area.ly()==64 && cs.area.lwidth()==64 && cs.area.lheight()==64 && encTestmode.type == ETM_INTER_ME){
//    debug = 1;
//  }
  
  ComprCUCtx& cuECtx = m_ComprCUCtxList.back();
  
  // Fast checks, partitioning depended
  // isHashPerfectMatch is used in screen content. We can maintain this heuristic
  if (cuECtx.isHashPerfectMatch && encTestmode.type != ETM_MERGE_SKIP && encTestmode.type != ETM_INTER_ME && encTestmode.type != ETM_AFFINE && encTestmode.type != ETM_MERGE_GEO)
  {
    return false;
  }

  // if early skip detected, skip all modes checking but the splits
  // This conditional is true only when (1) current is NOT SPLIT and (2) current is NOT INTER
  if( cuECtx.earlySkip && m_pcEncCfg->getUseEarlySkipDetection() && !isModeSplit( encTestmode ) && !( isModeInter( encTestmode ) ) )
  {
    return false;
  }
  
  const PartSplit implicitSplit = partitioner.getImplicitSplit( cs );
  const bool isBoundary         = implicitSplit != CU_DONT_SPLIT;
  
  // DO NOT use the ALLOW_TREE_HEURISTICS conditional in this if/else
  if( isBoundary && encTestmode.type != ETM_SPLIT_QT )
  {
    return getPartSplit( encTestmode ) == implicitSplit;
  }
  else if( isBoundary && encTestmode.type == ETM_SPLIT_QT )
  {
    return partitioner.canSplit( CU_QUAD_SPLIT, cs );
  }

#if REUSE_CU_RESULTS
  if( cuECtx.get<bool>( IS_REUSING_CU ) )
  {
    if( encTestmode.type == ETM_RECO_CACHED )
    {
      return true;
    }
    // DO NOT use the ALLOW_TREE_HEURISTICS conditional in this if/else
    if( isModeNoSplit( encTestmode ) )
    {
      return false;
    }
  }

#endif
  const Slice&           slice       = *m_slice;
  const SPS&             sps         = *slice.getSPS();
  const uint32_t             numComp     = getNumberValidComponents( slice.getSPS()->getChromaFormatIdc() );
  const uint32_t             width       = partitioner.currArea().lumaSize().width;
  const CodingStructure *bestCS      = cuECtx.bestCS;
  const CodingUnit      *bestCU      = cuECtx.bestCU;
  const EncTestMode      bestMode    = bestCS ? getCSEncMode( *bestCS ) : EncTestMode();

  CodedCUInfo    &relatedCU          = getBlkInfo( partitioner.currArea() );
  // Used for debugging purposes
  UnitArea	 relatedCuArea      = partitioner.currArea();

  // This heuristic enforces or avoids a quadtree partition
  if( cuECtx.minDepth > partitioner.currQtDepth && partitioner.canSplit( CU_QUAD_SPLIT, cs ) )
  {
    // used to avoid not returning a true value due to disabling the heuristics
    if(encTestmode.type == ETM_SPLIT_QT){
      return true;
    }
    
    // If applying a quadtree produces blocks smaller than 16x16, allow early termination
    if((storch::sHEUR_customizeTreeHeuristics && (cs.area.lwidth()<=CUSTOM_SIZE || cs.area.lheight()<=CUSTOM_SIZE)) || 
        storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0)
    {
      // enforce QT
      return encTestmode.type == ETM_SPLIT_QT;
    }
  }
  else if( encTestmode.type == ETM_SPLIT_QT && cuECtx.maxDepth <= partitioner.currQtDepth )
  {
    // don't check this QT depth
    // If applying a quadtree produces blocks smaller than 16x16, allow early termination
    if((storch::sHEUR_customizeTreeHeuristics && (cs.area.lwidth()<=CUSTOM_SIZE || cs.area.lheight()<=CUSTOM_SIZE)) || // The resulting block is smaller than 16x16, therefore it is not supported by affine
        storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0)
    {
      return false;
    }
  }

  if( bestCS && bestCS->cus.size() == 1 )
  {
    // update the best non-split cost
    cuECtx.set( BEST_NON_SPLIT_COST, bestCS->cost );
  }

  // All early terminations of ETM_INTRA can be kept since they do not interfere in affine prediction
  if( encTestmode.type == ETM_INTRA )
  {
    if( getFastDeltaQp() )
    {
      if( cs.area.lumaSize().width > cs.pcv->fastDeltaQPCuMaxSize )
      {
        return false; // only check necessary 2Nx2N Intra in fast delta-QP mode
      }
    }

    if( m_pcEncCfg->getUseFastLCTU() && partitioner.currArea().lumaSize().area() > 4096 )
    {
      return (m_pcEncCfg->getDualITree() == 0 && m_pcEncCfg->getMaxMTTHierarchyDepthI() == 0 && cs.sps->getMinQTSize(cs.slice->getSliceType(), partitioner.chType) > 64);
    }

    if (CS::isDualITree(cs) && (partitioner.currArea().lumaSize().width > 64 || partitioner.currArea().lumaSize().height > 64))
    {
      return false;
    }

    if (m_pcEncCfg->getUsePbIntraFast() && (!cs.slice->isIntra() || cs.slice->getSPS()->getIBCFlag()) && !interHadActive(cuECtx) && cuECtx.bestCU && !CU::isIntra(*cuECtx.bestCU))
    {
      return false;
    }

    // INTRA MODES
    if (cs.sps->getIBCFlag() && !cuECtx.bestTU)
      return true;
    if( partitioner.isConsIntra() && !cuECtx.bestTU )
    {
      return true;
    }
    if ( partitioner.currArea().lumaSize().width == 4 && partitioner.currArea().lumaSize().height == 4 && !slice.isIntra() && !cuECtx.bestTU )
    {
      return true;
    }
    if( !( slice.isIntra() || bestMode.type == ETM_INTRA || !cuECtx.bestTU ||
      ((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (!relatedCU.isInter || !relatedCU.isIBC) && (
                                         ( cuECtx.bestTU->cbf[0] != 0 ) ||
           ( ( numComp > COMPONENT_Cb ) && cuECtx.bestTU->cbf[1] != 0 ) ||
           ( ( numComp > COMPONENT_Cr ) && cuECtx.bestTU->cbf[2] != 0 )  // avoid very complex intra if it is unlikely
         ) ) ) )
    {
      return false;
    }
    if ((m_pcEncCfg->getIBCFastMethod() & IBC_FAST_METHOD_NOINTRA_IBCCBF0)
      && (bestMode.type == ETM_IBC || bestMode.type == ETM_IBC_MERGE)
      && (!cuECtx.bestCU->Y().valid() || cuECtx.bestTU->cbf[0] == 0)
      && (!cuECtx.bestCU->Cb().valid() || cuECtx.bestTU->cbf[1] == 0)
      && (!cuECtx.bestCU->Cr().valid() || cuECtx.bestTU->cbf[2] == 0))
    {
      return false;
    }
    if( lastTestMode().type != ETM_INTRA && cuECtx.bestCS && cuECtx.bestCU && interHadActive( cuECtx ) )
    {
      // Get SATD threshold from best Inter-CU
      if (!cs.slice->isIntra() && m_pcEncCfg->getUsePbIntraFast() && !cs.slice->getDisableSATDForRD())
      {
        CodingUnit* bestCU = cuECtx.bestCU;
        if (bestCU && !CU::isIntra(*bestCU))
        {
          DistParam distParam;
          const bool useHad = true;
          m_pcRdCost->setDistParam( distParam, cs.getOrgBuf( COMPONENT_Y ), cuECtx.bestCS->getPredBuf( COMPONENT_Y ), cs.sps->getBitDepth( CHANNEL_TYPE_LUMA ), COMPONENT_Y, useHad );
          cuECtx.interHad = distParam.distFunc( distParam );
        }
      }
    }
    if (bestMode.type == ETM_PALETTE && !slice.isIntra() && partitioner.treeType == TREE_D && !(partitioner.currArea().lumaSize().width == 4 && partitioner.currArea().lumaSize().height == 4)) // inter slice
    {
      return false;
    }
    if ( m_pcEncCfg->getUseFastISP() && relatedCU.relatedCuIsValid )
    {
      cuECtx.ispPredModeVal     = relatedCU.ispPredModeVal;
      cuECtx.bestDCT2NonISPCost = relatedCU.bestDCT2NonISPCost;
      cuECtx.relatedCuIsValid   = relatedCU.relatedCuIsValid;
      cuECtx.bestNonDCT2Cost    = relatedCU.bestNonDCT2Cost;
      cuECtx.bestISPIntraMode   = relatedCU.bestISPIntraMode;
    }
    return true;
  }
  // All early terminations of ETM_PALETTE (screen content) can be kept since they do not interfere in affine prediction
  else if (encTestmode.type == ETM_PALETTE)
  {
    if (partitioner.currArea().lumaSize().width > 64 || partitioner.currArea().lumaSize().height > 64
        || ((partitioner.currArea().lumaSize().width * partitioner.currArea().lumaSize().height <= 16) && (isLuma(partitioner.chType)) )
        || ((partitioner.currArea().chromaSize().width * partitioner.currArea().chromaSize().height <= 16) && (!isLuma(partitioner.chType)) && partitioner.isSepTree(cs) ) 
      || (partitioner.isLocalSepTree(cs)  && (!isLuma(partitioner.chType)) ) )
    {
      return false;
    }
    const Area curr_cu = CS::getArea(cs, cs.area, partitioner.chType).blocks[getFirstComponentOfChannel(partitioner.chType)];
    try
    {
      double stored_cost = slice.m_mapPltCost[isChroma(partitioner.chType)].at(curr_cu.pos()).at(curr_cu.size());
      if (bestMode.type != ETM_INVALID && stored_cost > cuECtx.bestCS->cost)
      {
        return false;
      }
    }
    catch (const std::out_of_range &)
    {
      // do nothing if no stored cost value was found.
    }
    return true;
  }
  else if (encTestmode.type == ETM_IBC || encTestmode.type == ETM_IBC_MERGE)
  {
    // IBC MODES
    return sps.getIBCFlag() && (partitioner.currArea().lumaSize().width < 128 && partitioner.currArea().lumaSize().height < 128);
  }
  // Here we are conducting early termination in interframes prediction
  // CAUTION IS REQUIRED
  else if( isModeInter( encTestmode ) )
  {
    // INTER MODES (ME + MERGE/SKIP)
    CHECK( slice.isIntra(), "Inter-mode should not be in the I-Slice mode list!" );

    if( getFastDeltaQp() ) // "getFastDeltaQp()" is zero, it never enters this if
    {
      if( encTestmode.type == ETM_MERGE_SKIP )
      {
        return false;
      }
      if( cs.area.lumaSize().width > cs.pcv->fastDeltaQPCuMaxSize )
      {
        return false; // only check necessary 2Nx2N Inter in fast deltaqp mode
      }
    }

    // --- Check if we can quit current mode using SAVE/LOAD coding history

    if( encTestmode.type == ETM_INTER_ME )
    {
      if( encTestmode.opts == ETO_STANDARD )
      {
	if(debug){
	  printf("  $$ tryMode ETO_STD Cust relatedCU = @(%dx%d) [%dx%d]\n", relatedCuArea.lx(), relatedCuArea.ly(), relatedCuArea.lwidth(), relatedCuArea.lheight());
          printf("   |- isInter %d\n", relatedCU.isInter);
          printf("   |- isSkip %d\n", relatedCU.isSkip);
          printf("   |- isIntra %d\n", relatedCU.isIntra);

          printf("   |- bestCost %f\n", relatedCU.bestCost);
          printf("   |- relatedCuIsValid %d\n", relatedCU.relatedCuIsValid);
          printf("   |- isMMVDSkip %d\n", relatedCU.isMMVDSkip);
          printf("   |- isIBC %d\n", relatedCU.isIBC);
          cout <<"   |- BEST_NO_IMV " << cuECtx.get<double>(BEST_NO_IMV_COST) << endl;
          cout <<"   |- BEST_IMV_COST " << cuECtx.get<double>(BEST_IMV_COST) << endl;
	}
        // NOTE: ETO_STANDARD is always done when early SKIP mode detection is enabled
        // It seems that when ESD is disabled and we are conducting inter prediction, in case relatedCU is SKIP or INTRA, we  avoid inter prediction for current block
        // What is the "relatedCU"?
        if( !m_pcEncCfg->getUseEarlySkipDetection() )
        {
          if( relatedCU.isSkip || relatedCU.isIntra )
          {
            if((storch::sHEUR_customizeTreeHeuristics && (cs.area.lwidth()<CUSTOM_SIZE || cs.area.lheight()<CUSTOM_SIZE)) || // If any block dimension is smaller than 16, the block is not supported by affine
                storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0)
            {
              return false;
            }
          }
        }
      }
      else if ((encTestmode.opts & ETO_IMV) != 0) // Isso quer dizer que os 3 bits usados pra dizer a precisão do MV não são "000"
      {
        if(debug){
          printf("  $$ tryMode ETO_IMV Cust relatedCU = @(%dx%d) [%dx%d]\n", relatedCuArea.lx(), relatedCuArea.ly(), relatedCuArea.lwidth(), relatedCuArea.lheight());
          printf("   |- isInter %d\n", relatedCU.isInter);
          printf("   |- isSkip %d\n", relatedCU.isSkip);
          printf("   |- isIntra %d\n", relatedCU.isIntra);

          printf("   |- bestCost %f\n", relatedCU.bestCost);
          printf("   |- relatedCuIsValid %d\n", relatedCU.relatedCuIsValid);
          printf("   |- isMMVDSkip %d\n", relatedCU.isMMVDSkip);
          printf("   |- isIBC %d\n", relatedCU.isIBC);
          int imvOpt_BK = (encTestmode.opts & ETO_IMV) >> ETO_IMV_SHIFT;
          cout <<"   |- imvOpt " << imvOpt_BK << endl;
          cout <<"   |- BEST_NO_IMV " << cuECtx.get<double>(BEST_NO_IMV_COST) << endl;
          cout <<"   |- BEST_IMV_COST " << cuECtx.get<double>(BEST_IMV_COST) << endl;
          cout <<"   |- !affineAmvr " << !m_pcEncCfg->getUseAffineAmvr() << endl;
        }
        // This avoids testing an encoding mode in case the current RD cost is not good enough (compared to 1.06 threshold)
        int imvOpt = (encTestmode.opts & ETO_IMV) >> ETO_IMV_SHIFT;

        if (imvOpt == 3 && cuECtx.get<double>(BEST_NO_IMV_COST) * 1.06 < cuECtx.get<double>(BEST_IMV_COST))
        {
          if ( !m_pcEncCfg->getUseAffineAmvr() )
          {
            if((storch::sHEUR_customizeTreeHeuristics && (cs.area.lwidth()<CUSTOM_SIZE || cs.area.lheight()<CUSTOM_SIZE)) || // If any block dimension is smaller than 16, the block is not supported by affine
                storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0)
            {
              return false;
            }
          }
        }
      }
    }

    // This avoids testing AFFINE MERGE in case the parent CU (or maybe the current bestMode) is intra
    // Since we are not working with affine merge this early termination can be kept
    if ( encTestmode.type == ETM_AFFINE && relatedCU.isIntra )
    {
      return false;
    }    
    if( encTestmode.type == ETM_MERGE_GEO && ( partitioner.currArea().lwidth() < GEO_MIN_CU_SIZE || partitioner.currArea().lheight() < GEO_MIN_CU_SIZE
                                            || partitioner.currArea().lwidth() > GEO_MAX_CU_SIZE || partitioner.currArea().lheight() > GEO_MAX_CU_SIZE
                                            || partitioner.currArea().lwidth() >= 8 * partitioner.currArea().lheight()
                                            || partitioner.currArea().lheight() >= 8 * partitioner.currArea().lwidth() ) )
    {
      // Discarding this early termination causes seg fault
      return false;
    }
    return true;
  }
  // Here we are conducting early termination in the partitioning mechanisms
  // CAUTION IS REQUIRED
  else if( isModeSplit( encTestmode ) )
  {
    //////////////////////////////////////////////////////////////////////////
    // skip-history rule - don't split further if at least for three past levels
    //                     in the split tree it was found that skip is the best mode
    //////////////////////////////////////////////////////////////////////////
    int skipScore = 0;
    
    // If the 2 parent blocks (parent and grandparent) were encoded with SKIP, we do not conduct further splitting for this  CU
    if ((!slice.isIntra() || slice.getSPS()->getIBCFlag()) && cuECtx.get<bool>(IS_BEST_NOSPLIT_SKIP))
    {
      for( int i = 2; i < m_ComprCUCtxList.size(); i++ )
      {
        if( ( m_ComprCUCtxList.end() - i )->get<bool>( IS_BEST_NOSPLIT_SKIP ) )
        {
          skipScore += 1;
        }
        else
        {
          break;
        }
      }
    }

    // If the current block size is partitioned with encTesmode, the resulting block size is not supported by affine prediction. Therefore we conduct an early skip as the original VTM would do
    if(
        (storch::sHEUR_customizeTreeHeuristics &&
            !isChildrenAffineCompatible(cs, encTestmode.type)
        )   
        || storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0
      )
    {
      // Allows early termination. Do nothing...
      skipScore = skipScore;
    }
    else
    {
      // Avoids early termination due to skipScore. Simply assign skipScore=0 to avoid early termination
      if(skipScore>=2){
        skipScore = 0; // Disabled early termination
      }
    }
    
    const PartSplit split = getPartSplit( encTestmode );
    if( !partitioner.canSplit( split, cs ) || skipScore >= 2 )
    {
      if( split == CU_HORZ_SPLIT ) cuECtx.set( DID_HORZ_SPLIT, false );
      if( split == CU_VERT_SPLIT ) cuECtx.set( DID_VERT_SPLIT, false );
      if( split == CU_QUAD_SPLIT ) cuECtx.set( DID_QUAD_SPLIT, false );

      // Avoiding this return causes seg fault
      return false;
    }

    // iagostorch BEGIN
    // never enters this if since ContentBasedFastQtbt is FALSE in LOWDELAY and RANDOMACCESS
    if( m_pcEncCfg->getUseContentBasedFastQtbt() )
    {
      const CompArea& currArea = partitioner.currArea().Y();
      int cuHeight  = currArea.height;
      int cuWidth   = currArea.width;

      const bool condIntraInter = m_pcEncCfg->getIntraPeriod() == 1 ? ( partitioner.currBtDepth == 0 ) : ( cuHeight > 32 && cuWidth > 32 );

      if( cuWidth == cuHeight && condIntraInter && getPartSplit( encTestmode ) != CU_QUAD_SPLIT )
      {
        const CPelBuf bufCurrArea = cs.getOrgBuf( partitioner.currArea().block( COMPONENT_Y ) );

        double horVal = 0;
        double verVal = 0;
        double dupVal = 0;
        double dowVal = 0;

        const double th = m_pcEncCfg->getIntraPeriod() == 1 ? 1.2 : 1.0;

        unsigned j, k;

        for( j = 0; j < cuWidth - 1; j++ )
        {
          for( k = 0; k < cuHeight - 1; k++ )
          {
            horVal += abs( bufCurrArea.at( j + 1, k     ) - bufCurrArea.at( j, k ) );
            verVal += abs( bufCurrArea.at( j    , k + 1 ) - bufCurrArea.at( j, k ) );
            dowVal += abs( bufCurrArea.at( j + 1, k )     - bufCurrArea.at( j, k + 1 ) );
            dupVal += abs( bufCurrArea.at( j + 1, k + 1 ) - bufCurrArea.at( j, k ) );
          }
        }
        if( horVal > th * verVal && sqrt( 2 ) * horVal > th * dowVal && sqrt( 2 ) * horVal > th * dupVal && ( getPartSplit( encTestmode ) == CU_HORZ_SPLIT || getPartSplit( encTestmode ) == CU_TRIH_SPLIT ) )
        {
          return false;
        }
        if( th * dupVal < sqrt( 2 ) * verVal && th * dowVal < sqrt( 2 ) * verVal && th * horVal < verVal && ( getPartSplit( encTestmode ) == CU_VERT_SPLIT || getPartSplit( encTestmode ) == CU_TRIV_SPLIT ) )
        {
          return false;
        }
      }

      if( m_pcEncCfg->getIntraPeriod() == 1 && cuWidth <= 32 && cuHeight <= 32 && bestCS && bestCS->tus.size() == 1 && bestCU && bestCU->depth == partitioner.currDepth && partitioner.currBtDepth > 1 && isLuma( partitioner.chType ) )
      {
        if( !bestCU->rootCbf )
        {
          return false;
        }
      }
    }

    // This if avoids further partitionings if:
    // (1) Current CU is bestCU and is SKIP and
    // (2) MTT has already passed through a given depth
    if( bestCU && bestCU->skip && bestCU->mtDepth >= m_skipThreshold && !isModeSplit( cuECtx.lastTestMode ) )
    {
      // If the current block size is partitioned with encTesmode, the resulting block size is not supported by affine prediction. Therefore we conduct an early skip as the original VTM would do
      if(
          (storch::sHEUR_customizeTreeHeuristics && !isChildrenAffineCompatible(cs, encTestmode.type)
            
          )   
          || storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0
        )
      {
        return false;
      }
    }

    int featureToSet = -1;

    // If the current block is partitioned with the current encTestmode, the children blocks will not be compatible with affine. Therefore we can conduct early terminaiton
    bool allowEarlyTerminationCondition = // The same condition used in the previous tests. Summarized here to improve readability inside the switch/case
              ((storch::sHEUR_customizeTreeHeuristics && !isChildrenAffineCompatible(cs, encTestmode.type))
              || storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0
                  );
    
    switch( getPartSplit( encTestmode ) )
    {
      case CU_QUAD_SPLIT:
        {
        // This if avoids the quadtree in some cases, such as when the current CU is bestCU and it is encoded with SKIP
          if( !cuECtx.get<bool>( QT_BEFORE_BT ) && bestCU )
          {
            unsigned maxBTD        = cs.pcv->getMaxBtDepth( slice, partitioner.chType );
            const CodingUnit *cuBR = bestCS->cus.back();
            unsigned height        = partitioner.currArea().lumaSize().height;

            if (bestCU && ((bestCU->btDepth == 0 && maxBTD >= ((slice.isIntra() && !slice.getSPS()->getIBCFlag()) ? 3 : 2))
              || (bestCU->btDepth == 1 && cuBR && cuBR->btDepth == 1 && maxBTD >= ((slice.isIntra() && !slice.getSPS()->getIBCFlag()) ? 4 : 3)))
              && (width <= MAX_TB_SIZEY && height <= MAX_TB_SIZEY)
              && cuECtx.get<bool>(DID_HORZ_SPLIT) && cuECtx.get<bool>(DID_VERT_SPLIT))
            {
              if(allowEarlyTerminationCondition)
              {
                return false;
              }
            }
          }
          if( m_pcEncCfg->getUseEarlyCU() && bestCS->cost != MAX_DOUBLE && bestCU && bestCU->skip )
          {
            if(allowEarlyTerminationCondition)
            {
              return false;
            }
          }
          if( getFastDeltaQp() && width <= slice.getPPS()->pcv->fastDeltaQPCuMaxSize )
          {
            if(allowEarlyTerminationCondition)
            {
              return false;
            }
          }
        }
        break;
      case CU_HORZ_SPLIT:
        featureToSet = DID_HORZ_SPLIT;
        break;
      case CU_VERT_SPLIT:
        featureToSet = DID_VERT_SPLIT;
        break;
      case CU_TRIH_SPLIT:
      // This if avoids a ternary horizontal split based on some conditions, including if this is the bestCU and the depth of binary tree
        if( cuECtx.get<bool>( DID_HORZ_SPLIT ) && bestCU && bestCU->btDepth == partitioner.currBtDepth && !bestCU->rootCbf )
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
        if( !cuECtx.get<bool>( DO_TRIH_SPLIT ) )
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
        break;
      case CU_TRIV_SPLIT:
        // This if avoids a ternary verticalsplit based on some conditions, including if this is the bestCU and the depth of binary tree
        if( cuECtx.get<bool>( DID_VERT_SPLIT ) && bestCU && bestCU->btDepth == partitioner.currBtDepth && !bestCU->rootCbf )
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }

        if( !cuECtx.get<bool>( DO_TRIV_SPLIT ) )
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
        break;
      default:
        THROW( "Only CU split modes are governed by the EncModeCtrl" );
        return false;
        break;
    }

    switch( split )
    {
      case CU_HORZ_SPLIT:
      case CU_TRIH_SPLIT:
        // This if avoids a ternary or binary horizontal split
        if( cuECtx.get<bool>( QT_BEFORE_BT ) && cuECtx.get<bool>( DID_QUAD_SPLIT ) )
        {
          if( cuECtx.get<int>( MAX_QT_SUB_DEPTH ) > partitioner.currQtDepth + 1 )
          {
            if(allowEarlyTerminationCondition)
            {
              if( featureToSet >= 0 ) cuECtx.set( featureToSet, false );
              return false;
            }
          }
        }
        break;
      case CU_VERT_SPLIT:
      case CU_TRIV_SPLIT:
        // This if avoids a ternary or binary vertical split
        if( cuECtx.get<bool>( QT_BEFORE_BT ) && cuECtx.get<bool>( DID_QUAD_SPLIT ) )
        {
          if( cuECtx.get<int>( MAX_QT_SUB_DEPTH ) > partitioner.currQtDepth + 1 )
          {
            if(allowEarlyTerminationCondition)
            {
              if( featureToSet >= 0 ) cuECtx.set( featureToSet, false );
              return false;
            }
          }
        }
        break;
      default:
        break;
    }

    if( split == CU_QUAD_SPLIT ) cuECtx.set( DID_QUAD_SPLIT, true );
    if (cs.sps->getLog2ParallelMergeLevelMinus2())
    {
      const CompArea& area = partitioner.currArea().Y();
      const SizeType size = 1 << (cs.sps->getLog2ParallelMergeLevelMinus2() + 2);
      if (!cs.slice->isIntra() && (area.width > size || area.height > size))
      {
        if (area.height <= size && split == CU_HORZ_SPLIT)
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
        if (area.width <= size && split == CU_VERT_SPLIT)
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
        if (area.height <= 2 * size && split == CU_TRIH_SPLIT)
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
        if (area.width <= 2 * size && split == CU_TRIV_SPLIT)
        {
          if(allowEarlyTerminationCondition)
          {
            return false;
          }
        }
      }
    }
    return true;
  } // the end of if( isModeSplit( encTestmode ) )
  else
  {
    // Only gets here if the encoding mode is ETM_POST_DONT_SPLIT
    CHECK( encTestmode.type != ETM_POST_DONT_SPLIT, "Unknown mode" );
    if ((cuECtx.get<double>(BEST_NO_IMV_COST) == (MAX_DOUBLE * .5) 
        #if REUSE_CU_RESULTS
            || cuECtx.get<bool>(IS_REUSING_CU)
        #endif  
            ) && !slice.isIntra())
    {
      unsigned idx1, idx2, idx3, idx4;
      getAreaIdx(partitioner.currArea().Y(), *slice.getPPS()->pcv, idx1, idx2, idx3, idx4);
      if (g_isReusedUniMVsFilled[idx1][idx2][idx3][idx4])
      {
        m_pcInterSearch->insertUniMvCands(partitioner.currArea().Y(), g_reusedUniMVs[idx1][idx2][idx3][idx4]);
      }
    }
    if( !bestCS || ( bestCS && isModeSplit( bestMode ) ) )
    {
      // This doesnt make much sense... If it is not best, OR if it is best and is a split mode, then we skip the current mode?
      if((storch::sHEUR_customizeTreeHeuristics && (cs.area.lwidth()<CUSTOM_SIZE || cs.area.lheight()<CUSTOM_SIZE)) || 
          storch::sHEUR_originalTreeHeuristics || cs.picture->poc==0)
      {
        return false;
      }
    }
    else
    {
#if REUSE_CU_RESULTS
      setFromCs( *bestCS, partitioner );

#endif
      if( partitioner.modeType == MODE_TYPE_INTRA && partitioner.chType == CHANNEL_TYPE_LUMA )
      {
        // I didnt understand this condition...
          return false; //not set best coding mode for intra coding pass
      }
      // assume the non-split modes are done and set the marks for the best found mode
      if( bestCS && bestCU )
      {
        if( CU::isInter( *bestCU ) )
        {
          relatedCU.isInter   = true;
          relatedCU.isSkip   |= bestCU->skip;
          relatedCU.isMMVDSkip |= bestCU->mmvdSkip;
          relatedCU.BcwIdx    = bestCU->BcwIdx;
          if (bestCU->slice->getSPS()->getUseColorTrans())
          {
            if (m_pcEncCfg->getRGBFormatFlag())
            {
              if (bestCU->colorTransform && bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
            else
            {
              if (!bestCU->colorTransform || !bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
          }
        }
        else if (CU::isIBC(*bestCU))
        {
          relatedCU.isIBC = true;
          relatedCU.isSkip |= bestCU->skip;
          if (bestCU->slice->getSPS()->getUseColorTrans())
          {
            if (m_pcEncCfg->getRGBFormatFlag())
            {
              if (bestCU->colorTransform && bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
            else
            {
              if (!bestCU->colorTransform || !bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
          }
        }
        else if( CU::isIntra( *bestCU ) )
        {
          relatedCU.isIntra   = true;
          if ( m_pcEncCfg->getUseFastISP() && cuECtx.ispWasTested && ( !relatedCU.relatedCuIsValid || bestCS->cost < relatedCU.bestCost ) )
          {
            // Compact data
            int bit0 = true;
            int bit1 = cuECtx.ispMode == NOT_INTRA_SUBPARTITIONS ? 1 : 0;
            int bit2 = cuECtx.ispMode == VER_INTRA_SUBPARTITIONS;
            int bit3 = cuECtx.ispLfnstIdx > 0;
            int bit4 = cuECtx.ispLfnstIdx == 2;
            int bit5 = cuECtx.mipFlag;
            int bit6 = cuECtx.bestCostIsp < cuECtx.bestNonDCT2Cost * 0.95;
            int val =
              (bit0) |
              (bit1 << 1) |
              (bit2 << 2) |
              (bit3 << 3) |
              (bit4 << 4) |
              (bit5 << 5) |
              (bit6 << 6) |
              ( cuECtx.bestPredModeDCT2 << 9 );
            relatedCU.ispPredModeVal     = val;
            relatedCU.bestDCT2NonISPCost = cuECtx.bestDCT2NonISPCost;
            relatedCU.bestCost           = bestCS->cost;
            relatedCU.bestNonDCT2Cost    = cuECtx.bestNonDCT2Cost;
            relatedCU.bestISPIntraMode   = cuECtx.bestISPIntraMode;
            relatedCU.relatedCuIsValid   = true;
          }
        }

        cuECtx.set( IS_BEST_NOSPLIT_SKIP, bestCU->skip );
      }
    }
    
    return false;
  }
}


// This is the original tryMode function, which uses all standard heuristics from VTM encoder. This is used when we are not enforcing any partitioning or encoding modes
bool EncModeCtrlMTnoRQT::tryMode_original( const EncTestMode& encTestmode, const CodingStructure &cs, Partitioner& partitioner )
{
  int debug = 0;
//  if(cs.area.lx()==0 && cs.area.ly()==64 && cs.area.lwidth()==64 && cs.area.lheight()==64 && encTestmode.type == ETM_INTER_ME){
//    debug = 1;
//  }
  
  ComprCUCtx& cuECtx = m_ComprCUCtxList.back();

  // Fast checks, partitioning depended
  if (cuECtx.isHashPerfectMatch && encTestmode.type != ETM_MERGE_SKIP && encTestmode.type != ETM_INTER_ME && encTestmode.type != ETM_AFFINE && encTestmode.type != ETM_MERGE_GEO)
  {
    return false;
  }

  // if early skip detected, skip all modes checking but the splits
  if( cuECtx.earlySkip && m_pcEncCfg->getUseEarlySkipDetection() && !isModeSplit( encTestmode ) && !( isModeInter( encTestmode ) ) )
  {
    return false;
  }

  const PartSplit implicitSplit = partitioner.getImplicitSplit( cs );
  const bool isBoundary         = implicitSplit != CU_DONT_SPLIT;

  if( isBoundary && encTestmode.type != ETM_SPLIT_QT )
  {
    return getPartSplit( encTestmode ) == implicitSplit;
  }
  else if( isBoundary && encTestmode.type == ETM_SPLIT_QT )
  {
    return partitioner.canSplit( CU_QUAD_SPLIT, cs );
  }

#if REUSE_CU_RESULTS
  if( cuECtx.get<bool>( IS_REUSING_CU ) )
  {
    if( encTestmode.type == ETM_RECO_CACHED )
    {
      return true;
    }

    if( isModeNoSplit( encTestmode ) )
    {
      return false;
    }
  }

#endif
  const Slice&           slice       = *m_slice;
  const SPS&             sps         = *slice.getSPS();
  const uint32_t             numComp     = getNumberValidComponents( slice.getSPS()->getChromaFormatIdc() );
  const uint32_t             width       = partitioner.currArea().lumaSize().width;
  const CodingStructure *bestCS      = cuECtx.bestCS;
  const CodingUnit      *bestCU      = cuECtx.bestCU;
  const EncTestMode      bestMode    = bestCS ? getCSEncMode( *bestCS ) : EncTestMode();

  CodedCUInfo    &relatedCU          = getBlkInfo( partitioner.currArea() );
  UnitArea	 relatedCuArea      = partitioner.currArea();

  if( cuECtx.minDepth > partitioner.currQtDepth && partitioner.canSplit( CU_QUAD_SPLIT, cs ) )
  {
    // enforce QT
    return encTestmode.type == ETM_SPLIT_QT;
  }
  else if( encTestmode.type == ETM_SPLIT_QT && cuECtx.maxDepth <= partitioner.currQtDepth )
  {
    // don't check this QT depth
    return false;
  }

  if( bestCS && bestCS->cus.size() == 1 )
  {
    // update the best non-split cost
    cuECtx.set( BEST_NON_SPLIT_COST, bestCS->cost );
  }

  if( encTestmode.type == ETM_INTRA )
  {
    if( getFastDeltaQp() )
    {
      if( cs.area.lumaSize().width > cs.pcv->fastDeltaQPCuMaxSize )
      {
        return false; // only check necessary 2Nx2N Intra in fast delta-QP mode
      }
    }

    if( m_pcEncCfg->getUseFastLCTU() && partitioner.currArea().lumaSize().area() > 4096 )
    {
      return (m_pcEncCfg->getDualITree() == 0 && m_pcEncCfg->getMaxMTTHierarchyDepthI() == 0 && cs.sps->getMinQTSize(cs.slice->getSliceType(), partitioner.chType) > 64);
    }

    if (CS::isDualITree(cs) && (partitioner.currArea().lumaSize().width > 64 || partitioner.currArea().lumaSize().height > 64))
    {
      return false;
    }

    if (m_pcEncCfg->getUsePbIntraFast() && (!cs.slice->isIntra() || cs.slice->getSPS()->getIBCFlag()) && !interHadActive(cuECtx) && cuECtx.bestCU && !CU::isIntra(*cuECtx.bestCU))
    {
      return false;
    }

    // INTRA MODES
    if (cs.sps->getIBCFlag() && !cuECtx.bestTU)
      return true;
    if( partitioner.isConsIntra() && !cuECtx.bestTU )
    {
      return true;
    }
    if ( partitioner.currArea().lumaSize().width == 4 && partitioner.currArea().lumaSize().height == 4 && !slice.isIntra() && !cuECtx.bestTU )
    {
      return true;
    }
    if( !( slice.isIntra() || bestMode.type == ETM_INTRA || !cuECtx.bestTU ||
      ((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (!relatedCU.isInter || !relatedCU.isIBC) && (
                                         ( cuECtx.bestTU->cbf[0] != 0 ) ||
           ( ( numComp > COMPONENT_Cb ) && cuECtx.bestTU->cbf[1] != 0 ) ||
           ( ( numComp > COMPONENT_Cr ) && cuECtx.bestTU->cbf[2] != 0 )  // avoid very complex intra if it is unlikely
         ) ) ) )
    {
      return false;
    }
    if ((m_pcEncCfg->getIBCFastMethod() & IBC_FAST_METHOD_NOINTRA_IBCCBF0)
      && (bestMode.type == ETM_IBC || bestMode.type == ETM_IBC_MERGE)
      && (!cuECtx.bestCU->Y().valid() || cuECtx.bestTU->cbf[0] == 0)
      && (!cuECtx.bestCU->Cb().valid() || cuECtx.bestTU->cbf[1] == 0)
      && (!cuECtx.bestCU->Cr().valid() || cuECtx.bestTU->cbf[2] == 0))
    {
      return false;
    }
    if( lastTestMode().type != ETM_INTRA && cuECtx.bestCS && cuECtx.bestCU && interHadActive( cuECtx ) )
    {
      // Get SATD threshold from best Inter-CU
      if (!cs.slice->isIntra() && m_pcEncCfg->getUsePbIntraFast() && !cs.slice->getDisableSATDForRD())
      {
        CodingUnit* bestCU = cuECtx.bestCU;
        if (bestCU && !CU::isIntra(*bestCU))
        {
          DistParam distParam;
          const bool useHad = true;
          m_pcRdCost->setDistParam( distParam, cs.getOrgBuf( COMPONENT_Y ), cuECtx.bestCS->getPredBuf( COMPONENT_Y ), cs.sps->getBitDepth( CHANNEL_TYPE_LUMA ), COMPONENT_Y, useHad );
          cuECtx.interHad = distParam.distFunc( distParam );
        }
      }
    }
    if (bestMode.type == ETM_PALETTE && !slice.isIntra() && partitioner.treeType == TREE_D && !(partitioner.currArea().lumaSize().width == 4 && partitioner.currArea().lumaSize().height == 4)) // inter slice
    {
      return false;
    }
    if ( m_pcEncCfg->getUseFastISP() && relatedCU.relatedCuIsValid )
    {
      cuECtx.ispPredModeVal     = relatedCU.ispPredModeVal;
      cuECtx.bestDCT2NonISPCost = relatedCU.bestDCT2NonISPCost;
      cuECtx.relatedCuIsValid   = relatedCU.relatedCuIsValid;
      cuECtx.bestNonDCT2Cost    = relatedCU.bestNonDCT2Cost;
      cuECtx.bestISPIntraMode   = relatedCU.bestISPIntraMode;
    }
    return true;
  }
  else if (encTestmode.type == ETM_PALETTE)
  {
    if (partitioner.currArea().lumaSize().width > 64 || partitioner.currArea().lumaSize().height > 64
        || ((partitioner.currArea().lumaSize().width * partitioner.currArea().lumaSize().height <= 16) && (isLuma(partitioner.chType)) )
        || ((partitioner.currArea().chromaSize().width * partitioner.currArea().chromaSize().height <= 16) && (!isLuma(partitioner.chType)) && partitioner.isSepTree(cs) ) 
      || (partitioner.isLocalSepTree(cs)  && (!isLuma(partitioner.chType)) ) )
    {
      return false;
    }
    const Area curr_cu = CS::getArea(cs, cs.area, partitioner.chType).blocks[getFirstComponentOfChannel(partitioner.chType)];
    try
    {
      double stored_cost = slice.m_mapPltCost[isChroma(partitioner.chType)].at(curr_cu.pos()).at(curr_cu.size());
      if (bestMode.type != ETM_INVALID && stored_cost > cuECtx.bestCS->cost)
      {
        return false;
      }
    }
    catch (const std::out_of_range &)
    {
      // do nothing if no stored cost value was found.
    }
    return true;
  }
  else if (encTestmode.type == ETM_IBC || encTestmode.type == ETM_IBC_MERGE)
  {
    // IBC MODES
    return sps.getIBCFlag() && (partitioner.currArea().lumaSize().width < 128 && partitioner.currArea().lumaSize().height < 128);
  }
  else if( isModeInter( encTestmode ) )
  {
    // INTER MODES (ME + MERGE/SKIP)
    CHECK( slice.isIntra(), "Inter-mode should not be in the I-Slice mode list!" );

    if( getFastDeltaQp() )
    {
      if( encTestmode.type == ETM_MERGE_SKIP )
      {
        return false;
      }
      if( cs.area.lumaSize().width > cs.pcv->fastDeltaQPCuMaxSize )
      {
        return false; // only check necessary 2Nx2N Inter in fast deltaqp mode
      }
    }

    // --- Check if we can quit current mode using SAVE/LOAD coding history

    if( encTestmode.type == ETM_INTER_ME )
    {
      if( encTestmode.opts == ETO_STANDARD )
      {
	if(debug){
	  printf("  $$ tryMode ETO_STD Orig relatedCU = @(%dx%d) [%dx%d]\n", relatedCuArea.lx(), relatedCuArea.ly(), relatedCuArea.lwidth(), relatedCuArea.lheight());
          printf("   |- isInter %d\n", relatedCU.isInter);
          printf("   |- isSkip %d\n", relatedCU.isSkip);
          printf("   |- isIntra %d\n", relatedCU.isIntra);

          printf("   |- bestCost %f\n", relatedCU.bestCost);
          printf("   |- relatedCuIsValid %d\n", relatedCU.relatedCuIsValid);
          printf("   |- isMMVDSkip %d\n", relatedCU.isMMVDSkip);
          printf("   |- isIBC %d\n", relatedCU.isIBC);
          cout <<"   |- BEST_NO_IMV " << cuECtx.get<double>(BEST_NO_IMV_COST) << endl;
          cout <<"   |- BEST_IMV_COST " << cuECtx.get<double>(BEST_IMV_COST) << endl;
	}
        // NOTE: ETO_STANDARD is always done when early SKIP mode detection is enabled
        if( !m_pcEncCfg->getUseEarlySkipDetection() )
        {
          if( relatedCU.isSkip || relatedCU.isIntra )
          {
	    if(debug){
	      printf("  $$ tryMode ELSE Orig retornamos falso no relatedCU. isSkip=%d  isIntra=%d\n", relatedCU.isSkip, relatedCU.isIntra);
            }
            return false;
          }
        }
      }
      else if ((encTestmode.opts & ETO_IMV) != 0)
      {
        if(debug){
	  printf("  $$ tryMode ETO_IMV Orig relatedCU = @(%dx%d) [%dx%d]\n", relatedCuArea.lx(), relatedCuArea.ly(), relatedCuArea.lwidth(), relatedCuArea.lheight());
          printf("   |- isInter %d\n", relatedCU.isInter);
          printf("   |- isSkip %d\n", relatedCU.isSkip);
          printf("   |- isIntra %d\n", relatedCU.isIntra);

          printf("   |- bestCost %f\n", relatedCU.bestCost);
          printf("   |- relatedCuIsValid %d\n", relatedCU.relatedCuIsValid);
          printf("   |- isMMVDSkip %d\n", relatedCU.isMMVDSkip);
          printf("   |- isIBC %d\n", relatedCU.isIBC);
          int imvOpt_BK = (encTestmode.opts & ETO_IMV) >> ETO_IMV_SHIFT;
          cout <<"   |- imvOpt " << imvOpt_BK << endl;
          cout <<"   |- BEST_NO_IMV " << cuECtx.get<double>(BEST_NO_IMV_COST) << endl;
          cout <<"   |- BEST_IMV_COST " << cuECtx.get<double>(BEST_IMV_COST) << endl;
          cout <<"   |- !affineAmvr " << !m_pcEncCfg->getUseAffineAmvr() << endl;
	}
        int imvOpt = (encTestmode.opts & ETO_IMV) >> ETO_IMV_SHIFT;

        if (imvOpt == 3 && cuECtx.get<double>(BEST_NO_IMV_COST) * 1.06 < cuECtx.get<double>(BEST_IMV_COST))
        {
          if ( !m_pcEncCfg->getUseAffineAmvr() ){
	    if(debug){
	      printf("  $$ tryMode ETO_IMV Orig retornamos falso no getUseAffineAmvr \n");
	    }
	    return false;
	  }
        }
      }
    }

    if ( encTestmode.type == ETM_AFFINE && relatedCU.isIntra )
    {
      return false;
    }
    if( encTestmode.type == ETM_MERGE_GEO && ( partitioner.currArea().lwidth() < GEO_MIN_CU_SIZE || partitioner.currArea().lheight() < GEO_MIN_CU_SIZE
                                            || partitioner.currArea().lwidth() > GEO_MAX_CU_SIZE || partitioner.currArea().lheight() > GEO_MAX_CU_SIZE
                                            || partitioner.currArea().lwidth() >= 8 * partitioner.currArea().lheight()
                                            || partitioner.currArea().lheight() >= 8 * partitioner.currArea().lwidth() ) )
    {
      return false;
    }
    return true;
  }
  else if( isModeSplit( encTestmode ) )
  {
    //////////////////////////////////////////////////////////////////////////
    // skip-history rule - don't split further if at least for three past levels
    //                     in the split tree it was found that skip is the best mode
    //////////////////////////////////////////////////////////////////////////
    int skipScore = 0;

    if ((!slice.isIntra() || slice.getSPS()->getIBCFlag()) && cuECtx.get<bool>(IS_BEST_NOSPLIT_SKIP))
    {
      for( int i = 2; i < m_ComprCUCtxList.size(); i++ )
      {
        if( ( m_ComprCUCtxList.end() - i )->get<bool>( IS_BEST_NOSPLIT_SKIP ) )
        {
          skipScore += 1;
        }
        else
        {
          break;
        }
      }
    }

    const PartSplit split = getPartSplit( encTestmode );
    if( !partitioner.canSplit( split, cs ) || skipScore >= 2 )
    {
      if( split == CU_HORZ_SPLIT ) cuECtx.set( DID_HORZ_SPLIT, false );
      if( split == CU_VERT_SPLIT ) cuECtx.set( DID_VERT_SPLIT, false );
      if( split == CU_QUAD_SPLIT ) cuECtx.set( DID_QUAD_SPLIT, false );

      return false;
    }

    if( m_pcEncCfg->getUseContentBasedFastQtbt() )
    {
      const CompArea& currArea = partitioner.currArea().Y();
      int cuHeight  = currArea.height;
      int cuWidth   = currArea.width;

      const bool condIntraInter = m_pcEncCfg->getIntraPeriod() == 1 ? ( partitioner.currBtDepth == 0 ) : ( cuHeight > 32 && cuWidth > 32 );

      if( cuWidth == cuHeight && condIntraInter && getPartSplit( encTestmode ) != CU_QUAD_SPLIT )
      {
        const CPelBuf bufCurrArea = cs.getOrgBuf( partitioner.currArea().block( COMPONENT_Y ) );

        double horVal = 0;
        double verVal = 0;
        double dupVal = 0;
        double dowVal = 0;

        const double th = m_pcEncCfg->getIntraPeriod() == 1 ? 1.2 : 1.0;

        unsigned j, k;

        for( j = 0; j < cuWidth - 1; j++ )
        {
          for( k = 0; k < cuHeight - 1; k++ )
          {
            horVal += abs( bufCurrArea.at( j + 1, k     ) - bufCurrArea.at( j, k ) );
            verVal += abs( bufCurrArea.at( j    , k + 1 ) - bufCurrArea.at( j, k ) );
            dowVal += abs( bufCurrArea.at( j + 1, k )     - bufCurrArea.at( j, k + 1 ) );
            dupVal += abs( bufCurrArea.at( j + 1, k + 1 ) - bufCurrArea.at( j, k ) );
          }
        }
        if( horVal > th * verVal && sqrt( 2 ) * horVal > th * dowVal && sqrt( 2 ) * horVal > th * dupVal && ( getPartSplit( encTestmode ) == CU_HORZ_SPLIT || getPartSplit( encTestmode ) == CU_TRIH_SPLIT ) )
        {
          return false;
        }
        if( th * dupVal < sqrt( 2 ) * verVal && th * dowVal < sqrt( 2 ) * verVal && th * horVal < verVal && ( getPartSplit( encTestmode ) == CU_VERT_SPLIT || getPartSplit( encTestmode ) == CU_TRIV_SPLIT ) )
        {
          return false;
        }
      }

      if( m_pcEncCfg->getIntraPeriod() == 1 && cuWidth <= 32 && cuHeight <= 32 && bestCS && bestCS->tus.size() == 1 && bestCU && bestCU->depth == partitioner.currDepth && partitioner.currBtDepth > 1 && isLuma( partitioner.chType ) )
      {
        if( !bestCU->rootCbf )
        {
          return false;
        }
      }
    }

    if( bestCU && bestCU->skip && bestCU->mtDepth >= m_skipThreshold && !isModeSplit( cuECtx.lastTestMode ) )
    {
      return false;
    }

    int featureToSet = -1;

    switch( getPartSplit( encTestmode ) )
    {
      case CU_QUAD_SPLIT:
        {
#if ENABLE_SPLIT_PARALLELISM
          if( !cuECtx.isLevelSplitParallel )
#endif
          if( !cuECtx.get<bool>( QT_BEFORE_BT ) && bestCU )
          {
            unsigned maxBTD        = cs.pcv->getMaxBtDepth( slice, partitioner.chType );
            const CodingUnit *cuBR = bestCS->cus.back();
            unsigned height        = partitioner.currArea().lumaSize().height;

            if (bestCU && ((bestCU->btDepth == 0 && maxBTD >= ((slice.isIntra() && !slice.getSPS()->getIBCFlag()) ? 3 : 2))
              || (bestCU->btDepth == 1 && cuBR && cuBR->btDepth == 1 && maxBTD >= ((slice.isIntra() && !slice.getSPS()->getIBCFlag()) ? 4 : 3)))
              && (width <= MAX_TB_SIZEY && height <= MAX_TB_SIZEY)
              && cuECtx.get<bool>(DID_HORZ_SPLIT) && cuECtx.get<bool>(DID_VERT_SPLIT))
            {
              return false;
            }
          }
          if( m_pcEncCfg->getUseEarlyCU() && bestCS->cost != MAX_DOUBLE && bestCU && bestCU->skip )
          {
            return false;
          }
          if( getFastDeltaQp() && width <= slice.getPPS()->pcv->fastDeltaQPCuMaxSize )
          {
            return false;
          }
        }
        break;
      case CU_HORZ_SPLIT:
        featureToSet = DID_HORZ_SPLIT;
        break;
      case CU_VERT_SPLIT:
        featureToSet = DID_VERT_SPLIT;
        break;
      case CU_TRIH_SPLIT:
        if( cuECtx.get<bool>( DID_HORZ_SPLIT ) && bestCU && bestCU->btDepth == partitioner.currBtDepth && !bestCU->rootCbf )
        {
          return false;
        }

        if( !cuECtx.get<bool>( DO_TRIH_SPLIT ) )
        {
          return false;
        }
        break;
      case CU_TRIV_SPLIT:
        if( cuECtx.get<bool>( DID_VERT_SPLIT ) && bestCU && bestCU->btDepth == partitioner.currBtDepth && !bestCU->rootCbf )
        {
          return false;
        }

        if( !cuECtx.get<bool>( DO_TRIV_SPLIT ) )
        {
          return false;
        }
        break;
      default:
        THROW( "Only CU split modes are governed by the EncModeCtrl" );
        return false;
        break;
    }

    switch( split )
    {
      case CU_HORZ_SPLIT:
      case CU_TRIH_SPLIT:
        if( cuECtx.get<bool>( QT_BEFORE_BT ) && cuECtx.get<bool>( DID_QUAD_SPLIT ) )
        {
          if( cuECtx.get<int>( MAX_QT_SUB_DEPTH ) > partitioner.currQtDepth + 1 )
          {
            if( featureToSet >= 0 ) cuECtx.set( featureToSet, false );
            return false;
          }
        }
        break;
      case CU_VERT_SPLIT:
      case CU_TRIV_SPLIT:
        if( cuECtx.get<bool>( QT_BEFORE_BT ) && cuECtx.get<bool>( DID_QUAD_SPLIT ) )
        {
          if( cuECtx.get<int>( MAX_QT_SUB_DEPTH ) > partitioner.currQtDepth + 1 )
          {
            if( featureToSet >= 0 ) cuECtx.set( featureToSet, false );
            return false;
          }
        }
        break;
      default:
        break;
    }

    if( split == CU_QUAD_SPLIT ) cuECtx.set( DID_QUAD_SPLIT, true );
    if (cs.sps->getLog2ParallelMergeLevelMinus2())
    {
      const CompArea& area = partitioner.currArea().Y();
      const SizeType size = 1 << (cs.sps->getLog2ParallelMergeLevelMinus2() + 2);
      if (!cs.slice->isIntra() && (area.width > size || area.height > size))
      {
        if (area.height <= size && split == CU_HORZ_SPLIT) return false;
        if (area.width <= size && split == CU_VERT_SPLIT) return false;
        if (area.height <= 2 * size && split == CU_TRIH_SPLIT) return false;
        if (area.width <= 2 * size && split == CU_TRIV_SPLIT) return false;
      }
    }
    return true;
  }
  else
  {
    CHECK( encTestmode.type != ETM_POST_DONT_SPLIT, "Unknown mode" );
    if ((cuECtx.get<double>(BEST_NO_IMV_COST) == (MAX_DOUBLE * .5) 
        #if REUSE_CU_RESULTS
            || cuECtx.get<bool>(IS_REUSING_CU)
        #endif
            ) && !slice.isIntra())
    {
      unsigned idx1, idx2, idx3, idx4;
      getAreaIdx(partitioner.currArea().Y(), *slice.getPPS()->pcv, idx1, idx2, idx3, idx4);
      if (g_isReusedUniMVsFilled[idx1][idx2][idx3][idx4])
      {
        m_pcInterSearch->insertUniMvCands(partitioner.currArea().Y(), g_reusedUniMVs[idx1][idx2][idx3][idx4]);
      }
    }
    if( !bestCS || ( bestCS && isModeSplit( bestMode ) ) )
    {
      return false;
    }
    else
    {
#if REUSE_CU_RESULTS
      setFromCs( *bestCS, partitioner );

#endif
      if( partitioner.modeType == MODE_TYPE_INTRA && partitioner.chType == CHANNEL_TYPE_LUMA )
      {
        return false; //not set best coding mode for intra coding pass
      }
      // assume the non-split modes are done and set the marks for the best found mode
      if( bestCS && bestCU )
      {
        if( CU::isInter( *bestCU ) )
        {
          relatedCU.isInter   = true;
          relatedCU.isSkip   |= bestCU->skip;
          relatedCU.isMMVDSkip |= bestCU->mmvdSkip;
          relatedCU.BcwIdx    = bestCU->BcwIdx;
          if (bestCU->slice->getSPS()->getUseColorTrans())
          {
            if (m_pcEncCfg->getRGBFormatFlag())
            {
              if (bestCU->colorTransform && bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
            else
            {
              if (!bestCU->colorTransform || !bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
          }
        }
        else if (CU::isIBC(*bestCU))
        {
          relatedCU.isIBC = true;
          relatedCU.isSkip |= bestCU->skip;
          if (bestCU->slice->getSPS()->getUseColorTrans())
          {
            if (m_pcEncCfg->getRGBFormatFlag())
            {
              if (bestCU->colorTransform && bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
            else
            {
              if (!bestCU->colorTransform || !bestCU->rootCbf)
              {
                relatedCU.selectColorSpaceOption = 1;
              }
              else
              {
                relatedCU.selectColorSpaceOption = 2;
              }
            }
          }
        }
        else if( CU::isIntra( *bestCU ) )
        {
          relatedCU.isIntra   = true;
          if ( m_pcEncCfg->getUseFastISP() && cuECtx.ispWasTested && ( !relatedCU.relatedCuIsValid || bestCS->cost < relatedCU.bestCost ) )
          {
            // Compact data
            int bit0 = true;
            int bit1 = cuECtx.ispMode == NOT_INTRA_SUBPARTITIONS ? 1 : 0;
            int bit2 = cuECtx.ispMode == VER_INTRA_SUBPARTITIONS;
            int bit3 = cuECtx.ispLfnstIdx > 0;
            int bit4 = cuECtx.ispLfnstIdx == 2;
            int bit5 = cuECtx.mipFlag;
            int bit6 = cuECtx.bestCostIsp < cuECtx.bestNonDCT2Cost * 0.95;
            int val =
              (bit0) |
              (bit1 << 1) |
              (bit2 << 2) |
              (bit3 << 3) |
              (bit4 << 4) |
              (bit5 << 5) |
              (bit6 << 6) |
              ( cuECtx.bestPredModeDCT2 << 9 );
            relatedCU.ispPredModeVal     = val;
            relatedCU.bestDCT2NonISPCost = cuECtx.bestDCT2NonISPCost;
            relatedCU.bestCost           = bestCS->cost;
            relatedCU.bestNonDCT2Cost    = cuECtx.bestNonDCT2Cost;
            relatedCU.bestISPIntraMode   = cuECtx.bestISPIntraMode;
            relatedCU.relatedCuIsValid   = true;
          }
        }
#if ENABLE_SPLIT_PARALLELISM
#if REUSE_CU_RESULTS
        BestEncInfoCache::touch(partitioner.currArea());
#endif
        CacheBlkInfoCtrl::touch(partitioner.currArea());
#endif
        cuECtx.set( IS_BEST_NOSPLIT_SKIP, bestCU->skip );
      }
    }

    return false;
  }
}


bool EncModeCtrlMTnoRQT::checkSkipOtherLfnst( const EncTestMode& encTestmode, CodingStructure*& tempCS, Partitioner& partitioner )
{
  xExtractFeatures( encTestmode, *tempCS );

  ComprCUCtx& cuECtx  = m_ComprCUCtxList.back();
  bool skipOtherLfnst = false;

  if( encTestmode.type == ETM_INTRA )
  {
    if( !cuECtx.bestCS || ( tempCS->cost >= cuECtx.bestCS->cost && cuECtx.bestCS->cus.size() == 1 && CU::isIntra( *cuECtx.bestCS->cus[ 0 ] ) )
      || ( tempCS->cost <  cuECtx.bestCS->cost && CU::isIntra( *tempCS->cus[ 0 ] ) ) )
    {
      skipOtherLfnst = !tempCS->cus[ 0 ]->rootCbf;
    }
  }

  return skipOtherLfnst;
}

bool EncModeCtrlMTnoRQT::useModeResult( const EncTestMode& encTestmode, CodingStructure*& tempCS, Partitioner& partitioner )
{
  xExtractFeatures( encTestmode, *tempCS );

  ComprCUCtx& cuECtx = m_ComprCUCtxList.back();


  if(      encTestmode.type == ETM_SPLIT_BT_H )
  {
    cuECtx.set( BEST_HORZ_SPLIT_COST, tempCS->cost );
  }
  else if( encTestmode.type == ETM_SPLIT_BT_V )
  {
    cuECtx.set( BEST_VERT_SPLIT_COST, tempCS->cost );
  }
  else if( encTestmode.type == ETM_SPLIT_TT_H )
  {
    cuECtx.set( BEST_TRIH_SPLIT_COST, tempCS->cost );
  }
  else if( encTestmode.type == ETM_SPLIT_TT_V )
  {
    cuECtx.set( BEST_TRIV_SPLIT_COST, tempCS->cost );
  }
  else if( encTestmode.type == ETM_INTRA )
  {
    const CodingUnit cu = *tempCS->getCU( partitioner.chType );

    if( !cu.mtsFlag )
    {
      cuECtx.bestMtsSize2Nx2N1stPass   = tempCS->cost;
    }
    if( !cu.ispMode )
    {
      cuECtx.bestCostMtsFirstPassNoIsp = tempCS->cost;
    }
  }

  if( m_pcEncCfg->getIMV4PelFast() && m_pcEncCfg->getIMV() && encTestmode.type == ETM_INTER_ME )
  {
    int imvMode = ( encTestmode.opts & ETO_IMV ) >> ETO_IMV_SHIFT;

    if( imvMode == 1 )
    {
      if( tempCS->cost < cuECtx.get<double>( BEST_IMV_COST ) )
      {
        cuECtx.set( BEST_IMV_COST, tempCS->cost );
      }
    }
    else if( imvMode == 0 )
    {
      if( tempCS->cost < cuECtx.get<double>( BEST_NO_IMV_COST ) )
      {
        cuECtx.set( BEST_NO_IMV_COST, tempCS->cost );
      }
    }
  }

  if( encTestmode.type == ETM_SPLIT_QT )
  {
    int maxQtD = 0;
    for( const auto& cu : tempCS->cus )
    {
      maxQtD = std::max<int>( maxQtD, cu->qtDepth );
    }
    cuECtx.set( MAX_QT_SUB_DEPTH, maxQtD );
  }

  int maxMtD = tempCS->pcv->getMaxBtDepth( *tempCS->slice, partitioner.chType ) + partitioner.currImplicitBtDepth;

  if( encTestmode.type == ETM_SPLIT_BT_H )
  {
    if( tempCS->cus.size() > 2 )
    {
      int h_2   = tempCS->area.blocks[partitioner.chType].height / 2;
      int cu1_h = tempCS->cus.front()->blocks[partitioner.chType].height;
      int cu2_h = tempCS->cus.back() ->blocks[partitioner.chType].height;

      cuECtx.set( DO_TRIH_SPLIT, cu1_h < h_2 || cu2_h < h_2 || partitioner.currMtDepth + 1 == maxMtD );
    }
  }
  else if( encTestmode.type == ETM_SPLIT_BT_V )
  {
    if( tempCS->cus.size() > 2 )
    {
      int w_2   = tempCS->area.blocks[partitioner.chType].width / 2;
      int cu1_w = tempCS->cus.front()->blocks[partitioner.chType].width;
      int cu2_w = tempCS->cus.back() ->blocks[partitioner.chType].width;

      cuECtx.set( DO_TRIV_SPLIT, cu1_w < w_2 || cu2_w < w_2 || partitioner.currMtDepth + 1 == maxMtD );
    }
  }

  // for now just a simple decision based on RD-cost or choose tempCS if bestCS is not yet coded
  if( tempCS->features[ENC_FT_RD_COST] != MAX_DOUBLE && ( !cuECtx.bestCS || ( ( tempCS->features[ENC_FT_RD_COST] + ( tempCS->useDbCost ? tempCS->costDbOffset : 0 ) ) < ( cuECtx.bestCS->features[ENC_FT_RD_COST] + ( tempCS->useDbCost ? cuECtx.bestCS->costDbOffset : 0 ) ) ) ) )
  {
    cuECtx.bestCS = tempCS;
    cuECtx.bestCU = tempCS->cus[0];
    cuECtx.bestTU = cuECtx.bestCU->firstTU;

    if( isModeInter( encTestmode ) )
    {
      //Here we take the best cost of both inter modes. We are assuming only the inter modes (and all of them) have come before the intra modes!!!
      cuECtx.bestInterCost = cuECtx.bestCS->cost;
    }

    return true;
  }
  else
  {
    return false;
  }
}

