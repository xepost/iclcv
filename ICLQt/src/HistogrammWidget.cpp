#include <ICLQt/HistogrammWidget.h>
#include <ICLUtils/Rect32f.h>

#include <QtGui/QPainter>

namespace icl{
  static inline float median_of_5(const float *p){
    float a[5]= {p[0],p[1],p[2],p[3],p[4]};
    std::sort(a,a+5);
    return a[2];
  }
  
  static inline float mean_of_5(float *p){
    return (p[0]+3*p[1]+5*p[2]+3*p[3]+p[4])/13.0f;
  }
  
  HistogrammWidget::HistogrammWidget(QWidget *parent):
    PlotWidget(parent),logOn(false),meanOn(false),medianOn(false),
    selChannel(-1), buf(512){
    
    setPropertyValue("labels.x-precision",0);
    setPropertyValue("labels.y-precision",0);
    setPropertyValue("tics.x-distance",32);
    setPropertyValue("tics.y-distance",1000);
    setPropertyValue("borders.left",50);
  }
    
  void HistogrammWidget::setFeatures(bool logOn, bool meanOn, bool medianOn, int selChannel){
    this->logOn = logOn;
    this->meanOn = meanOn;
    this->medianOn = medianOn;
    this->selChannel = selChannel;
  }

  
  
  void HistogrammWidget::updateData(const ImageStatistics &s){
    lock();
    clear();
    if(!s.isNull){
      float maxY = 0;
      float minX = 0, maxX = 255; // TODO this must be adapted dynamically
                                  // the histo contians 256 bins between
                                  // minVal and maxVal, but what if
                                  // the min- and max vals are not identical in
                                  // all channels ??
      
      bool haveData = false;
      for(size_t i=0;i<s.histos.size();++i){
        float *buf1 = buf.data();
        float *buf2 = buf1 + 256;
        
        if(selChannel != -1 && selChannel != int(i)) continue;
        haveData = true;
        
        label("channel " + str(i));
        switch(i){
          case 0: color(255,0,0); fill(255,0,0,100); break;
          case 1: color(0,255,0); fill(0,255,0,100); break;
          case 2: color(0,0,255); fill(0,0,255,100); break;
          default: color(100,100,100); fill(100,100,100,100); break;
        }
        std::copy(s.histos[i].begin(), s.histos[i].end(), buf1);
        if(logOn){
          for(size_t j=0;j<256;++j){
            buf1[j] = buf1[j]>1 ? log(buf1[j]) : 0;
          }
        }
        if(meanOn){
          buf2[0] = buf1[0];
          buf2[1] = buf1[1];
          buf2[254] = buf1[254];
          buf2[255] = buf1[255];
          
          for(size_t j=2;j<254;++j){
            buf2[j] = mean_of_5(&buf1[j-2]);
          }
          std::swap(buf1,buf2);
        }
        if(medianOn){
          buf2[0] = buf1[0];
          buf2[1] = buf1[1];
          buf2[254] = buf1[254];
          buf2[255] = buf1[255];

          for(size_t j=2;j<254;++j){
            buf2[j] = median_of_5(&buf1[j-2]);
          }
          std::swap(buf1, buf2);
        }
        maxY = iclMax(*std::max_element(buf1, buf1+256), maxY);
        series(buf1, 256);
      }

      if(haveData){
        setPropertyValue("tics.y-distance",
                         logOn ? 2 : 
                         maxY > 100 ? int(maxY/100)*20 :
                         20);
        setPropertyValue("labels.y-axis",logOn ? "log(number of pixels)" : "number of pixels");
        setDataViewPort(Range32f(minX,maxX), Range32f(0,maxY));
      }
    }    
    unlock();
    updateFromOtherThread();
  }
}
