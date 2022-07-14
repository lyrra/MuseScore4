if (APPLE)
      file(GLOB_RECURSE INCS "*.h")
else (APPLE)
      set(INCS "")
endif (APPLE)

add_library (
      beatroot STATIC
      Induction.cpp
      BeatTracker.cpp
      AgentList.cpp
      Agent.cpp
  )

if (NOT MSVC)
   set_target_properties( beatroot
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wextra -Winvalid-pch"
      )
else (NOT MSVC)
   set_target_properties ( beatroot
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE} /wd4267"
      )
endif (NOT MSVC)   

if (APPLE)
      xcode_pch(beatroot all)
endif (APPLE)

# Use MSVC pre-compiled headers
if (MSVC)
      vstudio_pch( beatroot )
endif (MSVC)

# MSVC does not depend on mops1 & mops2 for PCH
if (NOT MSVC)
   ADD_DEPENDENCIES(beatroot mops1)
   ADD_DEPENDENCIES(beatroot mops2)
endif (NOT MSVC)

