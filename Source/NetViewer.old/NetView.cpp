#include "pch.h"
#include "NetView.h"
#include "MainFrm.h"

#include "resource.h"

#include <tixml.h>

#include <gdiplus.h>
#include <memory>


using namespace Gdiplus;


extern CMainFrame* gpMainFrame;


std::vector<std::string>   Node::attrLabels;
std::vector<std::string>   Node::attrTypes;
std::map<std::string, int> Node::attrMap;


BEGIN_MESSAGE_MAP(NetView, CWnd)
   ON_WM_PAINT()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEMOVE()
   ON_WM_RBUTTONDOWN()
   ON_COMMAND(ID_VIEW_ZOOMFULL, &NetView::OnViewZoomfull)
   ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMFULL, &NetView::OnUpdateViewZoomfull)
END_MESSAGE_MAP()


void NetView::OnPaint()
   {
   CPaintDC dc(this); // device context for painting

   CRect rect;
   GetClientRect(rect);
   int cWidth = rect.Width();
   int cHeight = rect.Height();

   dc.SetMapMode(MM_ISOTROPIC);
   dc.SetWindowExt(xMax - xMin, yMax - yMin);
   dc.SetViewportExt(cWidth, cHeight);
   dc.SetWindowOrg(xMin, yMin);


   clock_t startTime = clock();

   Graphics graphics(dc.m_hDC);    // create a gdiplus graphics surface
   graphics.Clear(Color(255, 255, 255, 255));

   //graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

   //std::auto_ptr<Gdiplus::Pen> pPen;

   Color outlineColor(255, 134, 120, 0);  // black
   //std::auto_ptr<Gdiplus::Brush> pBrush(nullptr);
   Pen *pPen = new Pen(Color(edges[0]->color.a, edges[0]->color.r, edges[0]->color.g, edges[0]->color.b));
   SolidBrush* pBrush = new SolidBrush(Color(255,255,255,255));

   // first edges
   CPoint start;
   CPoint end;
   int i = 0;
   for (Edge* edge : edges)
      {
      Edge* edge = edges[i];

      if (i == 0
         || (edges[i - 1]->color.a != edge->color.a) || (edges[i - 1]->color.r != edge->color.r) || (edges[i - 1]->color.g != edge->color.g) || (edges[i - 1]->color.b != edge->color.b)
         || edge->thickness != edges[i]->thickness)
         {
         pPen->SetColor(Color(edges[i]->color.a, edges[i]->color.r, edges[i]->color.g, edges[i]->color.b));
         pPen->SetWidth(Gdiplus::REAL(edge->thickness / 5));
         //pPen.reset(new Pen(Color(edges[i]->color.a, edges[i]->color.r, edges[i]->color.g, edges[i]->color.b), Gdiplus::REAL(edge->thickness / 5)));
         }

      start.x = edge->pFromNode->x;
      start.y = edge->pFromNode->y;
      end.x = edge->pToNode->x;
      end.y = edge->pToNode->y;

      dc.LPtoDP(&start);
      dc.LPtoDP(&end);
 
      graphics.DrawLine(pPen, start.x, start.y, end.x, end.y);
      i++;
      }

   clock_t finish = clock();
   float edgeTime = (float)(finish - startTime) / CLOCKS_PER_SEC;
   const int NODESIZE_MAX = 20;
   // then nodes:
   for (Node* node : nodes)
      {
      CPoint pos(node->x, node->y);
      dc.LPtoDP(&pos);
   
      Color color(node->color.a, node->color.r, node->color.g, node->color.b);
      pBrush->SetColor(color);

      int nodeSize = 6 + NODESIZE_MAX * node->reactivity;
      
      graphics.FillEllipse(pBrush, pos.x - int(nodeSize / 2), pos.y - int(nodeSize / 2), nodeSize, nodeSize);
      }

   delete pBrush;
   delete pPen;

   graphics.ReleaseHDC(dc.m_hDC);

   finish = clock();
   float duration = (float)(finish - startTime) / CLOCKS_PER_SEC;
   //CString msg;
   //msg.Format("Finished drawing network in %.2f seconds (Edges: %.2f seconds)", duration, edgeTime);
   //gpMainFrame-> SetStatusMsg(msg);

   // Do not call CWnd::OnPaint() for painting messages
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

      Node::attrLabels.push_back(title);
      Node::attrMap[title] = Node::attrLabels.size();
      Node::attrTypes.push_back(type);
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
      pNode->attrs.resize(Node::attrLabels.size());

      TiXmlElement* pXmlAttr = pXmlNode->FirstChildElement("attvalues")->FirstChildElement("attvalue");
      while (pXmlAttr != nullptr)
         {
         std::string attrFor = pXmlAttr->Attribute("for");
         std::string attrVal = pXmlAttr->Attribute("value");
         int attrIndex = pNode->attrMap[attrFor.c_str()];
         
         if (pNode->attrTypes[attrIndex] == "float")
            pNode->attrs[attrIndex] = VData((float)atof(attrVal.c_str()));
         else if ( pNode->attrTypes[attrIndex] == "string")
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

      int rIndex = Node::attrMap["n1"];
      int iIndex = Node::attrMap["n2"];

      pNode->attrs[rIndex].GetAsFloat(pNode->reactivity);
      pNode->attrs[iIndex].GetAsFloat(pNode->influence);

      pNode->influence = 0;
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

      // <attvalues>
      TiXmlElement* pXmlAttr = pXmlEdge->FirstChildElement("attvalues")->FirstChildElement("attvalue");
      while (pXmlAttr != nullptr)
         {
         std::string attrFor = pXmlAttr->Attribute("for");
         std::string attrVal = pXmlAttr->Attribute("value");

         // DO SOMETHINGS
         pXmlAttr = pXmlAttr->NextSiblingElement("attvalue");
         }

      TiXmlElement* pXmlVizColor = pXmlEdge->FirstChildElement("viz:color");
      TiXmlElement* pXmlVizThick = pXmlEdge->FirstChildElement("viz:thickness");
      //TiXmlElement* pXmlVizShape = pXmlEdge->FirstChildElement("viz:shape");

      int r = 0, g = 0, b = 0;
      double a = 0;
      pXmlVizColor->Attribute("r", &r);
      pXmlVizColor->Attribute("g", &g);
      pXmlVizColor->Attribute("b", &b);
      pXmlVizColor->Attribute("a", &a);

      double thickness = 0;
      pXmlVizThick->Attribute("value", &thickness);


      Edge* pEdge = new Edge;
      pEdge->id = id;
      pEdge->pFromNode = this->FindNode(std::string(source));
      pEdge->pToNode = this->FindNode(std::string(target));

      pEdge->signalStrength = 0;
      pEdge->trust = 0;
      pEdge->color.r = (char)r;
      pEdge->color.g = (char)g;
      pEdge->color.b = (char)b;
      pEdge->color.a = (char)(int)(a * 255);
      pEdge->thickness = thickness;

      this->edges.push_back(pEdge);

      pXmlEdge = pXmlEdge->NextSiblingElement("edge");
      }

   msg.Format("Loaded %i node, %i edges", nodes.size(), edges.size());
   gpMainFrame->SetStatusMsg(msg);

   return true;
   }

