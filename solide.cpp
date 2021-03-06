//Copyright 2017 Laurent Monasse

/*
  This file is part of CELIA3D.
  
  CELIA3D is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  CELIA3D is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with CELIA3D.  If not, see <http://www.gnu.org/licenses/>.
*/

/*!
 *  \file 
 \authors Laurent Monasse and Maria Adela Puscas
 *  \brief Definition of the functions in class Solide. 
 * Specific coupling procedures are preceded by a "warning" sign.
 */
#include "solide.hpp"
#include "intersections.hpp"
#include <iostream>
#ifndef SOLIDE_CPP
#define SOLIDE_CPP

const double eps_relat =0.000001;
/*!
 * \brief Default constructor. 
 */
Vertex::Vertex()
{
  pos = Point_3(0.,0.,0.);
  num = 0;
}

/*!\brief Constructor overload.
 *\param p Vertex coordinates
 *\param parts Vector of the particles sharing vertex \a p 
 */
Vertex::Vertex(const Point_3 p, std::vector<int> & parts)
{
  pos = p;
  for(int i=0; i<parts.size(); i++){
    particules.push_back(parts[i]);
  }
}
/*!
 * \brief operator = overload.
 * \param V Vertex
 * \return Vertex
 */
Vertex & Vertex:: operator=(const Vertex &V){
	
  assert(this != &V);
  pos = V.pos;
  num = V.num;
  particules.resize(V.particules.size());
  for(int i=0; i<V.particules.size(); i++){
    particules[i]= V.particules[i];
  }
}
/*!
 * \brief Default constructor. 
 */
Face::Face()
{
  centre = Point_3(0.,0.,0.);
  normale = Vector_3(1.,0.,0.);
  voisin = -1;
  D0 = 1.;
}

/*!\brief Constructor overload
 *\param v vector of vertices 
 *\param part index of the neighbouring particle. It is equal to -1 if there is no solid neighbour
 */
Face::Face(std::vector<Vertex> & v, int part)
{
  std::vector<Point_3> points;
  for(int i=0; i<v.size(); i++){
    vertex.push_back(v[i]);
    points.push_back(v[i].pos);
  }
  centre = centroid(points.begin(),points.end());
  normale = orthogonal_vector(points[0],points[1],points[2]);
  double norm = sqrt(CGAL::to_double(normale.squared_length()));
  normale = normale*1./norm;
  voisin = part;
  D0 = 1.;
}
/*!\brief Constructor overload.
 * \param v vector of vertices
 * \param part index of the neighbouring particle. It is equal to -1 if there is no solid neighbour 
 * \param dist Equilibrium distance of the two particles sharing the face
 */
Face::Face(std::vector<Vertex> & v, int part, double dist)
{
  std::vector<Point_3> points;
  for(int i=0; i<v.size(); i++){
    vertex.push_back(v[i]);
    points.push_back(v[i].pos);
  }
  centre = centroid(points.begin(),points.end());
  normale = orthogonal_vector(points[0],points[1],points[2]);
  double norm = sqrt(CGAL::to_double(normale.squared_length()));
  normale = normale*1./norm;
  voisin = part;
  D0 = dist;
}
/*!
 * \brief Operator = overload
 * \param F Face
 * \return Face
 */
Face & Face:: operator=(const Face &F){
	
  assert(this != &F);
  centre = F.centre;
  normale = F.normale;
  voisin = F.voisin;
  D0  = F.D0; 
  Is = F.Is; 
  It = F.It; 
  s = F.s; 
  t = F.t;
  vertex.resize(F.vertex.size());
  for(int i= 0; i<F.vertex.size(); i++){
    vertex[i] = F.vertex[i];
  }
}
/*!\brief Computation of the face moments. 
 * \details 
 * First inertia moment of the face \n
 * Second inertia moment of the face \n
 * Eigenvectors associated with the two principal inertia moments of the face 
 * \return void
 */
void Face::Inertie(){
  //Choose an initial orthonormal frame for the face
  if(normale.operator[](0)!=0. || normale.operator[](1)!=0.){
    s = Vector_3(-normale.operator[](1),normale.operator[](0),0.);
    s = s/(sqrt(CGAL::to_double(s.squared_length())));
    t = cross_product(normale,s);
  } else {
    s = Vector_3(0.,0.,1.);
    s = s/(sqrt(CGAL::to_double(s.squared_length())));
    t = cross_product(normale,s);
  }
  //Compute the face center
  double T1 = 0.;
  double Ts = 0.;
  double Tt = 0.;
  for(int i=0;i<size();i++){
    int ip = (i+1)%(size());
    Vector_3 v1(centre,vertex[i].pos);
    Vector_3 v2(centre,vertex[ip].pos);
    T1 += 1./2.*CGAL::to_double(cross_product(v1,v2)*normale);
    Ts += 1./6.*CGAL::to_double(cross_product(v1,v2)*normale)*CGAL::to_double((v1+v2)*s);
    Tt += 1./6.*CGAL::to_double(cross_product(v1,v2)*normale)*CGAL::to_double((v1+v2)*t);
  }
  centre = centre + (Ts/T1)*s + (Tt/T1)*t;
  S = T1;
  //Compute the inertia matrix for the face in the chosen frame at the center of the face
  double Tss = 0.;
  double Ttt = 0.;
  double Tst = 0.;
  for(int i=0;i<size();i++){
    int ip = (i+1)%(size());
    Vector_3 v1(centre,vertex[i].pos);
    Vector_3 v2(centre,vertex[ip].pos);
    double As = CGAL::to_double(v1*s);
    double At = CGAL::to_double(v1*t);
    double Bs = CGAL::to_double(v2*s);
    double Bt = CGAL::to_double(v2*t);
    Tss += 1./12.*(As*As+As*Bs+Bs*Bs);
    Ttt += 1./12.*(At*At+At*Bt+Bt*Bt);
    Tst += 1./24.*(2.*As*At+As*Bt+At*Bs+2.*Bs*Bt);
  }
  //Compute the inertia moments
  double Delta = pow(Tss-Ttt,2)+4.*Tst*Tst;
  Is = (Tss+Ttt+sqrt(Delta))/2.;
  It = (Tss+Ttt-sqrt(Delta))/2.;
  //Diagonalization
  if(abs(Tss-Ttt)>eps){
    if(abs(Tss-Is)>eps){
      Vector_3 stemp = -Tst*s+(Tss-Is)*t;
      s = stemp/(sqrt(CGAL::to_double(stemp.squared_length())));
      t = cross_product(normale,s);
    } else {
      Vector_3 stemp = -Tst*t+(Ttt-Is)*s;
      s = stemp/(sqrt(CGAL::to_double(stemp.squared_length())));
      t = cross_product(normale,s);
    }
  } else {
    if(abs(Tst)>eps){
      Vector_3 stemp = s+t;
      Vector_3 ttemp = -s+t;
      s = stemp/(sqrt(CGAL::to_double(stemp.squared_length())));
      t = stemp/(sqrt(CGAL::to_double(ttemp.squared_length())));
    }
  }
}


/*!\brief Default constructor. 
 */

