
#include <ConvolutionOp.h>
#include <Img.h>

namespace icl {
  
  /* To improve performance in access to the kernel data, we may create an
     internal buffer of the kernel data. This is done, if the user explicitly
     requests buffering, providing the bBufferData flag in the constructor. In
     this case already the constructor creates new kernel arrays piKernel
     and pfKernel within the methods <em>bufferKernel()</em>. Additionally
     the flag m_bBuffered is set to true.
     Note, that an external float pointer can only be buffered as int* piKernel
     if the data can be interpreted as integers directly. Currently we do not
     search for a greatest common divisor, which may be used as a normalization factor.

     If data is not buffered, we create a suitable piKernel or pfKernel buffer
     only on demand within the apply method. eKernelDepth indentifies which
     of the two buffers (pfKernel or piKernel) is external. 

     Because IPP (as well as the fallback implementation) does not provide a method
     to convolute a float image with an int kernel, we always use a float
     kernel in this case, which has to be copied from an external int kernel
     for every application.
  */
  // {{{ fixed convolution masks

  // first element specifies normalization factor
  int ConvolutionOp::KERNEL_GAUSS_3x3[10] = { 16,
                                            1, 2, 1,
                                            2, 4, 2,
                                            1, 2, 1 };
  int ConvolutionOp::KERNEL_GAUSS_5x5[26] = { 571,
                                            2,  7,  12,  7,  2,
                                            7, 31,  52, 31,  7,
                                           12, 52, 127, 52, 12,
                                            7, 31,  52, 31,  7,
                                            2,  7,  12,  7,  2 };
 
  int ConvolutionOp::KERNEL_SOBEL_X_3x3[10] = { 1,
                                              1,  0, -1,
                                              2,  0, -2,
                                              1,  0, -1 };
  int ConvolutionOp::KERNEL_SOBEL_X_5x5[26] = { 1,
                                              1,  2,  0,  -2,  -1, 
                                              4,  8,  0,  -8,  -4,
                                              6, 12,  0, -12,  -6,
                                              4,  8,  0,  -8,  -4,
                                              1,  2,  0,  -2,  -1 };
   
  int ConvolutionOp::KERNEL_SOBEL_Y_3x3[10] = {  1, 
                                               1,  2,  1,
                                               0,  0,  0,
                                              -1, -2, -1  };
  int ConvolutionOp::KERNEL_SOBEL_Y_5x5[26] = {  1,
                                                1,  4,   6,  4,  1,
                                                2,  8,  12,  8,  2,
                                                0,  0,   0,  0,  0,
                                               -2, -8, -12, -8, -4,
                                               -1, -4,  -6, -4, -1 };
 
  int ConvolutionOp::KERNEL_LAPLACE_3x3[10] = { 1,
                                              1, 1, 1,
                                              1,-8, 1,
                                              1, 1, 1} ;
  int ConvolutionOp::KERNEL_LAPLACE_5x5[26] = { 1,
                                              -1, -3, -4, -3, -1,
                                              -3,  0,  6,  0, -3,
                                              -4,  6, 20,  6, -4,
                                              -3,  0,  6,  0, -3,
                                              -1, -3, -4, -3, -1 };
  // }}}

  // {{{ isConvertableToInt

  bool ConvolutionOp::isConvertableToInt (float *pfData, int iLen)
  {
     FUNCTION_LOG("");
     // tests if an element of the given float* has decimals
     // if it does: return 0, else 1
     for(int i=0;i<iLen;i++)
        if (pfData[i] != (float) rint (pfData[i])) return false;
     return true;
  }

  // }}}
  
  // {{{ Constructors / Destructor
   
  // {{{ buffering kernel data

  void ConvolutionOp::copyIntToFloatKernel (int iDim) {
     FUNCTION_LOG("");
     if (!pfKernel) pfKernel = new float[iDim];
     register int   *pi=piKernel, *piEnd=pi+iDim;
     register float *pf=pfKernel;
     // first element of piKernel contains normalization factor:
     for (; pi < piEnd; ++pi, ++pf)
        *pf = (float) *pi / (float) iNormFactor;
  }