static CPoint startPt;
static CPoint endPt;
static CPoint lastPt;
//static CRect lastRect;
static bool dragging = false;
static CDC* pDC = nullptr;


void NetView::OnLButtonDown(UINT nFlags, CPoint point)
   {
   if (!dragging)
      {
      dragging = true;
      SetCapture();
   
      CString msg;
      msg.Format("Starting Drag at (%i,%i)", point.x, point.y);
      gpMainFrame->SetStatusMsg(msg);

      startPt = lastPt = point; // save this for later use
      CRect startRect(point, point);

      pDC = this->GetDC();
      pDC->DrawDragRect(&startRect, CSize(2, 2), NULL, CSize(2, 2));
      this->ReleaseDC(pDC);
      }

   CWnd::OnLButtonDown(nFlags, point);
   }

void NetView::OnMouseMove(UINT nFlags, CPoint point)
   {
   if (dragging)
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

   CWnd::OnMouseMove(nFlags, point);
   }

void NetView::OnLButtonUp(UINT nFlags, CPoint point)
   {
   if (dragging)
      {
      dragging = false;
      ReleaseCapture();
      lastPt = point;

      CClientDC dc(this);

      // erase drag rect
      CRect lastRect(startPt, lastPt);
      dc.DrawDragRect(lastRect, CSize(2, 2), NULL, CSize(2, 2));
   

      if (startPt.x != lastPt.x && startPt.y != lastPt.y)
         {

         CRect rect;
         GetClientRect(rect);
         int cWidth = rect.Width();
         int cHeight = rect.Height();

         dc.SetMapMode(MM_ISOTROPIC);
         dc.SetWindowExt(xMax - xMin, yMax - yMin);
         dc.SetViewportExt(cWidth, cHeight);
         dc.SetWindowOrg(xMin, yMin);

         dc.DPtoLP(&startPt);
         dc.DPtoLP(&lastPt);

         xMin = min(startPt.x, lastPt.x);
         xMax = max(startPt.x, lastPt.x);
         yMin = min(startPt.y, lastPt.y);
         yMax = max(startPt.y, lastPt.y);

         CString msg;
         msg.Format("Zoom Extent: (%i,%i)-(%i,%i)", xMin, yMin, xMax, yMax);
         gpMainFrame->SetStatusMsg(msg);

         this->Invalidate();
         }


      //MoveWindow(&newRect);
      //Invalidate();
      //return;
      }
   CWnd::OnLButtonUp(nFlags, point);
   }


void NetView::OnRButtonDown(UINT nFlags, CPoint point)
   {
   // TODO: Add your message handler code here and/or call default

   CWnd::OnRButtonDown(nFlags, point);
   }






void NetView::OnViewZoomfull()
   {
   xMin = -20;
   xMax = 1020;
   yMin = -20;
   yMax = 1020;

   this->Invalidate();
   }


void NetView::OnUpdateViewZoomfull(CCmdUI* pCmdUI)
   {
   pCmdUI->Enable();
   }
