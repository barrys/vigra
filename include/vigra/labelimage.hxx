/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2000 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/
 
 
#ifndef VIGRA_LABELIMAGE_HXX
#define VIGRA_LABELIMAGE_HXX

#include <vector>
#include <functional>
#include "vigra/utilities.hxx"
#include "vigra/stdimage.hxx"

/** @name Connected Components Labeling
    @memo using 4 or 8 connectivity
*/
//@{

/********************************************************/
/*                                                      */
/*                        labelImage                    */
/*                                                      */
/********************************************************/

/** Find the connected components of a segmented image.
    Connected components are defined as regions with uniform 
    pixel values. Thus, #SrcAccessor::value_type# either must be equality 
    comparable (first form), or an EqualityFunctor must be provided that realizes the 
    desired predicate (second form). The destination's value type should be large enough 
    to hold the labels without overflow. Region numbers will be a 
    consecutive sequence starting with one and ending with the 
    region number returned by the function (inclusive). The parameter
    '#eight_neighbors#' determines whether the regions should be 
    4-connected or 8-connected.
    The function uses accessors. 
    
    {\bf Declarations:}
    
    pass arguments explicitly:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor>
    int labelImage(SrcIterator upperlefts, 
		   SrcIterator lowerrights, SrcAccessor sa,
		   DestIterator upperleftd, DestAccessor da,
		   bool eight_neighbors);
    
    template <class SrcIterator, class SrcAccessor,
              class DestIterator, class DestAccessor,
              class EqualityFunctor>
    int labelImage(SrcIterator upperlefts, 
                   SrcIterator lowerrights, SrcAccessor sa,
                   DestIterator upperleftd, DestAccessor da,
	           bool eight_neighbors, EqualityFunctor equal);
    \end{verbatim}
                   
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor>
    int labelImage(triple<SrcIterator, SrcIterator, SrcAccessor> src,
		   pair<DestIterator, DestAccessor> dest,
		   bool eight_neighbors);

    template <class SrcIterator, class SrcAccessor,
              class DestIterator, class DestAccessor,
              class EqualityFunctor>
    int labelImage(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                   pair<DestIterator, DestAccessor> dest,
	           bool eight_neighbors, EqualityFunctor equal)
    {
        return labelImage(src.first, src.second, src.third,
                          dest.first, dest.second, eight_neighbors, equal);
    }
    \end{verbatim}
    
    Return:  the number of regions found (= largest region label)
    
    {\bf Usage:}
    
        Include-File:
        \URL[vigra/labelimage.hxx]{../include/vigra/labelimage.hxx}
    
    \begin{verbatim}
    BImage src(w,h);
    IImage labels(w,h);
    
    // threshold at 128
    transformImage(srcImageRange(src), destImage(src),
                   Threshold<BImage::PixelType, BImage::PixelType>(
		                                    128, 256, 0, 255));
    
    // find 4-connected regions 
    labelImage(srcImageRange(src), destImage(labels), false);
    \end{verbatim}

    {\bf Required Interface:}
    
    \begin{verbatim}
    SrcImageIterator src_upperleft, src_lowerright;
    DestImageIterator dest_upperleft;
    
    SrcAccessor src_accessor;
    DestAccessor dest_accessor;
    
    SrcAccessor::value_type u = src_accessor(src_upperleft);
    
    u == u                  // first form
    
    EqualityFunctor equal;      // second form
    equal(u, u)                 // second form
    
    int i;
    dest_accessor.set(i, dest_upperleft);
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class EqualityFunctor>
int labelImage(SrcIterator upperlefts, 
               SrcIterator lowerrights, SrcAccessor sa,
               DestIterator upperleftd, DestAccessor da,
	       bool eight_neighbors, EqualityFunctor equal)
{
    int w = lowerrights.x - upperlefts.x;
    int h = lowerrights.y - upperlefts.y;
    int x,y,i;
    
    static const Diff2D left(-1,0);
    static const Diff2D top(0,-1);
    static const Diff2D topright(1,-1);
    static const Diff2D topleft(-1,-1);
    static const Diff2D neighbor[] = {left, topleft, top, topright};
    int inc = eight_neighbors ? 1 : 2;
    
    SrcIterator ys(upperlefts);
    SrcIterator xs(ys);

    // temporary image to store region labels
    IImage labelimage(w, h);
    
    IImage::Iterator yt = labelimage.upperLeft();
    IImage::Iterator xt(yt);
        
    // Kovalevsky's clever idea to use
    // image iterator and scan order iterator simultaneously
    IImage::ScanOrderIterator label = labelimage.begin();

    // pass 1: scan image from upper left to lower right
    // to find connected components
    
    for(y = 0; y != h; ++y, ++ys.y, ++yt.y)
    {
        xs = ys;
	xt = yt;
        
        int endNeighbor = (y == 0) ? 1 : 4;
	
	// regular processing
	for(x = 0; x != w; ++x, ++xs.x, ++xt.x)
	{
            int beginNeighbor = (x == 0) ? 2 : 0;
            if(x == w-1 && y != 0) --endNeighbor;
            
            for(i=beginNeighbor; i<endNeighbor; i+=inc)
            {
                if(equal(sa(xs), sa(xs, neighbor[i])))
                {
                    int popagateLabel = xt[neighbor[i]];
                    
                    for(int j=i+2; j<endNeighbor; j+=inc)
                    {
                        if(equal(sa(xs), sa(xs, neighbor[j])))
                        {
                            int equalLabel = xt[neighbor[j]];
                            if(equalLabel < popagateLabel)
                            {
                                label[popagateLabel] = equalLabel;
                                popagateLabel = equalLabel;
                            }
                            else if(popagateLabel < equalLabel)
                            {
                                label[equalLabel] = popagateLabel;
                            }
                            break;
                        }
                    }
                    *xt = popagateLabel;
                    break;
                }
            
            }
            if(i >= endNeighbor) *xt = x + y*w;  // new region
        }
    }
		    
    // pass 2: assign contiguous labels to the regions
    DestIterator yd(upperleftd);

    int count = 0;
    i = 0;
    for(y=0; y != h; ++y, ++yd.y)
    {
	DestIterator xd(yd);
	for(x = 0; x != w; ++x, ++xd.x, ++i)
	{
            if(label[i] == i)
            {
	        label[i] = count++;
            }
            else
            {
	        label[i] = label[label[i]];
            }
	    da.set(label[i]+1, xd);
	}
    }
    return count;
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class EqualityFunctor>
inline 
int labelImage(triple<SrcIterator, SrcIterator, SrcAccessor> src,
               pair<DestIterator, DestAccessor> dest,
	       bool eight_neighbors, EqualityFunctor equal)
{
    return labelImage(src.first, src.second, src.third,
                      dest.first, dest.second, eight_neighbors, equal);
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline 
int labelImage(SrcIterator upperlefts, 
               SrcIterator lowerrights, SrcAccessor sa,
               DestIterator upperleftd, DestAccessor da,
	       bool eight_neighbors)
{
    return labelImage(upperlefts, lowerrights, sa,
                 upperleftd, da, eight_neighbors, 
                 std::equal_to<typename SrcAccessor::value_type>());
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline 
int labelImage(triple<SrcIterator, SrcIterator, SrcAccessor> src,
               pair<DestIterator, DestAccessor> dest,
	       bool eight_neighbors)
{
    return labelImage(src.first, src.second, src.third,
                 dest.first, dest.second, eight_neighbors, 
                 std::equal_to<typename SrcAccessor::value_type>());
}

/********************************************************/
/*                                                      */
/*             labelImageWithBackground                 */
/*                                                      */
/********************************************************/

