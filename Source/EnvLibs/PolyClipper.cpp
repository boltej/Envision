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
/*
===========================================================================

Project:   Generic Polygon Clipper

A new algorithm for calculating the difference, intersection,
exclusive-or or union of arbitrary polygon sets.

File:      gpc.c
Author:    Alan Murta (email: gpc@cs.man.ac.uk)
Version:   2.32
Date:      17th December 2004

Copyright: (C) 1997-2004, Advanced Interfaces Group,
University of Manchester.

This software is free for non-commercial use. It may be copied,
modified, and redistributed provided that this copyright notice
is preserved on all copies. The intellectual property rights of
the algorithms used reside with the University of Manchester
Advanced Interfaces Group.

You may not use this software, in whole or in part, in support
of any commercial product without the express consent of the
author.

There is no warranty or other guarantee of fitness of this
software for any purpose. It is provided solely "as is".

===========================================================================
*/


/*
===========================================================================
Includes
===========================================================================
*/
#include "EnvLibs.h"

#include "PolyClipper.h"
#include <stdlib.h>
#include <float.h>
#include <math.h>


/*
===========================================================================
Constants
===========================================================================
*/

#define LEFT               0
#define RIGHT              1

#define ABOVE              0
#define BELOW              1

#define CLIP               0
#define SUBJ               1

#define INVERT_TRISTRIPS   FALSE


/*
===========================================================================
Macros 
===========================================================================
*/

#define EQ(a, b)           (fabs((a) - (b)) <= GPC_EPSILON)

#define PREV_INDEX(i, n)   ((i - 1 + n) % n)
#define NEXT_INDEX(i, n)   ((i + 1    ) % n)

#define OPTIMAL(v, i, n)   ((v[PREV_INDEX(i, n)].y != v[i].y) || \
   (v[NEXT_INDEX(i, n)].y != v[i].y))

#define FWD_MIN(v, i, n)   ((v[PREV_INDEX(i, n)].vertex.y >= v[i].vertex.y) \
   && (v[NEXT_INDEX(i, n)].vertex.y > v[i].vertex.y))

#define NOT_FMAX(v, i, n)   (v[NEXT_INDEX(i, n)].vertex.y > v[i].vertex.y)

#define REV_MIN(v, i, n)   ((v[PREV_INDEX(i, n)].vertex.y > v[i].vertex.y) \
   && (v[NEXT_INDEX(i, n)].vertex.y >= v[i].vertex.y))

#define NOT_RMAX(v, i, n)   (v[PREV_INDEX(i, n)].vertex.y > v[i].vertex.y)


#define MALLOC(p, b, s, t) {if ((b) > 0) { \
   p= (t*)malloc(b); if (!(p)) { \
   fprintf(stderr, "gpc malloc failure: %s\n", s); \
   exit(0);}} else p= NULL;}

#define FREE(p)            {if (p) {free(p); (p)= NULL;}}


/*
===========================================================================
Private Data Types
===========================================================================
*/

typedef enum                        /* Edge intersection classes         */
   {
   NUL,                              /* Empty non-intersection            */
   EMX,                              /* External maximum                  */
   ELI,                              /* External left intermediate        */
   TED,                              /* Top edge                          */
   ERI,                              /* External right intermediate       */
   RED,                              /* Right edge                        */
   IMM,                              /* Internal maximum and minimum      */
   IMN,                              /* Internal minimum                  */
   EMN,                              /* External minimum                  */
   EMM,                              /* External maximum and minimum      */
   LED,                              /* Left edge                         */
   ILI,                              /* Internal left intermediate        */
   BED,                              /* Bottom edge                       */
   IRI,                              /* Internal right intermediate       */
   IMX,                              /* Internal maximum                  */
   FUL                               /* Full non-intersection             */
   } vertex_type;

typedef enum                        /* Horizontal edge states            */
   {
   NH,                               /* No horizontal edge                */
   BH,                               /* Bottom horizontal edge            */
   TH                                /* Top horizontal edge               */
   } h_state;

typedef enum                        /* Edge bundle state                 */
   {
   UNBUNDLED,                        /* Isolated edge not within a bundle */
   BUNDLE_HEAD,                      /* Bundle head node                  */
   BUNDLE_TAIL                       /* Passive bundle tail node          */
   } bundle_state;

typedef struct v_shape              /* Internal vertex list datatype     */
   {
   double              x;            /* X coordinate component            */
   double              y;            /* Y coordinate component            */
   struct v_shape     *next;         /* Pointer to next vertex in list    */
   } vertex_node;

typedef struct p_shape              /* Internal contour / tristrip type  */
   {
   int                 active;       /* Active flag / vertex count        */
   int                 hole;         /* Hole / external contour flag      */
   vertex_node        *v[2];         /* Left and right vertex list ptrs   */
   struct p_shape     *next;         /* Pointer to next polygon contour   */
   struct p_shape     *proxy;        /* Pointer to actual structure used  */
   } polygon_node;

typedef struct edge_shape
   {
   Vertex          vertex;       /* Piggy-backed contour vertex data  */
   Vertex          bot;          /* Edge lower (x, y) coordinate      */
   Vertex          top;          /* Edge upper (x, y) coordinate      */
   double              xb;           /* Scanbeam bottom x coordinate      */
   double              xt;           /* Scanbeam top x coordinate         */
   double              dx;           /* Change in x for a unit y increase */
   int                 type;         /* Clip / subject edge flag          */
   int                 bundle[2][2]; /* Bundle edge flags                 */
   int                 bside[2];     /* Bundle left / right indicators    */
   bundle_state        bstate[2];    /* Edge bundle state                 */
   polygon_node       *outp[2];      /* Output polygon / tristrip pointer */
   struct edge_shape  *prev;         /* Previous edge in the AET          */
   struct edge_shape  *next;         /* Next edge in the AET              */
   struct edge_shape  *pred;         /* Edge connected at the lower end   */
   struct edge_shape  *succ;         /* Edge connected at the upper end   */
   struct edge_shape  *next_bound;   /* Pointer to next bound in LMT      */
   } edge_node;

typedef struct lmt_shape            /* Local minima table                */
   {
   double              y;            /* Y coordinate at local minimum     */
   edge_node          *first_bound;  /* Pointer to bound list             */
   struct lmt_shape   *next;         /* Pointer to next local minimum     */
   } lmt_node;

typedef struct sbt_t_shape          /* Scanbeam tree                     */
   {
   double              y;            /* Scanbeam node y value             */
   struct sbt_t_shape *less;         /* Pointer to nodes with lower y     */
   struct sbt_t_shape *more;         /* Pointer to nodes with higher y    */
   } sb_tree;

typedef struct it_shape             /* Intersection table                */
   {
   edge_node          *ie[2];        /* Intersecting edge (bundle) pair   */
   Vertex          point;        /* Point of intersection             */
   struct it_shape    *next;         /* The next intersection table node  */
   } it_node;

typedef struct st_shape             /* Sorted edge table                 */
   {
   edge_node          *edge;         /* Pointer to AET edge               */
   double              xb;           /* Scanbeam bottom x coordinate      */
   double              xt;           /* Scanbeam top x coordinate         */
   double              dx;           /* Change in x for a unit y increase */
   struct st_shape    *prev;         /* Previous edge in sorted list      */
   } st_node;

typedef struct bbox_shape           /* Contour axis-aligned bounding box */
   {
   double             xmin;          /* Minimum x coordinate              */
   double             ymin;          /* Minimum y coordinate              */
   double             xmax;          /* Maximum x coordinate              */
   double             ymax;          /* Maximum y coordinate              */
   } bbox;