Particule::Particule()
{   
  bbox = Bbox(0.,0.,0.,1.,1.,1.);
  
  x0 = Point_3(0.5,0.5,0.5);
	
  cube = true;
	
  const Point_3 s1(0.,0.,0.);
  const Point_3 r1(1.,0.,0.);
  const Point_3 t1(1.,1.,0.);
  const Point_3 v1(0.,1.,0.);
	
	
  const Point_3 s2(0.,0.,1.);
  const Point_3 r2(1.,0.,1.);
  const Point_3 t2(1.,1.,1.);
  const Point_3 v2(0.,1.,1.);

  //Face 1
  std::vector<Vertex> vert1(4);
  vert1[0].pos = s1;
  vert1[0].particules.push_back(-1);
  vert1[1].pos = v1;
  vert1[1].particules.push_back(-1);
  vert1[2].pos = t1;
  vert1[2].particules.push_back(-1);
  vert1[3].pos = r1;
  vert1[3].particules.push_back(-1);
  vector<Point_3> points1(4);
  for(int i=0;i<4;i++){
    points1[i] = vert1[i].pos;
  }
  Face face1 = Face(vert1,-1,1.);
  face1.centre = centroid(points1.begin(),points1.end());
  face1.normale = orthogonal_vector(points1[0],points1[1],points1[2]);
  double norm1 = sqrt(CGAL::to_double(face1.normale.squared_length()));
  face1.normale = face1.normale*1./norm1;

  //Face 2
  std::vector<Vertex> vert2(4);
  vert2[0].pos = s1;
  vert2[0].particules.push_back(-1);
  vert2[1].pos = r1;
  vert2[1].particules.push_back(-1);
  vert2[2].pos = r2;
  vert2[2].particules.push_back(-1);
  vert2[3].pos = s2;
  vert2[3].particules.push_back(-1);
  vector<Point_3> points2(4);
  for(int i=0;i<4;i++){
    points2[i] = vert2[i].pos;
  }
  Face face2 = Face(vert2,-1,1.);
  face2.centre = centroid(points2.begin(),points2.end());
  face2.normale = orthogonal_vector(points2[0],points2[1],points2[2]);
  double norm2 = sqrt(CGAL::to_double(face2.normale.squared_length()));
  face2.normale = face2.normale*1./norm2;

  //Face 3
  std::vector<Vertex> vert3(4);
  vert3[0].pos = r1;
  vert3[0].particules.push_back(-1);
  vert3[1].pos = t1;
  vert3[1].particules.push_back(-1);
  vert3[2].pos = t2;
  vert3[2].particules.push_back(-1);
  vert3[3].pos = r2;
  vert3[3].particules.push_back(-1);
  vector<Point_3> points3(4);
  for(int i=0;i<4;i++){
    points3[i] = vert3[i].pos;
  }
  Face face3 = Face(vert3,-1,1.);
  face3.centre = centroid(points3.begin(),points3.end());
  face3.normale = orthogonal_vector(points3[0],points3[1],points3[2]);
  double norm3 = sqrt(CGAL::to_double(face3.normale.squared_length()));
  face3.normale = face3.normale*1./norm3;

  //Face 4
  std::vector<Vertex> vert4(4);
  vert4[0].pos = t1;
  vert4[0].particules.push_back(-1);
  vert4[1].pos = v1;
  vert4[1].particules.push_back(-1);
  vert4[2].pos = v2;
  vert4[2].particules.push_back(-1);
  vert4[3].pos = t2;
  vert4[3].particules.push_back(-1);
  vector<Point_3> points4(4);
  for(int i=0;i<4;i++){
    points4[i] = vert4[i].pos;
  }
  Face face4 = Face(vert4,-1,1.);
  face4.centre = centroid(points4.begin(),points4.end());
  face4.normale = orthogonal_vector(points4[0],points4[1],points4[2]);
  double norm4 = sqrt(CGAL::to_double(face4.normale.squared_length()));
  face4.normale = face4.normale*1./norm4;
	
  //Face 5
  std::vector<Vertex> vert5(4);
  vert5[0].pos = s1;
  vert5[0].particules.push_back(-1);
  vert5[1].pos = r1;
  vert5[1].particules.push_back(-1);
  vert5[2].pos = r2;
  vert5[2].particules.push_back(-1);
  vert5[3].pos = s2;
  vert5[3].particules.push_back(-1);
  vector<Point_3> points5(4);
  for(int i=0;i<4;i++){
    points5[i] = vert5[i].pos;
  }
  Face face5 = Face(vert5,-1,1.);
  face5.centre = centroid(points5.begin(),points5.end());
  face5.normale = orthogonal_vector(points5[0],points5[1],points5[2]);
  double norm5 = sqrt(CGAL::to_double(face5.normale.squared_length()));
  face5.normale = face5.normale*1./norm5;
  
  //Face 6
  std::vector<Vertex> vert6(4);
  vert6[0].pos = s1;
  vert6[0].particules.push_back(-1);
  vert6[1].pos = r1;
  vert6[1].particules.push_back(-1);
  vert6[2].pos = r2;
  vert6[2].particules.push_back(-1);
  vert6[3].pos = s2;
  vert6[3].particules.push_back(-1);
  vector<Point_3> points6(4);
  for(int i=0;i<4;i++){
    points6[i] = vert6[i].pos;
  }
  Face face6 = Face(vert6,-1,1.);
  face6.centre = centroid(points6.begin(),points6.end());
  face6.normale = orthogonal_vector(points6[0],points6[1],points6[2]);
  double norm6 = sqrt(CGAL::to_double(face6.normale.squared_length()));
  face6.normale = face6.normale*1./norm6;

  std::vector<Face> f(6);
  f[0] = face1;
  f[1] = face2;
  f[2] = face3;
  f[3] = face4;
  f[4] = face5;
  f[5] = face6;

  faces = f;
	
  //Face triangles	
  Triangle_3 Tri1(s1,r1,v1);
  Triangle_3 Tri2(t1,r1,v1);	
  triangles.push_back(Tri1);
  triangles.push_back(Tri2);
  normales.push_back(face1.normale);
  normales.push_back(face1.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face2
  Triangle_3 Tri5(s2,r2,v2);
  Triangle_3 Tri6(t2,r2,v2);
  triangles.push_back(Tri5);
  triangles.push_back(Tri6);
  normales.push_back(face2.normale);
  normales.push_back(face2.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face3
  Triangle_3 Tri9(s2,s1,v2);
  Triangle_3 Tri10(v1,s1,v2);
  triangles.push_back(Tri9);
  triangles.push_back(Tri10);
  normales.push_back(face3.normale);
  normales.push_back(face3.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face4	
  Triangle_3 Tri13(r2,r1,t2);
  Triangle_3 Tri14(t1,r1,t2);
  triangles.push_back(Tri13);
  triangles.push_back(Tri14);
  normales.push_back(face4.normale);
  normales.push_back(face4.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face5	
  Triangle_3 Tri17(v2,v1,t2);
  Triangle_3 Tri18(t1,v1,t2);
  triangles.push_back(Tri17);
  triangles.push_back(Tri18);
  normales.push_back(face5.normale);
  normales.push_back(face5.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face6
  Triangle_3 Tri21(s2,s1,r2);
  Triangle_3 Tri22(r1,s1,r2);
  triangles.push_back(Tri21);
  triangles.push_back(Tri22); 
  normales.push_back(face6.normale);
  normales.push_back(face6.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  Points_interface.resize(triangles.size(), std::vector<Point_3>(0));
  Triangles_interface.resize(triangles.size(), std::vector<Triangle_3>(0));
  Position_Triangles_interface.resize(triangles.size(), std::vector< std::vector<int> >(0));
  Points_interface_prev.resize(triangles.size(), std::vector<Point_3>(0));
  Triangles_interface_prev.resize(triangles.size(), std::vector<Triangle_3>(0));
  Position_Triangles_interface_prev.resize(triangles.size(), std::vector<std::vector<int> >(0));
  Ff = Vector_3(0.,0.,0.); Ffprev = Vector_3(0.,0.,0.); 
  Mf = Vector_3(0.,0.,0.); Mfprev = Vector_3(0.,0.,0.);
}

/*!\brief Constructor overload.
 * \param (x_min, y_min, z_min), (x_max, y_max, z_max): bounding box coordinates
 */
Particule::Particule(const double x_min, const double y_min, const double z_min, 
		     const double x_max, const double y_max,const double z_max)
{   
  bbox = Bbox(x_min,y_min,z_min,x_max,y_max,z_max);
  
  x0 = Point_3((x_min+x_max)/2.,(y_min+y_max)/2.,(z_min+z_max)/2.);
	
  cube = true;
	
  const Point_3 s1(x_min, y_min, z_min);
  const Point_3 r1(x_max, y_min, z_min);
  const Point_3 t1(x_max, y_max, z_min);
  const Point_3 v1(x_min, y_max, z_min);
	
	
  const Point_3 s2(x_min, y_min, z_max);
  const Point_3 r2(x_max, y_min, z_max);
  const Point_3 t2(x_max, y_max, z_max);
  const Point_3 v2(x_min, y_max, z_max);
	
  //Face 1
  std::vector<Vertex> vert1(4);
  vert1[0].pos = s1;
  vert1[0].particules.push_back(-1);
  vert1[1].pos = v1;
  vert1[1].particules.push_back(-1);
  vert1[2].pos = t1;
  vert1[2].particules.push_back(-1);
  vert1[3].pos = r1;
  vert1[3].particules.push_back(-1);
  vector<Point_3> points1(4);
  for(int i=0;i<4;i++){
    points1[i] = vert1[i].pos;
  }
  Face face1 = Face(vert1,-1,1.);
  face1.centre = centroid(points1.begin(),points1.end());
  face1.normale = orthogonal_vector(points1[0],points1[1],points1[2]);
  double norm1 = sqrt(CGAL::to_double(face1.normale.squared_length()));
  face1.normale = face1.normale*1./norm1;
	
  //Face 2
  std::vector<Vertex> vert2(4);
  vert2[0].pos = s1;
  vert2[0].particules.push_back(-1);
  vert2[1].pos = r1;
  vert2[1].particules.push_back(-1);
  vert2[2].pos = r2;
  vert2[2].particules.push_back(-1);
  vert2[3].pos = s2;
  vert2[3].particules.push_back(-1);
  vector<Point_3> points2(4);
  for(int i=0;i<4;i++){
    points2[i] = vert2[i].pos;
  }
  Face face2 = Face(vert2,-1,1.);
  face2.centre = centroid(points2.begin(),points2.end());
  face2.normale = orthogonal_vector(points2[0],points2[1],points2[2]);
  double norm2 = sqrt(CGAL::to_double(face2.normale.squared_length()));
  face2.normale = face2.normale*1./norm2;
	
  //Face 3
  std::vector<Vertex> vert3(4);
  vert3[0].pos = r1;
  vert3[0].particules.push_back(-1);
  vert3[1].pos = t1;
  vert3[1].particules.push_back(-1);
  vert3[2].pos = t2;
  vert3[2].particules.push_back(-1);
  vert3[3].pos = r2;
  vert3[3].particules.push_back(-1);
  vector<Point_3> points3(4);
  for(int i=0;i<4;i++){
    points3[i] = vert3[i].pos;
  }
  Face face3 = Face(vert3,-1,1.);
  face3.centre = centroid(points3.begin(),points3.end());
  face3.normale = orthogonal_vector(points3[0],points3[1],points3[2]);
  double norm3 = sqrt(CGAL::to_double(face3.normale.squared_length()));
  face3.normale = face3.normale*1./norm3;

  //Face 4
  std::vector<Vertex> vert4(4);
  vert4[0].pos = t1;
  vert4[0].particules.push_back(-1);
  vert4[1].pos = v1;
  vert4[1].particules.push_back(-1);
  vert4[2].pos = v2;
  vert4[2].particules.push_back(-1);
  vert4[3].pos = t2;
  vert4[3].particules.push_back(-1);
  vector<Point_3> points4(4);
  for(int i=0;i<4;i++){
    points4[i] = vert4[i].pos;
  }
  Face face4 = Face(vert4,-1,1.);
  face4.centre = centroid(points4.begin(),points4.end());
  face4.normale = orthogonal_vector(points4[0],points4[1],points4[2]);
  double norm4 = sqrt(CGAL::to_double(face4.normale.squared_length()));
  face4.normale = face4.normale*1./norm4;

  //Face 5
  std::vector<Vertex> vert5(4);
  vert5[0].pos = s1;
  vert5[0].particules.push_back(-1);
  vert5[1].pos = r1;
  vert5[1].particules.push_back(-1);
  vert5[2].pos = r2;
  vert5[2].particules.push_back(-1);
  vert5[3].pos = s2;
  vert5[3].particules.push_back(-1);
  vector<Point_3> points5(4);
  for(int i=0;i<4;i++){
    points5[i] = vert5[i].pos;
  }
  Face face5 = Face(vert5,-1,1.);
  face5.centre = centroid(points5.begin(),points5.end());
  face5.normale = orthogonal_vector(points5[0],points5[1],points5[2]);
  double norm5 = sqrt(CGAL::to_double(face5.normale.squared_length()));
  face5.normale = face5.normale*1./norm5;

  //Face 6
  std::vector<Vertex> vert6(4);
  vert6[0].pos = s1;
  vert6[0].particules.push_back(-1);
  vert6[1].pos = r1;
  vert6[1].particules.push_back(-1);
  vert6[2].pos = r2;
  vert6[2].particules.push_back(-1);
  vert6[3].pos = s2;
  vert6[3].particules.push_back(-1);
  vector<Point_3> points6(4);
  for(int i=0;i<4;i++){
    points6[i] = vert6[i].pos;
  }
  Face face6 = Face(vert6,-1,1.);
  face6.centre = centroid(points6.begin(),points6.end());
  face6.normale = orthogonal_vector(points6[0],points6[1],points6[2]);
  double norm6 = sqrt(CGAL::to_double(face6.normale.squared_length()));
  face6.normale = face6.normale*1./norm6;

  std::vector<Face> f(6);
  f[0] = face1;
  f[1] = face2;
  f[2] = face3;
  f[3] = face4;
  f[4] = face5;
  f[5] = face6;

  faces = f;
	
  //Face triangles	
  Triangle_3 Tri1(s1,r1,v1);
  Triangle_3 Tri2(t1,r1,v1);	
  triangles.push_back(Tri1);
  triangles.push_back(Tri2);
  normales.push_back(face1.normale);
  normales.push_back(face1.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face2
  Triangle_3 Tri5(s2,r2,v2);
  Triangle_3 Tri6(t2,r2,v2);
  triangles.push_back(Tri5);
  triangles.push_back(Tri6);
  normales.push_back(face2.normale);
  normales.push_back(face2.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face3
  Triangle_3 Tri9(s2,s1,v2);
  Triangle_3 Tri10(v1,s1,v2);
  triangles.push_back(Tri9);
  triangles.push_back(Tri10);
  normales.push_back(face3.normale);
  normales.push_back(face3.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face4	
  Triangle_3 Tri13(r2,r1,t2);
  Triangle_3 Tri14(t1,r1,t2);
  triangles.push_back(Tri13);
  triangles.push_back(Tri14);
  normales.push_back(face4.normale);
  normales.push_back(face4.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face5	
  Triangle_3 Tri17(v2,v1,t2);
  Triangle_3 Tri18(t1,v1,t2);
  triangles.push_back(Tri17);
  triangles.push_back(Tri18);
  normales.push_back(face5.normale);
  normales.push_back(face5.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  //face6
  Triangle_3 Tri21(s2,s1,r2);
  Triangle_3 Tri22(r1,s1,r2);
  triangles.push_back(Tri21);
  triangles.push_back(Tri22); 
  normales.push_back(face6.normale);
  normales.push_back(face6.normale);
  fluide.push_back(true);
  fluide.push_back(true);
  Points_interface.resize(triangles.size(), std::vector<Point_3>(0));
  Triangles_interface.resize(triangles.size(), std::vector<Triangle_3>(0));
  Position_Triangles_interface.resize(triangles.size(), std::vector< std::vector<int> >(0));
  Points_interface_prev.resize(triangles.size(), std::vector<Point_3>(0));
  Triangles_interface_prev.resize(triangles.size(), std::vector<Triangle_3>(0));
  Position_Triangles_interface_prev.resize(triangles.size(), std::vector< std::vector<int> >(0));
  Ff = Vector_3(0.,0.,0.); Ffprev = Vector_3(0.,0.,0.); 
  Mf = Vector_3(0.,0.,0.); Mfprev = Vector_3(0.,0.,0.);
}

/*!\brief Constructor overload.
 * \param (x_min, y_min, z_min), (x_max, y_max, z_max): Bounding box coordinates
 * \param c Point
 * \param F: Face of the Particule
 */
Particule::Particule(Point_3 c, const double x_min, const double y_min, const double z_min, 
		     const double x_max, const double y_max,const double z_max, 
		     std::vector<Face> & F)
{   
  bbox = Bbox(x_min,y_min,z_min,x_max,y_max,z_max);
  
  x0 = c;
	
  cube = false;

  faces = F;

  for(int i=0;i<faces.size();i++){
		
    if(faces[i].size() == 3){
      Point_3 s,r,v;
      s = faces[i].vertex[0].pos;
      r = faces[i].vertex[1].pos;
      v = faces[i].vertex[2].pos;
			
      Vector_3 vect0(s,r);
      Vector_3 vect1(s,v);
      Vector_3 normale = CGAL::cross_product(vect0,vect1);
      normale = normale*(1./sqrt(CGAL::to_double(normale.squared_length())));
      if (normale*faces[i].normale > 0.){
	Triangle_3 Tri(s,r,v);
	triangles.push_back(Tri);
	normales.push_back(faces[i].normale);
	if(faces[i].voisin < 0){
	  fluide.push_back(true);
	} else {
	  fluide.push_back(false);
	}
      }
      else{
	Triangle_3 Tri(s,v,r);
	triangles.push_back(Tri);
	normales.push_back(faces[i].normale);
	if(faces[i].voisin < 0){
	  fluide.push_back(true);
	} else {
	  fluide.push_back(false);
	}
      }
      if(faces[i].voisin == -2){
	vide.push_back(true);
      } else {
	vide.push_back(false);
      }
    }
    else{
      Point_3 s,r,v;
      s = faces[i].centre;
      for(int k=0;k<faces[i].size();k++){
	int kp = (k+1)%(faces[i].size());
	r = faces[i].vertex[k].pos;
	v = faces[i].vertex[kp].pos;
	Vector_3 vect0(s,r);
	Vector_3 vect1(s,v);
				
	Triangle_3 Tri(s,r,v);
	triangles.push_back(Tri);
	normales.push_back(faces[i].normale);
				
	if(faces[i].voisin < 0){
	  fluide.push_back(true);
	} else {
	  fluide.push_back(false);
	}
	if(faces[i].voisin == -2){
	  vide.push_back(true);
	} else {
	  vide.push_back(false);
	}
      }
    }
		
  }
  
  Points_interface.resize(triangles.size(), std::vector<Point_3>(0));
  Triangles_interface.resize(triangles.size(), std::vector<Triangle_3>(0));
  Position_Triangles_interface.resize(triangles.size(), std::vector< std::vector<int> >(0));
  Points_interface_prev.resize(triangles.size(), std::vector<Point_3>(0));
  Triangles_interface_prev.resize(triangles.size(), std::vector<Triangle_3>(0));
  Position_Triangles_interface_prev.resize(triangles.size(), std::vector< std::vector<int> >(0));
  Ff = Vector_3(0.,0.,0.); Ffprev = Vector_3(0.,0.,0.); 
  Mf = Vector_3(0.,0.,0.); Mfprev = Vector_3(0.,0.,0.);
}

/*!\brief Destructor.
 */
Particule::~Particule(){
}

/*!\brief Operator = overload
 * \param P Particule
 * \return Particule
 */
Particule & Particule:: operator=(const Particule &P){
	
  assert(this != &P);
  bbox = P.bbox;
  cube  = P.cube;
	
  faces = P.faces;
  fixe = P.fixe;
  m  = P.m; 
  V = P.V; 
  Vl = P.Vl; 
  epsilon = P.epsilon; 
  for(int i=0; i<3;i++){
    I[i] = P.I[i];
    for(int j=0; j<3;j++){
      rotref[i][j] = P.rotref[i][j];
    }
  }
	
  x0 = P.x0;
  Dx = P.Dx;
  Dxprev = P.Dxprev;
  Fi = P.Fi;
  Ff = P.Ff;
  Ffprev = P.Ffprev;
  Mi = P.Mi;
  Mf = P.Mf;
  Mfprev = P.Mfprev;
  u = P.u;
  u_half = P.u_half;
  omega = P.omega;
  omega_half = P.omega_half;
  e = P.e;
  eprev= P.eprev;
  mvt_t = P.mvt_t;
  mvt_tprev = P.mvt_tprev;
	
  triangles.resize(P.triangles.size()); 
  for(int i = 0; i< P.triangles.size(); i++){
    triangles[i] = P.triangles[i];
  }
  vide.resize(P.vide.size()); 
  for(int i = 0; i< P.vide.size(); i++){
    vide[i] = P.vide[i];
  }
  triangles_prev.resize(P.triangles_prev.size()); 
  for(int i = 0; i< P.triangles_prev.size(); i++){
    triangles_prev[i] = P.triangles_prev[i];
  }
	
  normales.resize(P.normales.size()); 
  for(int i = 0; i< P.normales.size(); i++){
    normales[i] = P.normales[i];
  }
  normales_prev.resize(P.normales_prev.size()); 
  for(int i = 0; i< P.normales_prev.size(); i++){
    normales_prev[i] = P.normales_prev[i];
  }
  fluide.resize(P.fluide.size()); 
  for(int i = 0; i< P.fluide.size(); i++){
    fluide[i] = P.fluide[i];
  }
  fluide_prev.resize(P.fluide_prev.size()); 
  for(int i = 0; i< P.fluide_prev.size(); i++){
    fluide_prev[i] = P.fluide_prev[i];
  }
	
  Points_interface.resize(P.Points_interface.size(), std::vector<Point_3>(0));
  for(int i= 0; i< P.triangles.size(); i++){
    Points_interface[i].resize(P.Points_interface[i].size());
    for(int j=0; j<P.Points_interface[i].size(); j++ ){
      Points_interface[i][j] = P.Points_interface[i][j];
    }
  }
	
  Points_interface_prev.resize(P.Points_interface_prev.size(), std::vector<Point_3>(0));
  for(int i= 0; i< P.triangles.size(); i++){
    Points_interface_prev[i].resize(P.Points_interface_prev[i].size());
    for(int j=0; j<P.Points_interface_prev[i].size(); j++ ){
      Points_interface_prev[i][j] = P.Points_interface_prev[i][j];
    }
  }
	
  Triangles_interface.resize(P.Triangles_interface.size(), std::vector<Triangle_3>(0));
  for(int i= 0; i< P.triangles.size(); i++){
    Triangles_interface[i].resize(P.Triangles_interface[i].size());
    for(int j=0; j<P.Triangles_interface[i].size(); j++ ){
      Triangles_interface[i][j] = P.Triangles_interface[i][j];
    }
  }
	
  Triangles_interface_prev.resize(P.Triangles_interface_prev.size(), std::vector<Triangle_3>(0));
  for(int i= 0; i< P.triangles.size(); i++){
    Triangles_interface_prev[i].resize(P.Triangles_interface_prev[i].size());
    for(int j=0; j<P.Triangles_interface_prev[i].size(); j++ ){
      Triangles_interface_prev[i][j] = P.Triangles_interface_prev[i][j];
    }
  }
  Position_Triangles_interface.resize(P.Position_Triangles_interface.size(), std::vector< std::vector<int> >(0));
  for(int i= 0; i< P.triangles.size(); i++){
    Position_Triangles_interface[i].resize(P.Position_Triangles_interface[i].size());
    for(int j=0; j<P.Position_Triangles_interface[i].size(); j++ ){
      Position_Triangles_interface[i][j].resize(P.Position_Triangles_interface[i][j].size());
      for(int k=0; k<P.Position_Triangles_interface[i][j].size(); k++ ){
	Position_Triangles_interface[i][j][k] = P.Position_Triangles_interface[i][j][k];
      }
    }
  }
	
  Position_Triangles_interface_prev.resize(P.Position_Triangles_interface_prev.size(), std::vector< std::vector<int> >(0));
  for(int i= 0; i< P.triangles.size(); i++){
    Position_Triangles_interface_prev[i].resize(P.Position_Triangles_interface_prev[i].size());
    for(int j=0; j<P.Position_Triangles_interface_prev[i].size(); j++ ){
      Position_Triangles_interface_prev[i][j].resize(P.Position_Triangles_interface_prev[i][j].size());
      for(int k=0; k<P.Position_Triangles_interface_prev[i][j].size(); k++ ){
	Position_Triangles_interface_prev[i][j][k] = P.Position_Triangles_interface_prev[i][j][k];
      }
    }
  }
	
}
/*!\brief Test display function.
 */
void Particule::Affiche(){
	
  for(int i=0; i<faces.size(); i++){
    cout<<"face "<<i<<endl;
    cout<<" voisin "<<faces[i].voisin<<endl;
    for(int j=0; j<faces[i].size() ;j++){
      cout<<" vertex "<<faces[i].vertex[j].num<<endl;
      for(int k=0; k<faces[i].vertex[j].particules.size();k++){
	cout<<faces[i].vertex[j].particules[k]<<endl;
      }
    }
  }

}

/*!\brief Sign function
 */
inline double signe(const double x)
{
  return (x < 0.) ? -1. : 1. ;
}
/*!\brief Time-integration of the particle position.
 * \param dt Time-step
 * \details  \warning Uses \a Particule.Ff and \a Particule.Mf (fluid forces and torques moments fluides applied on the solid between times t and t+dt/2) \n
 <b>\a Particule.Ff and \a Particule.Mf are specific coupling parameters ! </b>
 * \return void
 */
void Particule::solve_position(double dt){
  double rot[3][3];
  if(fixe==1){
    Dx = Vector_3(0.,0.,0.);
    Dxprev = Vector_3(0.,0.,0.);
    u = Vector_3(0.,0.,0.);
    u_half = u;
    e = Vector_3(0.,0.,0.);
    rot[0][0] = rot[1][1]= rot[2][2] =1.;
    rot[0][1]=rot[0][2]=rot[1][0]=rot[1][2]=rot[2][0]=rot[2][1]=0.;
    eprev = Vector_3(0.,0.,0.);
    omega = Vector_3(0.,0.,0.);
    omega_half = omega;
  } else {
    if(fixe==0){ //fixe=0: free moving particle
      Dxprev = Dx;
      u = u+(Fi+Ff)/2.*(dt/m);
      u_half = u;
      Dx = Dx+u*dt;
    }
    else if(fixe==2 || fixe==3){//fixe=2: fixed particle ; fixe=3: fixee en deplacement et rotation seulement selon l'axe y
      Dx = Vector_3(0.,0.,0.);
      Dxprev = Vector_3(0.,0.,0.);
      u = Vector_3(0.,0.,0.);
      u_half = u;
    }
      
    //Check that the matrix is a rotation matrix
    for(int i=0;i<3;i++){
      double norm = rotref[i][0]*rotref[i][0]+rotref[i][1]*rotref[i][1]+rotref[i][2]*rotref[i][2];
      if(abs(norm-1.)>eps){
	cout << "Matrice de rotation rotref renomalisee" <<endl;
	cout << "Ligne " << i << " norme=" << norm << endl;
      }
    }
    double vectrot1 = rotref[0][2]-(rotref[1][0]*rotref[2][1]-rotref[2][0]*rotref[1][1]);
    double vectrot2 = rotref[1][2]-(rotref[2][0]*rotref[0][1]-rotref[0][0]*rotref[2][1]);
    double vectrot3 = rotref[2][2]-(rotref[0][0]*rotref[1][1]-rotref[1][0]*rotref[0][1]);
    if(vectrot1*vectrot1+vectrot2*vectrot2+vectrot3*vectrot3>eps){
      cout << "Erreur rotation rotref " << vectrot1 << " " << vectrot2 << " " << vectrot3 << endl;
      //getchar();
    }
    //Computation of the total rotation matrix from the inertial frame to the current frame at time t and storage of eprev
    double Q[3][3];
    double e0 = sqrt(1.-CGAL::to_double(e.squared_length()));
    //Recover the rotation matrix
    rot[0][0] = 1.-2.*CGAL::to_double(e.operator[](1)*e.operator[](1)+e.operator[](2)*e.operator[](2));
    rot[0][1] = 2.*CGAL::to_double(-e0*e.operator[](2)+e.operator[](0)*e.operator[](1));
    rot[0][2] = 2.*CGAL::to_double(e0*e.operator[](1)+e.operator[](0)*e.operator[](2));
    rot[1][0] = 2.*CGAL::to_double(e0*e.operator[](2)+e.operator[](1)*e.operator[](0));
    rot[1][1] = 1.-2.*CGAL::to_double(e.operator[](0)*e.operator[](0)+e.operator[](2)*e.operator[](2));
    rot[1][2] = 2.*CGAL::to_double(-e0*e.operator[](0)+e.operator[](1)*e.operator[](2));
    rot[2][0] = 2.*CGAL::to_double(-e0*e.operator[](1)+e.operator[](2)*e.operator[](0));
    rot[2][1] = 2.*CGAL::to_double(e0*e.operator[](0)+e.operator[](2)*e.operator[](1));
    rot[2][2] = 1.-2.*CGAL::to_double(e.operator[](0)*e.operator[](0)+e.operator[](1)*e.operator[](1));
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	Q[i][j] = rot[i][0]*rotref[0][j];
	Q[i][j] += rot[i][1]*rotref[1][j];
	Q[i][j] += rot[i][2]*rotref[2][j];
      }
    }    
    eprev = e;
    //Recover the global e from omega
    double Omega[3];
    Omega[0] = Omega[1] = Omega[2] = 0.;
    for(int j=0;j<3;j++){
      for(int k=0;k<3;k++){
	Omega[j] += CGAL::to_double(omega.operator[](k)*Q[k][j]);
      }
    }
    double norm = dt*(abs(Omega[0])+abs(Omega[1])+abs(Omega[2]));
    if(norm>0.25){
      cout << "pas de temps trop grand : dt=" << dt << " Omega=" << Omega[0] << " " << Omega[1] << " " << Omega[2] << endl;
    }
    double eglob0 = 1.;
    double eglob[3];
    for(int j=0;j<3;j++){
      eglob[j] = 0.;
    }
    //Recover matrix Zn
    double z[3][3];
    z[0][0] = 0.;
    z[0][1] = -Omega[2];
    z[0][2] = Omega[1];
    z[1][0] = Omega[2];
    z[1][1] = 0.;
    z[1][2] = -Omega[0];
    z[2][0] = -Omega[1];
    z[2][1] = Omega[0];
    z[2][2] = 0.;
    //Compute matrix A
    double a[3];
    double d1 = (I[0]+I[1]+I[2])/2.-I[0];
    double d2 = (I[0]+I[1]+I[2])/2.-I[1];
    double d3 = (I[0]+I[1]+I[2])/2.-I[2];
    //Compute the torque in the inertial frame
    double Mx = CGAL::to_double(Q[0][0]*((Mi+Mf).operator[](0))+Q[1][0]*((Mi+Mf).operator[](1))+Q[2][0]*((Mi+Mf).operator[](2)));
    double My = CGAL::to_double(Q[0][1]*((Mi+Mf).operator[](0))+Q[1][1]*((Mi+Mf).operator[](1))+Q[2][1]*((Mi+Mf).operator[](2)));
    double Mz = CGAL::to_double(Q[0][2]*((Mi+Mf).operator[](0))+Q[1][2]*((Mi+Mf).operator[](1))+Q[2][2]*((Mi+Mf).operator[](2)));
    norm = dt*dt/2.*(abs(Mx)/I[0]+abs(My)/I[1]+abs(Mz)/I[2]);
    if(norm>0.25){
      cout << "The time-step is too large: dt=" << dt << " M=" << Mx << " " << My << " " << Mz << " I=" << I[0] << " " << I[1] << " " << I[2] << endl;
      //getchar();
    }
    a[0] = -(I[0]*z[1][2]-dt/2.*Mx);
    a[1] = (I[1]*z[0][2]+dt/2.*My);
    a[2] = -(I[2]*z[0][1]-dt/2.*Mz);
    //Resolution of the nonlinear problem
    double etemp0 = 1.;
    double etemp1 = 0.;
    double etemp2 = 0.;
    double etemp3 = 0.;
    double err1 = 1.;
    double err2 = 1.;
    double err3 = 1.;
    double epsilon = 1.e-15;
    int k=0;
    for(k=0; k<1000 && (err1>epsilon || err2>epsilon || err3>epsilon); k++){
      if(d2+d3<eps){
	cout << "d2+d3=" << d2+d3 << " I[0]=" << I[0] << endl;
      }
      if(d3+d1<eps){
	cout << "d3+d1=" << d3+d1 << " I[1]=" << I[1] << endl;
      }
      if(d1+d2<eps){
	cout << "d1+d2=" << d1+d2 << " I[2]=" << I[2] << endl;
      }
      double x1 = (dt*a[0]-2.*(d2-d3)*etemp2*etemp3)/(2.*(d2+d3)*etemp0);
      double x2 = (dt*a[1]-2.*(d3-d1)*etemp1*etemp3)/(2.*(d1+d3)*etemp0);
      double x3 = (dt*a[2]-2.*(d1-d2)*etemp1*etemp2)/(2.*(d1+d2)*etemp0);
      etemp1 = x1;
      etemp2 = x2;
      etemp3 = x3;
      if(fixe==3){//fixe=3: the rotation is fixed in x and z
	etemp1=0.;
	etemp3=0.;
      }
      
      if(etemp1*etemp1+etemp2*etemp2+etemp3*etemp3>0.5){
	etemp1 /=2.;
	etemp2 /=2.;
	etemp3 /=2.;
	etemp0 = sqrt(1.-etemp1*etemp1-etemp2*etemp2-etemp3*etemp3);
      }
      else{etemp0 = sqrt(1.-etemp1*etemp1-etemp2*etemp2-etemp3*etemp3);}
      err1 = fabs((dt*a[0]-2.*(d2-d3)*etemp2*etemp3)/(2.*(d2+d3)*etemp0)-etemp1);
      err2 = fabs((dt*a[1]-2.*(d3-d1)*etemp1*etemp3)/(2.*(d1+d3)*etemp0)-etemp2);
      err3 = fabs((dt*a[2]-2.*(d1-d2)*etemp1*etemp2)/(2.*(d1+d2)*etemp0)-etemp3);
      if(fixe==3){
	err1 = 0.;
	err3 = 0.;
      }
      
    }
    if(err1>epsilon || err2>epsilon || err3>epsilon){
      cout << "Problem in the resolution of rotation, e1=" << etemp1 << " e2=" << etemp2 << " e3=" << etemp3 << endl;
      cout << "error=" << err1 << " " << err2 << " " << err3 << endl;
    }
    eglob[0] = etemp1;
    eglob[1] = etemp2;
    eglob[2] = etemp3;
    eglob0 = etemp0;
    //Reconstruction of Z^n+1/2
    z[0][0] = (-2.*(eglob[1]*eglob[1]+eglob[2]*eglob[2]))/dt;
    z[0][1] = (-2.*eglob0*eglob[2]+2.*eglob[0]*eglob[1])/dt;
    z[0][2] = (2.*eglob0*eglob[1]+2.*eglob[0]*eglob[2])/dt;
    z[1][0] = (2.*eglob0*eglob[2]+2.*eglob[0]*eglob[1])/dt;
    z[1][1] = (-2.*(eglob[0]*eglob[0]+eglob[2]*eglob[2]))/dt;
    z[1][2] = (-2.*eglob0*eglob[0]+2.*eglob[1]*eglob[2])/dt;
    z[2][0] = (-2.*eglob0*eglob[1]+2.*eglob[0]*eglob[2])/dt;
    z[2][1] = (2.*eglob0*eglob[0]+2.*eglob[1]*eglob[2])/dt;
    z[2][2] = (-2.*(eglob[0]*eglob[0]+eglob[1]*eglob[1]))/dt;
    //Update of matrix Q
    double Qprev[3][3];
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	Qprev[i][j] = Q[i][j];
      }
    }
    Q[0][0] = Qprev[0][0]*(1.+dt*z[0][0])+Qprev[0][1]*dt*z[1][0]+Qprev[0][2]*dt*z[2][0];
    Q[0][1] = Qprev[0][0]*dt*z[0][1]+Qprev[0][1]*(1.+dt*z[1][1])+Qprev[0][2]*dt*z[2][1];
    Q[0][2] = Qprev[0][0]*dt*z[0][2]+Qprev[0][1]*dt*z[1][2]+Qprev[0][2]*(1.+dt*z[2][2]);
    Q[1][0] = Qprev[1][0]*(1.+dt*z[0][0])+Qprev[1][1]*dt*z[1][0]+Qprev[1][2]*dt*z[2][0];
    Q[1][1] = Qprev[1][0]*dt*z[0][1]+Qprev[1][1]*(1.+dt*z[1][1])+Qprev[1][2]*dt*z[2][1];
    Q[1][2] = Qprev[1][0]*dt*z[0][2]+Qprev[1][1]*dt*z[1][2]+Qprev[1][2]*(1.+dt*z[2][2]);
    Q[2][0] = Qprev[2][0]*(1.+dt*z[0][0])+Qprev[2][1]*dt*z[1][0]+Qprev[2][2]*dt*z[2][0];
    Q[2][1] = Qprev[2][0]*dt*z[0][1]+Qprev[2][1]*(1.+dt*z[1][1])+Qprev[2][2]*dt*z[2][1];
    Q[2][2] = Qprev[2][0]*dt*z[0][2]+Qprev[2][1]*dt*z[1][2]+Qprev[2][2]*(1.+dt*z[2][2]);
    //Check that the matrix is still a rotation matrix
    for(int i=0;i<3;i++){
      double norm = Q[i][0]*Q[i][0]+Q[i][1]*Q[i][1]+Q[i][2]*Q[i][2];
      Q[i][0] /= norm;
      Q[i][1] /= norm;
      Q[i][2] /= norm;
      if(abs(norm-1.)>eps){
	cout << "Normalized rotation matrix" <<endl;
	cout << "Line " << i << " norm=" << norm << endl;
      }
    }
    double vect1 = Q[0][2]-(Q[1][0]*Q[2][1]-Q[2][0]*Q[1][1]);
    double vect2 = Q[1][2]-(Q[2][0]*Q[0][1]-Q[0][0]*Q[2][1]);
    double vect3 = Q[2][2]-(Q[0][0]*Q[1][1]-Q[1][0]*Q[0][1]);
    if(vect1*vect1+vect2*vect2+vect3*vect3>eps){
      cout << "Error rotation " << vect1 << " " << vect2 << " " << vect3 << endl;
      //getchar();
    }
    //Recover the particle rotation matrix
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	rot[i][j] = Q[i][0]*rotref[j][0];
	rot[i][j] += Q[i][1]*rotref[j][1];
	rot[i][j] += Q[i][2]*rotref[j][2];
      }
    }
    //Compute e from rot
    double q1 = rot[2][1]-rot[1][2];
    double q2 = rot[0][2]-rot[2][0];
    double q3 = rot[1][0]-rot[0][1];
    double e1,e2,e3;
    e1 = signe(q1)*sqrt(max((1.+rot[0][0]-rot[1][1]-rot[2][2])/4.,0.));
    e2 = signe(q2)*sqrt(max((1.+rot[1][1]-rot[0][0]-rot[2][2])/4.,0.));
    e3 = signe(q3)*sqrt(max((1.+rot[2][2]-rot[0][0]-rot[1][1])/4.,0.));
    e = Vector_3(e1,e2,e3);
    
    //Compute Omega^n+1/2
    double omega1 = 0.;
    for(int i=0;i<3;i++){
      omega1 += 1./2./dt*(Q[2][i]*Qprev[1][i]-Qprev[2][i]*Q[1][i]);
    }
    double omega2 = 0.;
    for(int i=0;i<3;i++){
      omega2 += 1./2./dt*(Q[0][i]*Qprev[2][i]-Qprev[0][i]*Q[2][i]);
    }
    double omega3 = 0.;
    for(int i=0;i<3;i++){
      omega3 += 1./2./dt*(Q[1][i]*Qprev[0][i]-Qprev[1][i]*Q[0][i]);
    }
    omega = Vector_3(omega1,omega2,omega3);
    omega_half = omega;
    if(flag_2d){
      omega = Vector_3(0.,0.,omega3);
      omega_half = omega;
    }
  }
  /*Test: block rotation
    rot[0][0]= rot[1][1] = rot[2][2] =1.;
    rot[0][1] = rot[0][2] =rot[1][0] = rot[1][2] = rot[2][0] = rot[2][1] = 0.;
    //rotprev[0][0]= rotprev[1][1] = rotprev[2][2] =1.;
    //rotprev[0][1] = rotprev[0][2] =rotprev[1][0] = rotprev[1][2] = rotprev[2][0] = rotprev[2][1] = 0.;

    omega = Vector_3(0.,0.,0.);
    omega_half = omega;
    //end test */

  //Update the particle movement
  mvt_tprev = mvt_t;
  Aff_transformation_3 rotation(rot[0][0],rot[0][1],rot[0][2],rot[1][0],rot[1][1],rot[1][2],rot[2][0],rot[2][1],rot[2][2]);
  Aff_transformation_3 translation(CGAL::TRANSLATION,Vector_3(Point_3(0.,0.,0.),x0)+Dx);
  Aff_transformation_3 translation_inv(CGAL::TRANSLATION,Vector_3(x0,Point_3(0.,0.,0.)));
  mvt_t = translation*(rotation*translation_inv);
}

/*!
 * \fn void Particule::solve_vitesse(double dt)
 * \brief Time-integration of the particle velocity.
 * \param dt Time-step
 \details  \warning Use of \a Particule.Ff and \a Particule.Mf (fluid forces and torques applied on the solid between times t and t+dt) \n
 <b>\a Particule.Ff and \a Particule.Mf are specific coupling parameters ! </b>
 * \return void
 */
void Particule::solve_vitesse(double dt){
  if(fixe==1){
    u = Vector_3(0.,0.,0.);
    omega = Vector_3(0.,0.,0.);
  } else {
    if(fixe==0){
      u = u+(Fi+Ff)/2.*(dt/m);
    }
    else if(fixe==2 || fixe==3){
      u = Vector_3(0.,0.,0.);
    }
    
    //Compute the total rotation matrix from the inertial frame to the current frame at time t
    double Q[3][3];
    double e0 = sqrt(1.-CGAL::to_double(e.squared_length()));
    //Recover the rotation matrix
    double rot[3][3];
    rot[0][0] = 1.-2.*CGAL::to_double(e.operator[](1)*e.operator[](1)+e.operator[](2)*e.operator[](2));
    rot[0][1] = 2.*CGAL::to_double(-e0*e.operator[](2)+e.operator[](0)*e.operator[](1));
    rot[0][2] = 2.*CGAL::to_double(e0*e.operator[](1)+e.operator[](0)*e.operator[](2));
    rot[1][0] = 2.*CGAL::to_double(e0*e.operator[](2)+e.operator[](1)*e.operator[](0));
    rot[1][1] = 1.-2.*CGAL::to_double(e.operator[](0)*e.operator[](0)+e.operator[](2)*e.operator[](2));
    rot[1][2] = 2.*CGAL::to_double(-e0*e.operator[](0)+e.operator[](1)*e.operator[](2));
    rot[2][0] = 2.*CGAL::to_double(-e0*e.operator[](1)+e.operator[](2)*e.operator[](0));
    rot[2][1] = 2.*CGAL::to_double(e0*e.operator[](0)+e.operator[](2)*e.operator[](1));
    rot[2][2] = 1.-2.*CGAL::to_double(e.operator[](0)*e.operator[](0)+e.operator[](1)*e.operator[](1));
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	Q[i][j] = rot[i][0]*rotref[0][j];
	Q[i][j] += rot[i][1]*rotref[1][j];
	Q[i][j] += rot[i][2]*rotref[2][j];
      }
    }    
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	Q[i][j] = rot[i][0]*rotref[0][j];
	Q[i][j] += rot[i][1]*rotref[1][j];
	Q[i][j] += rot[i][2]*rotref[2][j];
      }
    }    
    //Recover Zn+1/2 from omega
    double Omega[3];
    Omega[0] = Omega[1] = Omega[2] = 0.;
    for(int j=0;j<3;j++){
      for(int k=0;k<3;k++){
	Omega[j] += CGAL::to_double(omega.operator[](k)*Q[k][j]);
      }
    }
    double norm2 = dt*dt*(Omega[0]*Omega[0]+Omega[1]*Omega[1]+Omega[2]*Omega[2]);
    if(norm2>1.){
      cout << "The time-step is too large : dt=" << dt << " Omega=" << Omega[0] << " " << Omega[1] << " " << Omega[2] << endl;
      getchar();
    }
    double eglob0 = sqrt((1.+sqrt(1.-norm2))/2.);
    double eglob[3];
    for(int j=0;j<3;j++){
      eglob[j] = dt*Omega[j]/2./eglob0;
    }
    //Recover matrix Zn+1/2
    double z[3][3];
    z[0][0] = (-2.*(eglob[1]*eglob[1]+eglob[2]*eglob[2]))/dt;
    z[0][1] = (-2.*eglob0*eglob[2]+2.*eglob[0]*eglob[1])/dt;
    z[0][2] = (2.*eglob0*eglob[1]+2.*eglob[0]*eglob[2])/dt;
    z[1][0] = (2.*eglob0*eglob[2]+2.*eglob[0]*eglob[1])/dt;
    z[1][1] = (-2.*(eglob[0]*eglob[0]+eglob[2]*eglob[2]))/dt;
    z[1][2] = (-2.*eglob0*eglob[0]+2.*eglob[1]*eglob[2])/dt;
    z[2][0] = (-2.*eglob0*eglob[1]+2.*eglob[0]*eglob[2])/dt;
    z[2][1] = (2.*eglob0*eglob[0]+2.*eglob[1]*eglob[2])/dt;
    z[2][2] = (-2.*(eglob[0]*eglob[0]+eglob[1]*eglob[1]))/dt;
    //Compute matrix A
    double a[3];
    double d1 = (I[0]+I[1]+I[2])/2.-I[0];
    double d2 = (I[0]+I[1]+I[2])/2.-I[1];
    double d3 = (I[0]+I[1]+I[2])/2.-I[2];
    //Compute the torque in the inertial frame
    double Mx = CGAL::to_double(Q[0][0]*((Mi+Mf).operator[](0))+Q[1][0]*((Mi+Mf).operator[](1))+Q[2][0]*((Mi+Mf).operator[](2)));
    double My = CGAL::to_double(Q[0][1]*((Mi+Mf).operator[](0))+Q[1][1]*((Mi+Mf).operator[](1))+Q[2][1]*((Mi+Mf).operator[](2)));
    double Mz = CGAL::to_double(Q[0][2]*((Mi+Mf).operator[](0))+Q[1][2]*((Mi+Mf).operator[](1))+Q[2][2]*((Mi+Mf).operator[](2)));
    a[0] = -(d2*z[1][2]-d3*z[2][1]-dt/2.*Mx);
    a[1] = (d1*z[0][2]-d3*z[2][0]+dt/2.*My);
    a[2] = -(d1*z[0][1]-d2*z[1][0]-dt/2.*Mz);
    //Resolution of the linear problem on Zn+1
    z[0][0] = 0.;
    z[0][1] = -a[2]/I[2];
    z[0][2] = a[1]/I[1];
    z[1][0] = -z[0][1];
    z[1][1] = 0.;
    z[1][2] = -a[0]/I[0];
    z[2][0] = -z[0][2];
    z[2][1] = -z[1][2];
    z[2][2]= 0.;
    //Compute Omega^n+1
    double omega1 = 0.;
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	omega1 -= Q[1][i]*z[i][j]*Q[2][j];
      }
    }
    double omega2 = 0.;
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	omega2 += Q[0][i]*z[i][j]*Q[2][j];
      }
    }
    double omega3 = 0.;
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
	omega3 -= Q[0][i]*z[i][j]*Q[1][j];
      }
    }