  // initially create buffer array (within constructor only)
  void ConvolutionOp::bufferKernel (float *pfKernelExt) {
     FUNCTION_LOG("");
     int iDim = getMaskSize().getDim();
     if (!pfKernel) pfKernel = new float[iDim];
     std::copy (pfKernelExt, pfKernelExt+iDim, pfKernel);
     
     if (isConvertableToInt (pfKernelExt, iDim)) {
        if (!piKernel) piKernel = new int[iDim]; 
        iNormFactor = 1;
          
        register float *pf=pfKernelExt, *pfEnd=pfKernelExt+iDim;
        register int   *pi=piKernel;
        for (; pf < pfEnd; ++pf, ++pi) {
          *pi = static_cast<int>(*pf);
        }
     }
  }
  // initially create buffer array (within constructor only)
  void ConvolutionOp::bufferKernel (int *piKernelExt) {
     FUNCTION_LOG("");
     int iDim = getMaskSize().getDim();

     if (!piKernel) piKernel = new int[iDim];
     std::copy (piKernelExt, piKernelExt+iDim, piKernel);
     copyIntToFloatKernel (iDim);
  }

  // }}}

  // {{{ setKernel

  void ConvolutionOp::cleanupKernels (depth newDepth, const Size& size, bool bBufferData) {
     if (size != getMaskSize() ||
         m_eKernel != kernelCustom ||
         m_bBuffered != bBufferData ||
         m_eKernelDepth != newDepth) {
        // major mismatch: do not reuse buffers at all
        releaseBuffers ();

        // set new values
        setMask (size);
        m_eKernel = kernelCustom;
        m_eKernelDepth = newDepth;
        m_bBuffered = bBufferData;
     }
  }

#ifdef WITH_IPP_OPTIMIZATION
  void ConvolutionOp::setIPPFixedMethods (kernel eKernel) {
     pFixed8u  = 0; pFixedMask8u  = 0; 
     pFixed16s = 0; pFixedMask16s = 0;
     pFixed32f = 0; pFixedMask32f = 0;
     switch (m_eKernel) {
       case kernelSobelX3x3:
          pFixed8u  = ippiFilterSobelHoriz_8u_C1R;
          pFixed16s = ippiFilterSobelHoriz_16s_C1R;
          pFixed32f = ippiFilterSobelHoriz_32f_C1R;
          break;
       case kernelSobelX5x5:
          pFixedMask8u  = 0;
          pFixedMask16s = 0;
          pFixedMask32f = ippiFilterSobelHorizMask_32f_C1R;
          break;
       case kernelSobelY3x3:
          pFixed8u  = ippiFilterSobelVert_8u_C1R;
          pFixed16s = ippiFilterSobelVert_16s_C1R;
          pFixed32f = ippiFilterSobelVert_32f_C1R;
          break;
       case kernelSobelY5x5:
          pFixedMask8u  = 0;
          pFixedMask16s = 0;
          pFixedMask32f = ippiFilterSobelVertMask_32f_C1R;
          break;
       case kernelGauss3x3:
       case kernelGauss5x5:
          pFixedMask8u = ippiFilterGauss_8u_C1R;
          pFixedMask16s = ippiFilterGauss_16s_C1R;
          pFixedMask32f = ippiFilterGauss_32f_C1R;
          break;
       case kernelLaplace3x3:
       case kernelLaplace5x5:
          pFixedMask8u = ippiFilterLaplace_8u_C1R;
          pFixedMask16s = ippiFilterLaplace_16s_C1R;
          pFixedMask32f = ippiFilterLaplace_32f_C1R;
          break;
       default: break;
     }

     if (pFixed8u) aMethods[depth8u] = &ConvolutionOp::ippFixedConv<icl8u>;
     else if (pFixedMask8u) aMethods[depth8u] = &ConvolutionOp::ippFixedConvMask<icl8u>;

     if (pFixed16s) aMethods[depth16s] = &ConvolutionOp::ippFixedConv<icl16s>;
     else if (pFixedMask16s) aMethods[depth16s] = &ConvolutionOp::ippFixedConvMask<icl16s>;

     if (pFixed32f) aMethods[depth32f] = &ConvolutionOp::ippFixedConv<icl32f>;
     else if (pFixedMask32f) aMethods[depth32f] = &ConvolutionOp::ippFixedConvMask<icl32f>;
  }
#endif

