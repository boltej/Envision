#include "pch.h"
#include "NetView.h"
#include "MainFrm.h"
#include "NetForm.h"
#include "resource.h"

#include <tixml.h>

#include <gdiplus.h>
#include <memory>
#include <colors.hpp>
#include <ColorRamps.h>
#include <GEOMETRY.HPP>
#include <randgen/Randunif.hpp>


//using namespace Gdiplus;


extern CMainFrame* gpMainFrame;

RandUniform randUnif;

const int MAX_NODE_SIZE = 20;  // radius, logical coords
const int MIN_NODE_SIZE = 3;
const float MAX_EDGE_WIDTH = 30;  // 
const float MIN_EDGE_WIDTH = 0.5f;

std::vector<std::string>   Node::attrIDs;
std::vector<std::string>   Node::attrLabels;
std::vector<std::string>   Node::attrTypes;
std::map<std::string, int> Node::attrMap;


std::vector<std::string>   Edge::attrIDs;
std::vector<std::string>   Edge::attrLabels;
std::vector<std::string>   Edge::attrTypes;
std::map<std::string, int> Edge::attrMap;


BEGIN_MESSAGE_MAP(NetView, CWnd)
   ON_WM_PAINT()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEMOVE()
   ON_WM_RBUTTONDOWN()
   ON_COMMAND(ID_ZOOM_FULL, &NetView::OnZoomfull)
   //ON_COMMAND(ID_ZOOM_FULL, &NetView::OnZoomfull)
   //ON_COMMAND(ID_ZOOM_FULL, &NetView::OnZoomfull)
   ON_COMMAND(ID_PAN, &NetView::OnPan)
   ON_WM_CREATE()
   ON_WM_MOUSEHOVER()   
   ON_WM_MOUSEWHEEL()
   ON_COMMAND(ID_DARKMODE, &NetView::OnDarkMode)
   ON_COMMAND(ID_CURVED_EDGES, &NetView::OnCurvedEdges)
   ON_UPDATE_COMMAND_UI(ID_DARKMODE, &NetView::OnUpdateDarkmode)
   ON_COMMAND(ID_SHOWEDGELABELS, &NetView::OnShowEdgeLabels)
   ON_UPDATE_COMMAND_UI(ID_SHOWEDGELABELS, &NetView::OnUpdateShowEdgeLabels)
   ON_COMMAND(ID_SHOWNODELABELS, &NetView::OnShowNodeLabels)
   ON_UPDATE_COMMAND_UI(ID_SHOWNODELABELS, &NetView::OnUpdateShowNodeLabels)
   ON_COMMAND(ID_NODE_COLOR, &NetView::OnNodeColor)
   ON_COMMAND(ID_NODE_SIZE, &NetView::OnNodeSize)
   ON_COMMAND(ID_NODE_LABEL, &NetView::OnNodeLabel)
   ON_COMMAND(ID_EDGE_COLOR, &NetView::OnEdgeColor)
   ON_COMMAND(ID_EDGE_SIZE, &NetView::OnEdgeSize)
   ON_COMMAND(ID_EDGE_LABEL, &NetView::OnEdgeLabel)
   ON_COMMAND(ID_SHOW_NEIGHBORHOOD, &NetView::OnShowNeighborhood)
END_MESSAGE_MAP()


void NetView::OnPaint()
   {
   if (nodes.size() == 0)
      return;

   //clock_t startTime = clock();

   CPaintDC dc(this); // device context for painting

   /*
   const int barSize = 1000;
   const CRect barRect(10, 10, 40, 10 + barSize);
   CPen pen(PS_SOLID, 1, BLACK);
   CPen* pOldPen = dc.SelectObject(&pen);
   dc.Rectangle(barRect);

   for (int i = 1; i < barSize-1; i++)
      {
      RGBA color = ::RColorRamp(0, barSize, i);
      COLORREF _color = RGB(color.r, color.g, color.b);
      pen.DeleteObject();
      pen.CreatePen(PS_SOLID, 1, _color);
      dc.SelectObject(&pen);
      dc.MoveTo(11, 10 + i);
      dc.LineTo(39, 10 + i);
      }

    dc.SelectObject(pOldPen);
    pen.DeleteObject(); */

   ////// test /////////////////////
   /////CPoint p0(200,600);
   /////CPoint p1(300,100);  // horizontal
   /////CPoint p2(200,150);  // horizontal
   /////CPoint a0, a1;
   /////GetAnchorPoints(p1, p0, PI /8, .2f, a0, a1);
   /////
   /////DrawPoint(dc, p0, 4, 1);
   /////DrawPoint(dc, p1, 4, 1);
   /////DrawPoint(dc, a0, 2, 1);
   /////DrawPoint(dc, a1, 2, 1);
   
   //GetAnchorPoints(p2, p0, PI / 8, .2f, a0, a1);
   //DrawPoint(dc, p0, 4, 1);
   //DrawPoint(dc, p2, 4, 1);
   //DrawPoint(dc, a0, 2, 1);
   //DrawPoint(dc, a1, 2, 1);
   
   //dc.TextOut(p0.x, p0.y, CString("p0"));
   //dc.TextOut(p1.x, p1.y, CString("p1"));
   //dc.TextOut(p2.x, p2.y, CString("p2"));

   DrawNetwork(dc,true,DF_NORMAL);

   //clock_t finish = clock();
   //float duration = (float)(finish - startTime) / CLOCKS_PER_SEC;
   //CString msg;
   //msg.Format("Finished drawing network in %.2f seconds (Edges: %.2f seconds)", duration, edgeTime);
   //gpMainFrame-> SetStatusMsg(msg);

   // Do not call CWnd::OnPaint() for painting messages
   }


void NetView::InitDC(CDC &dc)
   {
   CRect rect;
   GetClientRect(rect);
   int cWidth = rect.Width();
   int cHeight = rect.Height();

   dc.SetMapMode(MM_ANISOTROPIC);
   dc.SetWindowExt(xMax - xMin, yMax - yMin);
   dc.SetViewportExt(cWidth, cHeight);       // client coords
   dc.SetWindowOrg(xMin, yMin);              // logical (world) points
   }