//     //Test: block the x and y components of rotation
    if(flag_2d){
      omega1 = 0.;
      omega2 = 0.;
    }
    if(fixe==3){
      omega1=0.;
      omega3=0.;
    }
			
// 		//end test 
    omega = Vector_3(omega1,omega2,omega3);
    /*Test: block rotation
      rot[0][0]= rot[1][1] = rot[2][2] =1.;
      rot[0][1] = rot[0][2] =rot[1][0] = rot[1][2] = rot[2][0] = rot[2][1] = 0.;
      //rotprev[0][0]= rotprev[1][1] = rotprev[2][2] =1.;
      //rotprev[0][1] = rotprev[0][2] =rotprev[1][0] = rotprev[1][2] = rotprev[2][0] = rotprev[2][1] = 0.;
      omega = Vector_3(0.,0.,0.);
      omega_half = omega;
      //end test */
  }
}

/*!\brief Test function to check the conservation of the volume of the particle.
 * \return double
 */
double Particule::volume(){
	
  double vol = 0.;
	
  Point_3 center;
	
  std::vector<Point_3> Points_poly; 
	
  for(int l= 0; l<triangles.size(); l++)
  {
    Points_poly.push_back(triangles[l].operator[](0));
    Points_poly.push_back(triangles[l].operator[](1));
    Points_poly.push_back(triangles[l].operator[](2));
  }	
  center = centroid(Points_poly.begin(),Points_poly.end());
	
  for(int l= 0; l<triangles.size(); l++)
  {
    Tetrahedron tetra(center, triangles[l].operator[](0), triangles[l].operator[](1), triangles[l].operator[](2));
    vol += std::abs(CGAL::to_double(tetra.volume()));
  }
	
  return vol;
}

