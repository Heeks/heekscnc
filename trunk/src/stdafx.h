// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma warning(disable : 4996)

#include "windows.h"

#include <list>
#include <vector>
#include <map>
#include <set>

extern "C" {
#include <GL/gl.h>
//#include <GL/glx.h>
#ifdef WIN32
#include <GL/glu.h>
#else
#include <GL/glut.h>
#endif
}

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <wx/splitter.h>
#ifdef WIN32
#include <wx/msw/regconf.h>
#endif
#include <wx/aui/aui.h>

#include "geometry/geometry.h"
using namespace geoff_geometry;

#include "HeeksCNC.h"

// TODO: reference additional headers your program requires here
