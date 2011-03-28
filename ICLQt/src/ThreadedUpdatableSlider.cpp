/********************************************************************
**                Image Component Library (ICL)                    **
**                                                                 **
** Copyright (C) 2006-2010 CITEC, University of Bielefeld          **
**                         Neuroinformatics Group                  **
** Website: www.iclcv.org and                                      **
**          http://opensource.cit-ec.de/projects/icl               **
**                                                                 **
** File   : ICLQt/src/ThreadedUpdatableSlider.cpp                  **
** Module : ICLQt                                                  **
** Authors: Christof Elbrechter                                    **
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

#include <ICLQt/ThreadedUpdatableSlider.h>
#include <ICLUtils/Exception.h>
#include <ICLUtils/StringUtils.h>

namespace icl{

  ThreadedUpdatableSlider::ThreadedUpdatableSlider(QWidget *parent): QSlider(parent){
    QObject::connect(this,SIGNAL(valueChanged(int)),this,SLOT(collectValueChanged(int)));
    QObject::connect(this,SIGNAL(sliderMoved(int)),this,SLOT(collectSliderMoved(int)));
    QObject::connect(this,SIGNAL(sliderPressed()),this,SLOT(collectSliderPressed()));
    QObject::connect(this,SIGNAL(sliderReleased()),this,SLOT(collectSliderReleased()));
  }

  ThreadedUpdatableSlider::ThreadedUpdatableSlider(Qt::Orientation o, QWidget *parent): QSlider(o, parent){
    QObject::connect(this,SIGNAL(valueChanged(int)),this,SLOT(collectValueChanged(int)));
    QObject::connect(this,SIGNAL(sliderMoved(int)),this,SLOT(collectSliderMoved(int)));
    QObject::connect(this,SIGNAL(sliderPressed()),this,SLOT(collectSliderPressed()));
    QObject::connect(this,SIGNAL(sliderReleased()),this,SLOT(collectSliderReleased()));
  }

  void ThreadedUpdatableSlider::registerCallback(const Function<void> &cb, const std::string &eventList){
    std::vector<std::string> ts = tok(eventList,",");
    for(unsigned int i=0;i<ts.size();++i){
      const std::string &events = ts[i];
      CB c = { events == "all" ? CB::all :
               events == "move" ? CB::move :
               events == "press" ? CB::press :
               events == "release" ? CB::release :
               events == "value" ? CB::value : (CB::Event)-1,
               cb };
      if(c.event < 0) throw ICLException("ThreadedUpdatableSlider::registerCallback: invalid event type " + events);
      callbacks.push_back(c);
    }
  }

  void ThreadedUpdatableSlider::removeCallbacks(){
    callbacks.clear();
  }
  
  void ThreadedUpdatableSlider::collectValueChanged(int){
    for(unsigned int i=0;i<callbacks.size();++i){
      if(callbacks[i].event == CB::all || callbacks[i].event == CB::value) callbacks[i].f();
    }
  }

  void ThreadedUpdatableSlider::collectSliderPressed(){
    for(unsigned int i=0;i<callbacks.size();++i){
      if(callbacks[i].event == CB::all || callbacks[i].event == CB::press) callbacks[i].f();
    }  
  }

  void ThreadedUpdatableSlider::collectSliderMoved(int){
    for(unsigned int i=0;i<callbacks.size();++i){
      if(callbacks[i].event == CB::all || callbacks[i].event == CB::move) callbacks[i].f();
    }  
  }

  void ThreadedUpdatableSlider::collectSliderReleased(){
    for(unsigned int i=0;i<callbacks.size();++i){
      if(callbacks[i].event == CB::all || callbacks[i].event == CB::release) callbacks[i].f();
    }  
  }
    
}