/*!\brief Velocity of the solid at the interface elements center at time t. \n
 * \details \f$ V_f = V_I + \Omega_{rot} \wedge \left( X_f - X_I \right). \f$ 
 * \f$ V_I \f$ is the particle velocity (\a Particule.u_half),  \f$ X_I \f$ is the particle center (\a Particule.x0 + \a Particule.Dx),  \f$ \Omega_{rot} \f$  is the particle rotation (\a Particule.omega_half).
 * \param X_f interface element center
 * \warning <b> Specific coupling procedure ! </b>
 * \return Vector_3
 */
Vector_3 Particule::vitesse_parois(const Point_3& X_f){
		
  Vector_3 V_f = u_half + cross_product(omega_half, Vector_3(x0 + Dx,X_f));

  return V_f;
}	
/*!\brief Velocity at the interface elements center at time t-dt.
 * \f$ V_f = V_I + \Omega_{rot} \wedge \left( X_f - X_I \right). \f$ \n
 * \f$ V_I \f$ is the particle velocity (\a Particule.u_half),  \f$ X_I \f$ is the particle center (\a Particule.x0 + \a Particule.Dxprev),  \f$ \Omega_{rot} \f$ is the particle rotation (\a Particule.omega_half).
 * \param X_f interface element center
 * \warning <b> Specific coupling procedure ! </b>
 * \return Vector_3
 */
Vector_3 Particule::vitesse_parois_prev(const Point_3& X_f){
	
  Vector_3 V_f = u_half + cross_product(omega_half, Vector_3(x0 + Dxprev,X_f));
	
  return V_f;
}	

/*!\brief Computation of projection integrals.
 * \details Uses the function described by <a href="http://www.cs.berkeley.edu/~jfc/mirtich/code/volumeIntegration.tar">Brian Mirtich (1996)</a>.
 * \return void
 */
void Face::compProjectionIntegrals(double &P1, double &Pa, double &Pb, double &Paa, double &Pab, double &Pbb, double &Paaa, double &Paab, double &Pabb, double &Pbbb, int a, int b, int c){
  P1 = Pa = Pb = Paa = Pab = Pbb = Paaa = Paab = Pabb = Pbbb = 0.;
  for(int i=0;i<size();i++){
    int j= (i+1)%(size());
    double a0 = CGAL::to_double(vertex[i].pos.operator[](a));
    double b0 = CGAL::to_double(vertex[i].pos.operator[](b));
    double a1 = CGAL::to_double(vertex[j].pos.operator[](a));
    double b1 = CGAL::to_double(vertex[j].pos.operator[](b));
    double Da = a1-a0;
    double Db = b1-b0;
    double a02 = a0*a0;
    double a03 = a0*a02;
    double a04 = a0*a03;
    double b02 = b0*b0;
    double b03 = b0*b02;
    double b04 = b0*b03;
    double a12 = a1*a1;
    double a13 = a1*a12;
    double b12 = b1*b1;
    double b13 = b1*b12;
    double C1 = a1+a0;
    double Ca = a1*C1+a02;
    double Caa = a1*Ca+a03;
    double Caaa = a1*Caa+a04;
    double Cb = b12+b1*b0+b02;
    double Cbb = b1*Cb+b03;
    double Cbbb = b1*Cbb+b04;
    double Cab = 3.*a12+2.*a1*a0+a02;
    double Kab = a12+2.*a1*a0+3.*a02;
    double Caab = a0*Cab+4.*a13;
    double Kaab = a1*Kab+4.*a03;
    double Cabb = 4.*b13+3.*b12*b0+2.*b1*b02+b03;
    double Kabb = b13+2.*b12*b0+3.*b1*b02+4.*b03;
    P1 += Db*C1;
    Pa += Db*Ca; Paa += Db*Caa; Paaa += Db*Caaa;
    Pb += Da*Cb; Pbb += Da*Cbb; Pbbb += Da*Cbbb;
    Pab += Db*(b1*Cab+b0*Kab);
    Paab += Db*(b1*Caab+b0*Kaab);
    Pabb += Da*(a1*Cabb+a0*Kabb);
  }
  P1 /= 2.;
  Pa /= 6.; Paa /= 12.; Paaa /= 20.;
  Pb /=-6.; Pbb /=-12.; Pbbb /=-20.;
  Pab /= 24.;
  Paab /= 60.;
  Pabb /= -60.;
}
/*!\brief Compute face integrals. 
 * \details Uses the function described by <a href="http://www.cs.berkeley.edu/~jfc/mirtich/code/volumeIntegration.tar">Brian Mirtich (1996)</a>.
 * \return void
 */
void Face::compFaceIntegrals(double &Fa, double &Fb, double &Fc, double &Faa, double &Fbb, double &Fcc, double &Faaa, double &Fbbb, double &Fccc, double &Faab, double &Fbbc, double &Fcca, double na, double nb, double nc, int a, int b, int c){
  double P1,Pa,Pb,Paa,Pab,Pbb,Paaa,Paab,Pabb,Pbbb;
  Vector_3 p(Point_3(0,0,0),vertex[0].pos);
  double w = -CGAL::to_double(normale*p);
  double k1 = 1./nc;
  double k2 = k1*k1;
  double k3 = k1*k2;
  double k4 = k1*k3;
  compProjectionIntegrals(P1, Pa, Pb, Paa, Pab, Pbb, Paaa, Paab, Pabb, Pbbb, a, b, c);
  Fa = k1*Pa;
  Fb = k1*Pb;
  Fc = -k2*(na*Pa+nb*Pb+w*P1);
  Faa = k1*Paa;
  Fbb = k1*Pbb;
  Fcc = k3*(na*na*Paa+2.*na*nb*Pab+nb*nb*Pbb+2.*na*w*Pa+2.*nb*w*Pb+w*w*P1);
  Faaa = k1*Paaa;
  Fbbb = k1*Pbbb;
  Fccc = -k4*(na*na*na*Paaa+3.*na*na*nb*Paab+3.*na*nb*nb*Pabb+nb*nb*nb*Pbbb+3.*na*na*w*Paa+6.*na*nb*w*Pab+3.*nb*nb*w*Pbb+3.*na*w*w*Pa+3.*nb*w*w*Pb+w*w*w*P1);
  Faab = k1*Paab;
  Fbbc = -k2*(na*Pabb+nb*Pbbb+w*Pbb);
  Fcca = k3*(na*na*Paaa+2.*na*nb*Paab+nb*nb*Pabb+2.*na*w*Paa+2.*nb*w*Pab+w*w*Pa);
}

