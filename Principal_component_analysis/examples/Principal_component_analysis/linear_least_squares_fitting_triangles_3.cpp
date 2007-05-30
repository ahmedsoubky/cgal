// Example program for the linear_least_square_fitting function

#include <CGAL/Cartesian.h>
#include <CGAL/linear_least_squares_fitting_3.h>

#include <list>

typedef double               FT;
typedef CGAL::Cartesian<FT>  K;
typedef K::Line_3            Line_3;
typedef K::Plane_3           Plane_3;
typedef K::Point_3           Point_3;
typedef K::Triangle_3          Triangle_3;

int main()
{
  std::list<Triangle_3> triangles;
  triangles.push_back(Triangle_3(Point_3(1.0,0.0,0.0),Point_3(0.0,1.0,0.0),Point_3(0.0,0.0,0.0)));

  Line_3 line;
  FT i = linear_least_squares_fitting_3(triangles.begin(),triangles.end(),line,CGAL::PCA_dimension_2_tag());

  std::cout<<"Line's accuracy: "<<i<<std::endl;
  std::cout<<"Line direction: "<<line.to_vector()<<std::endl;
  std::cout<<"Line's point: "<<line.point(0)<<std::endl;

  if(line.has_on(Point_3(2/3.0,0,0)) && line.has_on(Point_3(0,2/3.0,0)))
    std::cout<<"YES!!"<<std::endl;

  Plane_3 plane;
  FT j = linear_least_squares_fitting_3(triangles.begin(),triangles.end(),plane,CGAL::PCA_dimension_2_tag());
  
  std::cout<<"Plane's accuracy: "<<j<<std::endl;
  std::cout<<"Plane: "<<plane<<std::endl;
  
  return 0;
}