void NetView::DrawNetwork(CDC& dc, bool drawBkgr, DRAW_FLAG drawFlag)
   {
   CRect clientRect;
   this->GetClientRect(&clientRect);
   int cx = clientRect.Width();
   int cy = clientRect.Height();
   float aspect = float(cx) / cy;

   if (drawBkgr)
      {
      if (this->darkMode)
         dc.FillSolidRect(&clientRect, BLACK);
      else
         dc.FillSolidRect(&clientRect, WHITE);
      }

   InitDC(dc);
   CPen pen(PS_SOLID, 1, LTGRAY);
   CBrush brush(LTGRAY);

   CPen *pOldPen = dc.SelectObject(&pen);
   CBrush *pOldBrush = dc.SelectObject(&brush);

   dc.SetBkMode(TRANSPARENT);
   dc.SetTextAlign(TA_LEFT | TA_BASELINE);

   if (darkMode)
      dc.SetTextColor(RGB(225,225,225));
   else
      dc.SetTextColor(RGB(120, 120, 120));

   // ---  panels ----------------
   if (drawBkgr)
      DrawPanels(dc);

   CFont* pFont = (CFont*)dc.SelectStockObject(DEFAULT_GUI_FONT);
   dc.SetTextAlign(TA_CENTER | TA_BASELINE);
   dc.SetTextColor(darkMode ? WHITE : BLACK);

   CPoint bezierPts[4]; // for drawing bezier curves

   // reset pen (default to gray)
   pen.DeleteObject();
   pen.CreatePen(PS_SOLID, 1, LTGRAY);
   dc.SelectObject(&pen);

   // first edges
   CPoint start;
   CPoint end;
   int i = 0;
   for (Edge* edge : this->edges)
      {
      if (edge->show == false)
         continue;

      if ( drawFlag != DF_GRAYED ) //&& (i == 0
         //|| (edges[i - 1]->color.a != edge->color.a) || (edges[i - 1]->color.r != edge->color.r) || (edges[i - 1]->color.g != edge->color.g) || (edges[i - 1]->color.b != edge->color.b)
         //|| edge->thickness != edges[i-1]->thickness || edge->selected != edges[i-1]->selected))
         {
         BYTE r = edge->color.r;
         BYTE g = edge->color.g;
         BYTE b = edge->color.b;

         COLORREF color = RGB(r,g,b);
         if (edge->selected)
            color = YELLOW;
         else if (drawFlag == DF_GRAYED)
            color = LTGRAY;
         else if (drawFlag == DF_NEIGHBORHOODS)
            {
            // edge or either node not selected, don't draw it (it's not part of a neighborhood)
            if (!edge->selected && !edge->pFromNode->selected && !edge->pToNode->selected)
               {
               i++;
               continue;
               }
            }
         //else if (drawFlag == DF_SHORTESTROUTE)
         //   ;

         dc.SelectObject(pOldPen);
         pen.DeleteObject();
         pen.CreatePen(PS_SOLID, (int)edge->thickness / 5, color);
         dc.SelectObject(&pen);
         }
  
      start.x = edge->pFromNode->x;
      start.y = edge->pFromNode->y;
      end.x = edge->pToNode->x;
      end.y = edge->pToNode->y;

      //dc.LPtoDP(&start);
      //dc.LPtoDP(&end);


      if (this->edgeType == 1) // beziers
         {
         if (this->GetAnchorPoints(start, end, PI / 8 /*45deg*/, 0.3f, bezierPts[1], bezierPts[2]))
            {
            bezierPts[0] = start;
            bezierPts[3] = end;
            dc.PolyBezier(bezierPts, 4);
            }
         }
      else
         {
         dc.MoveTo(start.x, start.y);
         dc.LineTo(end.x, end.y);
         }

      if (this->showEdgeLabels)
         {
         int x = (start.x + end.x) / 2;
         int y = (start.y + end.y) / 2;
         dc.TextOut(x, y, CString(edge->displayLabel.c_str()));
         }
      i++;
      }

   // then nodes:
   i = 0;
   int lastNode = 0;

   for (Node* node : nodes)
      {
      i++;
      if (node->show == false)
         continue;

      if (i == 1
         || (nodes[lastNode]->color.a != node->color.a) 
         || (nodes[lastNode]->color.r != node->color.r) 
         || (nodes[lastNode]->color.g != node->color.g)
         || (nodes[lastNode]->color.b != node->color.b)
         //|| node->reactivity != nodes[i-1]->reactivity 
         || node->selected != nodes[i-1]->selected)
         {
         dc.SelectObject(pOldPen);
         dc.SelectObject(pOldBrush);
         
         // fill color
         BYTE r = node->color.r;
         BYTE g = node->color.g;
         BYTE b = node->color.b;
         COLORREF fillColor = RGB(r, g, b);
         
         // pen color
         COLORREF penColor(GRAY);    // grey 
         if (node->selected)
            penColor = YELLOW;
         else if (drawFlag == DF_GRAYED)
            penColor = fillColor = LTGRAY;
         else if (drawFlag == DF_NEIGHBORHOODS)
            {
            bool neighbor = false;
            for (Edge* edge : node->inEdges)
               {
               if (edge->pFromNode->selected)
                  { neighbor = true; break; }
               }

            if (!neighbor)
               {
               for (Edge* edge : node->outEdges)
                  {
                  if (edge->pToNode->selected)
                     { neighbor = true; break; }
                  }
               }
            if ( ! neighbor)
               penColor = fillColor = GRAY;
            }
         //else if (drawFlag == DF_SHORTESTROUTE)
         //   ;
         pen.DeleteObject();
         pen.CreatePen(PS_SOLID, 1, penColor);

         brush.DeleteObject();
         brush.CreateSolidBrush(fillColor);
         dc.SelectObject(&pen);
         dc.SelectObject(&brush);
         }

      CPoint pos(node->x, node->y);

      //const int NODESIZE_MAX = 24; // radius, pxs
      //int nodeSize = 3;
      //
      //switch (nodeSizeFlag)
      //   {
      //   case NS_INFLUENCE:   nodeSize += int(NODESIZE_MAX * node->influence);  break;
      //   case NS_REACTIVITY:  nodeSize += int(NODESIZE_MAX * node->reactivity); break;
      //   case NS_CONNECTIONS: nodeSize += int(NODESIZE_MAX * node->degree/this->maxNodeDegree);  break;
      //   }
      //
      //if (nodeSize > NODESIZE_MAX+3)
      //   nodeSize = NODESIZE_MAX+3;

      // logical coords
      DrawPoint(dc, pos, node->nodeSize, aspect);

      if (this->showNodeLabels)
         {
         //CString out;
         //out.Format("%s, (%i,%i)(dc)", node->label.c_str(), pos.x, pos.y);
         //dc.TextOut(pos.x, pos.y - nodeSize, out);   // logical coords
         dc.TextOut(pos.x, pos.y - node->nodeSize, CString(node->displayLabel.c_str()));
         }
     
      dc.LPtoDP(&pos);
      node->xDC = pos.x;
      node->yDC = pos.y;

      // determeine node radius in device coords
      CPoint pt0(0, 0);
      CPoint pt1(node->nodeSize, 0);
      dc.LPtoDP(&pt0);
      dc.LPtoDP(&pt1);
      node->nodeSizeDC = pt1.x - pt0.x;
      lastNode = i-1;
      }

   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
   dc.SelectObject(pFont);
   pen.DeleteObject();
   brush.DeleteObject();
   }



