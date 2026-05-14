/*
 * Copyright 1992-2026 AUTHORS.
 * See the legal/LICENSE file for license information and legal/AUTHORS for authors.
 */

# include "_glueDefs.cpp.incl"

// x_includes.hh (via the PCH) renamed X11's Cursor to SelfX11Cursor
// to avoid a clash with Carbon's Cursor struct, then #undef'd the macro.
// Xft.h pulls in Xrender.h which references bare Cursor (and Status on
// macOS), so we temporarily reinstate the macro while including Xft.
# define Cursor SelfX11Cursor
# ifdef __APPLE__
#     define Status int
# endif
# include <X11/Xlib.h>
# include <X11/Xft/Xft.h>
# undef Cursor
# ifdef __APPLE__
#     undef Status
# endif

# include "xft.primMaker.hh"


# define xftTypeSealsDo(template)		\
    template(XGlyphInfo)			\
    template(XRenderColor)			\
    template(XftColor)				\
    template(XftDraw)				\
    template(XftFont)

# define defineXftTypeSeals(stem)		\
    const char* CONC(stem,_seal) = STR(stem);

xftTypeSealsDo(defineXftTypeSeals)


# define xTypeSealsDo(template)			\
    template(Colormap)				\
    template(Display)				\
    template(Visual)

# define declareXTypeSeals(stem)		\
    extern const char* CONC(stem,_seal);

xTypeSealsDo(declareXTypeSeals)



void
XftTextExtents8_wrap(Display *display, XftFont *font,
		     char *string, int len,
		     XGlyphInfo *extents)
{
  FcChar8 *xstr = (FcChar8 *)string;
  XftTextExtents8(display, font, (FcChar8*)string, len, extents);
}


void
XftDrawString8_wrap(XftDraw  *draw,
		    XftColor *color,
		    XftFont  *pub,
		    int       x,
		    int       y,
		    char     *string,
		    int       len)
{
    XftDrawString8(draw, color, pub, x, y, (FcChar8*)string, len);
}


# define WHAT_GLUE FUNCTIONS
# undef  PRIMITIVE_GLUE_FLAG_CODE
# define PRIMITIVE_GLUE_FLAG_CODE BlockGlueFlag gf(xlib_semaphore); // must be right before xft_glue
    xft_glue
# undef WHAT_GLUE
