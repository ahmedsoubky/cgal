// Copyright (c) 2005  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL: svn+ssh://gankit@scm.gforge.inria.fr/svn/cgal/trunk/Principal_component_analysis/include/CGAL/linear_least_squares_fitting_segments.h $
// $Id: linear_least_squares_fitting_2.h 37882 2007-04-03 15:15:30Z spion $
//
// Author(s) : Pierre Alliez and Sylvain Pion and Ankit Gupta

#ifndef CGAL_LINEAR_LEAST_SQUARES_FITTING_SEGMENTS_2_H
#define CGAL_LINEAR_LEAST_SQUARES_FITTING_SEGMENTS_2_H

#include <CGAL/basic.h>
#include <CGAL/Object.h>
#include <CGAL/centroid.h>
#include <CGAL/eigen_2.h>
#include <CGAL/eigen.h>
#include <CGAL/Linear_algebraCd.h>
#include <CGAL/util.h>

#include <iterator>
#include <vector>
#include <cmath>

CGAL_BEGIN_NAMESPACE

namespace CGALi {
// Fits a line to a 2D segment set.
// Returns a fitting quality (1 - lambda_min/lambda_max):
//  1 is best  (zero variance orthogonally to the fitting line);
//  0 is worst (isotropic case, returns a line with horizontal
//              direction by default)

template < typename InputIterator, typename K >
typename K::FT
linear_least_squares_fitting_2(InputIterator first,
                               InputIterator beyond, 
                               typename K::Line_2& line,   // best fit line
                               typename K::Point_2& c,     // centroid
                               const K&,                   // kernel
                               const typename K::Segment_2*,// used for indirection
			       const bool non_standard_geometry)  // not useful 
{
  // types
  typedef typename K::FT       FT;
  typedef typename K::Line_2   Line;
  typedef typename K::Point_2  Point;
  typedef typename K::Vector_2 Vector;
  typedef typename K::Segment_2 Segment;
  typedef typename CGAL::Linear_algebraCd<FT> LA;
  typedef typename LA::Matrix Matrix;

  // precondition: at least one element in the container.
  CGAL_precondition(first != beyond);

  // compute centroid
  c = centroid(first,beyond,K());
  // assemble covariance matrix as a semi-definite matrix. 
  // Matrix numbering:
  // 0
  // 1 2
  //Final combined covariance matrix for all segments and their combined mass
  FT mass = 0.0;
  FT covariance[3] = {0.0,0.0,0.0};

  // assemble 2nd order moment about the origin.  
  FT temp[4] = {1.0, 0.5,
		0.5, 1.0};
  Matrix moment = (1.0/3.0) * init_Matrix<K>(2,temp);

  for(InputIterator it = first;
      it != beyond;
      it++)
  {
    // Now for each segment, construct the 2nd order moment about the origin.
    // assemble the transformation matrix.
    const Segment& t = *it;

    // defined for convenience.
    // FT example = CGAL::to_double(t[0].x());
    FT delta[4] = {t[0].x(), t[1].x(), 
		   t[0].y(), t[1].y()};
    Matrix transformation = init_Matrix<K>(2,delta);
    FT length = std::sqrt(t.squared_length());
    CGAL_assertion(length != 0.0);

    // Find the 2nd order moment for the segment wrt to the origin by an affine transformation.
    
    // Transform the standard 2nd order moment using the transformation matrix
    transformation = length * transformation * moment * LA::transpose(transformation);
    
    // add to covariance matrix
    covariance[0] += transformation[0][0];
    covariance[1] += transformation[0][1];
    covariance[2] += transformation[1][1];

    mass += length;
  }

  // Translate the 2nd order moment calculated about the origin to
  // the center of mass to get the covariance.
  covariance[0] += mass * (-1.0 * c.x() * c.x());
  covariance[1] += mass * (-1.0 * c.x() * c.y());
  covariance[2] += mass * (-1.0 * c.y() * c.y());

  // to remove later
  //  std::cout<<covariance[0]<<" "<<covariance[1]<<" "<<covariance[2]<<" "<<(std::sqrt(2)/3.0)<<std::endl;

  // solve for eigenvalues and eigenvectors.
  // eigen values are sorted in descending order, 
  // eigen vectors are sorted in accordance.
  std::pair<FT,FT> eigen_values;
  std::pair<Vector,Vector> eigen_vectors;
  //  CGALi::eigen_symmetric_2<K>(covariance, eigen_vectors, eigen_values);
    FT eigen_vectors1[4];
    FT eigen_values1[2];
    eigen_symmetric<FT>(covariance,2, eigen_vectors1, eigen_values1);
    eigen_values = std::make_pair(eigen_values1[0],eigen_values1[1]);
    eigen_vectors = std::make_pair(Vector(eigen_vectors1[0],eigen_vectors1[1]),Vector(eigen_vectors1[2],eigen_vectors1[3]));
  // check unicity and build fitting line accordingly
  if(eigen_values.first != eigen_values.second)
  {
    // regular case
    line = Line(c, eigen_vectors.first);
    return (FT)1.0 - eigen_values.second / eigen_values.first;
  } 
  else
  {
    // isotropic case (infinite number of directions)
    // by default: assemble a line that goes through 
    // the centroid and with a default horizontal vector.
    line = Line(c, Vector(1.0, 0.0));
    return (FT)0.0;
  } 
} // end linear_least_squares_fitting_2 for segment set

} // end namespace CGALi

CGAL_END_NAMESPACE

#endif // CGAL_LINEAR_LEAST_SQUARES_FITTING_SEGMENTS_2_H
