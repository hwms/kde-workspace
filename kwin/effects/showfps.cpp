/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2006 Lubos Lunak <l.lunak@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "showfps.h"

#include <config-X11.h>

#include <kconfiggroup.h>
#include <kglobal.h>
#include <ksharedconfig.h>

#ifdef HAVE_OPENGL
#include <GL/gl.h>
#endif
#ifdef HAVE_XRENDER
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#endif

#include <math.h>


namespace KWin
{

KWIN_EFFECT( showfps, ShowFpsEffect )

const int FPS_WIDTH = 10;
const int MAX_TIME = 100;

ShowFpsEffect::ShowFpsEffect()
    : paints_pos( 0 )
    , frames_pos( 0 )
    {
    for( int i = 0;
         i < NUM_PAINTS;
         ++i )
        {
        paints[ i ] = 0;
        paint_size[ i ] = 0;
        }
    for( int i = 0;
         i < MAX_FPS;
         ++i )
        frames[ i ] = 0;
    KConfigGroup config( KGlobal::config(), "EffectShowFps" );
    alpha = config.readEntry( "Alpha", 0.5 );
    x = config.readEntry( "X", -10000 );
    y = config.readEntry( "Y", 0 );
    if( x == -10000 ) // there's no -0 :(
        x = displayWidth() - 2*NUM_PAINTS - FPS_WIDTH;
    else if ( x < 0 )
        x = displayWidth() - 2*NUM_PAINTS - FPS_WIDTH - x;
    if( y == -10000 )
        y = displayHeight() - MAX_TIME;
    else if ( y < 0 )
        y = displayHeight() - MAX_TIME - y;
    fps_rect = QRect( x, y, FPS_WIDTH + 2*NUM_PAINTS, MAX_TIME );
    }

void ShowFpsEffect::prePaintScreen( ScreenPrePaintData& data, int time )
    {
    if( time == 0 ) {
        // TODO optimized away
    }
    t.start();
    frames[ frames_pos ] = t.minute() * 60000 + t.second() * 1000 + t.msec();
    if( ++frames_pos == MAX_FPS )
        frames_pos = 0;
    effects->prePaintScreen( data, time );
    data.paint += fps_rect;

    paint_size[ paints_pos ] = 0;
    }

void ShowFpsEffect::paintWindow( EffectWindow* w, int mask, QRegion region, WindowPaintData& data )
    {
    effects->paintWindow( w, mask, region, data );

    // Take intersection of region and actual window's rect, minus the fps area
    //  (since we keep repainting it) and count the pixels.
    QRegion r2 = region & QRect( w->x(), w->y(), w->width(), w->height() );
    r2 -= fps_rect;
    int winsize = 0;
    foreach( const QRect& r, r2.rects())
        winsize += r.width() * r.height();
    paint_size[ paints_pos ] += winsize;
    }

void ShowFpsEffect::paintScreen( int mask, QRegion region, ScreenPaintData& data )
    {
    effects->paintScreen( mask, region, data );
    int fps = 0;
    for( int i = 0;
         i < MAX_FPS;
         ++i )
        if( abs( t.minute() * 60000 + t.second() * 1000 + t.msec() - frames[ i ] ) < 1000 )
            ++fps; // count all frames in the last second
    if( fps > MAX_TIME )
        fps = MAX_TIME; // keep it the same height
#ifdef HAVE_OPENGL
    if( effects->compositingType() == OpenGLCompositing)
        {
        paintGL( fps );
        glFinish(); // make sure all rendering is done
        }
#endif
#ifdef HAVE_XRENDER
    if( effects->compositingType() == XRenderCompositing)
        {
        paintXrender( fps );
        XSync( display(), False ); // make sure all rendering is done
        }
#endif
    }

void ShowFpsEffect::paintGL( int fps )
    {
#ifdef HAVE_OPENGL
    int x = this->x;
    int y = this->y;
    glPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    // TODO painting first the background white and then the contents
    // means that the contents also blend with the background, I guess
    glColor4f( 1, 1, 1, alpha ); // white
    glBegin( GL_QUADS );
    glVertex2i( x, y );
    glVertex2i( x + 2*NUM_PAINTS + FPS_WIDTH, y );
    glVertex2i( x + 2*NUM_PAINTS + FPS_WIDTH, y + MAX_TIME );
    glVertex2i( x, y + MAX_TIME );
    glEnd();
    y += MAX_TIME; // paint up from the bottom
    glBegin( GL_QUADS );
    glColor4f( 0, 0, 1, alpha ); // blue
    glVertex2i( x, y );
    glVertex2i( x + FPS_WIDTH, y );
    glVertex2i( x + FPS_WIDTH, y - fps );
    glVertex2i( x, y - fps );
    glEnd();


    glColor4f( 0, 0, 0, alpha ); // black
    glBegin( GL_LINES );
    for( int i = 10;
         i < MAX_TIME;
         i += 10 )
    {
        glVertex2i( x, y - i );
        glVertex2i( x + FPS_WIDTH, y - i );
    }
    glEnd();
    x += FPS_WIDTH;

    // Paint FPS graph
    paintFPSGraph( x, y );
    x += NUM_PAINTS;

    // Paint amount of rendered pixels graph
    paintDrawSizeGraph( x, y );

    // Paint paint sizes
    glPopAttrib();
#endif
    }

/*
 Differences between OpenGL and XRender:
 - differently specified rectangles (X: width/height, O: x2,y2)
 - XRender uses pre-multiplied alpha
*/
void ShowFpsEffect::paintXrender( int fps )
    {
#ifdef HAVE_XRENDER
    Pixmap pixmap = XCreatePixmap( display(), rootWindow(), FPS_WIDTH, MAX_TIME, 32 );
    XRenderPictFormat* format = XRenderFindStandardFormat( display(), PictStandardARGB32 );
    Picture p = XRenderCreatePicture( display(), pixmap, format, 0, NULL );
    XFreePixmap( display(), pixmap );
    XRenderColor col;
    col.alpha = int( alpha * 0xffff );
    col.red = int( alpha * 0xffff ); // white
    col.green = int( alpha * 0xffff );
    col.blue= int( alpha * 0xffff );
    XRenderFillRectangle( display(), PictOpSrc, p, &col, 0, 0, FPS_WIDTH, MAX_TIME );
    col.red = 0; // blue
    col.green = 0;
    col.blue = int( alpha * 0xffff );
    XRenderFillRectangle( display(), PictOpSrc, p, &col, 0, MAX_TIME - fps, FPS_WIDTH, fps );
    col.red = 0; // black
    col.green = 0;
    col.blue = 0;
    for( int i = 10;
         i < MAX_TIME;
         i += 10 )
        {
        XRenderFillRectangle( display(), PictOpSrc, p, &col, 0, MAX_TIME - i, FPS_WIDTH, 1 );
        }
    XRenderComposite( display(), alpha != 1.0 ? PictOpOver : PictOpSrc, p, None,
        effects->xrenderBufferPicture(), 0, 0, 0, 0, x, y, FPS_WIDTH, MAX_TIME );
    XRenderFreePicture( display(), p );

    // Paint FPS graph
    paintFPSGraph( x + FPS_WIDTH, y );

    // Paint amount of rendered pixels graph
    paintDrawSizeGraph( x + FPS_WIDTH + MAX_TIME, y );

#endif
    }

void ShowFpsEffect::paintFPSGraph(int x, int y)
    {
    // Paint FPS graph
    QList<int> lines;
    lines << 10 << 20 << 50;
    QList<int> values;
    for( int i = 0;
         i < NUM_PAINTS;
         ++i )
        {
        values.append( paints[ ( i + paints_pos ) % NUM_PAINTS ] );
        }
    paintGraph( x, y, values, lines, true );
    }

void ShowFpsEffect::paintDrawSizeGraph(int x, int y)
{
    int max_drawsize = 0;
    for( int i = 0; i < NUM_PAINTS; i++)
        max_drawsize = qMax(max_drawsize, paint_size[ i ] );

    // Log of min/max values shown on graph
    const float max_pixels_log = 7.2f;
    const float min_pixels_log = 2.0f;
    const int minh = 5;  // Minimum height of the bar when  value > 0

    float drawscale = (MAX_TIME - minh) / (max_pixels_log - min_pixels_log);
    QList<int> drawlines;

    for( int logh = (int)min_pixels_log; logh <= max_pixels_log; logh++ )
        drawlines.append( (int)(( logh - min_pixels_log ) * drawscale ) + minh );

    QList<int> drawvalues;
    for( int i = 0;
         i < NUM_PAINTS;
         ++i )
        {
        int value = paint_size[ ( i + paints_pos ) % NUM_PAINTS ];
        int h = 0;
        if( value > 0)
            {
            h = (int)(( log10( value ) - min_pixels_log ) * drawscale );
            h = qMin( qMax( 0, h ) + minh, MAX_TIME );
            }
        drawvalues.append( h );
        }
    paintGraph( x, y, drawvalues, drawlines, false );
    }

void ShowFpsEffect::paintGraph( int x, int y, QList<int> values, QList<int> lines, bool colorize)
    {
#ifdef HAVE_OPENGL
    if( effects->compositingType() == OpenGLCompositing)
        {
        glColor4f( 0, 0, 0, alpha ); // black
        glBegin( GL_LINES );
        // First draw the lines
        foreach( int h, lines)
            {
            glVertex2i( x, y - h );
            glVertex2i( x + values.count(), y - h );
            }
        // Then the graph values
        glColor4f( 0.5, 0.5, 0.5, alpha );
        for( int i = 0; i < values.count(); i++ )
            {
            int value = values[ i ];
            if( colorize )
                {
                if( value <= 10 )
                    glColor4f( 0, 1, 0, alpha ); // green
                else if( value <= 20 )
                    glColor4f( 1, 1, 0, alpha ); // yellow
                else if( value <= 50 )
                    glColor4f( 1, 0, 0, alpha ); // red
                else
                    glColor4f( 0, 0, 0, alpha ); // black
                }
            glVertex2i( x + values.count() - i, y );
            glVertex2i( x + values.count() - i, y - value );
            }
        glEnd();
        }
#endif
#ifdef HAVE_XRENDER
    if( effects->compositingType() == XRenderCompositing)
        {
        Pixmap pixmap = XCreatePixmap( display(), rootWindow(), values.count(), MAX_TIME, 32 );
        XRenderPictFormat* format = XRenderFindStandardFormat( display(), PictStandardARGB32 );
        Picture p = XRenderCreatePicture( display(), pixmap, format, 0, NULL );
        XFreePixmap( display(), pixmap );
        XRenderColor col;
        col.alpha = int( alpha * 0xffff );

        // Draw background
        col.red = col.green = col.blue = int( alpha * 0xffff ); // white
        XRenderFillRectangle( display(), PictOpSrc, p, &col, 0, 0, values.count(), MAX_TIME );

        // Then the values
        col.red = col.green = col.blue = int( alpha * 0x8000 );  // grey
        for( int i = 0; i < values.count(); i++ )
            {
            int value = values[ i ];
            if( colorize )
                {
                if( value <= 10 )
                    { // green
                    col.red = 0;
                    col.green = int( alpha * 0xffff );
                    col.blue = 0;
                    }
                else if( value <= 20 )
                    { // yellow
                    col.red = int( alpha * 0xffff );
                    col.green = int( alpha * 0xffff );
                    col.blue = 0;
                    }
                else if( value <= 50 )
                    { // red
                    col.red = int( alpha * 0xffff );
                    col.green = 0;
                    col.blue = 0;
                    }
                else
                    { // black
                    col.red = 0;
                    col.green = 0;
                    col.blue = 0;
                    }
                }
            XRenderFillRectangle( display(), PictOpSrc, p, &col,
                                  values.count() - i, MAX_TIME - value, 1, value );
            }

        // Then the lines
        col.red = col.green = col.blue = 0;  // black
        foreach( int h, lines)
            XRenderFillRectangle( display(), PictOpSrc, p, &col, 0, MAX_TIME - h, values.count(), 1 );

        // Finally render the pixmap onto screen
        XRenderComposite( display(), alpha != 1.0 ? PictOpOver : PictOpSrc, p, None,
            effects->xrenderBufferPicture(), 0, 0, 0, 0, x, y, values.count(), MAX_TIME );
        XRenderFreePicture( display(), p );
        }
#endif
    }

void ShowFpsEffect::postPaintScreen()
    {
    effects->postPaintScreen();
    paints[ paints_pos ] = t.elapsed();
    if( ++paints_pos == NUM_PAINTS )
        paints_pos = 0;
    effects->addRepaint( fps_rect );
    }

} // namespace
