// Copyright (c) 2005  INRIA (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Laurent Saboret, Pierre Alliez, Bruno Levy


#ifndef CGAL_MEAN_VALUE_COORDINATES_PARAMETERIZER_3_H
#define CGAL_MEAN_VALUE_COORDINATES_PARAMETERIZER_3_H

#include <CGAL/license/Surface_mesh_parameterization.h>


#include <CGAL/Fixed_border_parameterizer_3.h>
#include <CGAL/surface_mesh_parameterization_assertions.h>
#include <CGAL/Eigen_solver_traits.h>

/// \file Mean_value_coordinates_parameterizer_3.h

namespace CGAL {


/// \ingroup  PkgSurfaceParameterizationMethods
///
/// The class Mean_value_coordinates_parameterizer_3
/// implements *Floater Mean Value Coordinates* parameterization  \cgalCite{cgal:f-mvc-03}.
/// This method is sometimes called simply *Floater parameterization*.
///
/// This is a conformal parameterization, i.e. it attempts to preserve angles.
///
/// One-to-one mapping is guaranteed if the surface's border is mapped to a convex polygon.
///
/// This class is used by the main
/// parameterization algorithm `Fixed_border_parameterizer_3::parameterize()`.
/// - It provides default` BorderParameterizer_3` and `SparseLinearAlgebraTraits_d` template
///   parameters that make sense.
/// - It implements `compute_w_ij()` to compute w_ij = (i, j) coefficient of matrix A
///   for j neighbor vertex of i based on Floater Mean Value Coordinates parameterization.
/// - It implements an optimized version of `is_one_to_one_mapping()`.
///
/// \cgalModels `ParameterizerTraits_3`
///
///
/// \sa `CGAL::Parameterizer_traits_3<ParameterizationMesh_3>`
/// \sa `CGAL::Fixed_border_parameterizer_3<ParameterizationMesh_3, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Barycentric_mapping_parameterizer_3<ParameterizationMesh_3, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Discrete_authalic_parameterizer_3<ParameterizationMesh_3, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::Discrete_conformal_map_parameterizer_3<ParameterizationMesh_3, BorderParameterizer_3, SparseLinearAlgebraTraits_d>`
/// \sa `CGAL::LSCM_parameterizer_3<ParameterizationMesh_3, BorderParameterizer_3>`

template
<
    class ParameterizationMesh_3,     ///< 3D surface mesh
    class BorderParameterizer_3       ///< Strategy to parameterize the surface border
                = Circular_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
    class SparseLinearAlgebraTraits_d ///< Traits class to solve a sparse linear system
                = Eigen_solver_traits<Eigen::BiCGSTAB<Eigen_sparse_matrix<double>::EigenType, Eigen::IncompleteLUT< double > > >
>

class Mean_value_coordinates_parameterizer_3
    : public Fixed_border_parameterizer_3<ParameterizationMesh_3,
                                        BorderParameterizer_3,
                                        SparseLinearAlgebraTraits_d>
{
// Private types
private:
    // Superclass
    typedef Fixed_border_parameterizer_3<ParameterizationMesh_3,
                                        BorderParameterizer_3,
                                        SparseLinearAlgebraTraits_d>
                                            Base;

// Public types
public:
    // We have to repeat the types exported by superclass
    /// @cond SKIP_IN_MANUAL
    typedef typename Base::Error_code       Error_code;
    typedef ParameterizationMesh_3          Adaptor;
    typedef BorderParameterizer_3           Border_param;
    typedef SparseLinearAlgebraTraits_d     Sparse_LA;
    /// @endcond

// Private types
private:
  typedef typename Adaptor::Polyhedron TriangleMesh;
  typedef typename boost::graph_traits<TriangleMesh>::vertex_descriptor vertex_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::face_descriptor face_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::face_iterator face_iterator;
  typedef typename boost::graph_traits<TriangleMesh>::vertex_iterator vertex_iterator;
 
  typedef CGAL::Vertex_around_target_circulator<TriangleMesh> vertex_around_target_circulator;
  typedef CGAL::Vertex_around_face_circulator<TriangleMesh> vertex_around_face_circulator;

    // Mesh_Adaptor_3 subtypes:
    typedef typename Adaptor::NT            NT;
    typedef typename Adaptor::Point_2       Point_2;
    typedef typename Adaptor::Point_3       Point_3;
    typedef typename Adaptor::Vector_2      Vector_2;
    typedef typename Adaptor::Vector_3      Vector_3;

    // SparseLinearAlgebraTraits_d subtypes:
    typedef typename Sparse_LA::Vector      Vector;
    typedef typename Sparse_LA::Matrix      Matrix;

    using Base::compute_angle_rad;

// Public operations
public:
    /// Constructor
    Mean_value_coordinates_parameterizer_3(Border_param border_param = Border_param(),
                                            ///< Object that maps the surface's border to 2D space.
                                          Sparse_LA sparse_la = Sparse_LA())
                                            ///< Traits object to access a sparse linear system.
    :   Fixed_border_parameterizer_3<Adaptor,
                                   Border_param,
                                   Sparse_LA>(border_param, sparse_la)
    {}

    // Default copy constructor and operator =() are fine

// Protected operations
protected:
    /// Compute w_ij = (i, j) coefficient of matrix A for j neighbor vertex of i.
    virtual NT compute_w_ij(const Adaptor& mesh,
                            vertex_descriptor main_vertex_v_i,
                            vertex_around_target_circulator neighbor_vertex_v_j)
    {
        typedef typename boost::property_map<typename Adaptor::Polyhedron, boost::vertex_point_t>::const_type PPmap;
        typedef typename boost::property_traits<PPmap>::reference Point_3;
 
        PPmap ppmap = get(vertex_point, mesh.get_adapted_mesh());
        Point_3 position_v_i = get(ppmap,main_vertex_v_i);
        Point_3 position_v_j = get(ppmap,*neighbor_vertex_v_j);

        // Compute the norm of v_j -> v_i vector
        Vector_3 edge = position_v_i - position_v_j;
        double len = std::sqrt(edge*edge);

        // Compute angle of (v_j,v_i,v_k) corner (i.e. angle of v_i corner)
        // if v_k is the vertex before v_j when circulating around v_i
        vertex_around_target_circulator previous_vertex_v_k = neighbor_vertex_v_j;
        previous_vertex_v_k --;
        Point_3 position_v_k = get(ppmap,*previous_vertex_v_k);
        double gamma_ij  = compute_angle_rad(position_v_j, position_v_i, position_v_k);

        // Compute angle of (v_l,v_i,v_j) corner (i.e. angle of v_i corner)
        // if v_l is the vertex after v_j when circulating around v_i
        vertex_around_target_circulator next_vertex_v_l = neighbor_vertex_v_j;
        next_vertex_v_l ++;
        Point_3 position_v_l = get(ppmap,*next_vertex_v_l);
        double delta_ij = compute_angle_rad(position_v_l, position_v_i, position_v_j);

        double weight = 0.0;
        CGAL_surface_mesh_parameterization_assertion(len != 0.0);    // two points are identical!
        if(len != 0.0)
            weight = (std::tan(0.5*gamma_ij) + std::tan(0.5*delta_ij)) / len;
        CGAL_surface_mesh_parameterization_assertion(weight > 0);

        return weight;
    }

    /// Check if 3D -> 2D mapping is one-to-one.
    virtual bool  is_one_to_one_mapping (const Adaptor& ,
                                         const Matrix& ,
                                         const Vector& ,
                                         const Vector& )
    {
        /// Theorem: one-to-one mapping is guaranteed if all w_ij coefficients
        ///          are > 0 (for j vertex neighbor of i) and if the surface
        ///          border is mapped onto a 2D convex polygon.
        /// Floater formula above implies that w_ij > 0 (for j vertex neighbor
        /// of i), thus mapping is guaranteed if the surface border is mapped
        /// onto a 2D convex polygon.
        return Base::get_border_parameterizer().is_border_convex ();
    }
};


} //namespace CGAL

#endif //CGAL_MEAN_VALUE_COORDINATES_PARAMETERIZER_3_H