  void ConvolutionOp::setKernel (kernel eKernel) {
     if (eKernel < kernelGauss3x3 || eKernel > kernelLaplace5x5) {
        ERROR_LOG ("unsupported kernel type");
        return;
     }
        
     releaseBuffers();
     m_bBuffered = true;
     m_eKernel = eKernel;
     m_eKernelDepth = depth32s;

     // select kernel
     static int* apiKernels[kernelCustom] = {KERNEL_GAUSS_3x3, KERNEL_GAUSS_5x5, 
                                             KERNEL_SOBEL_X_3x3, KERNEL_SOBEL_X_5x5,
                                             KERNEL_SOBEL_Y_3x3, KERNEL_SOBEL_Y_5x5,
                                             KERNEL_LAPLACE_3x3, KERNEL_LAPLACE_5x5};
     piKernel = apiKernels[eKernel];
     int nSize = (eKernel % 2 == 0) ? 3 : 5;
     setMask (Size(nSize, nSize));

     this->iNormFactor = *piKernel; ++piKernel;
     // create buffer for pfKernel:
     copyIntToFloatKernel (getMaskSize().getDim());
     
     // clear all method pointers
     for (unsigned int i=depth8u; i <= depthLast; i++) aMethods[i] = 0;
     
#ifdef WITH_IPP_OPTIMIZATION
     setIPPFixedMethods(eKernel);
#endif
     // set (still) undefined method pointers to generic fallback versions
     // this is needed also for IPP, because not all 5x5 version are IPP supported
     if (aMethods[depth8u] == 0)  aMethods[depth8u]  = &ConvolutionOp::cGenericConv<icl8u,  int, true>;
     if (aMethods[depth16s] == 0) aMethods[depth16s] = &ConvolutionOp::cGenericConv<icl16s, int, true>;
     if (aMethods[depth32s] == 0) aMethods[depth32s] = &ConvolutionOp::cGenericConv<icl32s, int, true>;

     if (aMethods[depth32f] == 0) aMethods[depth32f] = &ConvolutionOp::cGenericConv<icl32f, float, false>;
     if (aMethods[depth64f] == 0) aMethods[depth64f] = &ConvolutionOp::cGenericConv<icl64f, float, false>;
  }

  void ConvolutionOp::setKernel (icl32f *pfKernel, const Size& size, bool bBufferData) {
     cleanupKernels (depth32f, size, bBufferData);

     if (m_bBuffered) bufferKernel (pfKernel); // buffer kernel internally
     else this->pfKernel = pfKernel; // simply use kernel pointer

     // use float-kernel variants
     for (unsigned int i=0; i <= depthLast; ++i) aMethods[i] = aGenericMethods[i][1];
  }

  void ConvolutionOp::setKernel (int *piKernel, const Size& size, 
                               int iNormFactor, bool bBufferData) {
     cleanupKernels (depth32s, size, bBufferData);

     this->iNormFactor = iNormFactor;
     if (m_bBuffered) bufferKernel (piKernel); // buffer kernel internally
     else this->piKernel = piKernel; // simply use kernel pointer

     // use int-kernel variants for integer-valued images
     for (unsigned int i=0; i < depth32f; ++i) aMethods[i] = aGenericMethods[i][0];
     // but use float-kernel variants for float images
     for (unsigned int i=depth32f; i <= depthLast; ++i) aMethods[i] = aGenericMethods[i][1];
  }

  // }}}

  void ConvolutionOp::initMethods () {
     for (int i=0; i <= depthLast; ++i) aMethods[i] = &ConvolutionOp::dummyConvMethod;
  }

  ConvolutionOp::ConvolutionOp(kernel eKernel) :
     // {{{ open

     pfKernel(0), piKernel(0), m_bBuffered(true), 
     m_eKernel(eKernel), m_eKernelDepth(depth32s)
  {
     FUNCTION_LOG("");
     initMethods ();
     setKernel (eKernel);
  }

  // }}}

  ConvolutionOp::ConvolutionOp () :
     // {{{ open
     NeighborhoodOp (Size(INT_MAX, INT_MAX)), // huge kernel size -> prepare returns false
     pfKernel(0), piKernel(0), m_bBuffered(false), 
     m_eKernel(kernelCustom), m_eKernelDepth(depth32s)
  {
     FUNCTION_LOG("");
     initMethods ();
  }

  // }}}

  ConvolutionOp::ConvolutionOp (icl32f *pfKernel, const Size &size,
                            bool bBufferData) :
     // {{{ open
     NeighborhoodOp (size),
     pfKernel(0), piKernel(0), m_bBuffered(bBufferData), 
     m_eKernel(kernelCustom), m_eKernelDepth(depth32f)
  {
     FUNCTION_LOG("");
     setKernel(pfKernel, size, bBufferData);
  }

  // }}}
  
  ConvolutionOp::ConvolutionOp(int *piKernel, const Size &size, 
                           int iNormFactor, bool bBufferData) :
     // {{{ open
     NeighborhoodOp (size),
     pfKernel(0), piKernel(0), m_bBuffered(bBufferData), 
     m_eKernel(kernelCustom), m_eKernelDepth(depth32s)
  {
     FUNCTION_LOG("");
     setKernel(piKernel, size, iNormFactor, bBufferData);
  }

