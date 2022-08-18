#if defined(_WIN32) || defined(MS_WINDOWS) || defined(_MSC_VER)

#include <windows.h>
#include <libloaderapi.h>
#include <stdlib.h>
#include <stdio.h>
void *get_function(const char *libPath, const char *functionName) {
    HINSTANCE module = LoadLibraryA(libPath);
    if (module == NULL) {
        printf("Could not find shared library %s.\\n", libPath);
        exit(1);
        return NULL;
    }
    void *func = GetProcAddress(module, functionName);
    if (func == NULL) {
        printf("Could not find function %s in %s.\\n", functionName, libPath);
        exit(1);
        return NULL;
    }
    return func;
}


#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))

#include "dlfcn.h"
#include <stdlib.h>
#include <stdio.h>
void *get_function(const char *libPath, const char *functionName) {
    void *module = dlopen(libPath, RTLD_LAZY);
    if (module == NULL) {
        printf("Could not find shared library %s.\\n", libPath);
        exit(1);
        return NULL;
    }
    void *func = dlsym(module, functionName);
    if (func == NULL) {
        printf("Could not find function %s in %s.\\n", functionName, libPath);
        exit(1);
        return NULL;
    }
    return func;
}


#else

#include <stdlib.h>
#include <stdio.h>
void *get_function(const char *libPath, const char *functionName) {
    printf("The dynamic library importer does not support your platform.\\n");
    exit(1);
    return NULL;
}


#endif
