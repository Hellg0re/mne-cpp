//=============================================================================================================
/**
* @file     ecdview.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    ECDView class declaration.
*
*/

#ifndef ECDVIEW_H
#define ECDVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVERSELIB {
    class DipoleFitSettings;
    class ECDSet;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class View3D;
class Control3DWidget;
class Data3DTreeModel;


//=============================================================================================================
/**
* Adapter which provides visualization for ECD data and a control widget.
*
* @brief Visualizes ECD data.
*/
class DISP3DNEWSHARED_EXPORT ECDView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<ECDView> SPtr;             /**< Shared pointer type for ECDView class. */
    typedef QSharedPointer<const ECDView> ConstSPtr;  /**< Const shared pointer type for ECDView class. */

    //=========================================================================================================
    /**
    * Default constructor
    *
    */
    explicit ECDView(const INVERSELIB::DipoleFitSettings& dipFitSettings, const INVERSELIB::ECDSet& ecdSet, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~ECDView();

protected:
    QSharedPointer<DISP3DLIB::View3D>                   m_p3DView;          /**< The Disp3D view. */
    QSharedPointer<DISP3DLIB::Control3DWidget>          m_pControl3DView;   /**< The Disp3D control. */
    QSharedPointer<DISP3DLIB::Data3DTreeModel>          m_pData3DModel;     /**< The Disp3D model. */
};

} // NAMESPACE

#endif // ECDVIEW_H
