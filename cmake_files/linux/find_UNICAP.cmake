icl_check_external_package(UNICAP unicap.h "unicap;rt" lib include/unicap HAVE_UNICAP_COND TRUE)
if(HAVE_UNICAP_COND)
  add_definitions(-DUNICAP_FLAGS_NOT_AS_ENUM)
endif()