void NetView::DrawPanels(CDC& dc)
   {
   CFont panelFont;

   panelFont.CreateFontA(40, 0, 900, 900, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Arial");
   CFont* pOldFont = dc.SelectObject(&panelFont);

   COLORREF c0 = RGB(245, 245, 245);
   COLORREF c1 = RGB(225, 225, 225);
   if (darkMode)
      {
      c0 = RGB(25, 25, 25);
      c1 = RGB(50, 50, 50);
      }

   CRect rect;
   GetClientRect(rect);
   int cx = rect.Width();  // used below
   int cy = rect.Height();

   dc.DPtoLP(rect);
   rect.left = -60;
   rect.right = 60;

   dc.FillSolidRect(&rect, c0);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("Input Signal"));

   // next panel...   
   rect.left = rect.right;
   rect.right = 180;
   dc.FillSolidRect(&rect, c1);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("Assessors"));

   // next panel...   
   rect.left = rect.right;
   rect.right = 280;
   dc.FillSolidRect(&rect, c0);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("2nd Degree Assessors"));

   // next panel...   
   rect.left = 470;
   rect.right = 530;
   dc.TextOut(rect.left - 20, rect.bottom - 10, CString("Network Actors"));
   dc.FillSolidRect(&rect, c1);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("Spanners"));

   // next panel...   
   rect.left = 720;
   rect.right = 820;
   dc.TextOut(rect.left - 20, rect.bottom - 10, CString("Network Actors"));
   dc.FillSolidRect(&rect, c0);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("2nd Degree Engagers"));

   // next panel...   
   rect.left = rect.right;
   rect.right = 940;
   dc.FillSolidRect(&rect, c1);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("Engagers"));

   rect.left = rect.right;
   rect.right = 1060;
   dc.FillSolidRect(&rect, c0);
   dc.TextOut(rect.right - 10, rect.bottom - 10, CString("Landscape Actors"));

   dc.SelectObject(pOldFont);
   panelFont.DeleteObject();
   }



CPoint startPt;
CPoint lastPt;
bool dragging = false;
bool panning = false;
CDC* pDC = nullptr;
bool lBtnDown = false;


void NetView::OnLButtonDown(UINT nFlags, CPoint point)
   {
   lBtnDown = true;
   startPt = point;

   switch (this->drawMode)
      {
      case DM_SELECT:
         break;

      case DM_SELECTRECT:
      case DM_ZOOMING:
         {
         dragging = true;
         pDC = this->GetDC();

         lastPt = point; // save this for later use
         lastPt.Offset(2, 2);
         CRect startRect(startPt, lastPt);

         pDC->DrawDragRect(&startRect, CSize(2, 2), NULL, CSize(2, 2));
         this->ReleaseDC(pDC);

         SetCapture();

         CString msg;
         msg.Format("Starting Drag at (%i,%i)", startPt.x, startPt.y);
         gpMainFrame->SetStatusMsg(msg);
         }
         break;

      case DM_PANNING:
         {
         panning = true;
         lastPt = point;
         SetCapture();
         CString msg;
         msg.Format("Starting Pan at (%i,%i)", startPt.x, startPt.y);
         gpMainFrame->SetStatusMsg(msg);
         }
         break;
      }

   CString msg;
   msg.Format("Clicked at (%i,%i)(dc)", point.x, point.y);
   gpMainFrame->SetStatusMsg(msg);

   CWnd::OnLButtonDown(nFlags, point);
   }

void NetView::OnMouseMove(UINT nFlags, CPoint point)
   {
   if (panning)
      {
      pDC = this->GetDC();

      int r2 = pDC->SetROP2(R2_NOT);
      pDC->MoveTo(startPt);
      pDC->LineTo(lastPt);
      pDC->MoveTo(startPt);
      pDC->LineTo(point);
      pDC->SetROP2(r2);
      this->ReleaseDC(pDC);
      lastPt = point;
      }

   else if (dragging)
      {
      int dx = lastPt.x - point.x;
      int dy = lastPt.y - point.y;
      CRect newRect(startPt, point);
      CRect lastRect(startPt, lastPt);
      CString msg;
      msg.Format("Dragging (%i,%i) - (%i,%i), dx=%i, dy=%i", newRect.left, newRect.bottom, newRect.right, newRect.top, dx, dy);
      gpMainFrame->SetStatusMsg(msg);

      pDC = this->GetDC();
      pDC->DrawDragRect(&newRect, CSize(2, 2), &lastRect, CSize(2, 2));
      this->ReleaseDC(pDC);
      lastPt = point;
      }
   else // not dragging 
      {
      }

   CWnd::OnMouseMove(nFlags, point);
   }


BOOL NetView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
   {
   float sensitivity = .0005f;
   CRect rect(xMin, yMin, xMax, yMax);
   float dx = (xMax - xMin) * zDelta * sensitivity;
   float dy = (yMax - yMin) * zDelta * sensitivity;

   //if (zDelta < 0) // rotating back ( toward user ) - zoom in
   //   rect.DeflateRect(dx, dy);
   //else
      rect.InflateRect(int(dx), int(dy));

   xMin = rect.left;
   yMin = rect.top;
   xMax = rect.right;
   yMax = rect.bottom;
   
   Invalidate();
   return TRUE;
   }


void NetView::OnLButtonUp(UINT nFlags, CPoint point)
   {
   lBtnDown = false;


   switch (this->drawMode)
      {
      case DM_SELECT:
         {
         // not dragging, just clicking?
         if (abs(startPt.x - point.x) < 3 && abs(startPt.y - point.y < 3))
            {
            NetElement* pElem = HitTest(nFlags, point);  // device coords
            if (pElem != nullptr)
               {
               if (!(nFlags & MK_SHIFT))
                  ClearSelection();

               pElem->selected = true;
               Invalidate();

               CString msg;
               if (pElem->GetType() == NE_EDGE)
                  {
                  Edge* pEdge = (Edge*)pElem;
                  this->netForm->AddEdgePropertyPage(pEdge);
                  msg.Format("%s: SS=%.2f, T=%.2f, I=%.2f", (LPCTSTR)pEdge->pSNEdge->m_name.c_str(), pEdge->pSNEdge->m_signalStrength, pEdge->pSNEdge->m_trust, pEdge->pSNEdge->m_influence); // point.x, point.y);
                  gpMainFrame->SetStatusMsg(msg);
                  }
               else
                  {
                  Node* pNode = (Node*)pElem;
                  msg.Format("%s: R=%.2f, I=%.2f", (LPCTSTR) pNode->pSNNode->m_name.c_str(), pNode->pSNNode->m_reactivity, pNode->pSNNode->m_influence); // point.x, point.y);
                  gpMainFrame->SetStatusMsg(msg);

                  this->netForm->AddNodePropertyPage(pNode);
                  }
               }
            }
         }
         break;

      case DM_SELECTRECT:
      case DM_ZOOMING:
         {
         dragging = false;
         ReleaseCapture();
         lastPt = point;
         SetDrawMode(DM_SELECT);

         CClientDC dc(this);

         // erase drag rect
         CRect lastRect(startPt, lastPt);
         dc.DrawDragRect(lastRect, CSize(2, 2), NULL, CSize(2, 2));

         if (abs(startPt.x - lastPt.x) > 3 && abs(startPt.y - lastPt.y) > 3)
            {
            InitDC(dc);

            dc.DPtoLP(&startPt);
            dc.DPtoLP(&lastPt);

            if (this->drawMode == DM_ZOOMING)
               {
               xMin = min(startPt.x, lastPt.x);
               xMax = max(startPt.x, lastPt.x);
               yMin = min(startPt.y, lastPt.y);
               yMax = max(startPt.y, lastPt.y);

               CString msg;
               msg.Format("Zoom Extent: (%i,%i)-(%i,%i)", xMin, yMin, xMax, yMax);
               gpMainFrame->SetStatusMsg(msg);
               }
            else if (this->drawMode == DM_SELECTRECT)
               {
               }
            }
         this->Invalidate();
         }
         break;

      case DM_PANNING:
         {
         pDC = this->GetDC();

         int r2 = pDC->SetROP2(R2_NOT);
         pDC->MoveTo(startPt);
         pDC->LineTo(lastPt);
         pDC->SetROP2(r2);
         this->ReleaseDC(pDC);

         int dx = lastPt.x - startPt.x;
         int dy = lastPt.y - startPt.y;

         xMin -= dx;
         xMax -= dx;
         yMin -= dy;
         yMax -= dy;
         lastPt = point;
         panning = false;
         this->Invalidate();
         }
         break;
      }

   SetDrawMode(DM_SELECT);
   dragging = false;

   CWnd::OnLButtonUp(nFlags, point);
   }