/** Find the connected components of a segmented image.
    Connected components are defined as regions with uniform 
    pixel values. Thus, #SrcAccessor::value_type# either must be equality 
    comparable (first form), or an EqualityFunctor must be provided that realizes the 
    desired predicate (second form). All pixel equal to the given 
    '#background_value#' are ignored when determining connected components
    and remain untouched in the destination image and 
    
    The destination's value type should be large enough 
    to hold the labels without overflow. Region numbers will be a 
    consecutive sequence starting with one and ending with the 
    region number returned by the function (inclusive). The parameter
    '#eight_neighbors#' determines whether the regions should be 
    4-connected or 8-connected.
    The function uses accessors. 
    
    {\bf Declarations:}
    
    pass arguments explicitly:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor,
	      class ValueType>
    int labelImageWithBackground(SrcIterator upperlefts, 
		   SrcIterator lowerrights, SrcAccessor sa,
		   DestIterator upperleftd, DestAccessor da,
		   bool eight_neighbors,
		   ValueType background_value );
                                 
    template <class SrcIterator, class SrcAccessor,
              class DestIterator, class DestAccessor,
	      class ValueType, class EqualityFunctor>
    int labelImageWithBackground(SrcIterator upperlefts, 
                   SrcIterator lowerrights, SrcAccessor sa,
                   DestIterator upperleftd, DestAccessor da,
	           bool eight_neighbors,
	           ValueType background_value, EqualityFunctor equal);
    \end{verbatim}
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor,
	      class ValueType>
    inline
    int labelImageWithBackground(triple<SrcIterator, SrcIterator, SrcAccessor> src,
				 pair<DestIterator, DestAccessor> dest,
				 bool eight_neighbors,
				 ValueType background_value);

    template <class SrcIterator, class SrcAccessor,
              class DestIterator, class DestAccessor,
	      class ValueType, class EqualityFunctor>
    inline
    int labelImageWithBackground(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                                 pair<DestIterator, DestAccessor> dest,
	                         bool eight_neighbors,
			         ValueType background_value, EqualityFunctor equal);
        \end{verbatim}
    
    Return:  the number of regions found (= largest region label)
    
    {\bf Usage:}
    
        Include-File:
        \URL[vigra/labelimage.hxx]{../include/vigra/labelimage.hxx}
    
    \begin{verbatim}
    BImage src(w,h);
    IImage labels(w,h);
    
    // threshold at 128
    transformImage(srcImageRange(src), destImage(src),
                   Threshold<BImage::PixelType, BImage::PixelType>(
		                                    128, 256, 0, 255));
    
    // find 4-connected regions of foreground (= white pixels) only
    labelImageWithBackground(srcImageRange(src), destImage(labels), 
                             false, 0);
    \end{verbatim}

    {\bf Required Interface:}
    
    \begin{verbatim}
    SrcImageIterator src_upperleft, src_lowerright;
    DestImageIterator dest_upperleft;
    
    SrcAccessor src_accessor;
    DestAccessor dest_accessor;
    
    SrcAccessor::value_type u = src_accessor(src_upperleft);
    ValueType background_value;
    
    u == u                  // first form
    u == background_value   // first form
    
    EqualityFunctor equal;      // second form
    equal(u, u)                 // second form
    equal(u, background_value)  // second form
    
    int i;
    dest_accessor.set(i, dest_upperleft);
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
	  class ValueType, class EqualityFunctor>
int labelImageWithBackground(SrcIterator upperlefts, 
               SrcIterator lowerrights, SrcAccessor sa,
               DestIterator upperleftd, DestAccessor da,
	       bool eight_neighbors,
	       ValueType background_value, EqualityFunctor equal)
{
    int w = lowerrights.x - upperlefts.x;
    int h = lowerrights.y - upperlefts.y;
    int x,y,i;
    
    static const Diff2D left(-1,0);
    static const Diff2D top(0,-1);
    static const Diff2D topright(1,-1);
    static const Diff2D topleft(-1,-1);
    static const Diff2D neighbor[] = {left, topleft, top, topright};
    int inc = eight_neighbors ? 1 : 2;
    
    
    SrcIterator ys(upperlefts);
    SrcIterator xs(ys);
    
    // temporary image to store region labels
    IImage labelimage(w, h);
    IImage::ScanOrderIterator label = labelimage.begin();
    IImage::Iterator yt = labelimage.upperLeft();
    IImage::Iterator  xt(yt);
    
    // pass 1: scan image from upper left to lower right
    // find connected components
    
    for(y = 0; y != h; ++y, ++ys.y, ++yt.y)
    {
        xs = ys;
	xt = yt;
        
        int endNeighbor = (y == 0) ? 1 : 4;
	
	// regular processing
	for(x = 0; x != w; ++x, ++xs.x, ++xt.x)
	{
            int beginNeighbor = (x == 0) ? 2 : 0;
            if(x == w-1 && y != 0) --endNeighbor;
            
            if(equal(sa(xs), background_value))
            {
                *xt = -1;
            }
            else
            {
                for(i=beginNeighbor; i<endNeighbor; i+=inc)
                {
                    if(equal(sa(xs), sa(xs, neighbor[i])))
                    {
                        int popagateLabel = xt[neighbor[i]];

                        for(int j=i+2; j<endNeighbor; j+=inc)
                        {
                            if(equal(sa(xs), sa(xs, neighbor[j])))
                            {
                                int equalLabel = xt[neighbor[j]];
                                if(equalLabel < popagateLabel)
                                {
                                    label[popagateLabel] = equalLabel;
                                    popagateLabel = equalLabel;
                                }
                                else if(popagateLabel < equalLabel)
                                {
                                    label[equalLabel] = popagateLabel;
                                }
                                break;
                            }
                        }
                        *xt = popagateLabel;
                        break;
                    }

                }
                if(i >= endNeighbor) *xt = x + y*w;  // new region
            }
        }
    }

    // pass 2: assign contiguous labels to the regions
    DestIterator yd(upperleftd);

    int count = 0;
    i = 0;
    for(y=0; y != h; ++y, ++yd.y)
    {
	DestIterator xd(yd);
	for(x = 0; x != w; ++x, ++xd.x, ++i)
	{
            if(label[i] == -1) continue;
            
            if(label[i] == i)
            {
	        label[i] = count++;
            }
            else
            {
	        label[i] = label[label[i]];
            }
	    da.set(label[i]+1, xd);
	}
    }
    
    return count;
}
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
	  class ValueType, class EqualityFunctor>
inline
int labelImageWithBackground(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                             pair<DestIterator, DestAccessor> dest,
	                     bool eight_neighbors,
			     ValueType background_value, EqualityFunctor equal)
{
    return labelImageWithBackground(src.first, src.second, src.third,
                                    dest.first, dest.second, 
				    eight_neighbors, background_value, equal);
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
	  class ValueType>
inline
int labelImageWithBackground(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                             pair<DestIterator, DestAccessor> dest,
	                     bool eight_neighbors,
			     ValueType background_value)
{
    return labelImageWithBackground(src.first, src.second, src.third,
                            dest.first, dest.second, 
			    eight_neighbors, background_value,
                            std::equal_to<typename SrcAccessor::value_type>());
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
	  class ValueType>
inline
int labelImageWithBackground(SrcIterator upperlefts, 
               SrcIterator lowerrights, SrcAccessor sa,
               DestIterator upperleftd, DestAccessor da,
	       bool eight_neighbors,
	       ValueType background_value)
{
    return labelImageWithBackground(upperlefts, lowerrights, sa,
                            upperleftd, da, 
			    eight_neighbors, background_value,
                            std::equal_to<typename SrcAccessor::value_type>());
}

/********************************************************/
/*                                                      */
/*             regionImageToCellGridImage               */
/*                                                      */
/********************************************************/