/*
===========================================================================
Global Data
===========================================================================
*/

/* Horizontal edge state transitions within scanbeam boundary */
const h_state next_h_state[3][6]=
   {
   /*        ABOVE     BELOW     CROSS */
   /*        L   R     L   R     L   R */  
   /* NH */ {BH, TH,   TH, BH,   NH, NH},
   /* BH */ {NH, NH,   NH, NH,   TH, TH},
   /* TH */ {NH, NH,   NH, NH,   BH, BH}
   };


/*
===========================================================================
Private Functions
===========================================================================
*/

static void reset_it(it_node **it)
   {
   it_node *itn;

   while (*it)
      {
      itn= (*it)->next;
      FREE(*it);
      *it= itn;
      }
   }


static void reset_lmt(lmt_node **lmt)
   {
   lmt_node *lmtn;

   while (*lmt)
      {
      lmtn= (*lmt)->next;
      FREE(*lmt);
      *lmt= lmtn;
      }
   }


static void insert_bound(edge_node **b, edge_node *e)
   {
   edge_node *existing_bound;

   if (!*b)
      {
      /* Link node e to the tail of the list */
      *b= e;
      }
   else
      {
      /* Do primary sort on the x field */
      if (e[0].bot.x < (*b)[0].bot.x)
         {
         /* Insert a new node mid-list */
         existing_bound= *b;
         *b= e;
         (*b)->next_bound= existing_bound;
         }
      else
         {
         if (e[0].bot.x == (*b)[0].bot.x)
            {
            /* Do secondary sort on the dx field */
            if (e[0].dx < (*b)[0].dx)
               {
               /* Insert a new node mid-list */
               existing_bound= *b;
               *b= e;
               (*b)->next_bound= existing_bound;
               }
            else
               {
               /* Head further down the list */
               insert_bound(&((*b)->next_bound), e);
               }
            }
         else
            {
            /* Head further down the list */
            insert_bound(&((*b)->next_bound), e);
            }
         }
      }
   }


static edge_node **bound_list(lmt_node **lmt, double y)
   {
   lmt_node *existing_node;

   if (!*lmt)
      {
      /* Add node onto the tail end of the LMT */
      MALLOC(*lmt, sizeof(lmt_node), "LMT insertion", lmt_node);
      (*lmt)->y= y;
      (*lmt)->first_bound= NULL;
      (*lmt)->next= NULL;
      return &((*lmt)->first_bound);
      }
   else
      if (y < (*lmt)->y)
         {
         /* Insert a new LMT node before the current node */
         existing_node= *lmt;
         MALLOC(*lmt, sizeof(lmt_node), "LMT insertion", lmt_node);
         (*lmt)->y= y;
         (*lmt)->first_bound= NULL;
         (*lmt)->next= existing_node;
         return &((*lmt)->first_bound);
         }
      else
         if (y > (*lmt)->y)
            /* Head further up the LMT */
            return bound_list(&((*lmt)->next), y);
         else
            /* Use this existing LMT node */
            return &((*lmt)->first_bound);
   }


static void add_to_sbtree(int *entries, sb_tree **sbtree, double y)
   {
   if (!*sbtree)
      {
      /* Add a new tree node here */
      MALLOC(*sbtree, sizeof(sb_tree), "scanbeam tree insertion", sb_tree);
      (*sbtree)->y= y;
      (*sbtree)->less= NULL;
      (*sbtree)->more= NULL;
      (*entries)++;
      }
   else
      {
      if ((*sbtree)->y > y)
         {
         /* Head into the 'less' sub-tree */
         add_to_sbtree(entries, &((*sbtree)->less), y);
         }
      else
         {
         if ((*sbtree)->y < y)
            {
            /* Head into the 'more' sub-tree */
            add_to_sbtree(entries, &((*sbtree)->more), y);
            }
         }
      }
   }


static void build_sbt(int *entries, double *sbt, sb_tree *sbtree)
   {
   if (sbtree->less)
      build_sbt(entries, sbt, sbtree->less);
   sbt[*entries]= sbtree->y;
   (*entries)++;
   if (sbtree->more)
      build_sbt(entries, sbt, sbtree->more);
   }


static void free_sbtree(sb_tree **sbtree)
   {
   if (*sbtree)
      {
      free_sbtree(&((*sbtree)->less));
      free_sbtree(&((*sbtree)->more));
      FREE(*sbtree);
      }
   }


static int count_optimal_vertices(gpc_vertex_list c)
   {
   int result= 0, i;

   /* Ignore non-contributing contours */
   if (c.num_vertices > 0)
      {
      for (i= 0; i < c.num_vertices; i++)
         /* Ignore superfluous vertices embedded in horizontal edges */
         if (OPTIMAL(c.vertex, i, c.num_vertices))
            result++;
      }
   return result;
   }


