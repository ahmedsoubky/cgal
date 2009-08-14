//----------------------------------------------------------
// APSS reconstruction method:
// Reconstructs a surface mesh from a point set and returns it as a polyhedron.
//----------------------------------------------------------

// CGAL
#include <CGAL/AABB_tree.h> // must be included before kernel
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_polyhedron_triangle_primitive.h>
#include <CGAL/Timer.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/output_surface_facets_to_polyhedron.h>
#include <CGAL/APSS_reconstruction_function.h>
#include <CGAL/compute_average_spacing.h>

#include <math.h>

#include "Kernel_type.h"
#include "Polyhedron_type.h"
#include "Point_set_scene_item.h"


// APSS implicit function
typedef CGAL::APSS_reconstruction_function<Kernel> APSS_reconstruction_function;

// Surface mesher
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;
typedef CGAL::Implicit_surface_3<Kernel, APSS_reconstruction_function> Surface_3;

// AABB tree
typedef CGAL::AABB_polyhedron_triangle_primitive<Kernel,Polyhedron> Primitive;
typedef CGAL::AABB_traits<Kernel, Primitive> AABB_traits;
typedef CGAL::AABB_tree<AABB_traits> AABB_tree;


// APSS reconstruction method:
// Reconstructs a surface mesh from a point set and returns it as a polyhedron.
Polyhedron* APSS_reconstruct(const Point_set& points,
                             FT sm_angle, // Min triangle angle (degrees). 
                             FT sm_radius, // Max triangle size w.r.t. point set average spacing. 
                             FT sm_distance, // Approximation error w.r.t. point set average spacing.
                             FT smoothness = 2) // smoothness factor
{
    CGAL::Timer task_timer; task_timer.start();

    //***************************************
    // Checks requirements
    //***************************************

    int nb_points = points.size();
    if (nb_points == 0)
    {
      std::cerr << "Error: empty point set" << std::endl;
      return NULL;
    }

    bool points_have_normals = (points.begin()->normal() != CGAL::NULL_VECTOR);
    if ( ! points_have_normals )
    {
      std::cerr << "Input point set not supported: this reconstruction method requires oriented normals" << std::endl;
      return NULL;
    }

    CGAL::Timer reconstruction_timer; reconstruction_timer.start();

    //***************************************
    // Creates implicit function
    //***************************************

    std::cerr << "Creates APSS implicit function (smoothness=" << smoothness << ")...\n";

    // Creates implicit function from the point set.
    // Note: this method requires an iterator over points
    // + property maps to access each point's position and normal.
    // The position property map can be omitted here as we use iterators over Point_3 elements.
    APSS_reconstruction_function function(points.begin(), points.end(),
                                          CGAL::make_normal_of_point_with_normal_pmap(points.begin()),
                                          smoothness);

    // Prints status
    std::cerr << "Creates implicit function: " << task_timer.time() << " seconds\n";
    task_timer.reset();

    //***************************************
    // Surface mesh generation
    //***************************************

    std::cerr << "Surface meshing...\n";

    // Computes average spacing
    FT average_spacing = CGAL::compute_average_spacing(points.begin(), points.end(),
                                                       6 /* knn = 1 ring */);

    // Gets one point inside the implicit surface
    Point inner_point = function.get_inner_point();
    FT inner_point_value = function(inner_point);
    if(inner_point_value >= 0.0)
    {
      std::cerr << "Error: unable to seed (" << inner_point_value << " at inner_point)" << std::endl;
      return NULL;
    }

    // Gets implicit function's radius
    Sphere bsphere = function.bounding_sphere();
    FT radius = std::sqrt(bsphere.squared_radius());

    // Defines the implicit surface: requires defining a
    // conservative bounding sphere centered at inner point.
    FT sm_sphere_radius = 2.01 * radius;
    FT sm_dichotomy_error = sm_distance*average_spacing/10.0; // Dichotomy error must be << sm_distance
    Surface_3 surface(function,
                      Sphere(inner_point,sm_sphere_radius*sm_sphere_radius),
                      sm_dichotomy_error/sm_sphere_radius);

    // Defines surface mesh generation criteria
    CGAL::Surface_mesh_default_criteria_3<STr> criteria(sm_angle,  // Min triangle angle (degrees)
                                                        sm_radius*average_spacing,  // Max triangle size
                                                        sm_distance*average_spacing); // Approximation error

    CGAL_TRACE_STREAM << "  make_surface_mesh(sphere center=("<<inner_point << "),\n"
                      << "                    sphere radius="<<sm_sphere_radius<<",\n"
                      << "                    angle="<<sm_angle << " degrees,\n"
                      << "                    triangle size="<<sm_radius<<" * average spacing="<<sm_radius*average_spacing<<",\n"
                      << "                    distance="<<sm_distance<<" * average spacing="<<sm_distance*average_spacing<<",\n"
                      << "                    dichotomy error=distance/"<<sm_distance*average_spacing/sm_dichotomy_error<<",\n"
                      << "                    Manifold_tag)\n";

    // Generates surface mesh with manifold option
    STr tr; // 3D Delaunay triangulation for surface mesh generation
    C2t3 c2t3(tr); // 2D complex in 3D Delaunay triangulation
    CGAL::make_surface_mesh(c2t3,                  // reconstructed mesh
                            surface,               // implicit surface
                            criteria,              // meshing criteria
                            CGAL::Manifold_tag()); // require manifold mesh with no boundary

    // Prints status
    std::cerr << "Surface meshing: " << task_timer.time() << " seconds, "
                                     << tr.number_of_vertices() << " output vertices"
                                     << std::endl;
    task_timer.reset();

    if(tr.number_of_vertices() == 0)
      return NULL;

    // Converts to polyhedron
    Polyhedron* output_mesh = new Polyhedron;
    CGAL::output_surface_facets_to_polyhedron(c2t3, *output_mesh);

    //***************************************
    // Erases small connected components
    //***************************************

    std::cerr << "Erases small connected components...\n";

    unsigned int nb_erased_components =
      output_mesh->keep_largest_connected_components( 1 /* keep largest component only*/ );

    // Prints status
    std::cerr << "Erases small connected components: " << task_timer.time() << " seconds, "
                                                      << nb_erased_components << " component(s) erased"
                                                      << std::endl;
    task_timer.reset();

    // Prints total reconstruction duration
    std::cerr << "Total reconstruction (implicit function + meshing + erase small components): " << reconstruction_timer.time() << " seconds\n";

    //***************************************
    // Computes reconstruction error
    //***************************************

    // Constructs AABB tree and computes internal KD-tree
    // data structure to accelerate distance queries
    AABB_tree tree(output_mesh->facets_begin(), output_mesh->facets_end());
    tree.accelerate_distance_queries();

    // Computes distance from each input point to reconstructed mesh
    double max_distance = DBL_MIN;
    double avg_distance = 0;
    for (Point_set::const_iterator p=points.begin(); p!=points.end(); p++)
    {
      double distance = std::sqrt(tree.squared_distance(*p));

      max_distance = (std::max)(max_distance, distance);
      avg_distance += distance;
    }
    avg_distance /= double(points.size());

    std::cerr << "Reconstruction error:\n"
              << "  max = " << max_distance << " = " << max_distance/average_spacing << " * average spacing\n"
              << "  avg = " << avg_distance << " = " << avg_distance/average_spacing << " * average spacing\n";

    return output_mesh;
}
