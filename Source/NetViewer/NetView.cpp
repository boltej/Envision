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


//using namespace Gdiplus;


extern CMainFrame* gpMainFrame;


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


   ////// test /////////////////////
   CPoint p0(200,600);
   CPoint p1(300,100);  // horizontal
   CPoint p2(200,150);  // horizontal
   CPoint a0, a1;
   GetAnchorPoints(p1, p0, PI /8, .2f, a0, a1);
   
   DrawPoint(dc, p0, 4, 1);
   DrawPoint(dc, p1, 4, 1);
   DrawPoint(dc, a0, 2, 1);
   DrawPoint(dc, a1, 2, 1);
   
   //GetAnchorPoints(p2, p0, PI / 8, .2f, a0, a1);
   //DrawPoint(dc, p0, 4, 1);
   //DrawPoint(dc, p2, 4, 1);
   //DrawPoint(dc, a0, 2, 1);
   //DrawPoint(dc, a1, 2, 1);
   
   dc.TextOut(p0.x, p0.y, CString("p0"));
   dc.TextOut(p1.x, p1.y, CString("p1"));
   //dc.TextOut(p2.x, p2.y, CString("p2"));

   //DrawNetwork(dc,true,DF_NORMAL);

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
   for (Edge* edge : edges)
      {
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
         else if (drawFlag == DF_SHORTESTROUTE)
            ;

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
         dc.TextOut(x, y, CString(edge->label.c_str()));
         }
      i++;
      }

   // then nodes:
   i = 0;

   for (Node* node : nodes)
      {
      if (i == 0
         || (nodes[i - 1]->color.a != node->color.a) || (nodes[i - 1]->color.r != node->color.r) || (nodes[i - 1]->color.g != node->color.g) || (nodes[i - 1]->color.b != node->color.b)
         || node->reactivity != nodes[i-1]->reactivity || node->selected != nodes[i-1]->selected)
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
         else if (drawFlag == DF_SHORTESTROUTE)
            ;
         pen.DeleteObject();
         pen.CreatePen(PS_SOLID, 1, penColor);

         brush.DeleteObject();
         brush.CreateSolidBrush(fillColor);
         dc.SelectObject(&pen);
         dc.SelectObject(&brush);
         }

      CPoint pos(node->x, node->y);

      const int NODESIZE_MAX = 24; // radius, pxs
      int nodeSize = 3;

      switch (nodeSizeFlag)
         {
         case NS_INFLUENCE:   nodeSize += int(NODESIZE_MAX * node->influence);  break;
         case NS_REACTIVITY:  nodeSize += int(NODESIZE_MAX * node->reactivity); break;
         case NS_CONNECTIONS: nodeSize += int(NODESIZE_MAX * node->degree/this->maxDegree);  break;
         }
      
      if (nodeSize > NODESIZE_MAX+3)
         nodeSize = NODESIZE_MAX+3;

      // logical coords
      DrawPoint(dc, pos, nodeSize, aspect);
      //dc.Ellipse(pos.x - int(nodeSize/aspect), pos.y-nodeSize, pos.x + int(nodeSize/aspect), pos.y + nodeSize);

      if (this->showNodeLabels)
         {
         //CString out;
         //out.Format("%s, (%i,%i)(dc)", node->label.c_str(), pos.x, pos.y);
         //dc.TextOut(pos.x, pos.y - nodeSize, out);   // logical coords
         dc.TextOut(pos.x, pos.y - nodeSize, CString(node->label.c_str()));
         }
     
      dc.LPtoDP(&pos);
      node->xDC = pos.x;
      node->yDC = pos.y;

      // determeine node radius in device coords
      CPoint pt0(0, 0);
      CPoint pt1(nodeSize, 0);
      dc.LPtoDP(&pt0);
      dc.LPtoDP(&pt1);
      node->nodeSizeDC = pt1.x - pt0.x;
      i++;
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
                  msg.Format("%s: SS=%.2f, T=%.2f, I=%.2f", pEdge->label.c_str(), pEdge->signalStrength, pEdge->trust, pEdge->influence); // point.x, point.y);
                  gpMainFrame->SetStatusMsg(msg);
                  }
               else
                  {
                  Node* pNode = (Node*)pElem;
                  msg.Format("%s: R=%.2f, I=%.2f", pNode->label.c_str(), pNode->reactivity, pNode->influence); // point.x, point.y);
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
      pNode->id = id;
      pNode->label = label;
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

      int rIndex = Node::attrMap["reactivity"];
      int iIndex = Node::attrMap["influence"];

      pNode->attrs[rIndex].GetAsFloat(pNode->reactivity);
      pNode->attrs[iIndex].GetAsFloat(pNode->influence);

      pNode->color.r = (char)r;
      pNode->color.g = (char)g;
      pNode->color.b = (char)b;
      pNode->color.a = (char)(int)(a * 255);

      pNode->x = x;
      pNode->y = y;

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
      pEdge->id = id;
      pEdge->index = (int)this->edges.size();

      std::string _source(source);
      std::string _target(target);

      pEdge->pFromNode = this->FindNode(_source);
      pEdge->pToNode = this->FindNode(_target);

      ASSERT(pEdge->pFromNode != nullptr);
      ASSERT(pEdge->pToNode != nullptr);

      pEdge->label = pEdge->pFromNode->label + "->" + pEdge->pToNode->label;
      pEdge->displayLabel = pEdge->label;
      pEdge->pFromNode->outEdges.push_back(pEdge);
      pEdge->pToNode->inEdges.push_back(pEdge);

      pEdge->pFromNode->degree++;
      pEdge->pFromNode->outDegree++;
      pEdge->pToNode->degree++;
      pEdge->pToNode->inDegree++;

      if ( pEdge->pFromNode->degree > this->maxDegree)
         maxDegree = pEdge->pFromNode->degree;
      if (pEdge->pToNode->degree > this->maxDegree)
         maxDegree = pEdge->pToNode->degree;

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

      pEdge->color.r = (char)r;
      pEdge->color.g = (char)g;
      pEdge->color.b = (char)b;
      pEdge->color.a = (char)(int)(a * 255);
      pEdge->thickness = (int)thickness;

      int sIndex = Node::attrMap["ss"];
      int tIndex = Node::attrMap["trust"];
      int iIndex = Node::attrMap["influence"];

      pEdge->attrs[sIndex].GetAsFloat(pEdge->signalStrength);
      pEdge->attrs[tIndex].GetAsFloat(pEdge->trust);
      pEdge->attrs[iIndex].GetAsFloat(pEdge->influence);

      this->edges.push_back(pEdge);

      pXmlEdge = pXmlEdge->NextSiblingElement("edge");
      }

   msg.Format("Loaded %i node, %i edges", nodes.size(), edges.size());
   gpMainFrame->SetStatusMsg(msg);

   this->OnZoomfull();

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