/** Transform a labeled image into a \Ref{Cell Grid Image}.
    This algorithm inserts border pixels between regions in a labeled
    image like this (#a# and #c# are the original labels, and #0# 
    is the value of #edge_marker# and denotes the inserted edges): 
    
    \begin{verbatim}
       original image     insert zero- and one-cells 
    
					 a 0 c c c          
	  a c c                          a 0 0 0 c          
	  a a c               =>         a a a 0 c          
	  a a a                          a a a 0 0          
					 a a a a a          
    \end{verbatim}
    
    The algorithm assumes that the original labeled image contains 
    no background. Therefore, it is suitable as a post-processing 
    operation of \Ref{labelImage} or \Ref{seededRegionGrowing}. 
    
    The destination image must be double the size of the original
    (precisely, #(2*w-1)# by #(2*h-1)# pixels). The source value type
    (#SrcAccessor::value-type#) must be equality-comparable.
    
    {\bf Declarations:}
    
    pass arguments explicitly:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor, class DestValue>
    void regionImageToCellGridImage(
		   SrcIterator sul, SrcIterator slr, SrcAccessor sa,
		   DestIterator dul, DestAccessor da,
		   DestValue edge_marker)
    \end{verbatim}
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor, class DestValue>
    inline 
    void regionImageToCellGridImage(
	       triple<SrcIterator, SrcIterator, SrcAccessor> src,
	       pair<DestIterator, DestAccessor> dest,
	       DestValue edge_marker)
    \end{verbatim}
    
    {\bf Usage:}
    
        Include-File:
        \URL[vigra/labelimage.hxx]{../include/vigra/labelimage.hxx}
    
    \begin{verbatim}
    BImage src(w,h);
    IImage labels(w,h);
    IImage cellgrid(2*w-1, 2*h-1);
    
    // threshold at 128
    transformImage(srcImageRange(src), destImage(src),
                   Threshold<BImage::PixelType, BImage::PixelType>(
		                                    128, 256, 0, 255));
    
    // find 4-connected regions 
    labelImage(srcImageRange(src), destImage(labels), false);
    
    // create cell grid image, mark edges with 0
    regionImageToCellGridImage(srcImageRange(labels), destImage(cellgrid), 0);
    \end{verbatim}

    {\bf Required Interface:}
    
    \begin{verbatim}
    ImageIterator src_upperleft, src_lowerright;
    ImageIterator dest_upperleft;
    
    SrcAccessor src_accessor;
    DestAccessor dest_accessor;
    
    SrcAccessor::value_type u = src_accessor(src_upperleft);
    
    u != u
    
    DestValue edge_marker;
    dest_accessor.set(edge_marker, dest_upperleft);
    \end{verbatim}

    {\bf Preconditions:}
    
    The destination image must have twice the size of the source:
    \begin{verbatim}
    w_dest = 2 * w_src - 1
    h_dest = 2 * h_src - 1
    \end{verbatim}
    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor, class DestValue>
void regionImageToCellGridImage(
               SrcIterator sul, SrcIterator slr, SrcAccessor sa,
	       DestIterator dul, DestAccessor da,
	       DestValue edge_marker)
{
    int w = slr.x - sul.x;
    int h = slr.y - sul.y;
    int x,y;
    
    static const Diff2D right(1,0);
    static const Diff2D left(-1,0);
    static const Diff2D bottomright(1,1);
    static const Diff2D bottom(0,1);
    static const Diff2D top(0,-1);
    
    SrcIterator iy = sul;
    DestIterator dy = dul;
    
    for(y=0; y<h-1; ++y, ++iy.y, dy.y+=2)
    {
	SrcIterator ix = iy;
	DestIterator dx = dy;
	
	for(x=0; x<w-1; ++x, ++ix.x, dx.x+=2)
	{
	    da.set(sa(ix), dx);
	    da.set(sa(ix), dx, bottomright);
	    
	    if(sa(ix, right) != sa(ix))
	    {
	        da.set(edge_marker, dx, right);
	    }
	    else
	    {
	        da.set(sa(ix), dx, right);
	    }
	    if(sa(ix, bottom) != sa(ix))
	    {
	        da.set(edge_marker, dx, bottom);
	    }
	    else
	    {
	        da.set(sa(ix), dx, bottom);
	    }
	    
	}
	
	da.set(sa(ix), dx);
	if(sa(ix, bottom) != sa(ix))
	{
	    da.set(edge_marker, dx, bottom);
	}
	else
	{
	    da.set(sa(ix), dx, bottom);
	}
    }
    
    SrcIterator ix = iy;
    DestIterator dx = dy;

    for(x=0; x<w-1; ++x, ++ix.x, dx.x+=2)
    {
	da.set(sa(ix), dx);
	if(sa(ix, right) != sa(ix))
	{
	    da.set(edge_marker, dx, right);
	}
	else
	{
	    da.set(sa(ix), dx, right);
	}
    }
    da.set(sa(ix), dx);

    dy = dul + Diff2D(1,1);

    // find missing 0-cells 
    for(y=0; y<h-1; ++y, dy.y+=2)
    {
	DestIterator dx = dy;
	
	for(x=0; x<w-1; ++x, dx.x+=2)
	{
	    static const Diff2D dist[] = {right, top, left, bottom };
	    
	    int i;
	    for(i=0; i<4; ++i)
	    {
		if(da(dx, dist[i]) == edge_marker) break;
	    }
	    
	    if(i < 4) da.set(edge_marker, dx);
	}
    }
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor, class DestValue>
inline 
void regionImageToCellGridImage(
           triple<SrcIterator, SrcIterator, SrcAccessor> src,
	   pair<DestIterator, DestAccessor> dest,
	   DestValue edge_marker)
{
    regionImageToCellGridImage(src.first, src.second, src.third,
                                        dest.first, dest.second,
					edge_marker);
}

/********************************************************/
/*                                                      */
/*                regionImageToEdgeImage                */
/*                                                      */
/********************************************************/