void NetView::OnRButtonDown(UINT nFlags, CPoint point)
   {
   CWnd::OnRButtonDown(nFlags, point);
   }

void NetView::OnZoomfull()
   {
   xMin = -50;
   xMax = 1050;
   yMin = -200;
   yMax = 1040;

   this->Invalidate();
   }


//void NetView::OnUpdateViewZoomfull(CCmdUI* pCmdUI)
//   {
//   pCmdUI->Enable();
//   }


int NetView::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   //CRect rect( 10, 10, 120,30);
   //hoverText.Create("Info", WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 10000);
   return 0;
   }


void NetView::OnMouseHover(UINT nFlags, CPoint point)
   {
   // TODO: Add your message handler code here and/or call default

   if (!dragging)
      {
      if (HitTest(nFlags, point))
         return;
      }

   CWnd::OnMouseHover(nFlags, point);
   }


NetElement* NetView::HitTest(UINT nFlags, CPoint point)  // point is device coords
   {
   // are we on a node?
   for (Node* node : nodes)
      {
      int radius = 1+node->nodeSizeDC;

      if (node->xDC >= (point.x - radius) && node->xDC <= (point.x + radius)
       && node->yDC >= (point.y - radius) && node->yDC <= (point.y + radius))
         {
         return node;
         }
      }

   // are we on a edge?
   CPoint start, end;
   for (Edge* edge : edges)
      {
      start.SetPoint(edge->pFromNode->xDC, edge->pFromNode->yDC);
      end.SetPoint(edge->pToNode->xDC, edge->pToNode->yDC);

      float d = 0; // DistancePtToLine(point, start, end);
      DistancePtToLineSegment(point.x, point.y, start.x, start.y, end.x, end.y, d);

      if (d < 6)  // 6???
         return edge;
      }

   return nullptr;
   }

/*
void NetView::SetHoverText(LPCTSTR msg, CPoint pos)
   {
   CRect hRect;
   hoverText.GetWindowRect(&hRect);  // screen coords

   this->ScreenToClient(&hRect);

   CDC* pDC = hoverText.GetDC();
   CSize sz = pDC->GetTextExtent(msg);
   hoverText.ReleaseDC(pDC);

   CRect rect(0, 0, sz.cx, sz.cy);
   rect.InflateRect(2, 2);

   hoverText.MoveWindow(pos.x, pos.y, rect.Width(), rect.Height(), FALSE);
   this->InvalidateRect(&hRect, TRUE);
   hoverText.SetWindowText(msg);
   }
*/

void NetView::SetDrawMode(DRAW_MODE m)
   {
   this->drawMode = m;
   switch (m)
      {
      case DM_SELECT:      gpMainFrame->SetModeText("SELECT");   break;
      case DM_SELECTRECT:  gpMainFrame->SetModeText("DRAGGING");   break;
      case DM_PANNING:  gpMainFrame->SetModeText("PANNING");   break;
      case DM_ZOOMING:  gpMainFrame->SetModeText("ZOOMING");   break;
      }
   }


bool NetView::GetAnchorPoints(CPoint start, CPoint end, float alpha /*radians*/, float frac, CPoint& anchor0, CPoint& anchor1)
   {
   int lx = end.x - start.x;  // -
   int ly = end.y - start.y;  // +

   if ( (lx > 0 && ly < 0) || (lx < 0 && ly > 0))
      alpha = -alpha;

   // don't consider verticals for now
   if (lx == 0 )
      return false;

   int lxy = (int)sqrt((lx * lx) + (ly * ly));
   if (lxy == 0)
      return false;

   // anchor 0
   int dx = int(frac * lxy);
   if (start.x > end.x)
      dx = -dx;

   anchor0.x = start.x + dx;
   anchor0.y = start.y + int(dx*tan(alpha));

   anchor1.x = end.x - dx; // start.x + (lxy - dx);
   anchor1.y = anchor0.y;      // anchor 1 will have the same y (prior to rotation)


   // rotate the points
   float slope = float(end.y - start.y) / lx;
   float theta = atan(slope);  // slope to radians
   //this->RotatePoint(anchor0, start.x, start.y, theta);
   //this->RotatePoint(anchor1, start.x, start.y, theta);
   return true;
   }


void NetView::RotatePoint( CPoint& point, int x0, int y0, float theta)
   {
   // rotation
   double x1 = point.x - x0;
   double y1 = point.y - y0;

   double x2 = x1 * cos(theta) - y1 * sin(theta);
   double y2 = x1 * sin(theta) + y1 * cos(theta);

   point.x = int(x2) + x0;
   point.y = int(y2) + y0;
   }


void NetView::ClearSelection()
   { 
   for (Node* node : nodes) 
      node->selected = false;  
   
   for (Edge* edge : edges) 
      edge->selected = false; 
   
   netForm->RemoveAllProperties();
   }