static edge_node *build_lmt(lmt_node **lmt, sb_tree **sbtree,
                            int *sbt_entries, gpc_polygon *p, int type,
                            GPC_OP op)
   {
   int          c, i, min, max, num_edges, v, num_vertices;
   int          total_vertices= 0, e_index=0;
   edge_node   *e, *edge_table;

   for (c= 0; c < p->num_contours; c++)
      total_vertices+= count_optimal_vertices(p->contour[c]);

   /* Create the entire input polygon edge table in one go */
   MALLOC(edge_table, total_vertices * sizeof(edge_node),
      "edge table creation", edge_node);

   for (c= 0; c < p->num_contours; c++)
      {
      if (p->contour[c].num_vertices < 0)
         {
         /* Ignore the non-contributing contour and repair the vertex count */
         p->contour[c].num_vertices= -p->contour[c].num_vertices;
         }
      else
         {
         /* Perform contour optimisation */
         num_vertices= 0;
         for (i= 0; i < p->contour[c].num_vertices; i++)
            if (OPTIMAL(p->contour[c].vertex, i, p->contour[c].num_vertices))
               {
               edge_table[num_vertices].vertex.x= p->contour[c].vertex[i].x;
               edge_table[num_vertices].vertex.y= p->contour[c].vertex[i].y;

               /* Record vertex in the scanbeam table */
               add_to_sbtree(sbt_entries, sbtree,
                  edge_table[num_vertices].vertex.y);

               num_vertices++;
               }

            /* Do the contour forward pass */
            for (min= 0; min < num_vertices; min++)
               {
               /* If a forward local minimum... */
               if (FWD_MIN(edge_table, min, num_vertices))
                  {
                  /* Search for the next local maximum... */
                  num_edges= 1;
                  max= NEXT_INDEX(min, num_vertices);
                  while (NOT_FMAX(edge_table, max, num_vertices))
                     {
                     num_edges++;
                     max= NEXT_INDEX(max, num_vertices);
                     }

                  /* Build the next edge list */
                  e= &edge_table[e_index];
                  e_index+= num_edges;
                  v= min;
                  e[0].bstate[BELOW]= UNBUNDLED;
                  e[0].bundle[BELOW][CLIP]= FALSE;
                  e[0].bundle[BELOW][SUBJ]= FALSE;
                  for (i= 0; i < num_edges; i++)
                     {
                     e[i].xb= edge_table[v].vertex.x;
                     e[i].bot.x= edge_table[v].vertex.x;
                     e[i].bot.y= edge_table[v].vertex.y;

                     v= NEXT_INDEX(v, num_vertices);

                     e[i].top.x= edge_table[v].vertex.x;
                     e[i].top.y= edge_table[v].vertex.y;
                     e[i].dx= (edge_table[v].vertex.x - e[i].bot.x) /
                        (e[i].top.y - e[i].bot.y);
                     e[i].type= type;
                     e[i].outp[ABOVE]= NULL;
                     e[i].outp[BELOW]= NULL;
                     e[i].next= NULL;
                     e[i].prev= NULL;
                     e[i].succ= ((num_edges > 1) && (i < (num_edges - 1))) ?
                        &(e[i + 1]) : NULL;
                     e[i].pred= ((num_edges > 1) && (i > 0)) ? &(e[i - 1]) : NULL;
                     e[i].next_bound= NULL;
                     e[i].bside[CLIP]= (op == GPC_DIFF) ? RIGHT : LEFT;
                     e[i].bside[SUBJ]= LEFT;
                     }
                  insert_bound(bound_list(lmt, edge_table[min].vertex.y), e);
                  }
               }

            /* Do the contour reverse pass */
            for (min= 0; min < num_vertices; min++)
               {
               /* If a reverse local minimum... */
               if (REV_MIN(edge_table, min, num_vertices))
                  {
                  /* Search for the previous local maximum... */
                  num_edges= 1;
                  max= PREV_INDEX(min, num_vertices);
                  while (NOT_RMAX(edge_table, max, num_vertices))
                     {
                     num_edges++;
                     max= PREV_INDEX(max, num_vertices);
                     }

                  /* Build the previous edge list */
                  e= &edge_table[e_index];
                  e_index+= num_edges;
                  v= min;
                  e[0].bstate[BELOW]= UNBUNDLED;
                  e[0].bundle[BELOW][CLIP]= FALSE;
                  e[0].bundle[BELOW][SUBJ]= FALSE;
                  for (i= 0; i < num_edges; i++)
                     {
                     e[i].xb= edge_table[v].vertex.x;
                     e[i].bot.x= edge_table[v].vertex.x;
                     e[i].bot.y= edge_table[v].vertex.y;

                     v= PREV_INDEX(v, num_vertices);

                     e[i].top.x= edge_table[v].vertex.x;
                     e[i].top.y= edge_table[v].vertex.y;
                     e[i].dx= (edge_table[v].vertex.x - e[i].bot.x) /
                        (e[i].top.y - e[i].bot.y);
                     e[i].type= type;
                     e[i].outp[ABOVE]= NULL;
                     e[i].outp[BELOW]= NULL;
                     e[i].next= NULL;
                     e[i].prev= NULL;
                     e[i].succ= ((num_edges > 1) && (i < (num_edges - 1))) ?
                        &(e[i + 1]) : NULL;
                     e[i].pred= ((num_edges > 1) && (i > 0)) ? &(e[i - 1]) : NULL;
                     e[i].next_bound= NULL;
                     e[i].bside[CLIP]= (op == GPC_DIFF) ? RIGHT : LEFT;
                     e[i].bside[SUBJ]= LEFT;
                     }
                  insert_bound(bound_list(lmt, edge_table[min].vertex.y), e);
                  }
               }
         }
      }
   return edge_table;
   }


static void add_edge_to_aet(edge_node **aet, edge_node *edge, edge_node *prev)
   {
   if (!*aet)
      {
      /* Append edge onto the tail end of the AET */
      *aet= edge;
      edge->prev= prev;
      edge->next= NULL;
      }
   else
      {
      /* Do primary sort on the xb field */
      if (edge->xb < (*aet)->xb)
         {
         /* Insert edge here (before the AET edge) */
         edge->prev= prev;
         edge->next= *aet;
         (*aet)->prev= edge;
         *aet= edge;
         }
      else
         {
         if (edge->xb == (*aet)->xb)
            {
            /* Do secondary sort on the dx field */
            if (edge->dx < (*aet)->dx)
               {
               /* Insert edge here (before the AET edge) */
               edge->prev= prev;
               edge->next= *aet;
               (*aet)->prev= edge;
               *aet= edge;
               }
            else
               {
               /* Head further into the AET */
               add_edge_to_aet(&((*aet)->next), edge, *aet);
               }
            }
         else
            {
            /* Head further into the AET */
            add_edge_to_aet(&((*aet)->next), edge, *aet);
            }
         }
      }
   }


static void add_intersection(it_node **it, edge_node *edge0, edge_node *edge1,
                             double x, double y)
   {
   it_node *existing_node;

   if (!*it)
      {
      /* Append a new node to the tail of the list */
      MALLOC(*it, sizeof(it_node), "IT insertion", it_node);
      (*it)->ie[0]= edge0;
      (*it)->ie[1]= edge1;
      (*it)->point.x= (REAL) x;
      (*it)->point.y= (REAL) y;
      (*it)->next= NULL;
      }
   else
      {
      if ((*it)->point.y > y)
         {
         /* Insert a new node mid-list */
         existing_node= *it;
         MALLOC(*it, sizeof(it_node), "IT insertion", it_node);
         (*it)->ie[0]= edge0;
         (*it)->ie[1]= edge1;
         (*it)->point.x= (REAL) x;
         (*it)->point.y= (REAL) y;
         (*it)->next= existing_node;
         }
      else
         /* Head further down the list */
         add_intersection(&((*it)->next), edge0, edge1, x, y);
      }
   }


static void add_st_edge(st_node **st, it_node **it, edge_node *edge,
                        double dy)
   {
   st_node *existing_node;
   double   den, r, x, y;

   if (!*st)
      {
      /* Append edge onto the tail end of the ST */
      MALLOC(*st, sizeof(st_node), "ST insertion", st_node);
      (*st)->edge= edge;
      (*st)->xb= edge->xb;
      (*st)->xt= edge->xt;
      (*st)->dx= edge->dx;
      (*st)->prev= NULL;
      }
   else
      {
      den= ((*st)->xt - (*st)->xb) - (edge->xt - edge->xb);

      /* If new edge and ST edge don't cross */
      if ((edge->xt >= (*st)->xt) || (edge->dx == (*st)->dx) || 
         (fabs(den) <= DBL_EPSILON))
         {
         /* No intersection - insert edge here (before the ST edge) */
         existing_node= *st;
         MALLOC(*st, sizeof(st_node), "ST insertion", st_node);
         (*st)->edge= edge;
         (*st)->xb= edge->xb;
         (*st)->xt= edge->xt;
         (*st)->dx= edge->dx;
         (*st)->prev= existing_node;
         }
      else
         {
         /* Compute intersection between new edge and ST edge */
         r= (edge->xb - (*st)->xb) / den;
         x= (*st)->xb + r * ((*st)->xt - (*st)->xb);
         y= r * dy;

         /* Insert the edge pointers and the intersection point in the IT */
         add_intersection(it, (*st)->edge, edge, x, y);

         /* Head further into the ST */
         add_st_edge(&((*st)->prev), it, edge, dy);
         }
      }
   }


static void build_intersection_table(it_node **it, edge_node *aet, double dy)
   {
   st_node   *st, *stp;
   edge_node *edge;

   /* Build intersection table for the current scanbeam */
   reset_it(it);
   st= NULL;

   /* Process each AET edge */
   for (edge= aet; edge; edge= edge->next)
      {
      if ((edge->bstate[ABOVE] == BUNDLE_HEAD) ||
         edge->bundle[ABOVE][CLIP] || edge->bundle[ABOVE][SUBJ])
         add_st_edge(&st, it, edge, dy);
      }

   /* Free the sorted edge table */
   while (st)
      {
      stp= st->prev;
      FREE(st);
      st= stp;
      }
   }

