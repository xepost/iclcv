/********************************************************************
**                Image Component Library (ICL)                    **
**                                                                 **
** Copyright (C) 2006-2012 CITEC, University of Bielefeld          **
**                         Neuroinformatics Group                  **
** Website: www.iclcv.org and                                      **
**          http://opensource.cit-ec.de/projects/icl               **
**                                                                 **
** File   : ICLMath/src/TemplateTracker.cpp                        **
** Module : ICLMath                                                **
** Authors: Eckard Riedenklau, Christof Elbrechter                 **
**                                                                 **
**                                                                 **
** Commercial License                                              **
** ICL can be used commercially, please refer to our website       **
** www.iclcv.org for more details.                                 **
**                                                                 **
** GNU General Public License Usage                                **
** Alternatively, this file may be used under the terms of the     **
** GNU General Public License version 3.0 as published by the      **
** Free Software Foundation and appearing in the file LICENSE.GPL  **
** included in the packaging of this file.  Please review the      **
** following information to ensure the GNU General Public License  **
** version 3.0 requirements will be met:                           **
** http://www.gnu.org/copyleft/gpl.html.                           **
**                                                                 **
** The development of this software was supported by the           **
** Excellence Cluster EXC 277 Cognitive Interaction Technology.    **
** The Excellence Cluster EXC 277 is a grant of the Deutsche       **
** Forschungsgemeinschaft (DFG) in the context of the German       **
** Excellence Initiative.                                          **
**                                                                 **
*********************************************************************/
#include <ICLMath/TemplateTracker.h>

#include <ICLFilter/ProximityOp.h>
#include <ICLFilter/RotateOp.h>
#include <ICLQuick/Quick.h>

namespace icl{

  struct TemplateTracker::Data{
    ImgBase *buf;
    SmartPtr<ProximityOp> prox;
    std::vector<SmartPtr<Img8u> > lut;
    TemplateTracker::Result lastResult;
  };


  TemplateTracker::TemplateTracker(const Img8u *templateImage, 
                                   float rotationStepSizeDegree,
                                   int positionTrackingRangePix, 
                                   float rotationTrackingRangeDegree,
                                   int coarseSteps,int fineSteps,
                                   const Result &initialResult){
    data = new Data;
    data->buf = 0;
    data->prox = new ProximityOp(ProximityOp::crossCorrCoeff);
    data->lastResult = initialResult;
    
    addProperty("tracking.position range","range","[1,1000]:1",positionTrackingRangePix,0,
                "Search window size in positive and negative\n"
                "x- and y-direction in pixels. The tracker searches\n"
                "for the best-matching new position within a squared\n"
                "region that is centered at the current postion.");
    addProperty("tracking.rotation range","range","[0,360]",rotationTrackingRangeDegree,0,
                "Sets the rotation search window radius in deg.\n"               
                "The rotation search window is centered at the\n"
                "current rotation.");
    addProperty("tracking.coarse steps","range:spinbox","[1,100000]:1",coarseSteps,0,
                "Number of rotation tracking steps. The rotation\n"
                "search window size is partitioned into the given\n"
                "step count. The rotation search window size devided\n"
                "the amount of steps define the angle resolution.");
    addProperty("tracking.fine steps","range:spinbox","[1,100000]:1",fineSteps,0,
                "Parameter for coarse to fine search\n"
                "(not supported yet)");

    addChildConfigurable(data->prox.get(),"proximity");

    
    if(templateImage){
      setTemplateImage(*templateImage,rotationStepSizeDegree);
    }
  }
  
  void TemplateTracker::setTemplateImage(const Img8u &templateImage, 
                                         float rotationStepSizeDegree){
    
    Img8u test = cvt8u(scale(cvt(templateImage),4));

    RotateOp rot;    
    ICLASSERT_RETURN(rotationStepSizeDegree > 0.001);

    const int w = templateImage.getWidth();
    const int h = templateImage.getHeight();
    data->lut.clear();
    
    for(float a=0;a<=360;a+=rotationStepSizeDegree){
      rot.setAngle(a);
      const Img8u *r = rot.apply(&templateImage)->as8u();
      const int rw = r->getWidth();
      const int rh = r->getHeight();
      Rect center((rw-w)/2, (rh-h)/2, w,h);
      SmartPtr<const Img8u> roiimage = r->shallowCopy(center);
      
      Img8u *tmp = new Img8u(test.getSize()/4,1);
      //      data->lut.push_back(roiimage->deepCopyROI());
      roiimage->scaledCopyROI(tmp,interpolateLIN);
      data->lut.push_back(tmp);
    }    
  }

  void TemplateTracker::setRotationLUT(const std::vector<SmartPtr<Img8u> > &lut){
    data->lut = lut;
  }

  void TemplateTracker::showRotationLUT() const{
    ICLASSERT_RETURN(data->lut.size());
    static const int MAX_W = 2000;
    ImgQ row;
    ImgQ all;
    
    const int w = data->lut[0]->getWidth();
    for(unsigned int i=0;i<data->lut.size();++i){
      row = ( row , cvt(data->lut[i].get()) , zeros(1,1,1) );
      if(row.getWidth()+w > MAX_W || i == data->lut.size() -1) {
        all = (all % zeros(1,1,1) % row);
        row = ImgQ();
      }
    }
    show(cvt8u(all));
  }

  
  TemplateTracker::~TemplateTracker(){
    ICL_DELETE(data->buf);
    delete data;
  }
  
  TemplateTracker::Result TemplateTracker::track(const Img8u &image, 
                                                 const Result *initialResult,
                                                 std::vector<Result> *allResults){
    Result last = initialResult ? *initialResult : data->lastResult;
    if(last.pos == Point32f(-1,-1)){
      last.pos = Point(image.getWidth()/2,image.getHeight()/2); 
    }
    
    const double angle = last.angle;
    const int X = last.pos.x;
    const int Y = last.pos.y;
    const int lutSize = (int)data->lut.size();
    const int angleIndex = angle / (2*M_PI) * (lutSize-1);
    const int ROI = getPropertyValue("tracking.position range");
    const float rotationRange = getPropertyValue("tracking.rotation range");
    const int step1 = getPropertyValue("tracking.coarse steps");
    //const int step2 = getPropertyValue("tracking.fine steps");
    //const float angleStepSize = 360./lutSize;

    const Rect roi(X - ROI/2, Y-ROI/2, ROI, ROI);
    
    SmartPtr<const Img8u> roiImageTmp = image.shallowCopy(roi);
    SmartPtr<Img8u> roiImage = roiImageTmp->deepCopyROI();

    const int stepRadius = lutSize * rotationRange/720;

    Result bestResult = last;
    for(int i = -stepRadius; i <= stepRadius; i+= step1){
      int curIndex = angleIndex + i;
      while(curIndex >= lutSize) curIndex -= lutSize;
      while(curIndex < 0) curIndex += lutSize;
      
      data->prox->apply(roiImage.get(),data->lut.at(curIndex).get(),&data->buf);
      
      Point maxPos;
      float maxValue = data->buf->getMax(0,&maxPos);
    
      Result curr(maxPos+last.pos,
                  (float(curIndex)/lutSize) * 2 * M_PI, 
                  maxValue,
                  data->lut.at(curIndex).get());
      if(curr.proximityValue > bestResult.proximityValue){
        bestResult = curr;
      }
      if(allResults) allResults->push_back(curr);
    }
    data->lastResult = bestResult;
    return bestResult;
  }

  REGISTER_CONFIGURABLE_DEFAULT(TemplateTracker);
  
}