/*!\brief Computation of volume integrals.
 *\details Uses the function described by <a href="http://www.cs.berkeley.edu/~jfc/mirtich/code/volumeIntegration.tar">Brian Mirtich (1996)</a>.
 * \return void
 */
void Particule::CompVolumeIntegrals(double &T1, double &Tx, double &Ty, double &Tz, double &Txx, double &Tyy, double &Tzz, double &Txy, double &Tyz, double &Tzx){
  T1 = Tx=Ty=Tz=Txx=Tyy=Tzz=Txy=Tyz=Tzx=0.;
  for(int i=0;i<faces.size();i++){
    double Fx,Fy,Fz,Fxx,Fyy,Fzz,Fxxx,Fyyy,Fzzz,Fxxy,Fyyz,Fzzx;
    double nx,ny,nz,na,nb,nc;
    nx=CGAL::to_double(faces[i].normale.operator[](0));
    ny=CGAL::to_double(faces[i].normale.operator[](1));
    nz=CGAL::to_double(faces[i].normale.operator[](2));
    //Choice of an oriented permutation abc such that nc is maximal
    if(abs(nx)>abs(ny)){
      if(abs(nx)>abs(nz)){
	//Case a=y,b=z,c=x
	na = ny; nb = nz; nc = nx;
	faces[i].compFaceIntegrals(Fy, Fz, Fx, Fyy, Fzz, Fxx, Fyyy, Fzzz, Fxxx, Fyyz, Fzzx, Fxxy,na,nb,nc,1,2,0);
      }
      else {
	//Case a=x,b=y,c=z
	na = nx; nb = ny; nc = nz;
	faces[i].compFaceIntegrals(Fx, Fy, Fz, Fxx, Fyy, Fzz, Fxxx, Fyyy, Fzzz, Fxxy, Fyyz, Fzzx,na,nb,nc,0,1,2);
      }
    }
    else {
      if(abs(ny)>abs(nz)){
	//Case a=z,b=x,c=y
	na = nz; nb = nx; nc = ny;
	faces[i].compFaceIntegrals(Fz, Fx, Fy, Fzz, Fxx, Fyy, Fzzz, Fxxx, Fyyy, Fzzx, Fxxy, Fyyz,na,nb,nc,2,0,1);
      }
      else{
	//Case a=x,b=y,c=z
	na = nx; nb = ny; nc = nz;
	faces[i].compFaceIntegrals(Fx, Fy, Fz, Fxx, Fyy, Fzz, Fxxx, Fyyy, Fzzz, Fxxy, Fyyz, Fzzx,na,nb,nc,0,1,2);
      }
    }
    //Computation of integrals
    T1 += nx*Fx;
    Tx += nx*Fxx;
    Ty += ny*Fyy;
    Tz += nz*Fzz;
    Txx += nx*Fxxx;
    Tyy += ny*Fyyy;
    Tzz += nz*Fzzz;
    Txy += nx*Fxxy;
    Tyz += ny*Fyyz;
    Tzx += nz*Fzzx;
  }
  Tx /= 2.;
  Ty /= 2.;
  Tz /= 2.;
  Txx /=3.;
  Tyy /=3.;
  Tzz /=3.;
  Txy /=2.;
  Tyz /=2.;
  Tzx /=2.;
}

struct Mat3x3
{
  double tab[3][3];
};

struct Vect3
{
  double vec[3];
};

//!\brief Function rot for subroutine jacobi3x3
inline void rot(Mat3x3 &a, const double s, const double tau, const int i, const int j, const int k, const int l)
{
  double g,h;
  
  g = a.tab[i][j];
  h = a.tab[k][l];
  a.tab[i][j] = g-s*(h+g*tau);
  a.tab[k][l] = h+s*(g-h*tau);
}

/*!\brief Diagonalization of a 3x3 matrix a using Jacobi's method
  \details taken from Numerical Recipes C++ \n
  \param a Matrix diagonalized
  \param d Eigenvalues diagonal
  \param v Eigenvector matrix
  \param nrot Number of Jacobi iterations
*/
void jacobi3x3(Mat3x3 &a, Vect3 &d, Mat3x3 &v, int &nrot)
{
  int i,j,ip,iq;
  double tresh,theta,tau,t,sm,s,h,g,c;
  
  const int n=3;
  double b[n],z[n];
  //Initialization of v to identity
  for(ip=0;ip<n;ip++){
    for(iq=0;iq<n;iq++){
      v.tab[ip][iq]=0.;
    }
    v.tab[ip][ip]=1.;
  }
  //Initialization of b and d to the diagonal of a
  for(ip=0;ip<n;ip++){
    b[ip]=d.vec[ip]=a.tab[ip][ip];
    z[ip]=0.;
  }
  nrot = 0;
  for(i=1;i<=50;i++){
    sm=0.;
    //Sum of the off-diagonal elements magnitudes
    for(ip=0;ip<n-1;ip++){
      for(iq=ip+1;iq<n;iq++){
	sm+=fabs(a.tab[ip][iq]);
      }
    }
    //If there is exact convergence
    if(sm==0.){
      return;
    }
    //On the three first sweeps
    if(i<4){
      tresh=0.2*sm/(n*n);
    } 
    //then...
    else {
      tresh=0.;
    }
    //Beginning of the sweep
    for(ip=0;ip<n-1;ip++){
      for(iq=ip+1;iq<n;iq++){
	g = 100.*fabs(a.tab[ip][iq]);
	//After 4 sweeps, skip rotation if the off-diagonal element is small
	if(i>4 && (fabs(d.vec[ip])+g)==fabs(d.vec[ip]) && (fabs(d.vec[iq])+g)==fabs(d.vec[iq])){
	  a.tab[ip][iq]=0.;
	}
	else if(fabs(a.tab[ip][iq])>tresh){
	  h=d.vec[iq]-d.vec[ip];
	  if((fabs(h)+g)==fabs(h)){
	    t=(a.tab[ip][iq])/h;
	  } else {
	    theta=0.5*h/(a.tab[ip][iq]);
	    t = 1./(fabs(theta)+sqrt(1.+theta*theta));
	    if(theta<0.){
	      t = -t;
	    }
	  }
	  c = 1./sqrt(1+t*t);
	  s = t*c;
	  tau = s/(1.+c);
	  h = t*a.tab[ip][iq];
	  z[ip] -= h;
	  z[iq] += h;
	  d.vec[ip] -= h;
	  d.vec[iq] += h;
	  a.tab[ip][iq] = 0.;
	  for(j=0;j<ip;j++){
	    rot(a,s,tau,j,ip,j,iq);
	  }
	  for(j=ip+1;j<iq;j++){
	    rot(a,s,tau,ip,j,j,iq);
	  }
	  for(j=iq+1;j<n;j++){
	    rot(a,s,tau,ip,j,iq,j);
	  }
	  for(j=0;j<n;j++){
	    rot(v,s,tau,j,ip,j,iq);
	  }
	  ++nrot;
	}
      }
    }
    for(ip=0;ip<n;ip++){
      b[ip] += z[ip];
      d.vec[ip] = b[ip];
      z[ip] = 0.;
    }
  }
  cout << "The number of iterations in subroutine jacobi3x3 is too large" << endl;
}


/*!\brief Computation of the particle inertia matrix.
 * \return void
 */
void Particule::Inertie(){
  double T1,Tx,Ty,Tz,Txx,Tyy,Tzz,Txy,Tyz,Tzx;
  CompVolumeIntegrals(T1,Tx,Ty,Tz,Txx,Tyy,Tzz,Txy,Tyz,Tzx);
  double R[3][3];
  double xG = CGAL::to_double(x0.operator[](0));
  double yG = CGAL::to_double(x0.operator[](1));
  double zG = CGAL::to_double(x0.operator[](2));
  
  
  R[0][0] = rhos*(Tyy-2.*yG*Ty+yG*yG*T1+Tzz-2.*zG*Tz+zG*zG*T1);
  R[1][0] = R[0][1] = rhos*(Txy-yG*Tx-xG*Ty+xG*yG*T1);
  R[2][0] = R[0][2] = rhos*(Tzx-zG*Tx-xG*Tz+xG*zG*T1);
  R[1][1] = rhos*(Txx-2.*xG*Tx+xG*xG*T1+Tzz-2.*zG*Tz+zG*zG*T1);
  R[1][2] = R[2][1] = rhos*(Tyz-zG*Ty-yG*Tz+yG*zG*T1);
  R[2][2] = rhos*(Tyy-2.*yG*Ty+yG*yG*T1+Txx-2.*xG*Tx+xG*xG*T1);
  
  //Mass and volume
  V = T1;
  m = rhos*T1;
  if(m<eps){
    cout<< "Null mass " << m << endl;
    getchar();
  }
  //Compute the inertia moments
  //Use the Jacobi method
  Mat3x3 A,V;
  for(int i=0;i<3;i++){
    for(int j=0;j<3;j++){
      A.tab[i][j] = R[i][j];
    }
  }
  Vect3 d;
  int n=0;
  jacobi3x3(A,d,V,n);
  for(int i=0;i<3;i++){
    I[i] = d.vec[i];
    for(int j=0;j<3;j++){
      rotref[i][j] = V.tab[j][i];
    }
  }
  
  
  //Check the scalar product of the first two columns
  double scal = rotref[0][0]*rotref[0][1]+rotref[1][0]*rotref[1][1]+rotref[2][0]*rotref[2][1];
  if(abs(scal)>eps){
    cout << "scal=" << scal << endl;
  }
  
  for(int i=0;i<3;i++){
    for(int j=0;j<3;j++){
      if(rotref[i][j]!=rotref[i][j]){
	cout << "rotref "<< rotref[i][j] << " " << i << " " << j << endl;
	getchar();
      }
    }
  }
  
  //CHeck the rotation matrix determinant
  double det = rotref[0][2]*(rotref[1][0]*rotref[2][1]-rotref[2][0]*rotref[1][1]);
  det += rotref[1][2]*(rotref[2][0]*rotref[0][1]-rotref[0][0]*rotref[2][1]);
  det += rotref[2][2]*(rotref[0][0]*rotref[1][1]-rotref[1][0]*rotref[0][1]);
  if(det<0.){
    for(int i=0;i<3;i++){
      rotref[i][2] *= -1.;
    }
  }
  for(int i=0;i<3;i++){
    int j = (i+1)%3;
    if(abs(rotref[0][i]*rotref[0][j]+rotref[1][i]*rotref[1][j]+rotref[2][i]*rotref[2][j])>eps){
      cout << "Error in the inertia moments computation" << endl;
    }
  }
  
  //Compute the face inertia moments (for torsion and flexion)
  for(int i=0;i<faces.size();i++){
    faces[i].Inertie();
  }
}
/*!
 * \brief Computation of the free volume.
 * \return void
 */
void Particule::Volume_libre(){
  Vl = 0.;
  for(int i=0;i<faces.size();i++){
    if(faces[i].voisin == -1){
      Vector_3 v1(faces[i].vertex[0].pos,faces[i].vertex[1].pos);
      Vector_3 v2(faces[i].vertex[0].pos,faces[i].vertex[2].pos);
      Vector_3 v3(x0,faces[i].vertex[0].pos);
      Vl += 1./6.*CGAL::to_double(cross_product(v1,v2)*v3);
    }
  }
}

/*!\brief Default constructor. 
 */
Solide::Solide(){
	
}

/*!\brief Constructor overload.
 *\param Part particle vector. 
 */
Solide::Solide(std::vector<Particule> & Part){
  for(int i=0; i<Part.size(); i++){
    solide.push_back(Part[i]);
  }
}
/*!\brief Destructor.
 */ 
Solide::~Solide(){   
}

/*!\brief Operator = overload.
 *\param S Solide
 *\return Solide
 */
Solide & Solide:: operator=(const Solide &S){
	
  assert(this != &S);
  solide.resize(S.solide.size());
  for(int i=0; i<S.solide.size(); i++){
    solide[i]= S.solide[i];
  }
}

/*!
 *\fn void Solide::Affiche()
 *\brief Test display.
 */
void Solide::Affiche(){
	
  for(int i=0; i<solide.size(); i++){
    cout<<"Particle "<<i<<endl;
    solide[i].Affiche();
  }

}

/*!\brief Initialization of the solid from a mesh file du solide &agrave; partir d'un fichier. 
 *\param s mesh file
 *\return void
 */
