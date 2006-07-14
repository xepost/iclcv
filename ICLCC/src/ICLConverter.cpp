#include "ICLConverter.h"
#include "ICLcc.h"

namespace icl{
  ICLConverter::ICLConverter():m_poDepthBuffer(0),m_poSizeBuffer(0){}
  ICLConverter::~ICLConverter(){
    if(m_poDepthBuffer)delete m_poDepthBuffer;
    if(m_poSizeBuffer)delete m_poSizeBuffer;
  }

  void ICLConverter::convert(ICLBase *poDst, ICLBase *poSrc){
    iclformat eSrcFmt = poSrc->getFormat();
    iclformat eDstFmt = poDst->getFormat();
    icldepth eSrcDepth = poSrc->getDepth();
    icldepth eDstDepth = poDst->getDepth();
    int iNeedDepthConversion = eSrcDepth!=eDstDepth;
    int iNeedSizeConversion = poDst->getWidth() != poSrc->getWidth() || poDst->getHeight() != poSrc->getHeight();
    int iNeedColorConversion = eSrcFmt != formatMatrix && eDstFmt != formatMatrix && eSrcFmt != eDstFmt;
    
    //---- convert depth ----------------- 
    ICLBase *poNextSrcImage=poSrc;
    if(iNeedDepthConversion){ 
      // test if other convesion steps will follow:
      if(iNeedSizeConversion || iNeedColorConversion){
        iclEnsureDepth(&m_poDepthBuffer,eDstDepth);
        m_poDepthBuffer->setFormat(eSrcFmt);
        m_poDepthBuffer->resize(poNextSrcImage->getWidth(),poNextSrcImage->getHeight());
        poNextSrcImage->deepCopy(m_poDepthBuffer);
        poNextSrcImage=m_poDepthBuffer;
      }else{
        poNextSrcImage->deepCopy(poDst);
        return;
      }
    }

    //---- convert size ----------------- 
    if(iNeedSizeConversion){
      if(iNeedColorConversion){
        iclEnsureDepth(&m_poSizeBuffer,poNextSrcImage->getDepth());
        m_poSizeBuffer->renew(poDst->getWidth(),poDst->getHeight(),poDst->getChannels());
        m_poSizeBuffer->setFormat(eSrcFmt);
        poNextSrcImage->scaledCopy(m_poSizeBuffer);
        //---- convert color ----------------- 
        iclcc(poDst,m_poSizeBuffer);
      }else{
        poNextSrcImage->scaledCopy(poDst);
      }      
    }else if(iNeedColorConversion){
      iclcc(poDst,poNextSrcImage);
    }else if(!iNeedDepthConversion){
      poNextSrcImage->deepCopy(poDst);
    }
    
  }
}
