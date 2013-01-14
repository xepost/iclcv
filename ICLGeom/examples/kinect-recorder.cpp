#include <ICLQt/Common.h>
#include <ICLIO/GenericImageOutput.h>

HBox gui;
GenericGrabber c_in;
GenericGrabber d_in;
GenericImageOutput c_out,d_out,both_out; 


void init(){
  if(pa("-s")){
    c_in.init("kinectc","kinectc=0");
    d_in.init("kinectd","kinectd=0");
    c_out.init("file",*pa("-s")+"-color-######.bicl");
    d_out.init("file",*pa("-s")+"-depth-######.bicl");

    gui << Image().handle("depth").minSize(16,12).label("depth image")
        << Image().handle("color").minSize(16,12).label("color image");
  }else{
    if(!pa("-d") && !pa("-c")) throw ICLException("no input given");
    if(pa("-d")){
      d_in.init(pa("-d"));
      
      gui << Image().handle("depth").minSize(16,12).label("depth image");
    }else if(pa("-do")) throw ICLException("depth output given, but no input!");
    if(pa("-c")){
      c_in.init(pa("-c"));
      gui << Image().handle("color").minSize(16,12).label("color image");
    }else if(pa("-co")) throw ICLException("color output given, but no input!");
    
    if(pa("-do")) d_out.init(pa("-do"));
    if(pa("-co")) c_out.init(pa("-co"));
  }
  
  // Size s = pa("-size");
  //if(!c_in.isNull()) c_in.useDesired(s);
  //if(!d_in.isNull()) d_in.useDesired(s);

  gui << Show();
}


void run(){
  const ImgBase *c=0,*d=0;
  if(!c_in.isNull()) c = c_in.grab();
  if(!d_in.isNull()) d = d_in.grab();
  
  if(!c_out.isNull() && c) c_out.send(c);
  if(!d_out.isNull() && d) d_out.send(d);
  
  if(c) gui["color"] = c;
  if(d) gui["depth"] = d;
}

int main(int n, char **args){
  return ICLApp(n,args,"-depth-input|-d(2) -color-input|-c(2) -size(size=VGA)"
                "-depth-output|-do(2) -color-output|-oc(2) "
                "-simple-io-params|-s(output-file-base-name)",init,run).exec();
}




