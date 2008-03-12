/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2007 Lubos Lunak <l.lunak@kde.org>

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

#include "shadow.h"

#include <kwinglutils.h>

#include <kconfiggroup.h>
#include <kdebug.h>
#include <KStandardDirs>
#include <kcolorscheme.h>

namespace KWin
{

KWIN_EFFECT( shadow, ShadowEffect )

ShadowEffect::ShadowEffect()
    {
    KConfigGroup conf = effects->effectConfig("Shadow");
    shadowXOffset = conf.readEntry( "XOffset", 0 );
    shadowYOffset = conf.readEntry( "YOffset", 3 );
    shadowOpacity = conf.readEntry( "Opacity", 0.25 );
    shadowFuzzyness = conf.readEntry( "Fuzzyness", 10 );
    shadowSize = conf.readEntry( "Size", 5 );
    shadowColor = conf.readEntry( "Color",  KColorScheme::shade( Qt::white, KColorScheme::ShadowShade ) );
    intensifyActiveShadow = conf.readEntry( "IntensifyActiveShadow", true );

    QString shadowtexture =  KGlobal::dirs()->findResource("data", "kwin/shadow-texture.png");
    mShadowTexture = new GLTexture(shadowtexture);
    }

QRect ShadowEffect::shadowRectangle(const QRect& windowRectangle) const
    {
    int shadowGrow = shadowFuzzyness + shadowSize;
    return windowRectangle.adjusted( shadowXOffset - shadowGrow, shadowYOffset - shadowGrow,
            shadowXOffset + shadowGrow, shadowYOffset + shadowGrow);
    }

void ShadowEffect::paintScreen( int mask, QRegion region, ScreenPaintData& data )
    {
    shadowDatas.clear();

    // Draw windows
    effects->paintScreen( mask, region, data );

    // Draw shadows
    drawQueuedShadows( 0 );
    }

void ShadowEffect::prePaintWindow( EffectWindow* w, WindowPrePaintData& data, int time )
    {
    if( useShadow( w ))
        {
        data.paint |= shadowRectangle( data.paint.boundingRect() );
        }
    effects->prePaintWindow( w, data, time );
    }

void ShadowEffect::drawWindow( EffectWindow* w, int mask, QRegion region, WindowPaintData& data )
    {
    // Whether the shadow drawing can be delayed or not.
    bool optimize = !( mask & ( PAINT_WINDOW_TRANSFORMED | PAINT_SCREEN_TRANSFORMED |
            PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS | PAINT_WINDOW_TRANSLUCENT ));
    if( !optimize )
        {
        // Transformed or translucent windows are drawn bottom-to-top, so
        //  first we need to draw all queued shadows.
        drawQueuedShadows( w );
        }
    if( useShadow( w ))
        {
        if( !optimize )
            {
            // For translucent windows, shadow needs to be drawn before the
            //  window itself.
            drawShadow( w, mask, region, data );
            }
        else
            {
            // For opaque windows, just schedule the shadow to be drawn later
            ShadowData d(w, data);
            d.clip = w->shape().translated( w->x(), w->y());
            if( !shadowDatas.isEmpty())
                d.clip |= shadowDatas.last().clip;
            d.mask = mask;
            foreach(QRect r, region.rects())
                d.region |= shadowRectangle(r);
            d.region &= region;
            shadowDatas.append(d);
            }
        }

    effects->drawWindow( w, mask, region, data );
    }

QRect ShadowEffect::transformWindowDamage( EffectWindow* w, const QRect& r )
    {
    if( !useShadow( w ))
        return effects->transformWindowDamage( w, r );
    QRect r2 = r | shadowRectangle( r );
    return effects->transformWindowDamage( w, r2 );
    }

void ShadowEffect::windowClosed( EffectWindow* c )
    {
    effects->addRepaint( shadowRectangle( c->geometry() ));
    }

bool ShadowEffect::useShadow( EffectWindow* w ) const
    {
    return !w->isDeleted() && !w->isDesktop() && !w->isDock() && !w->hasOwnShape();
    }

void ShadowEffect::addQuadVertices(QVector<float>& verts, float x1, float y1, float x2, float y2) const
    {
    verts << x1 << y1;
    verts << x1 << y2;
    verts << x2 << y2;
    verts << x2 << y1;
    }

void ShadowEffect::drawQueuedShadows( EffectWindow* behindWindow )
    {
    QList<ShadowData> newShadowDatas;
    EffectWindowList stack = effects->stackingOrder();
    foreach( ShadowData d, shadowDatas )
        {
        // If behindWindow is given then only render shadows of the windows
        //  that are behind that window.
        if( !behindWindow || stack.indexOf(d.w) < stack.indexOf(behindWindow))
            {
            drawShadow( d.w, d.mask, d.region.subtracted( d.clip ), d.data );
            }
        else
            {
            newShadowDatas.append(d);
            }
        }
    shadowDatas = newShadowDatas;
    }

void ShadowEffect::drawShadow( EffectWindow* window, int mask, QRegion region, WindowPaintData& data )
    {
    glPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    int fuzzy = shadowFuzzyness;
    // Shadow's size must be a least 2*fuzzy in both directions (or the corners will be broken)
    int w = qMax(fuzzy*2, window->width() + 2*shadowSize);
    int h = qMax(fuzzy*2, window->height() + 2*shadowSize);

    glPushMatrix();
    if( mask & PAINT_WINDOW_TRANSFORMED )
        glTranslatef( data.xTranslate, data.yTranslate, 0 );
    glTranslatef( window->x() + shadowXOffset - qMax(0, w - window->width()) / 2.0,
                  window->y() + shadowYOffset - qMax(0, h - window->height()) / 2.0, 0 );
    if(( mask & PAINT_WINDOW_TRANSFORMED ) && ( data.xScale != 1 || data.yScale != 1 ))
        glScalef( data.xScale, data.yScale, 1 );

    QVector<float> verts, texcoords;
    verts.reserve(80);
    texcoords.reserve(80);
    // center
    addQuadVertices(verts, 0 + fuzzy, 0 + fuzzy, w - fuzzy, h - fuzzy);
    addQuadVertices(texcoords, 0.5, 0.5, 0.5, 0.5);
    // sides
    // left
    addQuadVertices(verts, 0 - fuzzy, 0 + fuzzy, 0 + fuzzy, h - fuzzy);
    addQuadVertices(texcoords, 0.0, 0.5, 0.5, 0.5);
    // top
    addQuadVertices(verts, 0 + fuzzy, 0 - fuzzy, w - fuzzy, 0 + fuzzy);
    addQuadVertices(texcoords, 0.5, 0.0, 0.5, 0.5);
    // right
    addQuadVertices(verts, w - fuzzy, 0 + fuzzy, w + fuzzy, h - fuzzy);
    addQuadVertices(texcoords, 0.5, 0.5, 1.0, 0.5);
    // bottom
    addQuadVertices(verts, 0 + fuzzy, h - fuzzy, w - fuzzy, h + fuzzy);
    addQuadVertices(texcoords, 0.5, 0.5, 0.5, 1.0);
    // corners
    // top-left
    addQuadVertices(verts, 0 - fuzzy, 0 - fuzzy, 0 + fuzzy, 0 + fuzzy);
    addQuadVertices(texcoords, 0.0, 0.0, 0.5, 0.5);
    // top-right
    addQuadVertices(verts, w - fuzzy, 0 - fuzzy, w + fuzzy, 0 + fuzzy);
    addQuadVertices(texcoords, 0.5, 0.0, 1.0, 0.5);
    // bottom-left
    addQuadVertices(verts, 0 - fuzzy, h - fuzzy, 0 + fuzzy, h + fuzzy);
    addQuadVertices(texcoords, 0.0, 0.5, 0.5, 1.0);
    // bottom-right
    addQuadVertices(verts, w - fuzzy, h - fuzzy, w + fuzzy, h + fuzzy);
    addQuadVertices(texcoords, 0.5, 0.5, 1.0, 1.0);

    mShadowTexture->bind();
    // Take the transparency settings and window's transparency into account.
    // Also make the shadow more transparent if we've made it bigger
    float opacity = shadowOpacity;
    if( intensifyActiveShadow && window == effects->activeWindow() )
    {
        opacity = 1 - (1 - shadowOpacity)*(1 - shadowOpacity);
    }
    glColor4f(shadowColor.redF(), shadowColor.greenF(), shadowColor.blueF(), opacity * data.opacity * (window->width() / (double)w) * (window->height() / (double)h));
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    // We have two elements per vertex in the verts array
    int verticesCount = verts.count() / 2;
    renderGLGeometry( region, verticesCount, verts.data(), texcoords.data() );
    mShadowTexture->unbind();

    glPopMatrix();
    glPopAttrib();
    }

} // namespace
