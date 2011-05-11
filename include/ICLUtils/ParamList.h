/********************************************************************
**                Image Component Library (ICL)                    **
**                                                                 **
** Copyright (C) 2006-2010 CITEC, University of Bielefeld          **
**                         Neuroinformatics Group                  **
** Website: www.iclcv.org and                                      **
**          http://opensource.cit-ec.de/projects/icl               **
**                                                                 **
** File   : include/ICLUtils/ParamList.h                           **
** Module : ICLUtils                                               **
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

#ifndef ICL_PARAM_LIST_H
#define ICL_PARAM_LIST_H

#include <ICLUtils/Any.h>
#include <map>

namespace icl{

  /// Utility structure that utilizes an std::map as parameter list
  /** The ParamList is supposed to be used in function interfaces
      for passing several parameters as one argument. Usually this
      should only be used where efficiency is not compulsory */
  struct ParamList : public std::map<std::string,Any>{

    /// The Key type (std::string)
    typedef std::string Key;
    
    /// The Value type (icl::Any)
    typedef Any Value;
    
    /// Constructor, that can get up to 10 key-value pairs
    /** zero-length keys are skipped! */
    inline ParamList(const Key &key0="", const Value &value0="",
                     const Key &key1="", const Value &value1="",
                     const Key &key2="", const Value &value2="",
                     const Key &key3="", const Value &value3="",
                     const Key &key4="", const Value &value4="",
                     const Key &key5="", const Value &value5="",
                     const Key &key6="", const Value &value6="",
                     const Key &key7="", const Value &value7="",
                     const Key &key8="", const Value &value8="",
                     const Key &key9="", const Value &value9="" ){
      if(key0.length()) operator[](key0) = value0;
      if(key1.length()) operator[](key1) = value1;
      if(key2.length()) operator[](key2) = value2;
      if(key3.length()) operator[](key3) = value3;
      if(key4.length()) operator[](key4) = value4;
      if(key5.length()) operator[](key5) = value5;
      if(key6.length()) operator[](key6) = value6;
      if(key7.length()) operator[](key7) = value7;
      if(key8.length()) operator[](key8) = value8;
      if(key9.length()) operator[](key9) = value9;
    }
    
    /// returns whether the map contains the given key
    /** This is just a shortcut to the find()-method that is provided by the std::map */
    inline bool hasKey(const Key &key) const { return find(key) != end(); }

    /// removes the given key from the map
    /** This is just a shortcut to the find()-method that is provided by the std::map */
    inline void removeKey(const Key &key){
      iterator it = find(key);
      if(it != end()) erase(it);
    }
    
    /// extension for the unconst operator that is provided by the std::map class
    inline const Any &operator[](const Key &key) const throw (ICLException){
      const_iterator it = find(key);
      if(it == end()) throw ICLException("error in ParamList::operator[](key) with key=" + key + ": key not found!");
      return it->second;
    }

    /// also provides the map's not-const index operator
    std::map<std::string,Any>::operator[];
  };

  
}

#endif