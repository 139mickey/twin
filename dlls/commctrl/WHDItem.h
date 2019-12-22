/* WHDItem.h
   Copyright 1997 Willows Software, Inc. 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.

The maintainer of the Willows TWIN Libraries may be reached (Email) 
at the address twin@willows.com	

*/


#ifndef CWHDITEM_H
#define CWHDITEM_H



#include "WUtilities.h"
#include "WElement.h"




class CWHDItem :
    public CWElement
{

    public:

        HD_ITEM                                 HDItem;

        
        CWHDItem (
            HD_ITEM                             *pItem );
        virtual ~CWHDItem ();

        virtual CWHDItem& operator= (
            CWHDItem                            &that )
        {
            return ( Assign ( that ) );
        }
        virtual CWElement& operator= (
            CWElement                           &that )
        {
            return ( ( CWElement& ) operator= ( ( CWHDItem& ) that ) );
        }
        virtual int Hash ()
        {
            return ( HDItem.cxy );
        }
        virtual char* String ()
        {
            return ( pString );
        }
        CWHDItem& Assign (
            HD_ITEM                             *pItem );
        void Get (
            HD_ITEM                             *pItem );

    
    private:

        char                                    *pString;

        CWHDItem& Assign (
            CWHDItem                            &that );


};


#endif /* #ifndef CWHDITEM_H*/
