/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2006 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#include "unmanaged.h"

#include "workspace.h"

#include <X11/extensions/shape.h>

namespace KWinInternal
{

Unmanaged::Unmanaged( Workspace* ws )
    : Toplevel( ws )
    , info( NULL )
    {
    }
    
Unmanaged::~Unmanaged()
    {
    delete info;
    }

bool Unmanaged::track( Window w )
    {
    XSelectInput( display(), w, StructureNotifyMask );
    XWindowAttributes attr;
    if( !XGetWindowAttributes(display(), w, &attr))
        return false;
    if( attr.c_class == InputOnly )
        return false;
    setHandle( w );
    geom = QRect( attr.x, attr.y, attr.width, attr.height );
    vis = attr.visual;
    bit_depth = attr.depth;
    unsigned long properties[ 2 ];
    properties[ NETWinInfo::PROTOCOLS ] =
        NET::WMWindowType |
        NET::WMPid |
        0;
    properties[ NETWinInfo::PROTOCOLS2 ] =
        NET::WM2Opacity |
        0;
    info = new NETWinInfo( display(), w, rootWindow(), properties, 2 );

    if( Extensions::shapeAvailable())
        XShapeSelectInput( display(), w, ShapeNotifyMask );
    detectShape( w );
    setupCompositing();
    return true;
    }

void Unmanaged::release()
    {
    workspace()->addDamage( geometry());
    finishCompositing();
    workspace()->removeUnmanaged( this, Allowed );
    delete this;
    }

void Unmanaged::deleteUnmanaged( Unmanaged* c, allowed_t )
    {
    delete c;
    }

NET::WindowType Unmanaged::windowType( bool, int supported_types ) const
    {
    return info->windowType( supported_types );
    }

double Unmanaged::opacity() const
    {
    if( info->opacity() == 0xffffffff )
        return 1.0;
    return info->opacity() * 1.0 / 0xffffffff;
    }

void Unmanaged::debug( kdbgstream& stream ) const
    {
    stream << "\'ID:" << handle() << "\'";
    }

} // namespace

#include "unmanaged.moc"
