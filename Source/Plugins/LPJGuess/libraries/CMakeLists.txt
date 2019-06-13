set(headers 
  gutil/gutil.h 
  plib/plib.h 
  plib/recursivefilereader.h
  guessnc/guessnc.h
  guessnc/cf.h
  guessnc/cftime.h
  guessnc/cfvariable.h
  )

set(source  
  gutil/gutil.cpp 
  plib/plib.cpp 
  plib/recursivefilereader.cpp
  guessnc/guessnc.cpp
  guessnc/cftime.cpp
  guessnc/cfvariable.cpp
  )

include(add_guess_sources)
