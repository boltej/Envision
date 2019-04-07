/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
// TreePropSheetResizableLibHook.h: interface for the CTreePropSheetResizableLibHook class.
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2004 by Yves Tkaczyk
// (http://www.tkaczyk.net - yves@tkaczyk.net)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// Documentation: http://www.codeproject.com/property/treepropsheetex.asp
// CVS tree:      http://sourceforge.net/projects/treepropsheetex
//
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _TREEPROPSHEET_RESIZABLELIBHOOK_H__INCLUDED_
#define _TREEPROPSHEET_RESIZABLELIBHOOK_H__INCLUDED_

#include "HookWnd.h"
#include "ResizableLayout.h"
#include "ResizableGrip.h"
#include "ResizableMinMax.h"
#include "ResizableState.h"

namespace TreePropSheet
{
  //********************************************************************
  // CResizableSheetHook
  //********************************************************************

  /*! \brief Window hook for CPropertySheet and ResizableLib

    @version 0.1
    @author Yves Tkaczyk <yves@tkaczyk.net> 
    @date 07/2004 */
  class CResizableSheetHook
    : public CHookWnd, 
      public CResizableLayout,
      public CResizableGrip, 
      public CResizableMinMax,
      public CResizableState
  {
  // Construction/Destruction
  public:
    CResizableSheetHook(CPropertySheet* const pSheet);
    virtual ~CResizableSheetHook();

  // Methods
    /*! Attach the hook to the window. */
    void Initialize();

  // Exposed methods from base-class.
    void SetMinSize(const CSize& size);
    void SetMaxSize(const CSize& size);

  // CHookWnd overrides
  protected:
    virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

  // CResizableLayout implementation
  protected:
    virtual CWnd* GetResizableWnd();
    virtual BOOL ArrangeLayoutCallback(LayoutInfo &layout);

  public:
    // Make AddAnchor public.
    using CResizableLayout::AddAnchor;

  // Overridable
  protected:
    virtual void PresetLayout();

  // Helpers
  protected:
	  void PrivateConstruct();
	  void SavePage();
	  void LoadPage();
	  BOOL IsWizard();

  // callable from derived classes
  protected:
	  // section to use in app's profile
	  void EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly = FALSE, BOOL bWithPage = FALSE);
	  int GetMinWidth();	// minimum width to display all buttons

  // Members
  protected:
    //! The parent window.
    CPropertySheet* m_pSheet;

	  // support for temporarily hiding the grip
	  DWORD m_dwGripTempState;

	  // flags
	  BOOL m_bEnableSaveRestore;
	  BOOL m_bRectOnly;
	  BOOL m_bSavePage;

	  // layout vars
	  CSize m_sizePageTL, m_sizePageBR;

	  // internal status
	  CString m_sSection;			// section name (identifies a parent window)
  };
  
  // Forward declaration of CTreePropSheetEx.
  class CTreePropSheetEx;

  //********************************************************************
  // CTreePropSheetResizableLibHook
  //********************************************************************

  /*! \brief Window hook for CTreePropSheetEx and ResizableLib

  @version 0.1
  @author Yves Tkaczyk <yves@tkaczyk.net> 
  @date 07/2004 */
  class CTreePropSheetResizableLibHook
    : public CResizableSheetHook
  {
  // Construction/Destruction
  public:
    CTreePropSheetResizableLibHook(TreePropSheet::CTreePropSheetEx* pTreePropSheet);
    ~CTreePropSheetResizableLibHook();

  // Overridable
  protected:
    /*! Register the different sheet's controls with the layout manager. The
        splitter control is registered with the layout manager as well as the
        sheet's buttons. */
    virtual void PresetLayout();

  // CResizableLayout implementation
  protected:
    /*! Layout manager callback method. This is used to position the active page 
        and the tab control. 
    \param layout Reference to LayoutInfo. The callback method populates this structure
                  by setting the HWND of the window to be move as well as resizing 
                  information.
    \retval TRUE if the LayoutInfo was populated successfully, FALSE otherwise. */
    virtual BOOL ArrangeLayoutCallback(LayoutInfo &layout);

  // Helpers
  protected:
    /*! Callback implementation for tree mode. 
    \retval TRUE if callback handled provided layout. */
    BOOL TreeModeCallbacks(LayoutInfo &layout);

  // Members
  protected:
    //! The parent window.
    TreePropSheet::CTreePropSheetEx* m_pTreePropSheet;
    //! Margin between frame and page in tree mode.
    CRect m_rectFramePageMargins;
  };

};

#endif // _TREEPROPSHEET_RESIZABLELIBHOOK_H__INCLUDED_