void Solide::Init(const char* s){
  std::ifstream maillage(s,ios::in);
  if(!maillage){
    cout <<"Opening " << s << " failed" << endl;
  }

  //Recover the solid mesh
  int Npoint;
  string sp;
  maillage >> sp >> Npoint;
  const int nb_points = Npoint;
  
  vector<Point_3> Points(nb_points);
  
  for(int i=0;i<nb_points;i++){
    double x,y,z;
    maillage >> x >> y >> z;
    Points[i] = Point_3(x,y,z);
  }
  
  int Npart;
  string sP;
  maillage >> sP >> Npart;
  const int nb_particule = Npart;
  
  vector<Particule> P(nb_particule);
  
  bool points_particules[nb_points][nb_particule];
  
  for(int i=0;i<nb_particule;i++){
    for(int j=0;j<nb_points;j++){
      //Clear the array 
      points_particules[j][i] = false;
    }
  }
  for(int i=0;i<nb_particule;i++){
    int Nfaces;
    int fixe;
    double X,Y,Z,u,v,w,theta,phi,psi,xmin,ymin,zmin,xmax,ymax,zmax;
    string s;
    maillage >> s >> Nfaces >> fixe;
    maillage >> s >> X >> Y >> Z;
    Point_3 centre(X,Y,Z);
    maillage >> s >> u >> v >> w;
    maillage >> s >> theta >> phi >> psi;
    xmin = X;
    ymin = Y;
    zmin = Z;
    xmax = X;
    ymax = Y;
    zmax = Z;
    const int nb_faces = Nfaces;
    std::vector<Face> Faces(nb_faces);
    std::vector<Point_3> points_c;
    for(int j=0;j<nb_faces;j++){
      int Nvertex;
      maillage >> Nvertex;
      const int nb_vertex = Nvertex;
      std::vector<Vertex> Vertex(nb_vertex);
      for(int k=0;k<nb_vertex;k++){
	int p;
	maillage >> p;
	Vertex[k].pos = Points[p];
	points_c.push_back(Points[p]);
	Vertex[k].num = p;
	points_particules[p][i] = true;
	double x = CGAL::to_double(Points[p].operator[](0));
	double y = CGAL::to_double(Points[p].operator[](1));
	double z = CGAL::to_double(Points[p].operator[](2));
	xmin = min(x,xmin);
	xmax = max(x,xmax);
	ymin = min(y,ymin);
	ymax = max(y,ymax);
	zmin = min(z,zmin);
	zmax = max(z,zmax);
      }
      int voisin;
      maillage >> voisin;
      Faces[j] = Face(Vertex, voisin);
    }
    
    Point_3 center_part= centroid(points_c.begin(), points_c.end());
	
    if(fixe==0 || fixe==1){
      P[i] = Particule(center_part, xmin, ymin, zmin, xmax, ymax, zmax, Faces);
    } else {
      P[i] = Particule(centre, xmin, ymin, zmin, xmax, ymax, zmax, Faces);
    }
    P[i].fixe = fixe;
    P[i].u = Vector_3(u,v,w);
    P[i].omega = Vector_3(theta,phi,psi);
    P[i].u_half = Vector_3(u,v,w);
    P[i].omega_half = Vector_3(theta,phi,psi);
  }
  //Update particles sharing the vertices
  //Update of the equilibrium distances between particles
  for(int i=0;i<P.size();i++){
    for(int j=0;j<P[i].faces.size();j++){
      for(int k=0;k<P[i].faces[j].size();k++){
	for(int l=0;l<P.size();l++){
	  if(points_particules[P[i].faces[j].vertex[k].num][l]){
	    P[i].faces[j].vertex[k].particules.push_back(l);
	  }
	}
	if(P[i].faces[j].voisin>=0){
	  P[i].faces[j].D0 = sqrt(CGAL::to_double(CGAL::squared_distance(P[i].x0,P[P[i].faces[j].voisin].x0)));
	}
      }
    }
  }

  for(int i=0; i<P.size(); i++){
    solide.push_back(P[i]);
  }
  
  //Initialization of the solid position and inertia
  for(int i=0; i<solide.size(); i++){
    solide[i].Dx = Vector_3(0.,0.,0.);
    solide[i].Dxprev = Vector_3(0.,0.,0.);
    solide[i].Fi = Vector_3(0.,0.,0.);
    solide[i].Ff = Vector_3(0.,0.,0.);
    solide[i].Ffprev = Vector_3(0.,0.,0.);
    solide[i].Mi = Vector_3(0.,0.,0.);
    solide[i].Mf = Vector_3(0.,0.,0.);
    solide[i].Mfprev = Vector_3(0.,0.,0.);
    solide[i].e = Vector_3(0.,0.,0.);
    solide[i].eprev = Vector_3(0.,0.,0.);
    solide[i].Inertie();
    solide[i].mvt_t = Aff_transformation_3(1,0,0,0,1,0,0,0,1);
    solide[i].mvt_tprev = Aff_transformation_3(1,0,0,0,1,0,0,0,1);
  }

  //In the case of restart
  if(rep){
    std::ostringstream oss;
    oss << "resultats/solide" << numrep << ".vtk";
    string s = oss.str();
    const char* nom = s.c_str();
    std::ifstream init(nom,std::ios::in);
    string dump;
    int nb_part = solide.size();
    int nb_triangles = 0.;
    for(int it=0; it<nb_part; it++){
      nb_triangles += solide[it].triangles.size();
    }
    for(int it=0;it<5*nb_triangles+13;it++){
      getline(init,dump);
    }
    cout << dump << endl;
    //Recover displacement
    for(int it=0; it<nb_part; it++){
      double Dx,Dy,Dz;
      init >> Dx >> Dy >> Dz;
      solide[it].Dx = Vector_3(Dx,Dy,Dz);
      for(int l= 0; l<solide[it].triangles.size(); l++){
	getline(init,dump);
      }
    }
    getline(init,dump);
    getline(init,dump);
    cout << dump << endl;
    //Recover velocity
    for(int it=0; it<nb_part; it++){
      double u,v,w;
      init >> u >> v >> w;
      solide[it].u = Vector_3(u,v,w);
      for(int l= 0; l<solide[it].triangles.size(); l++){
	getline(init,dump);
      }
    }
    getline(init,dump);
    getline(init,dump);
    cout << dump << endl;
    //Recover the rotation vector
    for(int it=0; it<nb_part; it++){
      double ex,ey,ez;
      init >> ex >> ey >> ez;
      solide[it].e = Vector_3(ex,ey,ez);
      for(int l= 0; l<solide[it].triangles.size(); l++){
	getline(init,dump);
      }
    }
    getline(init,dump);
    getline(init,dump);
    cout << dump << endl;
    //Recover the angular velocity
    for(int it=0; it<nb_part; it++){
      double omegax,omegay,omegaz;
      init >> omegax >> omegay >> omegaz;
      solide[it].omega = Vector_3(omegax,omegay,omegaz);
      for(int l= 0; l<solide[it].triangles.size(); l++){
	getline(init,dump);
      }
    }
    init.close();
    //Update misc parameters
    for(int i=0; i<solide.size(); i++){
      solide[i].Dxprev = solide[i].Dx;
      solide[i].eprev = solide[i].e;
      double rot[3][3];
      //Recover the rotation matrix
      double e0 = sqrt(1.-CGAL::to_double(solide[i].e.squared_length()));
      rot[0][0] = 1.-2.*CGAL::to_double(solide[i].e.operator[](1)*solide[i].e.operator[](1)+solide[i].e.operator[](2)*solide[i].e.operator[](2));
      rot[0][1] = 2.*CGAL::to_double(-e0*solide[i].e.operator[](2)+solide[i].e.operator[](0)*solide[i].e.operator[](1));
      rot[0][2] = 2.*CGAL::to_double(e0*solide[i].e.operator[](1)+solide[i].e.operator[](0)*solide[i].e.operator[](2));
      rot[1][0] = 2.*CGAL::to_double(e0*solide[i].e.operator[](2)+solide[i].e.operator[](1)*solide[i].e.operator[](0));
      rot[1][1] = 1.-2.*CGAL::to_double(solide[i].e.operator[](0)*solide[i].e.operator[](0)+solide[i].e.operator[](2)*solide[i].e.operator[](2));
      rot[1][2] = 2.*CGAL::to_double(-e0*solide[i].e.operator[](0)+solide[i].e.operator[](1)*solide[i].e.operator[](2));
      rot[2][0] = 2.*CGAL::to_double(-e0*solide[i].e.operator[](1)+solide[i].e.operator[](2)*solide[i].e.operator[](0));
      rot[2][1] = 2.*CGAL::to_double(e0*solide[i].e.operator[](0)+solide[i].e.operator[](2)*solide[i].e.operator[](1));
      rot[2][2] = 1.-2.*CGAL::to_double(solide[i].e.operator[](0)*solide[i].e.operator[](0)+solide[i].e.operator[](1)*solide[i].e.operator[](1));
      Aff_transformation_3 rotation(rot[0][0],rot[0][1],rot[0][2],rot[1][0],rot[1][1],rot[1][2],rot[2][0],rot[2][1],rot[2][2]);
      Aff_transformation_3 translation(CGAL::TRANSLATION,Vector_3(Point_3(0.,0.,0.),solide[i].x0)+solide[i].Dx);

      Aff_transformation_3 translation_inv(CGAL::TRANSLATION,Vector_3(solide[i].x0,Point_3(0.,0.,0.)));
      solide[i].mvt_tprev = solide[i].mvt_t;
      solide[i].mvt_t = translation*(rotation*translation_inv);
    }
    update_triangles();
  }
  
}

/*!\brief Time-integration of the solid position.
 *\param dt Time-step
 *\return void
 */
void Solide::Solve_position(double dt){
  for(int i=0;i<size();i++){
    solide[i].solve_position(dt);
  }
  breaking_criterion();
  update_triangles();
  for(int i=0;i<size();i++){
    for(std::vector<Triangle_3>::iterator it=solide[i].triangles.begin();it!=solide[i].triangles.end();it++){
      for(int k=0;k<3;k++){
	solide[i].bbox = Bbox(min(solide[i].bbox.xmin(),CGAL::to_double((*it).vertex(k).x())),min(solide[i].bbox.ymin(),CGAL::to_double((*it).vertex(k).y())),min(solide[i].bbox.zmin(),CGAL::to_double((*it).vertex(k).z())),max(solide[i].bbox.xmax(),CGAL::to_double((*it).vertex(k).x())),max(solide[i].bbox.ymax(),CGAL::to_double((*it).vertex(k).y())),max(solide[i].bbox.zmax(),CGAL::to_double((*it).vertex(k).z())));
      }
    }
	  
  }
	
}

/*!
 *\fn void Solide::Solve_vitesse(double dt)
 *\brief Calcul de la vitesse du solide.
 *\param dt pas de temps
 *\warning <b> Proc&eacute;dure sp&eacute;cifique au solide! </b>
 *\return void
 */
void Solide::Solve_vitesse(double dt){
  for(int i=0;i<size();i++){
    solide[i].solve_vitesse(dt);
  }
}

/*!\brief Computation of internal forces. 
 *\return void
 */
void Solide::Forces_internes(){
  //Initialization
  for(int i=0;i<size();i++){
    solide[i].Fi = Vector_3(0.,0.,0.);
    solide[i].Mi = Vector_3(0.,0.,0.);
  }
  //Computation of the volumetric deformation epsilon of each particle
  for(int i=0;i<size();i++){
    solide[i].Volume_libre();
    solide[i].epsilon = 0.;
    for(int j=0;j<solide[i].faces.size();j++){
      if(solide[i].faces[j].voisin>=0){
	int part = solide[i].faces[j].voisin;
	Vector_3 Sn = Vector_3(0.,0.,0.);
	for(int k=1;k<solide[i].faces[j].size()-1;k++){
	  Sn = Sn + 1./2.*cross_product(Vector_3(solide[i].faces[j].vertex[0].pos,solide[i].faces[j].vertex[k].pos),Vector_3(solide[i].faces[j].vertex[0].pos,solide[i].faces[j].vertex[k+1].pos));
	}
	Point_3 c1 = solide[i].mvt_t.transform(solide[i].faces[j].centre);
	Point_3 c2 = solide[part].mvt_t.transform(solide[i].faces[j].centre);
	Vector_3 Delta_u(c1,c2);
	solide[i].epsilon += 1./2./(solide[i].V+N_dim*nu/(1.-2.*nu)*solide[i].Vl)*CGAL::to_double(Sn*Delta_u);
      }
    }
  }
  //Computation of forces
  for(int i=0;i<size();i++){
    for(int j=0;j<solide[i].faces.size();j++){
      if(solide[i].faces[j].voisin>=0){
	int part = solide[i].faces[j].voisin;
	double S = solide[i].faces[j].S;
	Vector_3 X1X2(solide[i].mvt_t.transform(solide[i].x0),solide[part].mvt_t.transform(solide[part].x0));
	double DIJ = sqrt(CGAL::to_double(X1X2.squared_length()));
	Vector_3 nIJ = X1X2/DIJ;
	Point_3 c1 = solide[i].mvt_t.transform(solide[i].faces[j].centre);
	Point_3 c2 = solide[part].mvt_t.transform(solide[i].faces[j].centre);
	Vector_3 Delta_u(c1,c2);
	Vector_3 XC1(solide[i].x0,solide[i].faces[j].centre);
	Vector_3 XC2(solide[part].x0,solide[i].faces[j].centre);
	double alpha = sqrt(CGAL::to_double(XC1.squared_length()))/(solide[i].faces[j].D0);
	double epsilonIJ = alpha*solide[i].epsilon+(1.-alpha)*solide[part].epsilon;
	//Elastic traction-compression force
	solide[i].Fi = solide[i].Fi + S/solide[i].faces[j].D0*E/(1.+nu)*Delta_u;
	//Elastic volumetric deformation force
	solide[i].Fi = solide[i].Fi + S*E*nu/(1.+nu)/(1.-2.*nu)*epsilonIJ*(nIJ+Delta_u/DIJ-(Delta_u*nIJ)/DIJ*nIJ);
	//Torque of the applied forces
	solide[i].Mi = solide[i].Mi + cross_product(solide[i].mvt_t.transform(XC1),S/solide[i].faces[j].D0*E/(1.+nu)*Delta_u);
	solide[i].Mi = solide[i].Mi + cross_product(solide[i].mvt_t.transform(XC1),S*E*nu/(1.+nu)/(1.-2.*nu)*epsilonIJ*(nIJ+Delta_u/DIJ-(Delta_u*nIJ)/DIJ*nIJ));
	//Flexion/torsion torque
	double kappa = 1.;
	double alphan = (2.+2.*nu-kappa)*E/4./(1.+nu)/S*(solide[i].faces[j].Is+solide[i].faces[j].It);
	double alphas = E/4./(1.+nu)/S*((2.+2.*nu+kappa)*solide[i].faces[j].Is-(2.+2.*nu-kappa)*solide[i].faces[j].It);
	double alphat = E/4./(1.+nu)/S*((2.+2.*nu+kappa)*solide[i].faces[j].It-(2.+2.*nu-kappa)*solide[i].faces[j].Is);
	solide[i].Mi = solide[i].Mi + S/solide[i].faces[j].D0*(alphan*cross_product(solide[i].mvt_t.transform(solide[i].faces[j].normale),solide[part].mvt_t.transform(solide[i].faces[j].normale))+alphas*cross_product(solide[i].mvt_t.transform(solide[i].faces[j].s),solide[part].mvt_t.transform(solide[i].faces[j].s))+alphat*cross_product(solide[i].mvt_t.transform(solide[i].faces[j].t),solide[part].mvt_t.transform(solide[i].faces[j].t)));
      }
    }
  }
}

/*!\brief Compute the energy of the solid. 
 *\return void
 */
double Solide::Energie(){
  return Energie_cinetique()+Energie_potentielle();
}
/*!\brief Compute the kinetic energy of the solid.
 *\return void
 */
double Solide::Energie_cinetique(){
  double E = 0.;
  for(int i=0;i<size();i++){
    double u2 = CGAL::to_double(solide[i].u.squared_length());
    E += 1./2.*solide[i].m*u2;
    //Compute -1/2*tr(D j(Q^T omega)) = 1/2*(I1*Omega1^2+I2*Omega2^2+I3*Omega3^2)
    double Q[3][3];
    double rot[3][3];
    double e0 = sqrt(1.-CGAL::to_double(solide[i].e.squared_length()));
    //Recover the rotation matrix
    rot[0][0] = 1.-2.*CGAL::to_double(solide[i].e.operator[](1)*solide[i].e.operator[](1)+solide[i].e.operator[](2)*solide[i].e.operator[](2));
    rot[0][1] = 2.*CGAL::to_double(-e0*solide[i].e.operator[](2)+solide[i].e.operator[](0)*solide[i].e.operator[](1));
    rot[0][2] = 2.*CGAL::to_double(e0*solide[i].e.operator[](1)+solide[i].e.operator[](0)*solide[i].e.operator[](2));
    rot[1][0] = 2.*CGAL::to_double(e0*solide[i].e.operator[](2)+solide[i].e.operator[](1)*solide[i].e.operator[](0));
    rot[1][1] = 1.-2.*CGAL::to_double(solide[i].e.operator[](0)*solide[i].e.operator[](0)+solide[i].e.operator[](2)*solide[i].e.operator[](2));
    rot[1][2] = 2.*CGAL::to_double(-e0*solide[i].e.operator[](0)+solide[i].e.operator[](1)*solide[i].e.operator[](2));
    rot[2][0] = 2.*CGAL::to_double(-e0*solide[i].e.operator[](1)+solide[i].e.operator[](2)*solide[i].e.operator[](0));
    rot[2][1] = 2.*CGAL::to_double(e0*solide[i].e.operator[](0)+solide[i].e.operator[](2)*solide[i].e.operator[](1));
    rot[2][2] = 1.-2.*CGAL::to_double(solide[i].e.operator[](0)*solide[i].e.operator[](0)+solide[i].e.operator[](1)*solide[i].e.operator[](1));
    for(int j=0;j<3;j++){
      for(int k=0;k<3;k++){
	Q[j][k] = rot[j][0]*solide[i].rotref[0][k];
	Q[j][k] += rot[j][1]*solide[i].rotref[1][k];
	Q[j][k] += rot[j][2]*solide[i].rotref[2][k];
      }
    }  
    double Omega[3];
    Omega[0] = Omega[1] = Omega[2] = 0.;
    for(int j=0;j<3;j++){
      for(int k=0;k<3;k++){
	Omega[j] += CGAL::to_double(solide[i].omega.operator[](k)*Q[k][j]);
      }
    }
    E += 1./2.*(solide[i].I[0]*Omega[0]*Omega[0]+solide[i].I[1]*Omega[1]*Omega[1]+solide[i].I[2]*Omega[2]*Omega[2]);
  }
  return E;
}