bool NetView::LoadNetwork(LPCTSTR path)
   {
   //this->m_pSNLayer = new SNLayer(NULL);
   //this->m_pSNIPModel = this->m_pSNLayer->m_pSNIPModel;

   // search for file along path

   TiXmlDocument doc;
   bool ok = doc.LoadFile(path);

   if (!ok)
      {
      return false;
      }

   CString msg("Loading input file ");
   msg += path;

   gpMainFrame->SetStatusMsg(msg);

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // field_calculator

   TiXmlElement* pXmlMeta = pXmlRoot->FirstChildElement("meta");
   name = pXmlMeta->FirstChildElement("creator")->GetText();
   desc = pXmlMeta->FirstChildElement("description")->GetText();

   TiXmlElement* pXmlGraph = pXmlRoot->FirstChildElement("graph");

   TiXmlElement* pXmlAttributes = pXmlGraph->FirstChildElement("attributes");

   // node attributes
   TiXmlElement* pXmlAttr = pXmlAttributes->FirstChildElement("attribute");
   while (pXmlAttr != NULL)
      {
      LPTSTR id = NULL, title = NULL, type = NULL;

      XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                            { "id",    TYPE_STRING,   &id,      true, 0 },
                            { "title", TYPE_STRING,   &title,   true, 0 },
                            { "type",  TYPE_STRING,   &type,   true, 0 },
                            { NULL,    TYPE_NULL,     NULL,     true, 0 } };

      if (TiXmlGetAttributes(pXmlAttr, attrs, path, NULL) == false)
         return false;

      Node::attrIDs.push_back(id);
      Node::attrLabels.push_back(title);
      Node::attrTypes.push_back(type);
      Node::attrMap[id] = (int)Node::attrIDs.size() - 1;
      pXmlAttr = pXmlAttr->NextSiblingElement("attribute");
      }

   // edge attributes
   pXmlAttributes = pXmlAttributes->NextSiblingElement("attributes");
   pXmlAttr = pXmlAttributes->FirstChildElement("attribute");
   while (pXmlAttr != NULL)
      {
      LPTSTR id = NULL, title = NULL, type = NULL;

      XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                            { "id",    TYPE_STRING,   &id,      true, 0 },
                            { "title", TYPE_STRING,   &title,   true, 0 },
                            { "type",  TYPE_STRING,   &type,   true, 0 },
                            { NULL,    TYPE_NULL,     NULL,     true, 0 } };

      if (TiXmlGetAttributes(pXmlAttr, attrs, path, NULL) == false)
         return false;

      Edge::attrIDs.push_back(id);
      Edge::attrLabels.push_back(title);
      Edge::attrTypes.push_back(type);
      Edge::attrMap[id] = (int)Edge::attrIDs.size() - 1;
      pXmlAttr = pXmlAttr->NextSiblingElement("attribute");
      }

   TiXmlElement* pXmlNodes = pXmlGraph->FirstChildElement("nodes");

   TiXmlElement* pXmlNode = pXmlNodes->FirstChildElement("node");
   this->maxNodeInfluence = 0;
   this->maxNodeDegree = 0;

   while (pXmlNode != NULL)
      {
      LPTSTR id = NULL, label = NULL;

      XML_ATTR nattrs[] = { // attr          type           address       isReq checkCol
                            { "id",    TYPE_STRING,   &id,      false, 0 },
                            { "label", TYPE_STRING,   &label,   false, 0 },
                            { NULL,    TYPE_NULL,     NULL,     false, 0 } };

      if (TiXmlGetAttributes(pXmlNode, nattrs, path, NULL) == false)
         return false;

      Node* pNode = new Node;
      pNode->pSNNode->m_id = id;
      pNode->pSNNode->m_name = label;
      pNode->displayLabel = label;
      pNode->index = (int) this->nodes.size();
      pNode->attrs.resize(Node::attrLabels.size());

      TiXmlElement* pXmlAttr = pXmlNode->FirstChildElement("attvalues")->FirstChildElement("attvalue");
      while (pXmlAttr != nullptr)
         {
         std::string attrFor = pXmlAttr->Attribute("for");
         std::string attrVal = pXmlAttr->Attribute("value");
         int attrIndex = pNode->attrMap[attrFor.c_str()];

         if (pNode->attrTypes[attrIndex] == "float")
            pNode->attrs[attrIndex] = VData((float)atof(attrVal.c_str()));
         else if (pNode->attrTypes[attrIndex] == "string")
            pNode->attrs[attrIndex] = VData(attrVal.c_str());
         else
            pNode->attrs[attrIndex] = VData(atoi(attrVal.c_str()));

         pXmlAttr = pXmlAttr->NextSiblingElement("attvalue");
         }

      TiXmlElement* pXmlVizColor = pXmlNode->FirstChildElement("viz:color");
      TiXmlElement* pXmlVizPosition = pXmlNode->FirstChildElement("viz:position");
      TiXmlElement* pXmlVizSize = pXmlNode->FirstChildElement("viz:size");

      int r = 0, g = 0, b = 0;
      double a = 0;
      pXmlVizColor->Attribute("r", &r);
      pXmlVizColor->Attribute("g", &g);
      pXmlVizColor->Attribute("b", &b);
      pXmlVizColor->Attribute("a", &a);

      int x = 0, y = 0;
      pXmlVizPosition->Attribute("x", &x);
      pXmlVizPosition->Attribute("y", &y);
      //pXmlVizPosition->Attribute("z", &z);

      int tIndex = Node::attrMap["type"];
      int rIndex = Node::attrMap["reactivity"];
      int iIndex = Node::attrMap["influence"];

      pNode->attrs[rIndex].GetAsFloat(pNode->pSNNode->m_reactivity);
      pNode->attrs[iIndex].GetAsFloat(pNode->pSNNode->m_influence);

      std::string nodeType = pNode->attrs[tIndex].GetAsString();
      if (nodeType == "Assessor")
         { pNode->pSNNode->m_nodeType = NT_ASSESSOR; nodeCountASS++; }
      else if (nodeType == "Engager") 
         { pNode->pSNNode->m_nodeType = NT_ENGAGER; nodeCountENG++; }
      else if (nodeType == "Input") 
         { pNode->pSNNode->m_nodeType = NT_INPUT_SIGNAL; nodeCountIS++; }
      else if (nodeType == "Network Actor") 
         {pNode->pSNNode->m_nodeType = NT_NETWORK_ACTOR; nodeCountNA++; }
      else if (nodeType == "Landscape Actor") 
         { pNode->pSNNode->m_nodeType = NT_LANDSCAPE_ACTOR; nodeCountLA++; }

      nodeCount++;

      // by default, nodes are sized by reactivity, colored by Influence
      pNode->color = ::WRColorRamp(0, 1, pNode->pSNNode->m_influence);
      //pNode->color.r = (char)r;
      //pNode->color.g = (char)g;
      //pNode->color.b = (char)b;
      //pNode->color.a = (char)(int)(a * 255);
      pNode->nodeSize = MIN_NODE_SIZE +  int(pNode->pSNNode->m_reactivity * MAX_NODE_SIZE);
      pNode->x = x;
      pNode->y = y;

      if (pNode->pSNNode->m_influence > this->maxNodeInfluence)
         this->maxNodeInfluence = pNode->pSNNode->m_influence;

      this->nodes.push_back(pNode);

      pXmlNode = pXmlNode->NextSiblingElement("node");
      }

   // get edges
   TiXmlElement* pXmlEdges = pXmlGraph->FirstChildElement("edges");

   TiXmlElement* pXmlEdge = pXmlEdges->FirstChildElement("edge");
   while (pXmlEdge != NULL)
      {
      LPTSTR id = NULL, source = NULL, target = NULL;

      XML_ATTR eattrs[] = { // attr       type           address       isReq checkCol
                            { "id",      TYPE_STRING,   &id,      true, 0 },
                            { "source",  TYPE_STRING,   &source,   true, 0 },
                            { "target",  TYPE_STRING,   &target,   true, 0 },
                            { NULL,    TYPE_NULL,     NULL,     false, 0 } };

      if (TiXmlGetAttributes(pXmlEdge, eattrs, path, NULL) == false)
         return false;

      Edge* pEdge = new Edge;
      pEdge->pSNEdge->m_id = id;
      pEdge->index = (int)this->edges.size();

      std::string _source(source);
      std::string _target(target);

      pEdge->pFromNode = this->FindNode(_source);
      pEdge->pToNode = this->FindNode(_target);

      ASSERT(pEdge->pFromNode != nullptr);
      ASSERT(pEdge->pToNode != nullptr);

      pEdge->pSNEdge->m_name = pEdge->pFromNode->pSNNode->m_name + "->" + pEdge->pToNode->pSNNode->m_name;
      pEdge->displayLabel = pEdge->pSNEdge->m_name;
      
      // add in/out edges to appropriate nodes
      pEdge->pFromNode->outEdges.push_back(pEdge);
      pEdge->pToNode->inEdges.push_back(pEdge);

      pEdge->pFromNode->degree++;
      pEdge->pFromNode->outDegree++;
      pEdge->pToNode->degree++;
      pEdge->pToNode->inDegree++;

      if ( pEdge->pFromNode->degree > this->maxNodeDegree)
         this->maxNodeDegree = pEdge->pFromNode->degree;
      if (pEdge->pToNode->degree > this->maxNodeDegree)
         maxNodeDegree = pEdge->pToNode->degree;

      pEdge->attrs.resize(Edge::attrIDs.size());
      // <attvalues>
      TiXmlElement* pXmlAttr = pXmlEdge->FirstChildElement("attvalues")->FirstChildElement("attvalue");
      while (pXmlAttr != nullptr)
         {
         std::string attrFor = pXmlAttr->Attribute("for");
         std::string attrVal = pXmlAttr->Attribute("value");

         int attrIndex = pEdge->attrMap[attrFor.c_str()];

         if (pEdge->attrTypes[attrIndex] == "float")
            pEdge->attrs[attrIndex] = VData((float)atof(attrVal.c_str()));
         else if (pEdge->attrTypes[attrIndex] == "string")
            pEdge->attrs[attrIndex] = VData(attrVal.c_str());
         else
            pEdge->attrs[attrIndex] = VData(atoi(attrVal.c_str()));

         pXmlAttr = pXmlAttr->NextSiblingElement("attvalue");
         }

      TiXmlElement* pXmlVizColor = pXmlEdge->FirstChildElement("viz:color");
      TiXmlElement* pXmlVizThick = pXmlEdge->FirstChildElement("viz:thickness");
      //TiXmlElement* pXmlVizShape = pXmlEdge->FirstChildElement("viz:shape");

      int r = 0, g = 0, b = 0;
      double a = 0;
      double thickness = 0;
      pXmlVizColor->Attribute("r", &r);
      pXmlVizColor->Attribute("g", &g);
      pXmlVizColor->Attribute("b", &b);
      pXmlVizColor->Attribute("a", &a);
      pXmlVizThick->Attribute("value", &thickness);

      pEdge->color = ::WRColorRamp(0, 1, pEdge->pSNEdge->m_signalStrength);
      //pEdge->color.r = (char)r;
      //pEdge->color.g = (char)g;
      //pEdge->color.b = (char)b;
      //pEdge->color.a = (char)(int)(a * 255);
      pEdge->thickness = int(0.5f + 5 * pEdge->pSNEdge->m_trust);

      int sIndex = Node::attrMap["ss"];
      int tIndex = Node::attrMap["trust"];
      int iIndex = Node::attrMap["influence"];

      pEdge->attrs[sIndex].GetAsFloat(pEdge->pSNEdge->m_signalStrength);
      pEdge->attrs[tIndex].GetAsFloat(pEdge->pSNEdge->m_trust);
      pEdge->attrs[iIndex].GetAsFloat(pEdge->pSNEdge->m_influence);

      this->edges.push_back(pEdge);

      pXmlEdge = pXmlEdge->NextSiblingElement("edge");
      }


   SampleNodes(NT_LANDSCAPE_ACTOR, 20);

   msg.Format("Loaded %i node, %i edges", nodes.size(), edges.size());
   gpMainFrame->SetStatusMsg(msg);
   this->OnZoomfull();

   OnNodeColor();
   OnNodeSize();
   OnNodeLabel();
   OnEdgeColor();
   OnEdgeSize();
   OnEdgeLabel();

   return true;
   }