const int MAX_NODE_SIZE = 20;  // radius, logical coords
const int MIN_NODE_SIZE = 3;

void NetView::OnNodeSize()
   {
   CMFCRibbonComboBox* pCtrl = DYNAMIC_DOWNCAST(CMFCRibbonComboBox, gpMainFrame->m_wndRibbonBar.FindByID(ID_NODE_SIZE));
   int item = pCtrl->GetCurSel();

   this->nodeSizeFlag = (NODESIZE) item;

   for (Node* node : this->nodes)
      {
      float nodeSize = MIN_NODE_SIZE + (node->reactivity * MAX_NODE_SIZE);
      if (this->nodeSizeFlag == NS_CONNECTIONS)
         nodeSize = MIN_NODE_SIZE + (node->inDegree * MAX_NODE_SIZE / this->maxDegree);
      else if (this->nodeSizeFlag == NS_INFLUENCE)
         nodeSize = MIN_NODE_SIZE + (node->influence * MAX_NODE_SIZE);

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
      float _color = node->reactivity;
      if (this->nodeColorFlag == NC_CONNECTIONS)
         _color = node->influence;
      else if (this->nodeColorFlag == NC_INFLUENCE)
         _color = node->influence;

      node->color = ::RColorRamp(0, 1, _color);
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
         case NL_LABEL:          node->displayLabel = node->label;      break;
         case NL_INFLUENCE:      node->displayLabel = std::format("{}", node->influence);      break;
         case NL_REACTIVITY:     node->displayLabel = std::format("{}", node->reactivity);     break;
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
      float thickness = edge->trust;
      if (this->edgeSizeFlag == ES_SIGNALSTRENGTH)
         thickness = edge->signalStrength;
      else if (this->edgeSizeFlag == ES_INFLUENCE)
         thickness = edge->influence;

      edge->thickness = int(0.5f + 5 * thickness);
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
      float _color = edge->trust;
      if (this->edgeColorFlag == EC_SIGNALSTRENGTH)
         _color = edge->signalStrength;
      else if (this->edgeColorFlag == EC_INFLUENCE)
         _color = edge->influence;

      edge->color = ::RColorRamp(0, 1, _color); 
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
      switch (this->nodeLabelFlag)
         {
         case EL_LABEL:          edge->displayLabel = edge->label;      break;
         case EL_TRUST:          edge->displayLabel = std::format("{}", edge->trust);              break;
         case EL_SIGNALSTRENGTH: edge->displayLabel = std::format("{}", edge->signalStrength);     break;
         case EL_INFLUENCE:      edge->displayLabel = std::format("{}", edge->influence);          break;
         }
      }
   this->Invalidate();
   }


void NetView::OnShowNeighborhood()
   {
   pDC = this->GetDC();

   DrawNetwork(*pDC, true, DF_GRAYED);
   DrawNetwork(*pDC, false, DF_NEIGHBORHOODS);
   for (Node* node : this->nodes)
      {
      if (node->selected)
         { 
         }
      }
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
   float x0 = edge->pFromNode->x;
   float y0 = edge->pFromNode->y;   // WRONG - assumes ellipse origin=0,0
   float b = sqrt((x0 * m0 * y0) + (y0 * y0));
   float a = b * sqrt(x0 / (m0 * y0));


   }