/*! \brief Compute the potential energy of the solid. 
 * \return void
 */
double Solide::Energie_potentielle(){
  double Ep = 0.;
  //Compute the volumetric deformation epsilon of each particle
  for(int i=0;i<size();i++){
    solide[i].Volume_libre();
    solide[i].epsilon = 0.;
    for(int j=0;j<solide[i].faces.size();j++){
      if(solide[i].faces[j].voisin>=0){
	int part = solide[i].faces[j].voisin;
	Vector_3 Sn = Vector_3(0.,0.,0.);
	for(int k=1;k<solide[i].faces[j].size()-1;k++){
	  Sn = Sn + 1./2.*cross_product(Vector_3(solide[i].faces[j].vertex[0].pos,solide[i].faces[j].vertex[k].pos),Vector_3(solide[i].faces[j].vertex[0].pos,solide[i].faces[j].vertex[k+1].pos));
	}
	Point_3 c1 = solide[i].mvt_t.transform(solide[i].faces[j].centre);
	Point_3 c2 = solide[part].mvt_t.transform(solide[i].faces[j].centre);
	Vector_3 Delta_u(c1,c2);
	solide[i].epsilon += 1./2./(solide[i].V+N_dim*nu/(1.-2.*nu)*solide[i].Vl)*CGAL::to_double(Sn*Delta_u);
      }
    }
    //Volumetric deformation energy
    Ep += E*nu/2./(1.+nu)/(1.-2.*nu)*(solide[i].V+N_dim*nu/(1.-2.*nu)*solide[i].Vl)*pow(solide[i].epsilon,2);
  }
  //Compute the energy associated with each particle link
  for(int i=0;i<size();i++){
    for(int j=0;j<solide[i].faces.size();j++){
      if(solide[i].faces[j].voisin>=0){
	int part = solide[i].faces[j].voisin;
	double S = solide[i].faces[j].S;
	Vector_3 X1X2(solide[i].mvt_t.transform(solide[i].x0),solide[part].mvt_t.transform(solide[part].x0));
	double DIJ = sqrt(CGAL::to_double(X1X2.squared_length()));
	Vector_3 nIJ = X1X2/DIJ;
	Point_3 c1 = solide[i].mvt_t.transform(solide[i].faces[j].centre);
	Point_3 c2 = solide[part].mvt_t.transform(solide[i].faces[j].centre);
	Vector_3 Delta_u(c1,c2);
	Vector_3 XC1(solide[i].x0,solide[i].faces[j].centre);
	Vector_3 XC2(solide[part].x0,solide[i].faces[j].centre);
	double alpha = sqrt(CGAL::to_double(XC1.squared_length()))/(solide[i].faces[j].D0);
	double epsilonIJ = alpha*solide[i].epsilon+(1.-alpha)*solide[part].epsilon;
	//Elastic traction/compression energy
	Ep += 1./4.*S/solide[i].faces[j].D0*E/(1.+nu)*CGAL::to_double(Delta_u*Delta_u);
	//Flexion/torsion torques
	double kappa = 1.;
	double alphan = (2.+2.*nu-kappa)*E/4./(1.+nu)/S*(solide[i].faces[j].Is+solide[i].faces[j].It);
	double alphas = E/4./(1.+nu)/S*((2.+2.*nu+kappa)*solide[i].faces[j].Is-(2.+2.*nu-kappa)*solide[i].faces[j].It);
	double alphat = E/4./(1.+nu)/S*((2.+2.*nu+kappa)*solide[i].faces[j].It-(2.+2.*nu-kappa)*solide[i].faces[j].Is);
	Ep += S/2./solide[i].faces[j].D0*(alphan*(1.-CGAL::to_double(solide[i].mvt_t.transform(solide[i].faces[j].normale)*solide[part].mvt_t.transform(solide[i].faces[j].normale)))+alphas*(1.-CGAL::to_double(solide[i].mvt_t.transform(solide[i].faces[j].s)*solide[part].mvt_t.transform(solide[i].faces[j].s)))+alphat*(1.-CGAL::to_double(solide[i].mvt_t.transform(solide[i].faces[j].t)*solide[part].mvt_t.transform(solide[i].faces[j].t))));
      }
    }
  }
  return Ep;
}

double Solide::pas_temps(double t, double T){
  double dt = 10000.;
  //CFL condition on the angular momentum
  for(int i=0;i<size();i++){
    double dt1 = cfls*0.26/(abs(CGAL::to_double(solide[i].omega.operator[](0)))+abs(CGAL::to_double(solide[i].omega.operator[](1)))+abs(CGAL::to_double(solide[i].omega.operator[](2)))+eps);
    dt = min(dt,dt1); 
  }
  //CFL condition on internal forces
  double cs = sqrt(E*(1.-nu)/rhos/(1.+nu)/(1.-2.*nu));
  //Radius of the inscribed sphere
  double sigma = 100000.;
  for(int i=0;i<size();i++){
    for(int j=0;j<solide[i].faces.size();j++){
      sigma = min(sigma,solide[i].faces[j].D0);
    }
  }
  for(int i=0;i<size();i++){
    for(int j=0;j<solide[i].faces.size();j++){
      if(solide[i].faces[j].voisin>=0){
	dt = min(dt,cfls*solide[i].faces[j].D0/cs);
      }
    }
  }
  dt = min(dt,T-t); 
  return dt;
}

/*!\brief Update the fluid/solid interface.
 *\details Update \a Particule.triangles_prev, \a Particule.triangles, \a Particule.normales_prev, \a Particule.normales, \a Particule.fluide_prev, \a Particule.fluide, \a Particule.Points_interface_prev, \a Particule.Points_interface, \a Particule.Triangles_interface_prev, \a Particule.Triangles_interface, \a Particule.Position_Triangles_interface_prev and \a Particule.Position_Triangles_interface.
 *\return void
 */
void Solide::update_triangles(){
  for(int i=0;i<solide.size();i++){
    solide[i].triangles_prev = solide[i].triangles;
    solide[i].normales_prev = solide[i].normales;
    solide[i].fluide_prev = solide[i].fluide;
    for(int it=0;it<solide[i].triangles.size();it++){
      solide[i].Points_interface_prev[it] = solide[i].Points_interface[it];
      solide[i].Triangles_interface_prev[it] = solide[i].Triangles_interface[it];
      solide[i].Position_Triangles_interface_prev[it] = solide[i].Position_Triangles_interface[it];
      solide[i].Points_interface[it].erase(solide[i].Points_interface[it].begin(),solide[i].Points_interface[it].end());
      solide[i].Triangles_interface[it].erase(solide[i].Triangles_interface[it].begin(),solide[i].Triangles_interface[it].end());	solide[i].Position_Triangles_interface[it].erase(solide[i].Position_Triangles_interface[it].begin(),
																							 solide[i].Position_Triangles_interface[it].end());
    }
    solide[i].triangles.erase(solide[i].triangles.begin(),solide[i].triangles.end());
    solide[i].normales.erase(solide[i].normales.begin(),solide[i].normales.end());
    solide[i].fluide.erase(solide[i].fluide.begin(),solide[i].fluide.end());
    solide[i].vide.erase(solide[i].vide.begin(),solide[i].vide.end());
		
    //Compute the new position of triangles
    for(int f=0;f<solide[i].faces.size();f++){
      Point_3 s,r,v,t;
			
      if(solide[i].faces[f].size() == 3){
	vector<Point_3> ri,vi,si ;
	for(int part=0; part<solide[i].faces[f].vertex[0].size();part++){
	  int p = solide[i].faces[f].vertex[0].particules[part];
	  ri.push_back(solide[p].mvt_t.transform(solide[i].faces[f].vertex[0].pos));
	}
	r = centroid(ri.begin(),ri.end());
				
				
	for(int part=0;part<solide[i].faces[f].vertex[1].size();part++){
	  int p = solide[i].faces[f].vertex[1].particules[part];
	  vi.push_back(solide[p].mvt_t.transform(solide[i].faces[f].vertex[1].pos));
	}
	v = centroid(vi.begin(),vi.end());

	for(int part=0;part<solide[i].faces[f].vertex[2].size();part++){
	  int p = solide[i].faces[f].vertex[2].particules[part];
	  si.push_back(solide[p].mvt_t.transform(solide[i].faces[f].vertex[2].pos));
	}
	s = centroid(si.begin(),si.end());
				
	Vector_3 vect0(r,v);
	Vector_3 vect1(r,s);
	Triangle_3 Tri(r,v,s);
	solide[i].triangles.push_back(Tri);
	Vector_3 normale = CGAL::cross_product(vect0,vect1);
	normale = normale*(1./sqrt(CGAL::to_double(normale.squared_length())));
	solide[i].normales.push_back(normale);
	if(solide[i].faces[f].voisin < 0){
	  solide[i].fluide.push_back(true);
	} else {
	  solide[i].fluide.push_back(false);
	}
	if( solide[i].faces[f].voisin == -2){
	  solide[i].vide.push_back(true);
	} else {
	  solide[i].vide.push_back(false);
	}
      }
      else{
			
	vector<Point_3> si;
	si.push_back(solide[i].mvt_t.transform(solide[i].faces[f].centre));
	int j = solide[i].faces[f].voisin;
	if(j>=0){
	  si.push_back(solide[j].mvt_t.transform(solide[i].faces[f].centre));
	}
	s = centroid(si.begin(),si.end());
			
	for(int k=0;k<solide[i].faces[f].size();k++){
	  int kp = (k+1)%(solide[i].faces[f].size());
	  vector<Point_3> ri,vi;
	  for(int part=0;part<solide[i].faces[f].vertex[k].size();part++){
	    int p = solide[i].faces[f].vertex[k].particules[part];
	    ri.push_back(solide[p].mvt_t.transform(solide[i].faces[f].vertex[k].pos));
	  }
	  r = centroid(ri.begin(),ri.end());
	  for(int part=0;part<solide[i].faces[f].vertex[kp].size();part++){
	    int p = solide[i].faces[f].vertex[kp].particules[part];
	    vi.push_back(solide[p].mvt_t.transform(solide[i].faces[f].vertex[kp].pos));
	  }
	  v = centroid(vi.begin(),vi.end());
	  Vector_3 vect0(s,r);
	  Vector_3 vect1(s,v);
	  Triangle_3 Tri(s,r,v);
	  solide[i].triangles.push_back(Tri);
	  Vector_3 normale = CGAL::cross_product(vect0,vect1);
	  normale = normale*(1./sqrt(CGAL::to_double(normale.squared_length())));			  
	  solide[i].normales.push_back(normale);
	  if(solide[i].faces[f].voisin < 0){
	    solide[i].fluide.push_back(true);
	  } 
	  else {
	    solide[i].fluide.push_back(false);
	  }
	  if( solide[i].faces[f].voisin == -2){
	    solide[i].vide.push_back(true);
	  } else {
	    solide[i].vide.push_back(false);
	  }
	}
      }
			
    }
		
  }
}


/*!\brief Compute the error for the semi-implicit scheme.
 *\details The function is used as a stopping criterion in the semi-implicit scheme:  \n
 \f{eqnarray*}{ error = max( \, \Vert S1.solide[i].Dx -  S2.solide[i].Dx \, \Vert_{\infty} + h_{max} \Vert \, S1.solide[i].e -  S2.solide[i].e \, \Vert_{\infty})_i  \f}\n
 \f{eqnarray*}{
 h_{max}=& max(abs(S1.max\_x - S1.min\_x),abs(S1.max\_y - S1.min\_y), \\ 
 & abs(S1.max\_z - S1.min\_z), abs(S2.max\_x - S2.min\_x),\\ 
 & abs(S2.max\_y - S2.min\_y),abs(S2.max\_z - S2.min\_z)).
 \f}
				 
 *\param S1 \a Solide at iteration k of the fixed-point procedure
 *\param S2 \a Solide at iteration k-1 of the fixed-point procedure
 *\warning <b> Specific coupling procedure ! </b>
 *\return double
 */

double Error(Solide& S1, Solide& S2){
	
  double erreur = -1.;
	
  for(int it=0; it<S1.size(); it++){
		
    double h_max1 = std::max(std::max((S1.solide[it].bbox.xmax() - S1.solide[it].bbox.xmin()),(S1.solide[it].bbox.ymax() - S1.solide[it].bbox.ymin())),              (S1.solide[it].bbox.zmax() - S1.solide[it].bbox.zmin())); 
    double h_max2 = std::max(std::max((S2.solide[it].bbox.xmax() - S2.solide[it].bbox.xmin()),(S2.solide[it].bbox.ymax() - S2.solide[it].bbox.ymin())),              (S2.solide[it].bbox.zmax() - S2.solide[it].bbox.zmin())); 
    double h_max = max(h_max1, h_max2);
    double err1 = std::max(std::max(std::abs(CGAL::to_double(S1.solide[it].Dx.operator[](0) - S2.solide[it].Dx.operator[](0))), std::abs(CGAL::to_double(S1.solide[it].Dx.operator[](1) - S2.solide[it].Dx.operator[](1)) )), std::abs(CGAL::to_double(S1.solide[it].Dx.operator[](2) - S2.solide[it].Dx.operator[](2)))); 
    double err2 = std::max(std::max(std::abs(CGAL::to_double(S1.solide[it].e.operator[](0) - S2.solide[it].e.operator[](0))), std::abs(CGAL::to_double(S1.solide[it].e.operator[](1) - S2.solide[it].e.operator[](1)))), std::abs(CGAL::to_double(S1.solide[it].e.operator[](2) - S2.solide[it].e.operator[](2)))); ;
    double erreur_temp = err1 + h_max * err2;
    erreur = std::max(erreur_temp, erreur);
  }
	
  return erreur;
}	

/*! \brief Copy values of fluid forces \a Ff and torques \a Mf from S2 to S1.
 *  \details Function used in the semi-implicit scheme fixed-point procedure. 
 *	\param S1 \a Solide at time t
 *	\param S2 \a Solide at iteration k of the fixed-point procedure
 *	\warning <b> Specific coupling procedure ! </b>
 *	\return void
 */
void Copy_f_m(Solide& S1, Solide& S2){
	
  for(int it=0; it<S1.size(); it++){
    S1.solide[it].Ff =  S2.solide[it].Ff ;
    S1.solide[it].Mf =  S2.solide[it].Mf ;
  }
	
}	
/*!\brief Check whether point P is in the Bbox cell
  \details Returns true if P is inside cell and false otherwise.
  *\param cell Bbox 
  *\param P Point_3
  *\warning <b> Specific coupling procedure ! </b>
  *\return bool
  */
bool inside_box(const Bbox& cell, const Point_3& P){
  return ((cell.xmin() - P.x())<= eps_relat && (cell.ymin() - P.y())<= eps_relat && (cell.zmin() - P.z())<= eps_relat && (cell.xmax() - P.x())>=-eps_relat && (cell.ymax() - P.y())>=-eps_relat && (cell.zmax() - P.z())>=-eps_relat );
  
}	

/*!\brief Check whether point P is inside the convex polygon S
  \details Returns true if P is inside S and false otherwise.
  *\param S \a Particule
  *\param P \a Point_3
  *\warning <b> Specific coupling procedure ! </b>
  *\return bool
  */