void NetView::OnDarkMode()
   {
   darkMode = !darkMode; // gpMainFrame->IsChecked(IDD_DARKMODE);
   this->Invalidate();
   }


void NetView::OnUpdateDarkmode(CCmdUI* pCmdUI)
   {
   pCmdUI->Enable();
   if ( pCmdUI->m_nID == ID_DARKMODE)
      pCmdUI->SetCheck(darkMode ? 1 : 0);
   }

void NetView::OnShowEdgeLabels()
   {
   showEdgeLabels = ! showEdgeLabels; // gpMainFrame->IsChecked(ID_SHOWEDGELABELS);
   this->Invalidate();
   }


void NetView::OnUpdateShowEdgeLabels(CCmdUI* pCmdUI)
   {
   pCmdUI->SetCheck(showEdgeLabels ? 1 : 0);
   }

void NetView::OnShowNodeLabels()
   {
   showNodeLabels = !showNodeLabels; // gpMainFrame->IsChecked(IDD_SHOWNODELABELS);
   this->Invalidate();
   }

void NetView::OnUpdateShowNodeLabels(CCmdUI* pCmdUI)
   {
   pCmdUI->SetCheck(showNodeLabels ? 1 : 0);
   }

void NetView::OnPan()
   {
   SetDrawMode(DM_PANNING);
   }



void NetView::OnNodeSize()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_NODE_SIZE));
   int item = pCtrl->GetCurSel();

   this->nodeSizeFlag = (NODESIZE) item;

   for (Node* node : this->nodes)
      {
      float nodeSize = MIN_NODE_SIZE + (node->pSNNode->m_reactivity * MAX_NODE_SIZE);
      
      if (this->nodeSizeFlag == NS_CONNECTIONS)
         nodeSize = float(MIN_NODE_SIZE + (node->inDegree * MAX_NODE_SIZE / this->maxNodeDegree));

      else if (this->nodeSizeFlag == NS_INFLUENCE)
         nodeSize = MIN_NODE_SIZE + (node->pSNNode->m_influence * MAX_NODE_SIZE);

      node->nodeSize = int(nodeSize);
      }

   this->Invalidate();
   }

void NetView::OnNodeColor()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_NODE_COLOR));
   int item = pCtrl->GetCurSel();

   this->nodeColorFlag = (NODECOLOR)item;

   for (Node* node : this->nodes)
      {
      float _color = node->pSNNode->m_reactivity;
      if (this->nodeColorFlag == NC_CONNECTIONS)
         _color = float(node->degree);
      else if (this->nodeColorFlag == NC_INFLUENCE)
         _color = node->pSNNode->m_influence;

      node->color = ::WRColorRamp(0, 1, _color);
      }

   this->Invalidate();
   }

void NetView::OnNodeLabel()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_NODE_LABEL));
   int item = pCtrl->GetCurSel();

   this->nodeLabelFlag = (NODELABEL)item;

   for (Node* node : nodes)
      {
      switch (this->nodeLabelFlag)
         {
         case NL_LABEL:          node->displayLabel = node->pSNNode->m_name;      break;
         case NL_INFLUENCE:      node->displayLabel = std::format("{:.2f}", node->pSNNode->m_influence);      break;
         case NL_REACTIVITY:     node->displayLabel = std::format("{:.2f}", node->pSNNode->m_reactivity);     break;
         }

      }
   this->Invalidate();
   }

void NetView::OnEdgeSize()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_EDGE_SIZE));
   int item = pCtrl->GetCurSel();

   this->edgeSizeFlag = (EDGESIZE)item;

   for (Edge* edge : this->edges)
      {
      float thickness = edge->pSNEdge->m_trust;
      if (this->edgeSizeFlag == ES_SIGNALSTRENGTH)
         thickness = edge->pSNEdge->m_signalStrength;
      else if (this->edgeSizeFlag == ES_INFLUENCE)
         thickness = edge->pSNEdge->m_influence;

      edge->thickness = int(MIN_EDGE_WIDTH + (MAX_EDGE_WIDTH * thickness));
      }

   this->Invalidate();
   }

