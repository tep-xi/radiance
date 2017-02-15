#ifdef LINUX
    #include <GL/glu.h>
#elif OSX
    #include <OpenGL/glu.h>
#endif

#ifdef LINUX
    #define GLU_ERROR_STRING(e) gluErrorString(e)
#elif OSX
    const char* osxGluErrorString(GLenum error);
    #define GLU_ERROR_STRING(e) osxGluErrorString(e)
#endif
