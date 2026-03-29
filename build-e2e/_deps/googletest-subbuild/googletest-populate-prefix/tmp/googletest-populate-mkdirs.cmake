# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-src"
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-build"
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix"
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix/tmp"
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix/src"
  "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/d/workspace/pipeleap/llm-gateway/build-e2e/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