void NetView::OnEdgeColor()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_EDGE_COLOR));
   int item = pCtrl->GetCurSel();

   this->edgeColorFlag = (EDGECOLOR)item;

   for (Edge* edge : this->edges)
      {
      float _color = edge->pSNEdge->m_trust;
      if (this->edgeColorFlag == EC_SIGNALSTRENGTH)
         _color = edge->pSNEdge->m_signalStrength;
      else if (this->edgeColorFlag == EC_INFLUENCE)
         _color = edge->pSNEdge->m_influence;

      edge->color = ::WRColorRamp(0, 1, _color); 
      }

   this->Invalidate();
   }

void NetView::OnEdgeLabel()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_EDGE_LABEL));
   int item = pCtrl->GetCurSel();

   this->edgeLabelFlag = (EDGELABEL)item;

   for (Edge* edge : edges)
      {
      switch (this->edgeLabelFlag)
         {
         case EL_LABEL:          edge->displayLabel = edge->pSNEdge->m_name;      break;
         case EL_TRUST:          edge->displayLabel = std::format("tr:{:.2f}", edge->pSNEdge->m_trust);              break;
         case EL_SIGNALSTRENGTH: edge->displayLabel = std::format("ss:{:.2f}", edge->pSNEdge->m_signalStrength);     break;
         case EL_INFLUENCE:      edge->displayLabel = std::format("inf:{:.2f}", edge->pSNEdge->m_influence);          break;
         }
      }
   this->Invalidate();
   }


void NetView::OnShowNeighborhood()
   {
   pDC = this->GetDC();

   DrawNetwork(*pDC, true, DF_GRAYED);
   DrawNetwork(*pDC, false, DF_NEIGHBORHOODS);
   //for (Node* node : this->nodes)
   //   {
   //   if (node->selected)
   //      { 
   //      }
   //   }
   }


void NetView::OnCurvedEdges()
   {
   this->edgeType = 1-this->edgeType;
   Invalidate();
   }

void NetView::DrawPoint(CDC& dc, CPoint pt, int radius, float aspect)
   {
   dc.Ellipse(pt.x - int(radius/ aspect), pt.y - radius, pt.x + int(radius / aspect), pt.y + radius);
   }


void NetView::ComputeEdgeArc(Edge* edge, float alpha)
   {
   // step 1 - determine the tangent line slopes for the start and ending points

   if (edge->pToNode->x == edge->pFromNode->x)
      return;


   float mEdge = float(edge->pToNode->y - edge->pFromNode->y) / (edge->pToNode->x - edge->pFromNode->x);

   float theta = atan(mEdge);  // slope to radians


   // compute ellipse params
   float m0 = theta + alpha;
   float x0 = (float) edge->pFromNode->x;
   float y0 = (float) edge->pFromNode->y;   // WRONG - assumes ellipse origin=0,0
   float b = sqrt((x0 * m0 * y0) + (y0 * y0));
   float a = b * sqrt(x0 / (m0 * y0));


   }


void NetView::GetNetworkStats(std::string& str)
   {
   int nodeCount = (int) this->nodes.size();
   int edgeCount = (int) this->edges.size();
   int maxInDegree = 0, maxOutDegree = 0, maxNodeDegree = 0;
   float meanNodeReactivity = 0;
   float meanNodeInfluence = 0;
   float meanEdgeSignalStrength = 0;
   float meanEdgeTrust = 0;
   float networkEfficiency = 0;

   for (Node* node : nodes)
      {
      if (node->inDegree > maxInDegree)
         maxInDegree = node->inDegree;

      if (node->outDegree > maxOutDegree)
         maxOutDegree = node->outDegree;

      if (node->degree > maxNodeDegree)
         maxNodeDegree = node->degree;

      meanNodeReactivity += node->pSNNode->m_reactivity;
      meanNodeInfluence += node->pSNNode->m_influence;
      }

   meanNodeReactivity /= nodeCount;
   meanNodeInfluence /= nodeCount;

   for ( Edge* edge : edges)
      {
      meanEdgeSignalStrength += edge->pSNEdge->m_signalStrength;
      meanEdgeTrust += edge->pSNEdge->m_trust;
      }

   meanEdgeSignalStrength /= edgeCount;
   meanEdgeTrust /= edgeCount;

   str = std::format("Nodes: {}, Edges: {}\nMax Degrees: In={}, Out={}, Total={}\nMean Node Reactivity: {:.2f}\nMean Node Influence: {:.2f}\nMean Edge Signal Strength: {:.2f}\nMean Edge Trust: {:.2f}\n",
      nodeCount, edgeCount, maxInDegree, maxOutDegree, maxNodeDegree, meanNodeReactivity, meanNodeInfluence, meanEdgeSignalStrength, meanEdgeTrust);

   return;
   }



int NetView::SampleNodes(SNIP_NODETYPE nodeType, int count)
   {
   int total = 0;
   switch (nodeType)
      {
      case NT_UNKNOWN:  break;
      case NT_INPUT_SIGNAL: total = nodeCountIS;   break;
      case NT_NETWORK_ACTOR: total = nodeCountNA;   break;
      case NT_ASSESSOR: total = nodeCountASS;   break;
      case NT_ENGAGER: total = nodeCountENG;   break;
      case NT_LANDSCAPE_ACTOR: total = nodeCountLA;   break;
      }

   float frac = float(count) / total;

   int i = 0;
   float infrac = 1.0f / frac;
   int inc = int( infrac + 0.1f);
   int showCount = 0;
   for (Node* node : nodes)
      {
      if (node->pSNNode->m_nodeType == nodeType)
         {
         bool show = false;
         if (i % inc == 0)
            {
            show = true;
            showCount++;
            }

         node->show = show;
         for (Edge* edge : node->inEdges)
            edge->show = show;
         for (Edge* edge : node->outEdges)
            edge->show = show;

         i++;
         }
      }
   return showCount;
   }




/*
bool NetView::AttachSNIPModel(SNIPModel* pModel)
   {
   m_pSNIPModel = pModel;
   m_pSNLayer = pModel->m_pSNLayer;

   // build nodes
   int index = 0;
   for ( SNNode *pSNNode : m_pSNLayer->m_nodes)
      {      
      Node* node = new Node;
      node->pSNNode = pSNNode;
      node->displayLabel = pSNNode->m_name;
      node->index = index++;
      

      // by default, nodes are sized by reactivity, colored by Influence
      node->color = ::WRColorRamp(0, 1, pSNNode->m_influence);
      node->nodeSize = MIN_NODE_SIZE + (pSNNode->m_reactivity * MAX_NODE_SIZE);
      //node->x = x;
      //node->y = y;
      this->nodes.push_back(node);
      }

   index = 0;
   for (SNEdge* pSNEdge: m_pSNLayer->m_edges)
      {
      Edge* edge = new Edge;
      edge->index = index++;
      edge->pSNEdge = pSNEdge;
      edge->displayLabel = pSNEdge->m_name;

      edge->pFromNode = FindNode(pSNEdge->Source()->m_id);
      edge->pFromNode->outEdges.push_back(edge);
     
      edge->pToNode = FindNode(pSNEdge->Target()->m_id);
      edge->pToNode->inEdges.push_back(edge);

      edge->pFromNode->degree++;
      edge->pFromNode->outDegree++;
      edge->pToNode->degree++;
      edge->pToNode->inDegree++;

      if (edge->pFromNode->degree > this->maxNodeDegree)
         maxNodeDegree = edge->pFromNode->degree;
      if (edge->pToNode->degree > this->maxNodeDegree)
         maxDegree = edge->pToNode->degree;

      
      this->edges.push_back(edge);
      }

   LayoutNetwork();

   SampleNodes(NT_LANDSCAPE_ACTOR, 20);

   CString msg;
   msg.Format("Loaded %i node, %i edges", nodes.size(), edges.size());
   gpMainFrame->SetStatusMsg(msg);

   this->OnZoomfull();

   }
   */