  // }}}

  void ConvolutionOp::releaseBuffers () {
     // free allocated kernel buffers
     if (m_bBuffered) { // can delete both buffers
        delete[] pfKernel;
        // in case of special kernel and no ipp usage
        // piKernel points to static class data and may not be freed
        if (m_eKernel == kernelCustom) delete[] piKernel;
     } else {
        if (m_eKernelDepth == depth32s) delete[] pfKernel;
        if (m_eKernelDepth == depth32f) delete[] piKernel;
     }
     pfKernel = 0; piKernel = 0;
  }

  ConvolutionOp::~ConvolutionOp(){
     // {{{ open

     FUNCTION_LOG("");
     releaseBuffers();
  }

  // }}}

  // }}}
 
#ifdef WITH_IPP_OPTIMIZATION 

  // {{{ generic ipp convolution

  template<typename T, IppStatus (IPP_DECL *pMethod)(const T*, int, T*, int, IppiSize, const Ipp32s*, IppiSize, IppiPoint, int)>
  void ConvolutionOp::ippGenericConvIntKernel (const ImgBase *poSrc, ImgBase *poDst) {
     const Img<T> *poS = poSrc->asImg<T>();
     Img<T> *poD = poDst->asImg<T>();
     for(int c=0; c < poSrc->getChannels(); c++) {
        pMethod (poS->getROIData (c, getROIOffset()), poS->getLineStep(),
                 poD->getROIData (c), poD->getLineStep(), 
                 poD->getROISize(), piKernel, getMaskSize(),getAnchor(), iNormFactor);
     }
  }

  template<typename T, IppStatus (IPP_DECL *pMethod)(const T*, int, T*, int, IppiSize, const Ipp32f*, IppiSize, IppiPoint)>
  void ConvolutionOp::ippGenericConvFloatKernel (const ImgBase *poSrc, ImgBase *poDst) {
     const Img<T> *poS = poSrc->asImg<T>();
     Img<T> *poD = poDst->asImg<T>();
     for(int c=0; c < poSrc->getChannels(); c++) {
        pMethod (poS->getROIData (c, getROIOffset()), poS->getLineStep(),
                 poD->getROIData (c), poD->getLineStep(), 
                 poD->getROISize(), pfKernel, getMaskSize(), getAnchor());
     }
  }

  // }}}
   