bool inside_convex_polygon(const Particule& S, const Point_3& P){
	
  bool in = false;
	
  if(CGAL::do_overlap(S.bbox,P.bbox()))
  {
    if(S.cube) {in = true;}
    else{
      in = true;
      for(int l= 0; l<S.triangles.size() && in; l++){
	const Point_3& vertex = S.triangles[l].operator[](0);
	Vector_3 vect(P,vertex);
	if((CGAL::to_double(vect*S.normales[l])) < 0.){in = false;}
      }
    }
  }
	
  return in;
}	



/*!\brief Check whether cell is fully contained in S
  \details Returns true if cell is fully contained in S and false otherwise.
  *\param S \a Particule
  *\param cell \a Bbox
  *\warning <b> Specific coupling procedure ! </b>
  *\return bool
  */
bool box_inside_convex_polygon(const Particule& S, const Bbox& cell){
	
  bool in = false;
	
  if(CGAL::do_overlap(S.bbox,cell))
  {
    if(S.cube) { return S.cube;}
		
    else{
			
      in = true;
			
      Point_3 p1(cell.xmin(),cell.ymin(),cell.zmin());
      in = inside_convex_polygon(S,p1);
      if(!in) {return in;}
			
      Point_3 p2(cell.xmax(),cell.ymax(),cell.zmax());
      in = inside_convex_polygon(S,p2);
      if(!in) {return in;}
			
      Point_3 p3(cell.xmax(),cell.ymin(),cell.zmin());
      in = inside_convex_polygon(S,p3);
      if(!in) {return in;}
			
      Point_3 p4(cell.xmax(),cell.ymin(),cell.zmax());
      in = inside_convex_polygon(S,p4);
      if(!in) {return in;}
			
      Point_3 p5(cell.xmax(),cell.ymax(),cell.zmin());
      in = inside_convex_polygon(S,p5);
      if(!in) {return in;}
			
      Point_3 p6(cell.xmin(),cell.ymax(),cell.zmin());
      in = inside_convex_polygon(S,p6);
      if(!in) {return in;}
			
      Point_3 p7(cell.xmin(),cell.ymax(),cell.zmax());
      in = inside_convex_polygon(S,p7);
      if(!in) {return in;}
			
      Point_3 p8(cell.xmin(),cell.ymin(),cell.zmax());
      in = inside_convex_polygon(S,p8);
      if(!in) {return in;}
			
    }
  }
	
  return in;
}

/*!\brief Check whether cell is fully contained in tetra
  \details Returns true if cell is fully contained in tetra and false otherwise.
  *\param tetra \a Tetrahedron
  *\param cell \a Bbox
  *\warning <b> Specific coupling procedure ! </b>
  *\return bool
  */
bool box_inside_tetra(const Tetrahedron &tetra, const Bbox& cell){
	
  bool in = false;
	
  if(CGAL::do_overlap(tetra.bbox(),cell))
  {
    in = true;
			
    Point_3 p1(cell.xmin(),cell.ymin(),cell.zmin());
    in = inside_tetra(tetra,p1);
    if(!in) {return in;}
			
    Point_3 p2(cell.xmax(),cell.ymax(),cell.zmax());
    in = inside_tetra(tetra,p2);
    if(!in) {return in;}
			
    Point_3 p3(cell.xmax(),cell.ymin(),cell.zmin());
    in = inside_tetra(tetra,p3);
    if(!in) {return in;}
			
    Point_3 p4(cell.xmax(),cell.ymin(),cell.zmax());
    in = inside_tetra(tetra,p4);
    if(!in) {return in;}
			
    Point_3 p5(cell.xmax(),cell.ymax(),cell.zmin());
    in = inside_tetra(tetra,p5);
    if(!in) {return in;}
			
    Point_3 p6(cell.xmin(),cell.ymax(),cell.zmin());
    in = inside_tetra(tetra,p6);
    if(!in) {return in;}
			
    Point_3 p7(cell.xmin(),cell.ymax(),cell.zmax());
    in = inside_tetra(tetra,p7);
    if(!in) {return in;}
			
    Point_3 p8(cell.xmin(),cell.ymin(),cell.zmax());
    in = inside_tetra(tetra,p8);
    if(!in) {return in;}

  }
	
  return in;
}



/*!\brief Output the solid results in the vtk file format. 
 *\param n index of the output iteration
 *\return void
 */
void Solide::Impression(int n){ 
  int nb_part = solide.size();

  int nb_triangles = 0.;
  for(int it=0; it<nb_part; it++){
    nb_triangles += solide[it].triangles.size();
  }

  std::ostringstream oss;
  oss << "resultats/solide" << n << ".vtk";
  string s = oss.str();
  const char* const solidevtk = s.c_str();
	
  //Open the output flux
  std::ofstream vtk;
  vtk.open(solidevtk,ios::out);
  if(!vtk.is_open()){
    cout <<"Opening solide" << n << ".vtk failed" << endl;
  }
  vtk << setprecision(15);
  //Initialization of the vtk file
  vtk << "# vtk DataFile Version 3.0" << endl;
  vtk << "#Simulation Euler" << endl;
  vtk << "ASCII" << endl;
  vtk<<"\n";
  vtk << "DATASET UNSTRUCTURED_GRID" << endl;
  vtk << "POINTS " << 3*nb_triangles << " DOUBLE" << endl;
	
  for(int it=0; it<nb_part; it++){
    for(int l= 0; l<solide[it].triangles.size(); l++){
      vtk << solide[it].triangles[l].operator[](0).operator[](0) << " " << solide[it].triangles[l].operator[](0).operator[](1) << " " << solide[it].triangles[l].operator[](0).operator[](2) << endl;
      vtk << solide[it].triangles[l].operator[](1).operator[](0) << " " << solide[it].triangles[l].operator[](1).operator[](1) << " " << solide[it].triangles[l].operator[](1).operator[](2) << endl;
      vtk << solide[it].triangles[l].operator[](2).operator[](0) << " " << solide[it].triangles[l].operator[](2).operator[](1) << " " << solide[it].triangles[l].operator[](2).operator[](2) << endl;
    }
  }
  vtk<<"\n";
  vtk << "CELLS " << nb_triangles << " " << 4*nb_triangles<< endl;
  int num=0;
  for(int it=0; it<nb_part; it++){
    for(int l= 0; l<solide[it].triangles.size(); l++){
      vtk << 3 << " " << 3*num << " " << 3*num+1 << " " << 3*num+2 << endl;
      num++;
    }
  }
  vtk << "\n";
  vtk << "CELL_TYPES " << nb_triangles << endl;
  for(int l= 0; l<nb_triangles; l++)
  {
    vtk << 5 << endl;
  }
  vtk << "\n";
  vtk << "CELL_DATA " << nb_triangles << endl;
  //Displacement
  vtk << "VECTORS displacement double" << endl;
  //vtk << "LOOKUP_TABLE default" << endl;
  for(int it=0; it<nb_part; it++){
    for(int l= 0; l<solide[it].triangles.size(); l++)
    {
      vtk << solide[it].Dx.operator[](0) << " " << solide[it].Dx.operator[](1) << " " << solide[it].Dx.operator[](2) << endl;
    }
  }
  vtk << "\n";
  //Velocity
  vtk << "VECTORS velocity double" << endl;
  //vtk << "LOOKUP_TABLE default" << endl;
  for(int it=0; it<nb_part; it++){
    for(int l= 0; l<solide[it].triangles.size(); l++)
    {
      vtk << solide[it].u.operator[](0) << " " << solide[it].u.operator[](1) << " " << solide[it].u.operator[](2) << endl;
    }
  }
  vtk << "\n";
  //Rotation vector
  vtk << "VECTORS e double" << endl;
  //vtk << "LOOKUP_TABLE default" << endl;
  for(int it=0; it<nb_part; it++){
    for(int l= 0; l<solide[it].triangles.size(); l++)
    {
      vtk << solide[it].e.operator[](0) << " " << solide[it].e.operator[](1) << " " << solide[it].e.operator[](2) << endl;
    }
  }
  vtk << "\n";
  //Angular velocity
  vtk << "VECTORS omega double" << endl;
  //vtk << "LOOKUP_TABLE default" << endl;
  for(int it=0; it<nb_part; it++){
    for(int l= 0; l<solide[it].triangles.size(); l++)
    {
      vtk << solide[it].omega.operator[](0) << " " << solide[it].omega.operator[](1) << " " << solide[it].omega.operator[](2) << endl;
    }
  }
  vtk << "\n";
  vtk.close();
}

/*!\brief Simple breaking criterion for fracture tests
  \detailed A maximum elongation is allowed, after which the particle link is broken.
*/
void Solide::breaking_criterion(){
  for(int it=0; it<solide.size(); it++){
    for(int i=0; i<solide[it].faces.size(); i++){
      if(solide[it].faces[i].voisin >= 0){
	for(int iter=0; iter<solide.size(); iter++){
	  if(it!=iter){
	    if(solide[it].faces[i].voisin == iter){
	      double distance = sqrt(CGAL::to_double(CGAL::squared_distance(Point_3(solide[it].Dx.x() + solide[it].x0.x(), solide[it].Dx.y() + solide[it].x0.y(), solide[it].Dx.z() + solide[it].x0.z())  ,Point_3(solide[iter].Dx.x() + solide[iter].x0.x(),solide[iter].Dx.y() + solide[iter].x0.y(), solide[iter].Dx.z() + solide[iter].x0.z()) )));
	      if( (distance - solide[it].faces[i].D0)/solide[it].faces[i].D0 >= k_max){
		cout<<"BREAK!!!!"<<endl;
		solide[it].faces[i].voisin = -2;
		int j;
		for(int f=0; f<solide[iter].faces.size(); f++){ //
		  if(solide[iter].faces[f].voisin == it){
		    solide[iter].faces[f].voisin = -2;
		    j=f;
		  }
		}
		for(int count=0; count<solide[it].faces.size() ; count++){
		  for(int ii=0; ii<solide[it].faces[count].vertex.size(); ii++)
		  { 
		    std::vector<int> particules;
		    for(int part=0; part<solide[it].faces[count].vertex[ii].particules.size(); part++){
		      if(solide[it].faces[count].vertex[ii].particules[part] != iter){
			particules.push_back(solide[it].faces[count].vertex[ii].particules[part]);
		      }
		    }
		    solide[it].faces[count].vertex[ii].particules.erase(solide[it].faces[count].vertex[ii].particules.begin(),
									solide[it].faces[count].vertex[ii].particules.end());
		    solide[it].faces[count].vertex[ii].particules = particules;
																																					
		  }
		}
		for(int count=0; count<solide[iter].faces.size() ; count++){
		  for(int ii=0; ii<solide[iter].faces[count].vertex.size(); ii++)
		  { 
		    std::vector<int> particules;
		    for(int part=0; part<solide[iter].faces[count].vertex[ii].particules.size(); part++){
		      if(solide[iter].faces[count].vertex[ii].particules[part] != it){
			particules.push_back(solide[iter].faces[count].vertex[ii].particules[part]);
		      }
		    }
		    solide[iter].faces[count].vertex[ii].particules.erase(solide[iter].faces[count].vertex[ii].particules.begin(),
									  solide[iter].faces[count].vertex[ii].particules.end());
		    solide[iter].faces[count].vertex[ii].particules = particules;
		  }
		}
		for(int count=0; count<solide.size() ; count++){
		  if(count != it && count != iter){
		    bool voisin_it = false;
		    for(int jj=0; jj<solide[count].faces.size() ; jj++){
		      if(solide[count].faces[jj].voisin==it){
			voisin_it=true;
		      }
		    }
		    if(!voisin_it){
		      for(int kk=0; kk<solide[it].faces[i].size() ; kk++){
			for(int jj=0; jj<solide[count].faces.size() ; jj++){
			  for(int ii=0; ii<solide[count].faces[jj].vertex.size(); ii++)
			  {   
			    if(solide[it].faces[i].vertex[kk].num == solide[count].faces[jj].vertex[ii].num){
			      std::vector<int> particules;
			      for(int part=0; part<solide[count].faces[jj].vertex[ii].particules.size(); part++){
				if(solide[count].faces[jj].vertex[ii].particules[part] != it){
				  particules.push_back(solide[count].faces[jj].vertex[ii].particules[part]);
				}
			      }
			      solide[count].faces[jj].vertex[ii].particules.erase(solide[count].faces[jj].vertex[ii].particules.begin(), solide[count].faces[jj].vertex[ii].particules.end());
			      solide[count].faces[jj].vertex[ii].particules = particules;
			    }
			  }
			}
		      }
		      for(int kk=0; kk<solide[it].faces[i].size() ; kk++){
			for(int jj=0; jj<solide[it].faces.size() ; jj++){
			  for(int ii=0; ii<solide[it].faces[jj].vertex.size(); ii++)
			  {   
			    if(solide[it].faces[i].vertex[kk].num == solide[it].faces[jj].vertex[ii].num){
			      std::vector<int> particules;
			      for(int part=0; part<solide[it].faces[jj].vertex[ii].particules.size(); part++){
				if(solide[it].faces[jj].vertex[ii].particules[part] != count){
				  particules.push_back(solide[it].faces[jj].vertex[ii].particules[part]);
				}
			      }
			      solide[it].faces[jj].vertex[ii].particules.erase(solide[it].faces[jj].vertex[ii].particules.begin(), solide[it].faces[jj].vertex[ii].particules.end());
			      solide[it].faces[jj].vertex[ii].particules = particules;
			    }
			  }
			}
		      }
		    }
		    bool voisin_iter = false;
		    for(int jj=0; jj<solide[count].faces.size() ; jj++){
		      if(solide[count].faces[jj].voisin == iter){
			voisin_iter=true;
		      }
		    }
		    if(!voisin_iter){
		      for(int kk=0; kk<solide[iter].faces[j].size() ; kk++){
			for(int jj=0; jj<solide[count].faces.size() ; jj++){
			  for(int ii=0; ii<solide[count].faces[jj].vertex.size(); ii++)
			  {   
			    if(solide[iter].faces[j].vertex[kk].num == solide[count].faces[jj].vertex[ii].num){
			      std::vector<int> particules;
			      for(int part=0; part<solide[count].faces[jj].vertex[ii].particules.size(); part++){
				if(solide[count].faces[jj].vertex[ii].particules[part] != iter){
				  particules.push_back(solide[count].faces[jj].vertex[ii].particules[part]);
				}
			      }
			      solide[count].faces[jj].vertex[ii].particules.erase(solide[count].faces[jj].vertex[ii].particules.begin(), solide[count].faces[jj].vertex[ii].particules.end());
			      solide[count].faces[jj].vertex[ii].particules = particules;
			    }
			  }
			}
		      }
		      for(int kk=0; kk<solide[iter].faces[j].size() ; kk++){
			for(int jj=0; jj<solide[iter].faces.size() ; jj++){
			  for(int ii=0; ii<solide[iter].faces[jj].vertex.size(); ii++)
			  {   
			    if(solide[iter].faces[j].vertex[kk].num == solide[iter].faces[jj].vertex[ii].num){
			      std::vector<int> particules;
			      for(int part=0; part<solide[iter].faces[jj].vertex[ii].particules.size(); part++){
				if(solide[iter].faces[jj].vertex[ii].particules[part] != count){
				  particules.push_back(solide[iter].faces[jj].vertex[ii].particules[part]);
				}
			      }
			      solide[iter].faces[jj].vertex[ii].particules.erase(solide[iter].faces[jj].vertex[ii].particules.begin(), solide[iter].faces[jj].vertex[ii].particules.end());
			      solide[iter].faces[jj].vertex[ii].particules = particules;
			    }
			  }
			}
		      }
		    }
		    
		  }
		}
									  
	      } 
	    }
	  }
	}
      }
    }
  }
}


#endif