void NetView::LayoutNetwork()
   {
   int nNodes = (int)this->nodes.size();

   int nAssessors = 0;
   int nEngagers = 0;
   int nLAs = 0;

   int nNA_Assessors = 0;
   int nNA_Engagers = 0;
   int nNA_Both = 0;
   int nNA_Only = 0;

   for (Node* node : this->nodes)
   {
      switch (node->pSNNode->m_nodeType)
      {
      case  NT_INPUT_SIGNAL:  break;
      case  NT_NETWORK_ACTOR:
      {

         bool assessor = false;
         bool engager = false;

         //bool counted = false;
         for (int j = 0; j < (int) node->pSNNode->m_inEdges.size(); j++)
         {
            if (node->pSNNode->m_inEdges[j]->m_pFromNode->m_nodeType == NT_ASSESSOR)
            {
               nNA_Assessors++;
               assessor = true;
               //counted = true;
               break;
            }
         }

         for (int j = 0; j < (int) node->pSNNode->m_outEdges.size(); j++)
         {
            if ( node->pSNNode->m_outEdges[j]->m_pToNode->m_nodeType == NT_ENGAGER)
            {
               nNA_Engagers++;
               engager = true;
               //counted = true;
               break;
            }
         }

         if (assessor && engager)
            nNA_Both++;
         else if (!assessor && !engager)
            nNA_Only++;
      } 
      break;

      case NT_ASSESSOR: nAssessors++;  break;
      case NT_ENGAGER:  nEngagers++;   break;
      case NT_LANDSCAPE_ACTOR: nLAs++; break;
      }
   }



   // gather various stats first
   for (Node *pNode : this->nodes)
      {
      if (pNode->pSNNode->m_nodeType == NT_NETWORK_ACTOR)
         {
         bool assessor = false;
         bool engager = false;

         //bool counted = false;
         for (int j = 0; j < (int)pNode->pSNNode->m_inEdges.size(); j++)
            {
            if (pNode->pSNNode->m_inEdges[j]->m_pFromNode->m_nodeType == NT_ASSESSOR)
               {
               nNA_Assessors++;
               assessor = true;
               //counted = true;
               break;
               }
            }

         for (int j = 0; j < (int)pNode->pSNNode->m_outEdges.size(); j++)
            {
            if (pNode->pSNNode->m_outEdges[j]->m_pToNode->m_nodeType == NT_ENGAGER)
               {
               nNA_Engagers++;
               engager = true;
               //counted = true;
               break;
               }
            }

         if (assessor && engager)
            nNA_Both++;
         else if (!assessor && !engager)
            nNA_Only++;
         }
      }

   // updating y positions
   int na = 0, nnLeft = 0, nnRight = 0, nnCenter = 0, ne = 0, nla = 0;
   int x = 0, y = 0;
   const int xMax = 1000;
   const int yMax = 1000;

   // get number of network actor nodes of various types
   for (Node* pNode : this->nodes )
      {
      // skip lanscape actors, replace with groups below
      //if (pNode->m_nodeType == NT_LANDSCAPE_ACTOR)
      //   continue;

      // bounds for actor types:
      // | INPUT  |  ASSESSOR   |  LA->ASS | LA_UNAFFILATED | LA->BOTH | LA_UNAFFILIATED | LA_ENG | ENGAGER | LANDSCAPE_ACTOR
      //      0       .12           .24        .30-.45           .5         .55-.70         0.76       .88         1.0   |
      // -.6     .6            .18        .28              .47        .53               .72       .82      .94         1.06                             

      //string color = "black";
      switch (pNode->pSNNode->m_nodeType)
         {
         case NT_INPUT_SIGNAL:    x = 0;               y = int(yMax / 2);                          break;
         case NT_ASSESSOR:        x = int(0.12f * xMax); y = int(float(yMax * na) / nAssessors); na++; break;
         case NT_ENGAGER:         x = int(0.88f * xMax); y = int(float(yMax * ne) / nEngagers);  ne++; break;
         case NT_LANDSCAPE_ACTOR: x = xMax; y = int(float(yMax * nla) / nLAs);  nla++; break; 
         case NT_NETWORK_ACTOR:
            {
            bool assessor = false;
            for (int j = 0; j < (int)pNode->pSNNode->m_inEdges.size(); j++)
               if (pNode->pSNNode->m_inEdges[j]->m_pFromNode->m_nodeType == NT_ASSESSOR)
                  {
                  assessor = true;
                  break;
                  }

            bool engager = false;
            for (int j = 0; j < (int)pNode->pSNNode->m_outEdges.size(); j++)
               if (pNode->pSNNode->m_outEdges[j]->m_pToNode->m_nodeType == NT_ENGAGER)
                  {
                  engager = true;
                  break;
                  }

            // input only - left col
            if (assessor && !engager)
               {
               y = int(float(yMax) * nnLeft / nNA_Assessors);
               x = int(0.24f * xMax);
               nnLeft++;
               }
            else if (engager && !assessor)  // enager only, right column 
               {
               y = int(yMax * nnRight / nNA_Engagers);
               x = int(0.76f * xMax);
               nnRight++;
               }
            else if (engager && assessor)  // both, center column
               {
               x = int(0.5 * xMax);
               y = int(yMax * nnCenter / nNA_Both);
               nnCenter++;
               }
            else // connect to neither engager or assessor
               {
               x = int(xMax * randUnif.RandValue(0.30, 0.70));
               if (x > (0.45f * xMax) && x <= 0.50f * xMax)
                  x = int(randUnif.RandValue(0.30, 0.45) * xMax);
               if (x < (0.55f * xMax) && x > 0.50f * xMax)
                  x = int(randUnif.RandValue(0.55, 0.70) * xMax);
               y = int(randUnif.RandValue(0, yMax));
               }
            break;
            }
         }

      const int maxNodeSize = 20;
      float size = maxNodeSize / 2;
      if (!isnan(pNode->pSNNode->m_influence) && !isinf(pNode->pSNNode->m_influence))
         size = maxNodeSize * pNode->pSNNode->m_influence / this->maxNodeInfluence;
      if (size <= 0)
         size = maxNodeSize / 2;

      //_AddGEFXNode(out, date, pNode->m_id.c_str(), pNode->m_name, pNode->m_nodeType, pNode->m_reactivity, pNode->m_influence, size, x, y);
      }
   }

