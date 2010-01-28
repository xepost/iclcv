#ifndef ICL_COMBO_HANDLE_H
#define ICL_COMBO_HANDLE_H

#include <string>
#include <ICLQt/GUIHandle.h>

/** \cond */
class QComboBox;
/** \endcond */

namespace icl{
  
  /// Handle class for combo components \ingroup HANDLES
  class ComboHandle : public GUIHandle<QComboBox>{
    public:
    /// create an empty handle
    ComboHandle(){}

    /// create a new ComboHandle wrapping a given QComboBox
    ComboHandle(QComboBox *cb, GUIWidget *w):GUIHandle<QComboBox>(cb,w){}
    
    /// add an item
    void add(const std::string &item);

    /// remove an item
    void remove(const std::string &item);
    
    /// remove item at given index
    void remove(int idx);
    
    /// void remove all items
    void clear();
    
    /// returns the item at given index
    std::string getItem(int idx) const;

    /// returns the index of a given item
    int getIndex(const std::string &item) const;
    
    /// returns the currently selected index
    int getSelectedIndex() const;
    
    /// returns the currently selected item
    std::string getSelectedItem() const;
    
    /// returns the count of elements
    int getItemCount() const;
    
    /// sets the current index
    void setSelectedIndex(int idx);
    
    /// sets the current item
    void setSelectedItem(const std::string &item);
    
    private:
    /// utility function (internally used)
    QComboBox *cbx() { return **this; } 

    /// utility function (internally used)
    const QComboBox *cbx() const{ return **this; } 
  };
  
}


#endif