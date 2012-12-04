/********************************************************************
**                Image Component Library (ICL)                    **
**                                                                 **
** Copyright (C) 2006-2012 CITEC, University of Bielefeld          **
**                         Neuroinformatics Group                  **
** Website: www.iclcv.org and                                      **
**          http://opensource.cit-ec.de/projects/icl               **
**                                                                 **
** File   : ICLGeom/src/PointCloudObjectBase.cpp                   **
** Module : ICLGeom                                                **
** Authors: Christof Elbrechter, Patrick Nobou                     **
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

#include <ICLGeom/PointCloudObjectBase.h>
#include <ICLQt/GLFragmentShader.h>
#include <ICLUtils/StringUtils.h>

#ifdef ICL_SYSTEM_APPLE
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glew.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

using namespace icl::utils;
using namespace icl::math;
using namespace icl::core;
using namespace icl::qt;

namespace icl{
  namespace geom{
  
    void PointCloudObjectBase::customRender(){
      drawNormalLines();
      
      if(!supports(XYZ)) return;
  
      const DataSegment<float,3> xyz = selectXYZ(); 
  
      glDisable(GL_LIGHTING);
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(3, GL_FLOAT, xyz.stride, xyz.data);
      
      size_t numElements = xyz.numElements;
  
      static GLFragmentShader swapRB( "void main(){\n"
                                      "  gl_FragColor = vec4(gl_Color[2],gl_Color[1],gl_Color[0],gl_Color[3]);\n"
                                      "}\n");
  
      if(supports(RGBA32f)){
        const DataSegment<icl32f,4> rgba = selectRGBA32f(); 
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, rgba.stride, rgba.data);
        numElements = iclMin(numElements,rgba.numElements);
      }else if(supports(BGR)){
        const DataSegment<icl8u,3> bgr = selectBGR(); 
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_UNSIGNED_BYTE, bgr.stride, bgr.data);
        numElements = iclMin(numElements,bgr.numElements);
        swapRB.activate();
      }else if(supports(BGRA)){
        const DataSegment<icl8u,4> bgra = selectBGRA(); 
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, bgra.stride, bgra.data);
        numElements = iclMin(numElements,bgra.numElements);
        swapRB.activate();
      }
  #if 0
      else if(supports(Intensity)){
        const DataSegment<float,1> rgba = selectIntensity(); 
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_UNSIGNED_BYTE, rgb.padding, rgb.data);
        numElements = iclMin(numElements,rgba.numElements);
      }
  #endif
      else{
        glColor3fv(m_defaultPointColor.data());
  
      }
      
      glDrawArrays(GL_POINTS, 0, numElements);
  
      swapRB.deactivate();
  
  
      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_COLOR_ARRAY);
      glEnable(GL_LIGHTING);
    }
    
    void PointCloudObjectBase::setUseDrawNormalLines(float lineLength, int granularity){
      useDrawNormalLines=true;
      normalLineLength=lineLength;
      normalLineGranularity=granularity;
    }
    
    void PointCloudObjectBase::drawNormalLines(){
      if(!supports(XYZ) || !supports(Normal)) return;
      
      if(useDrawNormalLines){
        const DataSegment<float,3> xyz = selectXYZ();
        const DataSegment<float,4> normal = selectNormal();
        size_t numElementsXYZ = xyz.numElements;
        size_t numElementsNormal = normal.numElements;
        utils::Size size = getSize();
        glBegin(GL_LINES);
          glColor4f(1.0f,1.0f,1.0f,1.0f);
          for(int y=0; y<size.height; y+=normalLineGranularity){
      	    for(int x=0; x<size.width; x+=normalLineGranularity){
      	      int i=x+size.width*y;
      	      glVertex3f(xyz[i][0],xyz[i][1],xyz[i][2]);
      	      glVertex3f(xyz[i][0]+normalLineLength*normal[i][0], xyz[i][1]+normalLineLength*normal[i][1], xyz[i][2]+normalLineLength*normal[i][2]); 
      	    }
      	  }
      	glEnd();
      }
      useDrawNormalLines=false;
    }
    
    
    
    std::map<std::string,std::string> & PointCloudObjectBase::getMetaData(){
        return m_metaData;
    }
    
    const std::map<std::string,std::string> & PointCloudObjectBase::getMetaData() const{
      return m_metaData;
    }
    
    const std::string & PointCloudObjectBase::getMetaData(const std::string &key) const throw (ICLException){
      std::map<std::string,std::string>::const_iterator it = m_metaData.find(key);
      if(it == m_metaData.end()) throw ICLException("PointCloudObjectBase::getMetaData(key): no meta data with given key ("
                                                     + key + ") was associated with this point cloud");
      return it->second;
    }
    
    
    bool PointCloudObjectBase::hasMetaData(const std::string &key) const{
      return m_metaData.find(key) != m_metaData.end();
    }
    
    
    bool PointCloudObjectBase::hasAnyMetaData() const{
      return m_metaData.size();
    }
      
    void PointCloudObjectBase::setMetaData(const std::string &key, const std::string &value){
      m_metaData[key] = value;
    }
    
    void PointCloudObjectBase::clearAllMetaData(){
      m_metaData.clear();
    }
    
    void PointCloudObjectBase::clearMetaData(const std::string &key){
      std::map<std::string,std::string>::iterator it = m_metaData.find(key);
      if(it != m_metaData.end()){
        m_metaData.erase(it);
      }
    }
    
    std::vector<std::string> PointCloudObjectBase::getAllMetaDataEntries() const{
      std::vector<std::string> all(m_metaData.size());
      int i = 0;
      for(std::map<std::string,std::string>::const_iterator it = m_metaData.begin();
          it != m_metaData.end(); ++it){
        all[i++] = it->first;
      }
      return all;
    }
  
    std::ostream &operator<<(std::ostream &s, const PointCloudObjectBase::FeatureType t){
      static std::string names[PointCloudObjectBase::NUM_FEATURES] = {
        "Intensity","Label", "BGR", "BGRA", "BGRA32s", "XYZ", "Normal", "RGBA32f"
      };
      if( (unsigned int)t < (int)PointCloudObjectBase::NUM_FEATURES){
        return s << names[(int)t];
      }else{
        return s << "*unknown feature type*";
      }
    }
    
    
    namespace{
      template<class T> T create_opaque_color() { return 255; }
      template<> float create_opaque_color<float>() { return 1.0f; }
      template<> double create_opaque_color<double>() { return 1.0; }
      
      
      template<class S, class D>
      struct ConvertColor{
        static D cc(const S &s){   return s;  }
      };
      
      template<class S>
      struct ConvertColor<S,icl32f>{
        static icl32f cc(const S &s){   return icl32f(s)/255;  }
      };
      
      template<class S>
      struct ConvertColor<S,icl64f>{
        static icl64f cc(const S &s){   return icl64f(s)/255;  }
      };
      
      template<class S, class D>
      D ccc(const S &s){ return ConvertColor<S,D>::cc(s); }
      
      
      template<class SRC_T, class DST_T, int SRC_C, int DST_C>
      void assign_point_cloud_colors(const int dim, const SRC_T *src[SRC_C], DataSegment<DST_T,DST_C> dst){
        switch(SRC_C){
          case 1:{
            const SRC_T *s = src[0];
            for(int i=0;i<dim;++i){
              DST_T sval = ccc<SRC_T,DST_T>(s[i]);
              dst[i][0] = sval;
              dst[i][1] = sval;
              dst[i][2] = sval;
              if(DST_C == 4) dst[i][3] = create_opaque_color<DST_T>();
            }
            break;
          }
          case 3:{
            const SRC_T *r = src[0], *g = src[1], *b = src[2];
            for(int i=0;i<dim;++i){
              dst[i][0] = ccc<SRC_T,DST_T>(r[i]);
              dst[i][1] = ccc<SRC_T,DST_T>(g[i]);
              dst[i][2] = ccc<SRC_T,DST_T>(b[i]);
              if(DST_C == 4) dst[i][3] = create_opaque_color<DST_T>();
            }
            break;
          }
          case 4:{
            const SRC_T *r = src[0], *g = src[1], *b = src[2], *a = src[3];
            for(int i=0;i<dim;++i){
              dst[i][0] = ccc<SRC_T,DST_T>(r[i]);
              dst[i][1] = ccc<SRC_T,DST_T>(g[i]);
              dst[i][2] = ccc<SRC_T,DST_T>(b[i]);
              if(DST_C == 4) dst[i][3] = ccc<SRC_T,DST_T>(a[i]); // otherwise color is skipped
            }
            break;
          }
        }
      }
      
      
      template<class T, int SRC_C>
      void set_color_from_image(PointCloudObjectBase &pc,const Img<T> &image){
        const T *src[SRC_C];
        int dim = image.getDim();
        for(int i=0;i<SRC_C;++i) src[i] = image.begin(i);
        if(pc.supports(PointCloudObjectBase::BGR)){
          std::swap(src[0],src[2]);
          assign_point_cloud_colors<T,icl8u,SRC_C,3>(dim, src, pc.selectBGR());
        }else if(pc.supports(PointCloudObjectBase::BGRA)){
          std::swap(src[0],src[2]);
          assign_point_cloud_colors<T,icl8u,SRC_C,4>(dim, src, pc.selectBGRA());
        }else if(pc.supports(PointCloudObjectBase::RGBA32f)){
          assign_point_cloud_colors<T,float,SRC_C,4>(dim, src, pc.selectRGBA32f());
        }
      }

    } // end of anonymous namespace

    void PointCloudObjectBase::setColorsFromImage(const ImgBase &image) throw (ICLException){
      ICLASSERT_THROW(image.getSize() == getSize(), 
                      ICLException("PointCloudObjectBase::setColorsFromImage: "
                                   "image size and point cloud size differ!"));
      int c = image.getChannels();
      ICLASSERT_THROW(c==1 || c==3 || c==4,
                      ICLException("PointCloudObjectBase::setColorsFromImage: "
                                   "image must have 1,3 or 4 channels"));
      
      switch(image.getDepth()){
#define ICL_INSTANTIATE_DEPTH(D)                                        \
        case depth##D:                                                  \
          if(c==1) set_color_from_image<icl##D,1>(*this,*image.asImg<icl##D>()); \
          else if(c == 3)set_color_from_image<icl##D,3>(*this,*image.asImg<icl##D>()); \
          else set_color_from_image<icl##D,4>(*this,*image.asImg<icl##D>()); \
          break;
        ICL_INSTANTIATE_ALL_DEPTHS
#undef ICL_INSTANTIATE_DEPTH
      }
    }

    void PointCloudObjectBase::setDefaultVertexColor(const GeomColor &color){
      m_defaultPointColor = color/255;
    }
  

    void PointCloudObjectBase::deepCopy(PointCloudObjectBase &dst) const{
      dst.setSize(getSize());
      const int dim = getDim();

      bool intensity = supports(Intensity) && dst.supports(Intensity);
      bool label = supports(Label) && dst.supports(Label);
      bool color = ( (supports(BGR) || supports(BGRA) || supports(BGRA32s) || supports(RGBA32f) ) &&
                     (dst.supports(BGR) || dst.supports(BGRA) || dst.supports(BGRA32s) || dst.supports(RGBA32f) ) );
      bool xyz = ( supports(XYZ) || supports(XYZH) ) && ( dst.supports(XYZ) || dst.supports(XYZH) );
      bool normal = supports(Normal) && dst.supports(Normal);
      
      /// simple version: copy element wise!
      if(intensity) selectIntensity().deepCopy(dst.selectIntensity());
      if(label) selectLabel().deepCopy(dst.selectLabel());
      if(normal) selectNormal().deepCopy(dst.selectNormal());
      
      if(xyz){
        if(supports(XYZH) && dst.supports(XYZH)) selectXYZH().deepCopy(dst.selectXYZH());
        else selectXYZ().deepCopy(dst.selectXYZ());
      }
      
      if(color){
        if(supports(BGRA) && dst.supports(BGRA)) selectBGRA().deepCopy(dst.selectBGRA());
        else if(supports(BGR) && dst.supports(BGR)) selectBGR().deepCopy(dst.selectBGR());
        else if(supports(BGRA32s) && dst.supports(BGRA32s)) selectBGRA32s().deepCopy(dst.selectBGRA32s());
        else if(supports(RGBA32f) && dst.supports(RGBA32f)) selectRGBA32f().deepCopy(dst.selectRGBA32f());
        else{
          const float f = 1./255;

          if(supports(BGRA)){
            const DataSegment<icl8u,4> s = selectBGRA();
            if(dst.supports(BGR)){
              DataSegment<icl8u,3> d = dst.selectBGR();
              for(int i=0;i<dim;++i) d[i] = FixedColVector<icl8u,3>(s[i][0],s[i][1],s[i][2]);
            }else if(dst.supports(RGBA32f)){
              DataSegment<float,4> d = dst.selectRGBA32f();
              for(int i=0;i<dim;++i) d[i] = FixedColVector<float,4>(s[i][2]*f,s[i][1]*f,s[i][0]*f,s[i][3]*f); //??
            }
          }else if(supports(BGR)){
            const DataSegment<icl8u,3> s = selectBGR();
            if(dst.supports(BGRA)){
              DataSegment<icl8u,4> d = dst.selectBGRA();
              for(int i=0;i<dim;++i) d[i] = FixedColVector<icl8u,4>(s[i][0],s[i][1],s[i][2],255);
            }else if(dst.supports(RGBA32f)){
              DataSegment<float,4> d = dst.selectRGBA32f();
              for(int i=0;i<dim;++i) d[i] = FixedColVector<float,4>(s[i][2]*f,s[i][1]*f,s[i][0]*f,1);
            }
          }else if(supports(RGBA32f)){
            const DataSegment<float,4> s = selectRGBA32f();
            if(dst.supports(BGRA)){
              DataSegment<icl8u,4> d = dst.selectBGRA();
              for(int i=0;i<dim;++i) d[i] = FixedColVector<icl8u,4>(s[i][2]*255,s[i][1]*255,s[i][0]*255,s[i][3]*255);
            }else if(dst.supports(BGR)){
              DataSegment<icl8u,3> d = dst.selectBGR();
              for(int i=0;i<dim;++i) d[i] = FixedColVector<icl8u,3>(s[i][2]*255,s[i][1]*255,s[i][0]*255);
            }
          }else if(supports(BGRA32s) || dst.supports(BGRA32s)){
            throw ICLException("uncommon point cloud type that supports BGRA32s but not BGRA cannot be copied deeply!");
          }
        }
      }

    }

#define SAY(X) std::cout << "equals: " << #X << " differs!" << std::endl;
#define SAY2(X) std::cout << "equals: " << X << " differs!" << std::endl;

    bool PointCloudObjectBase::equals(const PointCloudObjectBase &dst, 
                                      bool compareOnlySharedFeatures,
                                      bool allowDifferentColorTypes,
                                      float tollerance) const{
      if(getDim() != dst.getDim()){ SAY(dim);  return false; }
      if(isOrganized() != dst.isOrganized()) { SAY(isOrganized); return false; } 
      if(isOrganized() && getSize() != dst.getSize()) { SAY(size); return false; }
      
      if(!compareOnlySharedFeatures){
        for(int i=0;i<NUM_FEATURES;++i){
          FeatureType t = (FeatureType)i;
          if(allowDifferentColorTypes){
            if(t != BGR && t != BGRA && t != BGRA32s && t != RGBA32f){
              if(supports(t) != dst.supports(t)) {
                SAY2( ("support of feature type (" + str(i) + ")") );
                return false;
              }
            }
          }else{
            if(supports(t) != dst.supports(t)) {
              SAY2( ("support of feature type (" + str(i) + ")"));
              return false;
            }
          }
        }        
      }
      if(allowDifferentColorTypes && !compareOnlySharedFeatures){
        if( (supports(BGR) || supports(BGRA) || supports(BGRA32s) || supports(RGBA32f)) !=
            (dst.supports(BGR) || dst.supports(BGRA) || dst.supports(BGRA32s) || dst.supports(RGBA32f)) ){
          SAY(color_support);
          return false;
        }
      }
      
      if(supports(XYZH) && dst.supports(XYZH)){
        if(!selectXYZH().equals(dst.selectXYZH(),tollerance)){
          SAY(xyzh);
          return false;
        }
      }else if(supports(XYZ) && dst.supports(XYZ)){
        if(!selectXYZ().equals(dst.selectXYZ(),tollerance)){
          SAY(xyz);
          return false;
        }
      }
      
      if(supports(Label) && dst.supports(Label)){
        if(!selectLabel().equals(dst.selectLabel(),tollerance)){
          SAY(label);
          return false;
        }
      }

      if(supports(Intensity) && dst.supports(Intensity)){
        if(!selectIntensity().equals(dst.selectIntensity(),tollerance)) {
          SAY(intensity);
          return false;
        }
      }

      if(supports(Normal) && dst.supports(Normal)){
        if(!selectNormal().equals(dst.selectNormal(),tollerance)){
          SAY(normal);
          return false;
        }
      }

      if(!compareOnlySharedFeatures){
        if(supports(BGRA) && dst.supports(BGRA)){
          if(!selectBGRA().equals(dst.selectBGRA(),tollerance)) {
            SAY(BGRA);
            return false;
          }
        }
        else if(supports(BGR) && dst.supports(BGR)){
          if(!selectBGR().equals(dst.selectBGR(),tollerance)) {
            SAY(BGR);
            return false;
          }
        }
        else if(supports(RGBA32f) && dst.supports(RGBA32f)){
          if(!selectRGBA32f().equals(dst.selectRGBA32f(),tollerance)) {
            SAY(RGBA32f);
            return false;
          }
        }
      }else{ // x-color-compare
        if(supports(BGRA)){
          if(dst.supports(BGRA)){
            if(!selectBGRA().equals(dst.selectBGRA(),tollerance)) return false;
          }else if(dst.supports(BGR)){
            const DataSegment<icl8u,4> a = selectBGRA();
            const DataSegment<icl8u,3> b = dst.selectBGR();
            for(int i=0;i<a.getDim();++i){
              if(fabs(a[i][0]-b[i][0]) > tollerance) return false;
              if(fabs(a[i][1]-b[i][1]) > tollerance) return false;
              if(fabs(a[i][2]-b[i][2]) > tollerance) return false;
            }
          }else if(dst.supports(RGBA32f)){
            const DataSegment<icl8u,4> a = selectBGRA();
            const DataSegment<float,4> b = dst.selectRGBA32f();
            for(int i=0;i<a.getDim();++i){
              if(fabs(a[i][2]-b[i][0]*255) > tollerance) return false;
              if(fabs(a[i][1]-b[i][1]*255) > tollerance) return false;
              if(fabs(a[i][0]-b[i][2]*255) > tollerance) return false;
              if(fabs(a[i][3]-b[i][3]*255) > tollerance) return false;
            }
          }
        }else if(supports(BGR)){
          if(dst.supports(BGRA)){ 
            const DataSegment<icl8u,4> a = dst.selectBGRA();
            const DataSegment<icl8u,3> b = selectBGR();
            for(int i=0;i<a.getDim();++i){
              if(fabs(a[i][0]-b[i][0]) > tollerance) return false;
              if(fabs(a[i][1]-b[i][1]) > tollerance) return false;
              if(fabs(a[i][2]-b[i][2]) > tollerance) return false;
            }
          }else if(dst.supports(BGR)){
            if(!selectBGR().equals(dst.selectBGR(),tollerance)) return false;
          }else if(dst.supports(RGBA32f)){
            const DataSegment<icl8u,3> a = selectBGR();
            const DataSegment<float,4> b = dst.selectRGBA32f();
            for(int i=0;i<a.getDim();++i){
              if(fabs(a[i][2]-b[i][0]*255) > tollerance) return false;
              if(fabs(a[i][1]-b[i][1]*255) > tollerance) return false;
              if(fabs(a[i][0]-b[i][2]*255) > tollerance) return false;
            }
          }
        }else if(supports(RGBA32f)){
          if(dst.supports(BGR)){
            const DataSegment<icl8u,3> a = dst.selectBGR();
            const DataSegment<float,4> b = selectRGBA32f();
            for(int i=0;i<a.getDim();++i){
              if(fabs(a[i][2]-b[i][0]*255) > tollerance ||
                 fabs(a[i][1]-b[i][1]*255) > tollerance ||
                 fabs(a[i][0]-b[i][2]*255) > tollerance ) return false;
            }
          }else if(dst.supports(BGRA)){
            const DataSegment<icl8u,4> a = dst.selectBGRA();
            const DataSegment<float,4> b = selectRGBA32f();
            for(int i=0;i<a.getDim();++i){
              if(fabs(a[i][2]-b[i][0]*255) > tollerance) return false;
              if(fabs(a[i][1]-b[i][1]*255) > tollerance) return false;
              if(fabs(a[i][0]-b[i][2]*255) > tollerance) return false;
              if(fabs(a[i][3]-b[i][3]*255) > tollerance) return false;
            }
          }else if(dst.supports(RGBA32f)){
            if(!dst.selectRGBA32f().equals(dst.selectRGBA32f(), tollerance)) return false;
          }
        }
      }
      return true;
    
    }

  } // namespace geom
}