static int count_contours(polygon_node *polygon)
   {
   int          nc, nv;
   vertex_node *v, *nextv;

   for (nc= 0; polygon; polygon= polygon->next)
      if (polygon->active)
         {
         /* Count the vertices in the current contour */
         nv= 0;
         for (v= polygon->proxy->v[LEFT]; v; v= v->next)
            nv++;

         /* Record valid vertex counts in the active field */
         if (nv > 2)
            {
            polygon->active= nv;
            nc++;
            }
         else
            {
            /* Invalid contour: just free the heap */
            for (v= polygon->proxy->v[LEFT]; v; v= nextv)
               {
               nextv= v->next;
               FREE(v);
               }
            polygon->active= 0;
            }
         }
      return nc;
   }


static void add_left(polygon_node *p, double x, double y)
   {
   vertex_node *nv;

   /* Create a new vertex node and set its fields */
   MALLOC(nv, sizeof(vertex_node), "vertex node creation", vertex_node);
   nv->x= x;
   nv->y= y;

   /* Add vertex nv to the left end of the polygon's vertex list */
   nv->next= p->proxy->v[LEFT];

   /* Update proxy->[LEFT] to point to nv */
   p->proxy->v[LEFT]= nv;
   }


static void merge_left(polygon_node *p, polygon_node *q, polygon_node *list)
   {
   polygon_node *target;

   /* Label contour as a hole */
   q->proxy->hole= TRUE;

   if (p->proxy != q->proxy)
      {
      /* Assign p's vertex list to the left end of q's list */
      p->proxy->v[RIGHT]->next= q->proxy->v[LEFT];
      q->proxy->v[LEFT]= p->proxy->v[LEFT];

      /* Redirect any p->proxy references to q->proxy */

      for (target= p->proxy; list; list= list->next)
         {
         if (list->proxy == target)
            {
            list->active= FALSE;
            list->proxy= q->proxy;
            }
         }
      }
   }


static void add_right(polygon_node *p, double x, double y)
   {
   vertex_node *nv;

   /* Create a new vertex node and set its fields */
   MALLOC(nv, sizeof(vertex_node), "vertex node creation", vertex_node);
   nv->x= x;
   nv->y= y;
   nv->next= NULL;

   /* Add vertex nv to the right end of the polygon's vertex list */
   p->proxy->v[RIGHT]->next= nv;

   /* Update proxy->v[RIGHT] to point to nv */
   p->proxy->v[RIGHT]= nv;
   }


static void merge_right(polygon_node *p, polygon_node *q, polygon_node *list)
   {
   polygon_node *target;

   /* Label contour as external */
   q->proxy->hole= FALSE;

   if (p->proxy != q->proxy)
      {
      /* Assign p's vertex list to the right end of q's list */
      q->proxy->v[RIGHT]->next= p->proxy->v[LEFT];
      q->proxy->v[RIGHT]= p->proxy->v[RIGHT];

      /* Redirect any p->proxy references to q->proxy */
      for (target= p->proxy; list; list= list->next)
         {
         if (list->proxy == target)
            {
            list->active= FALSE;
            list->proxy= q->proxy;
            }
         }
      }
   }


static void add_local_min(polygon_node **p, edge_node *edge,
                          double x, double y)
   {
   polygon_node *existing_min;
   vertex_node  *nv;

   existing_min= *p;

   MALLOC(*p, sizeof(polygon_node), "polygon node creation", polygon_node);

   /* Create a new vertex node and set its fields */
   MALLOC(nv, sizeof(vertex_node), "vertex node creation", vertex_node);
   nv->x= x;
   nv->y= y;
   nv->next= NULL;

   /* Initialise proxy to point to p itself */
   (*p)->proxy= (*p);
   (*p)->active= TRUE;
   (*p)->next= existing_min;

   /* Make v[LEFT] and v[RIGHT] point to new vertex nv */
   (*p)->v[LEFT]= nv;
   (*p)->v[RIGHT]= nv;

   /* Assign polygon p to the edge */
   edge->outp[ABOVE]= *p;
   }


static int count_tristrips(polygon_node *tn)
   {
   int total;

   for (total= 0; tn; tn= tn->next)
      if (tn->active > 2)
         total++;
   return total;
   }


static void add_vertex(vertex_node **t, double x, double y)
   {
   if (!(*t))
      {
      MALLOC(*t, sizeof(vertex_node), "tristrip vertex creation", vertex_node);
      (*t)->x= x;
      (*t)->y= y;
      (*t)->next= NULL;
      }
   else
      /* Head further down the list */
      add_vertex(&((*t)->next), x, y);
   }


static bbox *create_contour_bboxes(gpc_polygon *p)
   {
   bbox *box;
   int   c, v;

   MALLOC(box, p->num_contours * sizeof(bbox), "Bounding box creation", bbox);

   /* Construct contour bounding boxes */
   for (c= 0; c < p->num_contours; c++)
      {
      /* Initialise bounding box extent */
      box[c].xmin= DBL_MAX;
      box[c].ymin= DBL_MAX;
      box[c].xmax= -DBL_MAX;
      box[c].ymax= -DBL_MAX;

      for (v= 0; v < p->contour[c].num_vertices; v++)
         {
         /* Adjust bounding box */
         if (p->contour[c].vertex[v].x < box[c].xmin)
            box[c].xmin= p->contour[c].vertex[v].x;
         if (p->contour[c].vertex[v].y < box[c].ymin)
            box[c].ymin= p->contour[c].vertex[v].y;
         if (p->contour[c].vertex[v].x > box[c].xmax)
            box[c].xmax= p->contour[c].vertex[v].x;
         if (p->contour[c].vertex[v].y > box[c].ymax)
            box[c].ymax= p->contour[c].vertex[v].y;
         }
      }
   return box;  
   }


static void minimax_test(gpc_polygon *subj, gpc_polygon *clip, GPC_OP op)
   {
   bbox *s_bbox, *c_bbox;
   int   s, c, *o_table, overlap;

   s_bbox= create_contour_bboxes(subj);
   c_bbox= create_contour_bboxes(clip);

   MALLOC(o_table, subj->num_contours * clip->num_contours * sizeof(int),
      "overlap table creation", int);

   /* Check all subject contour bounding boxes against clip boxes */
   for (s= 0; s < subj->num_contours; s++)
      for (c= 0; c < clip->num_contours; c++)
         o_table[c * subj->num_contours + s]=
         (!((s_bbox[s].xmax < c_bbox[c].xmin) ||
         (s_bbox[s].xmin > c_bbox[c].xmax))) &&
         (!((s_bbox[s].ymax < c_bbox[c].ymin) ||
         (s_bbox[s].ymin > c_bbox[c].ymax)));

   /* For each clip contour, search for any subject contour overlaps */
   for (c= 0; c < clip->num_contours; c++)
      {
      overlap= 0;
      for (s= 0; (!overlap) && (s < subj->num_contours); s++)
         overlap= o_table[c * subj->num_contours + s];

      if (!overlap)
         /* Flag non contributing status by negating vertex count */
         clip->contour[c].num_vertices = -clip->contour[c].num_vertices;
      }  

   if (op == GPC_INT)
      {  
      /* For each subject contour, search for any clip contour overlaps */
      for (s= 0; s < subj->num_contours; s++)
         {
         overlap= 0;
         for (c= 0; (!overlap) && (c < clip->num_contours); c++)
            overlap= o_table[c * subj->num_contours + s];

         if (!overlap)
            /* Flag non contributing status by negating vertex count */
            subj->contour[s].num_vertices = -subj->contour[s].num_vertices;
         }  
      }

   FREE(s_bbox);
   FREE(c_bbox);
   FREE(o_table);
   }