  // {{{ fixed ipp convolution

#define ICL_INSTANTIATE_DEPTH(T) \
  template<> inline IppStatus (IPP_DECL *ConvolutionOp::getIppFixedMethod() const) \
   (const Ipp ## T*, int, Ipp ## T*, int, IppiSize) {return pFixed ## T;} \
  template<> inline IppStatus (IPP_DECL *ConvolutionOp::getIppFixedMaskMethod() const) \
   (const Ipp ## T*, int, Ipp ## T*, int, IppiSize, IppiMaskSize) {return pFixedMask ## T;}

  ICL_INSTANTIATE_DEPTH(8u)
  ICL_INSTANTIATE_DEPTH(16s)
  ICL_INSTANTIATE_DEPTH(32f)
#undef ICL_INSTANTIATE_DEPTH

  template<typename T>
  void ConvolutionOp::ippFixedConv (const ImgBase *poSrc, ImgBase *poDst) {
     IppStatus (IPP_DECL *pMethod)(const T* pSrc, int srcStep, T* pDst, int dstStep, IppiSize roiSize)
        = getIppFixedMethod<T>();
     const Img<T> *poS = (Img<T>*) poSrc;
     Img<T> *poD = (Img<T>*) poDst;
     for(int c=0; c < poSrc->getChannels(); c++) {
        pMethod (poS->getROIData (c, getROIOffset()), poS->getLineStep(),
                 poD->getROIData (c), poD->getLineStep(), 
                 poD->getROISize());
     }
  }
  template<typename T>
  void ConvolutionOp::ippFixedConvMask (const ImgBase *poSrc, ImgBase *poDst) {
     IppStatus (IPP_DECL *pMethod)(const T* pSrc, int srcStep, T* pDst, int dstStep, IppiSize roiSize, IppiMaskSize mask)
        = getIppFixedMaskMethod<T>();
     IppiMaskSize eMaskSize = (IppiMaskSize)(11 * getMaskSize().width);
     WARNING_LOG("what is that 11 about here ?");
     const Img<T> *poS = (Img<T>*) poSrc;
     Img<T> *poD = (Img<T>*) poDst;
     
     for(int c=0; c < poSrc->getChannels(); c++) {
        pMethod(poS->getROIData (c, getROIOffset()), poS->getLineStep(),
                poD->getROIData (c), poD->getLineStep(), 
                poD->getROISize(), eMaskSize);
     }
  }

  // }}}

#endif // cGenericConv is also used for non-supported depths in IPP mode

  // {{{ generic fallback convolution

  template<typename ImageT, typename KernelT, bool bUseFactor>
  void ConvolutionOp::cGenericConv (const ImgBase *poSrc, ImgBase *poDst)
  {
    const Img<ImageT> *poS = (Img<ImageT>*) poSrc;
    Img<ImageT> *poD = (Img<ImageT>*) poDst;

    // accumulator for each pixel result of Type M
    KernelT buffer, factor = bUseFactor ? iNormFactor : 1; 
    
    // pointer to the mask
    const KernelT *m;
    for(int c=0; c < poSrc->getChannels(); c++) {
       ConstImgIterator<ImageT> s (poS->getData(c), poS->getSize().width, 
                                   Rect (getROIOffset(), poD->getROISize()));
       ImgIterator<ImageT>      d = poD->getROIIterator(c);
       for(; s.inRegion(); ++s, ++d)
       {
          m = this->getKernel<KernelT>(); buffer = 0;
          for(ConstImgIterator<ImageT> sR (s,getMaskSize(),getAnchor()); 
              sR.inRegion(); ++sR, ++m)
          {
             buffer += (*m) * (*sR);
          }
          *d = Cast<KernelT, ImageT>::cast(buffer / factor);
       }
    }
  }

  // }}}

  // {{{ static MethodPointers aGenericMethods

  // array of image- and kernel-type selective generic convolution methods
  void (ConvolutionOp::*ConvolutionOp::aGenericMethods[depthLast+1][2])(const ImgBase *poSrc, ImgBase *poDst) = {
#ifdef WITH_IPP_OPTIMIZATION 
     {&ConvolutionOp::ippGenericConvIntKernel<icl8u,ippiFilter_8u_C1R>,
      &ConvolutionOp::ippGenericConvFloatKernel<icl8u,ippiFilter32f_8u_C1R>},

     {&ConvolutionOp::ippGenericConvIntKernel<icl16s,ippiFilter_16s_C1R>,
      &ConvolutionOp::ippGenericConvFloatKernel<icl16s,ippiFilter32f_16s_C1R>},
#else
     {&ConvolutionOp::cGenericConv<icl8u,int,true>,
      &ConvolutionOp::cGenericConv<icl8u,float,false>},

     {&ConvolutionOp::cGenericConv<icl16s,int,true>,
      &ConvolutionOp::cGenericConv<icl16s,float,false>},
#endif
     {&ConvolutionOp::cGenericConv<icl32s,int,true>,
      &ConvolutionOp::cGenericConv<icl32s,float,false>},
#ifdef WITH_IPP_OPTIMIZATION 
     {0,
      &ConvolutionOp::ippGenericConvFloatKernel<icl32f,ippiFilter_32f_C1R>},
#else
     {0,
      &ConvolutionOp::cGenericConv<icl32f,float,false>},
#endif
     {0,
      &ConvolutionOp::cGenericConv<icl64f,float,false>}
  };

  // }}}

  // {{{ ConvolutionOp::apply (ImgBase *poSrc, ImgBase **ppoDst)

  void ConvolutionOp::apply(const ImgBase *poSrc, ImgBase **ppoDst)
  {
    FUNCTION_LOG("");
    if (!prepare (ppoDst, poSrc)) return;

    if (m_eKernelDepth == depth32s && !m_bBuffered && poSrc->getDepth () >= depth32f)
       // data has to be copied from (external) int* kernel to internal float buffer
       copyIntToFloatKernel (getMaskSize().getDim());
    // before applying the actual convolution
    (this->*(aMethods[poSrc->getDepth()])) (poSrc, *ppoDst);
  }

  // }}}

  // {{{ ConvolutionOp::apply (ImgBase *poSrc, ImgBase *poDst)

  void ConvolutionOp::apply(const ImgBase *poSrc, ImgBase *poDst)
  {
     ICLASSERT_RETURN(poSrc->getDepth() == poDst->getDepth());
     ImgBase **ppoDst = &poDst;

     apply (poSrc, ppoDst);
     ICLASSERT(*ppoDst == poDst);
  }

  // }}}


 
} // namespace icl