/** Transform a labeled image into an edge image.
    This algorithm marks all pixels with the given #edge_marker# how belong 
    to a different region (label) than their right or lower neighbors: 
    
    \begin{verbatim}
       original image                     edges 
                                 (assuming edge_marker == 1)
    
	  a c c                            1 1 *          
	  a a c               =>           * 1 1          
	  a a a                            * * *          
    \end{verbatim}
    
    The non-edge pixels of the destination image will not be touched.
    The source value type (#SrcAccessor::value-type#) must be 
    equality-comparable.
    
    {\bf Declarations:}
    
    pass arguments explicitly:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
              class DestIterator, class DestAccessor, class DestValue>
    void regionImageToEdgeImage(
                   SrcIterator sul, SrcIterator slr, SrcAccessor sa,
	           DestIterator dul, DestAccessor da,
	           DestValue edge_marker)
    \end{verbatim}
    
    use argument objects in conjuction with \Ref{Argument Object Factories}:
    \begin{verbatim}
    template <class SrcIterator, class SrcAccessor,
	      class DestIterator, class DestAccessor, class DestValue>
    inline 
    void regionImageToEdgeImage(
	       triple<SrcIterator, SrcIterator, SrcAccessor> src,
	       pair<DestIterator, DestAccessor> dest,
	       DestValue edge_marker)
    \end{verbatim}
    
    {\bf Usage:}
    
        Include-File:
        \URL[vigra/labelimage.hxx]{../include/vigra/labelimage.hxx}
    
    \begin{verbatim}
    BImage src(w,h);
    IImage labels(w,h);
    IImage edges(w, h);
    edges = 255;  // init background (non-edge) to 255
    
    // threshold at 128
    transformImage(srcImageRange(src), destImage(src),
                   Threshold<BImage::PixelType, BImage::PixelType>(
		                                    128, 256, 0, 255));
    
    // find 4-connected regions 
    labelImage(srcImageRange(src), destImage(labels), false);
    
    // create edge image, mark edges with 0
    regionImageToEdgeImage(srcImageRange(labels), destImage(edges), 0);
    \end{verbatim}

    {\bf Required Interface:}
    
    \begin{verbatim}
    ImageIterator src_upperleft, src_lowerright;
    ImageIterator dest_upperleft;
    
    SrcAccessor src_accessor;
    DestAccessor dest_accessor;
    
    SrcAccessor::value_type u = src_accessor(src_upperleft);
    
    u != u
    
    DestValue edge_marker;
    dest_accessor.set(edge_marker, dest_upperleft);
    \end{verbatim}

    @memo
*/
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor, class DestValue>
void regionImageToEdgeImage(
               SrcIterator sul, SrcIterator slr, SrcAccessor sa,
	       DestIterator dul, DestAccessor da,
	       DestValue edge_marker)
{
    int w = slr.x - sul.x;
    int h = slr.y - sul.y;
    int x,y;
    
    static const Diff2D right(1,0);
    static const Diff2D left(-1,0);
    static const Diff2D bottomright(1,1);
    static const Diff2D bottom(0,1);
    static const Diff2D top(0,-1);
    
    SrcIterator iy = sul;
    DestIterator dy = dul;
    
    for(y=0; y<h-1; ++y, ++iy.y, ++dy.y)
    {
	SrcIterator ix = iy;
	DestIterator dx = dy;
	
	for(x=0; x<w-1; ++x, ++ix.x, ++dx.x)
	{
	    if(sa(ix, right) != sa(ix))
	    {
	        da.set(edge_marker, dx);
	    }
	    if(sa(ix, bottom) != sa(ix))
	    {
	        da.set(edge_marker, dx);
	    }
	}
	
	if(sa(ix, bottom) != sa(ix))
	{
	    da.set(edge_marker, dx);
	}
    }
    
    SrcIterator ix = iy;
    DestIterator dx = dy;

    for(x=0; x<w-1; ++x, ++ix.x, ++dx.x)
    {
	if(sa(ix, right) != sa(ix))
	{
	    da.set(edge_marker, dx);
	}
    }
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor, class DestValue>
inline 
void regionImageToEdgeImage(
           triple<SrcIterator, SrcIterator, SrcAccessor> src,
	   pair<DestIterator, DestAccessor> dest,
	   DestValue edge_marker)
{
    regionImageToEdgeImage(src.first, src.second, src.third,
                                        dest.first, dest.second,
					edge_marker);
}

//@}

#endif // VIGRA_LABELIMAGE_HXX