/*
===========================================================================
Public Functions
===========================================================================
*/

void ArraySwap( void *object, size_t objectCount, size_t objectSize )
   {
   char *p0 = (char*) object;
   char *p1 = (char*) object + ( objectCount - 1 ) * objectSize;

   size_t count = objectCount/2;

   for ( size_t i=0; i<count; i++ )
      {
      for ( size_t j=0; j<objectSize; j++ )
         {
         *p0 ^= *p1 ^= *p0 ^= *p1;
         p0++;
         p1++;
         }

      p1 -= 2*objectSize;
      }
   }

void PolyClipper::PolygonClip( GPC_OP set_operation, Poly &subjectPolygon, Poly &clipPolygon, Poly &resultPolygon )
   {
   // subj
   gpc_polygon *subj = new gpc_polygon; 
   
   int parts = (int) subjectPolygon.m_vertexPartArray.GetCount();
   ASSERT( parts > 0 ); //must have atleast one part
   
   subj->num_contours = parts;
   subj->contour = new gpc_vertex_list[ parts ];

   int startPart = subjectPolygon.m_vertexPartArray[ 0 ];
   int nextPart = 0;

   for ( int part=0; part < parts; part++ )
      {
      if ( part < parts-1 )
         nextPart = subjectPolygon.m_vertexPartArray[ part+1 ];
      else
         nextPart = subjectPolygon.GetVertexCount();

      ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

      subj->contour[part].num_vertices = nextPart - startPart - 1;
      subj->contour[part].vertex       = subjectPolygon.m_vertexArray.GetData() + startPart;

      startPart = nextPart;
      }
   
   // clip
   gpc_polygon *clip = new gpc_polygon; 

   parts = (int) clipPolygon.m_vertexPartArray.GetCount();
   ASSERT( parts > 0 ); //must have atleast one part
   
   clip->num_contours = parts;
   clip->contour = new gpc_vertex_list[ parts ];

   startPart = clipPolygon.m_vertexPartArray[ 0 ];
   nextPart = 0;

   for ( int part=0; part < parts; part++ )
      {
      if ( part < parts-1 )
         nextPart = clipPolygon.m_vertexPartArray[ part+1 ];
      else
         nextPart = clipPolygon.GetVertexCount();

      ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

      clip->contour[part].num_vertices = nextPart - startPart - 1;
      clip->contour[part].vertex       = clipPolygon.m_vertexArray.GetData() + startPart;

      startPart = nextPart;
      }

   gpc_polygon_clip( set_operation, subj, clip, resultPolygon );

   delete [] subj->contour;
   delete subj;

   delete [] clip->contour;
   delete clip;
   }

void PolyClipper::PolygonUnion( /*const*/ CArray< Poly*, Poly* > &polyArray, const CArray< int, int > &queryResults, Poly &resultPolygon )
   {
   // subj
   gpc_polygon *subj = new gpc_polygon; 
   
   // make subj->contour
   int allocatedListCount = (int) polyArray.GetCount() * 2;
   int totalParts = 0;
   gpc_vertex_list *vertexList = new gpc_vertex_list[ allocatedListCount ];

   int polyCount = (int) queryResults.GetCount();
   Poly *pPoly;
   int parts;

   for ( int i=0; i<polyCount; i++ )
      {
      pPoly = polyArray.GetAt( queryResults.GetAt( i ) );
      parts = (int) pPoly->m_vertexPartArray.GetCount();
      //totalParts += parts;

      // reallocate
      if ( parts + totalParts > allocatedListCount )
         {
         int newAllocatedListCount = allocatedListCount;
         gpc_vertex_list *newVertexList = NULL;

         while ( parts + totalParts > newAllocatedListCount )
            {
            newAllocatedListCount += allocatedListCount;
            }

         newVertexList = new gpc_vertex_list[ newAllocatedListCount ];
         memcpy( newVertexList, vertexList, totalParts );

         delete [] vertexList;
         vertexList = newVertexList;
         allocatedListCount = newAllocatedListCount;
         }

      int startPart = pPoly->m_vertexPartArray[ 0 ];
      int nextPart = 0;

      for ( int part=0; part < parts; part++ )
         {
         totalParts++;
         ASSERT( totalParts < allocatedListCount );

         if ( part < parts-1 )
            nextPart = pPoly->m_vertexPartArray[ part+1 ];
         else
            nextPart = pPoly->GetVertexCount();

         ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

         vertexList[ totalParts ].num_vertices = nextPart - startPart - 1;
         vertexList[ totalParts ].vertex       = pPoly->m_vertexArray.GetData() + startPart;

         startPart = nextPart;
         }
      }
   
   subj->num_contours = totalParts;
   subj->contour = vertexList;
 
   
   // clip
   gpc_polygon *clip = new gpc_polygon;
   clip->num_contours = 0;
   clip->contour = NULL;

   gpc_polygon_clip( GPC_UNION, subj, clip, resultPolygon );

   delete [] vertexList;
   delete subj;

   delete clip;
   }

void PolyClipper::gpc_polygon_clip( GPC_OP op, gpc_polygon *subj, gpc_polygon *clip, Poly &resultPolygon )
   {
   sb_tree       *sbtree= NULL;
   it_node       *it= NULL, *intersect;
   edge_node     *edge, *prev_edge, *next_edge, *succ_edge, *e0, *e1;
   edge_node     *aet= NULL, *c_heap= NULL, *s_heap= NULL;
   lmt_node      *lmt= NULL, *local_min;
   polygon_node  *out_poly= NULL, *p, *q, *poly, *npoly, *cf= NULL;
   vertex_node   *vtx, *nv;
   h_state        horiz[2];
   int            in[2], exists[2], parity[2]= {LEFT, LEFT};
   int            c, contributing, search, scanbeam= 0, sbt_entries= 0;
   int            vclass, bl, br, tl, tr;
   double        *sbt= NULL, xb, px, yb, yt, dy, ix, iy;

   /* Test for trivial NULL result cases */
   if (((subj->num_contours == 0) && (clip->num_contours == 0))
      || ((subj->num_contours == 0) && ((op == GPC_INT) || (op == GPC_DIFF)))
      || ((clip->num_contours == 0) &&  (op == GPC_INT)))
      {
      resultPolygon.ClearPoly();
      return;
      }

   /* Identify potentialy contributing contours */
   if (((op == GPC_INT) || (op == GPC_DIFF))
      && (subj->num_contours > 0) && (clip->num_contours > 0))
      minimax_test(subj, clip, op);

   /* Build LMT */
   if (subj->num_contours > 0)
      s_heap= build_lmt(&lmt, &sbtree, &sbt_entries, subj, SUBJ, op);
   if (clip->num_contours > 0)
      c_heap= build_lmt(&lmt, &sbtree, &sbt_entries, clip, CLIP, op);

   /* Return a NULL result if no contours contribute */
   if (lmt == NULL)
      {
      resultPolygon.ClearPoly();
      reset_lmt(&lmt);
      FREE(s_heap);
      FREE(c_heap);
      return;
      }

   /* Build scanbeam table from scanbeam tree */
   MALLOC(sbt, sbt_entries * sizeof(double), "sbt creation", double);
   build_sbt(&scanbeam, sbt, sbtree);
   scanbeam= 0;
   free_sbtree(&sbtree);

   resultPolygon.ClearPoly();

   /* Invert clip polygon for difference operation */
   if (op == GPC_DIFF)
      parity[CLIP]= RIGHT;

   local_min= lmt;

   /* Process each scanbeam */
   while (scanbeam < sbt_entries)
      {
      /* Set yb and yt to the bottom and top of the scanbeam */
      yb= sbt[scanbeam++];
      if (scanbeam < sbt_entries)
         {
         yt= sbt[scanbeam];
         dy= yt - yb;
         }

      /* === SCANBEAM BOUNDARY PROCESSING ================================ */

      /* If LMT node corresponding to yb exists */
      if (local_min)
         {
         if (local_min->y == yb)
            {
            /* Add edges starting at this local minimum to the AET */
            for (edge= local_min->first_bound; edge; edge= edge->next_bound)
               add_edge_to_aet(&aet, edge, NULL);

            local_min= local_min->next;
            }
         }

      /* Set dummy previous x value */
      px= -DBL_MAX;

      /* Create bundles within AET */
      e0= aet;
      e1= aet;

      /* Set up bundle fields of first edge */
      aet->bundle[ABOVE][ aet->type]= (aet->top.y != yb);
      aet->bundle[ABOVE][!aet->type]= FALSE;
      aet->bstate[ABOVE]= UNBUNDLED;

      for (next_edge= aet->next; next_edge; next_edge= next_edge->next)
         {
         /* Set up bundle fields of next edge */
         next_edge->bundle[ABOVE][ next_edge->type]= (next_edge->top.y != yb);
         next_edge->bundle[ABOVE][!next_edge->type]= FALSE;
         next_edge->bstate[ABOVE]= UNBUNDLED;

         /* Bundle edges above the scanbeam boundary if they coincide */
         if (next_edge->bundle[ABOVE][next_edge->type])
            {
            if (EQ(e0->xb, next_edge->xb) && EQ(e0->dx, next_edge->dx)
               && (e0->top.y != yb))
               {
               next_edge->bundle[ABOVE][ next_edge->type]^= 
                  e0->bundle[ABOVE][ next_edge->type];
               next_edge->bundle[ABOVE][!next_edge->type]= 
                  e0->bundle[ABOVE][!next_edge->type];
               next_edge->bstate[ABOVE]= BUNDLE_HEAD;
               e0->bundle[ABOVE][CLIP]= FALSE;
               e0->bundle[ABOVE][SUBJ]= FALSE;
               e0->bstate[ABOVE]= BUNDLE_TAIL;
               }
            e0= next_edge;
            }
         }

      horiz[CLIP]= NH;
      horiz[SUBJ]= NH;

      /* Process each edge at this scanbeam boundary */
      for (edge= aet; edge; edge= edge->next)
         {
         exists[CLIP]= edge->bundle[ABOVE][CLIP] + 
            (edge->bundle[BELOW][CLIP] << 1);
         exists[SUBJ]= edge->bundle[ABOVE][SUBJ] + 
            (edge->bundle[BELOW][SUBJ] << 1);

         if (exists[CLIP] || exists[SUBJ])
            {
            /* Set bundle side */
            edge->bside[CLIP]= parity[CLIP];
            edge->bside[SUBJ]= parity[SUBJ];

            /* Determine contributing status and quadrant occupancies */
            switch (op)
               {
               case GPC_DIFF:
               case GPC_INT:
                  contributing= (exists[CLIP] && (parity[SUBJ] || horiz[SUBJ]))
                     || (exists[SUBJ] && (parity[CLIP] || horiz[CLIP]))
                     || (exists[CLIP] && exists[SUBJ]
                  && (parity[CLIP] == parity[SUBJ]));
                  br= (parity[CLIP])
                     && (parity[SUBJ]);
                  bl= (parity[CLIP] ^ edge->bundle[ABOVE][CLIP])
                     && (parity[SUBJ] ^ edge->bundle[ABOVE][SUBJ]);
                  tr= (parity[CLIP] ^ (horiz[CLIP]!=NH))
                     && (parity[SUBJ] ^ (horiz[SUBJ]!=NH));
                  tl= (parity[CLIP] ^ (horiz[CLIP]!=NH) ^ edge->bundle[BELOW][CLIP]) 
                     && (parity[SUBJ] ^ (horiz[SUBJ]!=NH) ^ edge->bundle[BELOW][SUBJ]);
                  break;
               case GPC_XOR:
                  contributing= exists[CLIP] || exists[SUBJ];
                  br= (parity[CLIP])
                     ^ (parity[SUBJ]);
                  bl= (parity[CLIP] ^ edge->bundle[ABOVE][CLIP])
                     ^ (parity[SUBJ] ^ edge->bundle[ABOVE][SUBJ]);
                  tr= (parity[CLIP] ^ (horiz[CLIP]!=NH))
                     ^ (parity[SUBJ] ^ (horiz[SUBJ]!=NH));
                  tl= (parity[CLIP] ^ (horiz[CLIP]!=NH) ^ edge->bundle[BELOW][CLIP]) 
                     ^ (parity[SUBJ] ^ (horiz[SUBJ]!=NH) ^ edge->bundle[BELOW][SUBJ]);
                  break;
               case GPC_UNION:
                  contributing= (exists[CLIP] && (!parity[SUBJ] || horiz[SUBJ]))
                     || (exists[SUBJ] && (!parity[CLIP] || horiz[CLIP]))
                     || (exists[CLIP] && exists[SUBJ]
                  && (parity[CLIP] == parity[SUBJ]));
                  br= (parity[CLIP])
                     || (parity[SUBJ]);
                  bl= (parity[CLIP] ^ edge->bundle[ABOVE][CLIP])
                     || (parity[SUBJ] ^ edge->bundle[ABOVE][SUBJ]);
                  tr= (parity[CLIP] ^ (horiz[CLIP]!=NH))
                     || (parity[SUBJ] ^ (horiz[SUBJ]!=NH));
                  tl= (parity[CLIP] ^ (horiz[CLIP]!=NH) ^ edge->bundle[BELOW][CLIP]) 
                     || (parity[SUBJ] ^ (horiz[SUBJ]!=NH) ^ edge->bundle[BELOW][SUBJ]);
                  break;
               }

            /* Update parity */
            parity[CLIP]^= edge->bundle[ABOVE][CLIP];
            parity[SUBJ]^= edge->bundle[ABOVE][SUBJ];

            /* Update horizontal state */
            if (exists[CLIP])         
               horiz[CLIP]=
               next_h_state[horiz[CLIP]]
            [((exists[CLIP] - 1) << 1) + parity[CLIP]];
            if (exists[SUBJ])         
               horiz[SUBJ]=
               next_h_state[horiz[SUBJ]]
            [((exists[SUBJ] - 1) << 1) + parity[SUBJ]];

            vclass= tr + (tl << 1) + (br << 2) + (bl << 3);

            if (contributing)
               {
               xb= edge->xb;

               switch (vclass)
                  {
                  case EMN:
                  case IMN:
                     add_local_min(&out_poly, edge, xb, yb);
                     px= xb;
                     cf= edge->outp[ABOVE];
                     break;
                  case ERI:
                     if (xb != px)
                        {
                        add_right(cf, xb, yb);
                        px= xb;
                        }
                     edge->outp[ABOVE]= cf;
                     cf= NULL;
                     break;
                  case ELI:
                     add_left(edge->outp[BELOW], xb, yb);
                     px= xb;
                     cf= edge->outp[BELOW];
                     break;
                  case EMX:
                     if (xb != px)
                        {
                        add_left(cf, xb, yb);
                        px= xb;
                        }
                     merge_right(cf, edge->outp[BELOW], out_poly);
                     cf= NULL;
                     break;
                  case ILI:
                     if (xb != px)
                        {
                        add_left(cf, xb, yb);
                        px= xb;
                        }
                     edge->outp[ABOVE]= cf;
                     cf= NULL;
                     break;
                  case IRI:
                     add_right(edge->outp[BELOW], xb, yb);
                     px= xb;
                     cf= edge->outp[BELOW];
                     edge->outp[BELOW]= NULL;
                     break;
                  case IMX:
                     if (xb != px)
                        {
                        add_right(cf, xb, yb);
                        px= xb;
                        }
                     merge_left(cf, edge->outp[BELOW], out_poly);
                     cf= NULL;
                     edge->outp[BELOW]= NULL;
                     break;
                  case IMM:
                     if (xb != px)
                        {
                        add_right(cf, xb, yb);
                        px= xb;
                        }
                     merge_left(cf, edge->outp[BELOW], out_poly);
                     edge->outp[BELOW]= NULL;
                     add_local_min(&out_poly, edge, xb, yb);
                     cf= edge->outp[ABOVE];
                     break;
                  case EMM:
                     if (xb != px)
                        {
                        add_left(cf, xb, yb);
                        px= xb;
                        }
                     merge_right(cf, edge->outp[BELOW], out_poly);
                     edge->outp[BELOW]= NULL;
                     add_local_min(&out_poly, edge, xb, yb);
                     cf= edge->outp[ABOVE];
                     break;
                  case LED:
                     if (edge->bot.y == yb)
                        add_left(edge->outp[BELOW], xb, yb);
                     edge->outp[ABOVE]= edge->outp[BELOW];
                     px= xb;
                     break;
                  case RED:
                     if (edge->bot.y == yb)
                        add_right(edge->outp[BELOW], xb, yb);
                     edge->outp[ABOVE]= edge->outp[BELOW];
                     px= xb;
                     break;
                  default:
                     break;
                  } /* End of switch */
               } /* End of contributing conditional */
            } /* End of edge exists conditional */
         } /* End of AET loop */

      /* Delete terminating edges from the AET, otherwise compute xt */
      for (edge= aet; edge; edge= edge->next)
         {
         if (edge->top.y == yb)
            {
            prev_edge= edge->prev;
            next_edge= edge->next;
            if (prev_edge)
               prev_edge->next= next_edge;
            else
               aet= next_edge;
            if (next_edge)
               next_edge->prev= prev_edge;

            /* Copy bundle head state to the adjacent tail edge if required */
            if ((edge->bstate[BELOW] == BUNDLE_HEAD) && prev_edge)
               {
               if (prev_edge->bstate[BELOW] == BUNDLE_TAIL)
                  {
                  prev_edge->outp[BELOW]= edge->outp[BELOW];
                  prev_edge->bstate[BELOW]= UNBUNDLED;
                  if (prev_edge->prev)
                     if (prev_edge->prev->bstate[BELOW] == BUNDLE_TAIL)
                        prev_edge->bstate[BELOW]= BUNDLE_HEAD;
                  }
               }
            }
         else
            {
            if (edge->top.y == yt)
               edge->xt= edge->top.x;
            else
               edge->xt= edge->bot.x + edge->dx * (yt - edge->bot.y);
            }
         }

      if (scanbeam < sbt_entries)
         {
         /* === SCANBEAM INTERIOR PROCESSING ============================== */

         build_intersection_table(&it, aet, dy);

         /* Process each node in the intersection table */
         for (intersect= it; intersect; intersect= intersect->next)
            {
            e0= intersect->ie[0];
            e1= intersect->ie[1];

            /* Only generate output for contributing intersections */
            if ((e0->bundle[ABOVE][CLIP] || e0->bundle[ABOVE][SUBJ])
               && (e1->bundle[ABOVE][CLIP] || e1->bundle[ABOVE][SUBJ]))
               {
               p= e0->outp[ABOVE];
               q= e1->outp[ABOVE];
               ix= intersect->point.x;
               iy= intersect->point.y + yb;

               in[CLIP]= ( e0->bundle[ABOVE][CLIP] && !e0->bside[CLIP])
                  || ( e1->bundle[ABOVE][CLIP] &&  e1->bside[CLIP])
                  || (!e0->bundle[ABOVE][CLIP] && !e1->bundle[ABOVE][CLIP]
               && e0->bside[CLIP] && e1->bside[CLIP]);
               in[SUBJ]= ( e0->bundle[ABOVE][SUBJ] && !e0->bside[SUBJ])
                  || ( e1->bundle[ABOVE][SUBJ] &&  e1->bside[SUBJ])
                  || (!e0->bundle[ABOVE][SUBJ] && !e1->bundle[ABOVE][SUBJ]
               && e0->bside[SUBJ] && e1->bside[SUBJ]);

               /* Determine quadrant occupancies */
               switch (op)
                  {
                  case GPC_DIFF:
                  case GPC_INT:
                     tr= (in[CLIP])
                        && (in[SUBJ]);
                     tl= (in[CLIP] ^ e1->bundle[ABOVE][CLIP])
                        && (in[SUBJ] ^ e1->bundle[ABOVE][SUBJ]);
                     br= (in[CLIP] ^ e0->bundle[ABOVE][CLIP])
                        && (in[SUBJ] ^ e0->bundle[ABOVE][SUBJ]);
                     bl= (in[CLIP] ^ e1->bundle[ABOVE][CLIP] ^ e0->bundle[ABOVE][CLIP])
                        && (in[SUBJ] ^ e1->bundle[ABOVE][SUBJ] ^ e0->bundle[ABOVE][SUBJ]);
                     break;
                  case GPC_XOR:
                     tr= (in[CLIP])
                        ^ (in[SUBJ]);
                     tl= (in[CLIP] ^ e1->bundle[ABOVE][CLIP])
                        ^ (in[SUBJ] ^ e1->bundle[ABOVE][SUBJ]);
                     br= (in[CLIP] ^ e0->bundle[ABOVE][CLIP])
                        ^ (in[SUBJ] ^ e0->bundle[ABOVE][SUBJ]);
                     bl= (in[CLIP] ^ e1->bundle[ABOVE][CLIP] ^ e0->bundle[ABOVE][CLIP])
                        ^ (in[SUBJ] ^ e1->bundle[ABOVE][SUBJ] ^ e0->bundle[ABOVE][SUBJ]);
                     break;
                  case GPC_UNION:
                     tr= (in[CLIP])
                        || (in[SUBJ]);
                     tl= (in[CLIP] ^ e1->bundle[ABOVE][CLIP])
                        || (in[SUBJ] ^ e1->bundle[ABOVE][SUBJ]);
                     br= (in[CLIP] ^ e0->bundle[ABOVE][CLIP])
                        || (in[SUBJ] ^ e0->bundle[ABOVE][SUBJ]);
                     bl= (in[CLIP] ^ e1->bundle[ABOVE][CLIP] ^ e0->bundle[ABOVE][CLIP])
                        || (in[SUBJ] ^ e1->bundle[ABOVE][SUBJ] ^ e0->bundle[ABOVE][SUBJ]);
                     break;
                  }

               vclass= tr + (tl << 1) + (br << 2) + (bl << 3);

               switch (vclass)
                  {
                  case EMN:
                     add_local_min(&out_poly, e0, ix, iy);
                     e1->outp[ABOVE]= e0->outp[ABOVE];
                     break;
                  case ERI:
                     if (p)
                        {
                        add_right(p, ix, iy);
                        e1->outp[ABOVE]= p;
                        e0->outp[ABOVE]= NULL;
                        }
                     break;
                  case ELI:
                     if (q)
                        {
                        add_left(q, ix, iy);
                        e0->outp[ABOVE]= q;
                        e1->outp[ABOVE]= NULL;
                        }
                     break;
                  case EMX:
                     if (p && q)
                        {
                        add_left(p, ix, iy);
                        merge_right(p, q, out_poly);
                        e0->outp[ABOVE]= NULL;
                        e1->outp[ABOVE]= NULL;
                        }
                     break;
                  case IMN:
                     add_local_min(&out_poly, e0, ix, iy);
                     e1->outp[ABOVE]= e0->outp[ABOVE];
                     break;
                  case ILI:
                     if (p)
                        {
                        add_left(p, ix, iy);
                        e1->outp[ABOVE]= p;
                        e0->outp[ABOVE]= NULL;
                        }
                     break;
                  case IRI:
                     if (q)
                        {
                        add_right(q, ix, iy);
                        e0->outp[ABOVE]= q;
                        e1->outp[ABOVE]= NULL;
                        }
                     break;
                  case IMX:
                     if (p && q)
                        {
                        add_right(p, ix, iy);
                        merge_left(p, q, out_poly);
                        e0->outp[ABOVE]= NULL;
                        e1->outp[ABOVE]= NULL;
                        }
                     break;
                  case IMM:
                     if (p && q)
                        {
                        add_right(p, ix, iy);
                        merge_left(p, q, out_poly);
                        add_local_min(&out_poly, e0, ix, iy);
                        e1->outp[ABOVE]= e0->outp[ABOVE];
                        }
                     break;
                  case EMM:
                     if (p && q)
                        {
                        add_left(p, ix, iy);
                        merge_right(p, q, out_poly);
                        add_local_min(&out_poly, e0, ix, iy);
                        e1->outp[ABOVE]= e0->outp[ABOVE];
                        }
                     break;
                  default:
                     break;
                  } /* End of switch */
               } /* End of contributing intersection conditional */

            /* Swap bundle sides in response to edge crossing */
            if (e0->bundle[ABOVE][CLIP])
               e1->bside[CLIP]= !e1->bside[CLIP];
            if (e1->bundle[ABOVE][CLIP])
               e0->bside[CLIP]= !e0->bside[CLIP];
            if (e0->bundle[ABOVE][SUBJ])
               e1->bside[SUBJ]= !e1->bside[SUBJ];
            if (e1->bundle[ABOVE][SUBJ])
               e0->bside[SUBJ]= !e0->bside[SUBJ];

            /* Swap e0 and e1 bundles in the AET */
            prev_edge= e0->prev;
            next_edge= e1->next;
            if (next_edge)
               next_edge->prev= e0;

            if (e0->bstate[ABOVE] == BUNDLE_HEAD)
               {
               search= TRUE;
               while (search)
                  {
                  prev_edge= prev_edge->prev;
                  if (prev_edge)
                     {
                     if (prev_edge->bstate[ABOVE] != BUNDLE_TAIL)
                        search= FALSE;
                     }
                  else
                     search= FALSE;
                  }
               }
            if (!prev_edge)
               {
               aet->prev= e1;
               e1->next= aet;
               aet= e0->next;
               }
            else
               {
               prev_edge->next->prev= e1;
               e1->next= prev_edge->next;
               prev_edge->next= e0->next;
               }
            e0->next->prev= prev_edge;
            e1->next->prev= e1;
            e0->next= next_edge;
            } /* End of IT loop*/

         /* Prepare for next scanbeam */
         for (edge= aet; edge; edge= next_edge)
            {
            next_edge= edge->next;
            succ_edge= edge->succ;

            if ((edge->top.y == yt) && succ_edge)
               {
               /* Replace AET edge by its successor */
               succ_edge->outp[BELOW]= edge->outp[ABOVE];
               succ_edge->bstate[BELOW]= edge->bstate[ABOVE];
               succ_edge->bundle[BELOW][CLIP]= edge->bundle[ABOVE][CLIP];
               succ_edge->bundle[BELOW][SUBJ]= edge->bundle[ABOVE][SUBJ];
               prev_edge= edge->prev;
               if (prev_edge)
                  prev_edge->next= succ_edge;
               else
                  aet= succ_edge;
               if (next_edge)
                  next_edge->prev= succ_edge;
               succ_edge->prev= prev_edge;
               succ_edge->next= next_edge;
               }
            else
               {
               /* Update this edge */
               edge->outp[BELOW]= edge->outp[ABOVE];
               edge->bstate[BELOW]= edge->bstate[ABOVE];
               edge->bundle[BELOW][CLIP]= edge->bundle[ABOVE][CLIP];
               edge->bundle[BELOW][SUBJ]= edge->bundle[ABOVE][SUBJ];
               edge->xb= edge->xt;
               }
            edge->outp[ABOVE]= NULL;
            }
         }
      } /* === END OF SCANBEAM PROCESSING ================================== */

   int num_contours = count_contours(out_poly);
   int num_vertices = 0;
   int index = 0;
   float xMin, yMin, xMax, yMax;
   xMin = yMin = FLT_MAX;
   xMax = yMax = FLT_MIN;
   double area;

   if ( num_contours > 0)
      {
      c = 0;
      for ( poly = out_poly; poly; poly = npoly )
         {
         npoly = poly->next;

         if ( poly->active )
            {
            int startIndex = index;
            resultPolygon.m_vertexPartArray.Add( index );
            ASSERT( resultPolygon.m_vertexPartArray.GetAt( c ) == index );

			bool hole = poly->proxy->hole ? true : false;
            
            num_vertices = poly->active;
            
            area = 0.0;
            vtx = poly->proxy->v[LEFT];
            REAL x0 = (REAL) vtx->x;
            REAL y0 = (REAL) vtx->y;
            for ( ; vtx; vtx = nv )
               {
               nv= vtx->next;
               resultPolygon.AddVertex( Vertex( (REAL) vtx->x, (REAL) vtx->y ) );
               index++;

               if ( vtx->x < xMin ) xMin = (float) vtx->x;
               if ( vtx->y < yMin ) yMin = (float) vtx->y;
               if ( vtx->x > xMax ) xMax = (float) vtx->x;
               if ( vtx->y > yMax ) yMax = (float) vtx->y;

               if ( nv )
                  area += nv->x * vtx->y - vtx->x * nv->y;
               else
                  area += x0 * vtx->y - vtx->x * y0;

               FREE(vtx);
               }

            // close part
            ASSERT( resultPolygon.m_vertexArray.GetAt( startIndex ).x == x0 && resultPolygon.m_vertexArray.GetAt( startIndex ).y == y0 );
            resultPolygon.AddVertex( Vertex( x0, y0 ) );
            index++;

            // make part conform to ESRI shapefile standard
//            ASSERT( area != 0.0 );
            if ( ( hole && area > 0.0 ) || ( !hole && area < 0.0 ) )
               ArraySwap( resultPolygon.m_vertexArray.GetData() + startIndex, index-startIndex, sizeof( Vertex ) );

            c++;
            }

         FREE(poly);
         }

      resultPolygon.m_xMin = xMin;
      resultPolygon.m_xMax = xMax;
      resultPolygon.m_yMin = yMin;
      resultPolygon.m_yMax = yMax;
      }
   else
      {
      for (poly= out_poly; poly; poly= npoly)
         {
         npoly= poly->next;
         FREE(poly);
         }
      }

   /* Tidy up */
   reset_it(&it);
   reset_lmt(&lmt);
   FREE(c_heap);
   FREE(s_heap);
   FREE(sbt);
   }
