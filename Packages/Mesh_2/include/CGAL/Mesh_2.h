#ifndef CGAL_MESH_2_H
#define CGAL_MESH_2_H
#include <CGAL/Conform_2.h>
#include <CGAL/Double_map.h>

#include <queue>

CGAL_BEGIN_NAMESPACE

/**
   Tr is a Delaunay constrained triangulation (with intersections or not)
*/
template <class Tr>
class Mesh_2: public Conform_triangulation_2<Tr>
{
public:
  // --- public typedefs ---
  typedef Conform_triangulation_2<Tr> Conform;

  typedef Conform Base;
  typedef Mesh_2<Tr> Self;

  // -- types inherited from the templated base class --
  typedef typename Base::Geom_traits Geom_traits;
  typedef typename Geom_traits::FT FT;
  typedef FT      Squared_length;

  typedef typename Base::Vertex_handle        Vertex_handle;
  typedef typename Base::Face_handle          Face_handle;
 
  typedef typename Base::Face_circulator        Face_circulator;
  typedef typename Base::Finite_faces_iterator  Finite_faces_iterator;
  typedef typename Base::All_faces_iterator     All_faces_iterator;
  typedef typename Base::Point                  Point;

  // -- types needed to access member datas
  typedef std::list<Point> Seeds;
  typedef typename Seeds::const_iterator Seeds_iterator;
  typedef Seeds_iterator Seeds_const_iterator;

  // --- CONSTRUCTORS ---
  
  explicit Mesh_2(const Geom_traits& gt = Geom_traits()):
    Base(gt), initialized(false) {}

  // --- ACCESS FUNCTION ---
  bool is_bad(const Face_handle fh) const;

  double squared_minimum_sine(const Face_handle fh) const;
  double squared_minimum_sine(const Vertex_handle& va,
			      const Vertex_handle& vb,
			      const Vertex_handle& vc) const;

  Seeds_const_iterator seeds_begin() const;
  Seeds_const_iterator seeds_end() const;

  // --- HELPING FUNCTION ---
  void clear();

  // --- MARKING FUNCTIONS ---
  /** The value type of InputIterator should be Point, and represents
      seeds. Connected components of seeds are marked with the value of 
      "mark". Other components are marked with !mark. The connected
      component of infinite faces is always marked with false.
  */
  template <class InputIterator>
  void set_seeds(InputIterator b, InputIterator e,
		 const bool mark = false,
		 const bool do_it_now = false)
  {
    seeds.clear();
    copy(b, e, std::back_inserter(seeds));
    seeds_mark=mark;
    if(do_it_now) mark_facets();
  }

  void clear_seeds()
  {
    seeds.clear();
    seeds_mark = false;
  }

  /** Procedure that forces facets to be marked immediatelly. */
  void mark_facets();

  // --- MESHING FUNCTIONS ---

  // Perform meshing. 
  void refine();

  // --- REMESHING FUNCTIONS ---

  // Set the geom_traits nut DO NOT recalculate the list of bad faces (must
  // call set_bad_faces of calculate_bad_faces bellow)
  void set_geom_traits(const Geom_traits& gt);

  // re-calculate the list of bad faces
  void calculate_bad_faces();

  // Set the geom_traits and add the sequence [begin, end[ to the list
  // of bad faces.
  // Fh_it is a iterator of Face_Handle.
  // Use this overriden function if the list of bad faces can be
  // computed easily without testing all faces.
  template <class Fh_it>
  void set_bad_faces(Fh_it begin, Fh_it end)
  {
    bad_faces.clear();
    for(Fh_it pfit=begin; pfit!=end; ++pfit)
      push_in_bad_faces(*pfit);
  }

  // --- STEP BY STEP FUNCTIONS ---

  /**
     init(): Initialize the data structures 
     (The call of this function is REQUIRED before any step by step
     operation).
  */
  void init();

  /** Execute one step of the algorithm.
      Needs init() see above */
  bool refine_step();

private:
  // --- PRIVATE TYPES ---
  typedef typename Conform::Is_locally_gabriel_conform
    Is_locally_gabriel_conform;

  typedef CGAL::Triple<Vertex_handle,
                       Vertex_handle,
                       Vertex_handle> Threevertices; 

  typedef std::list<typename Base::Edge> List_of_edges;
  typedef std::list<Face_handle> List_of_face_handles;
  typedef typename Base::Cluster Cluster;

  // -- traits type --
  typedef typename Geom_traits::Is_bad Is_bad;
  typedef typename Geom_traits::Compute_squared_minimum_sine_2 
      Compute_squared_minimum_sine_2;
  typedef typename Geom_traits::Compute_squared_distance_2
      Compute_squared_distance_2;

  // -- typedefs for private members types --
  typedef CGAL::Double_map<Face_handle, double> Bad_faces;

private:
  // --- PRIVATE MEMBER DATAS ---

  // bad_faces: list of bad finite faces
  // warning: some faces could be recycled during insertion in the
  //  triangulation, that's why we need to be able to remoce faces
  //  from the map.
  Bad_faces bad_faces;
  bool initialized;

  Seeds seeds;
  bool seeds_mark;

private: 
  // --- PRIVATE MEMBER FUNCTIONS ---

  // -- auxiliary functions to set markers --

  // mark all faces of the convex_hull but those connected to the
  // infinite faces
  void mark_convex_hull();

  // propagate the mark m recursivly
  void propagate_marks(Face_handle f, bool m);



  // -- functions that maintain the map of bad faces

  // auxiliary functions called to put a new face in the map, two
  // forms
  void push_in_bad_faces(Face_handle fh);
  void push_in_bad_faces(Vertex_handle va,
			 Vertex_handle vb,
			 Vertex_handle vc);
 
  // scan all faces and put them if needed in the map
  void fill_facet_map();

  // update the map with faces incident to the vertex v
  void compute_new_bad_faces(Vertex_handle v);

  // -- inlined functions that compose the refinement process --

  // take one face in the queue and call refine_face
  void process_one_face();

  // handle one face; call split_face or put in the edges_to_be_conformed the
  // list of edges that would be encroached by the circum_center of f
  // This function uses Shewchuk's terminator criteria.
  void refine_face(Face_handle f);



  // -- functions that really insert points --

  // split the face f by inserting its circum center circum_center
  void split_face(const Face_handle& f, const Point& circum_center);

  // overrideen functions that inserts the point p in the edge
  // (fh,edge_index)
  Vertex_handle virtual_insert_in_the_edge(Face_handle fh,
					   const int edge_index,
					   const Point& p);

  // -- helping computing functions -- 

  // return the squared length of the triangle corresponding to the
  // face f
  Squared_length shortest_edge_squared_length(Face_handle f);


  // -- debugging functions --

  bool is_bad_faces_valid();

}; // end of Mesh_2

// --- ACCESS FUNCTIONS ---

// ?????????????
// ->traits
// the measure of faces quality
// # We can add here other contraints, such as a bound on the size
template <class Tr>
inline
bool Mesh_2<Tr>::
is_bad(const Face_handle f) const
{
  const Point
    & a = f->vertex(0)->point(),
    & b = f->vertex(1)->point(),
    & c = f->vertex(2)->point();

  return geom_traits().is_bad_object()(a,b,c);
}

template <class Tr>
inline
double Mesh_2<Tr>::
squared_minimum_sine(const Vertex_handle& va, const Vertex_handle& vb,
		     const Vertex_handle& vc) const
{
  Compute_squared_minimum_sine_2 squared_sine = 
    geom_traits().compute_squared_minimum_sine_2_object();

  return squared_sine(va->point(), vb->point(), vc->point());
}

template <class Tr>
inline
double Mesh_2<Tr>::
squared_minimum_sine(const Face_handle fh) const
{
  const Vertex_handle
    & va = fh->vertex(0),
    & vb = fh->vertex(1),
    & vc = fh->vertex(2);

  return squared_minimum_sine(va, vb, vc);
}

template <class Tr>
inline
typename Mesh_2<Tr>::Seeds_const_iterator
Mesh_2<Tr>::
seeds_begin() const
{
  return seeds.begin();
}

template <class Tr>
inline
typename Mesh_2<Tr>::Seeds_const_iterator
Mesh_2<Tr>::
seeds_end() const
{
  return seeds.end();
}



// --- HELPING FUNCTIONS ---

template <class Tr>
void Mesh_2<Tr>::
clear() 
{
  bad_faces.clear();
  seeds.clear();
  Base::clear();
}

// --- MARKING FUNCTIONS ---

template <class Tr>
void Mesh_2<Tr>::
mark_facets()
{
  if (Base::dimension()<2) return;
  if( seeds_begin() != seeds_end() )
    {
      for(All_faces_iterator it=all_faces_begin();
	  it!=all_faces_end();
	  ++it)
	it->set_marked(!seeds_mark);
	  
      for(Seeds_const_iterator sit=seeds_begin(); sit!=seeds_end(); ++sit)
	{
	  Face_handle fh=locate(*sit);
	  if(fh!=NULL)
	    propagate_marks(fh, seeds_mark);
	}
    }
  else
    mark_convex_hull();
  propagate_marks(infinite_face(), false);
};

// --- MESHING FUNCTIONS ---

//the mesh refine function 
template <class Tr>
inline
void Mesh_2<Tr>::
refine()
{
  if(!initialized) init();

  while(!Conform::is_conformed() || !bad_faces.empty() )
    {
      Conform::conform(Is_locally_gabriel_conform());
      if ( !bad_faces.empty() )
	process_one_face();
    }
}

// --- REMESHING FUNCTIONS ---

template <class Tr>
inline
void Mesh_2<Tr>::
set_geom_traits(const Geom_traits& gt)
{
  this->_gt = gt;
}

template <class Tr>
inline
void Mesh_2<Tr>::
calculate_bad_faces()
{
  fill_facet_map();
}

// --- STEP BY STEP FUNCTIONS ---

template <class Tr>
inline
void Mesh_2<Tr>::
init()
{
  bad_faces.clear();
  mark_facets(); // facets should be marked before the call to Base::init()

  Base::init(Is_locally_gabriel_conform());
  // handles clusters and edges

  fill_facet_map();
}

template <class Tr>
inline
bool Mesh_2<Tr>::
refine_step()
{
  if( !Conform::refine_step(Is_locally_gabriel_conform()) )
    if ( !bad_faces.empty() )
      process_one_face();
    else
      return false;
  return true;
}

// --- PRIVATE MEMBER FUNCTIONS ---

template <class Tr>
void Mesh_2<Tr>::
mark_convex_hull()
{
  for(All_faces_iterator fit=all_faces_begin();
      fit!=all_faces_end();
      ++fit)
    fit->set_marked(true);
  propagate_marks(infinite_face(), false);
}

template <class Tr>
void Mesh_2<Tr>::
propagate_marks(const Face_handle fh, bool mark)
{
  // std::queue only works with std::list on VC++6, and not with
  // std::deque, which is the default
  // But it should be fixed by VC++7 know. [Laurent Rineau 2003/03/24]
  std::queue<Face_handle/*, std::list<Face_handle>*/> face_queue;
  fh->set_marked(mark);
  face_queue.push(fh);
  while( !face_queue.empty() )
    {
      Face_handle fh = face_queue.front();
      face_queue.pop();
      for(int i=0;i<3;i++)
	{
	  const Face_handle& nb = fh->neighbor(i);
	  if( !fh->is_constrained(i) && (mark != nb->is_marked()) )
	    {
	      nb->set_marked(mark);
	      face_queue.push(nb);
	    }
	}
    }
};

template <class Tr>
inline
void Mesh_2<Tr>::
push_in_bad_faces(Face_handle fh)
{
  CGAL_assertion(fh->is_marked());
  bad_faces.insert(fh, squared_minimum_sine(fh));
}

template <class Tr>
inline
void Mesh_2<Tr>::
push_in_bad_faces(Vertex_handle va, Vertex_handle vb,
		  Vertex_handle vc)
{
  Face_handle fh;
  is_face(va, vb, vc, fh);
  push_in_bad_faces(fh);
}

//it is necessarry for process_facet_map
template <class Tr>
void Mesh_2<Tr>::
fill_facet_map()
{
  for(Finite_faces_iterator fit = finite_faces_begin();
      fit != finite_faces_end();
      ++fit)
    if( is_bad(fit) && fit->is_marked() )
      push_in_bad_faces(fit);
}

template <class Tr>
void Mesh_2<Tr>::
compute_new_bad_faces(Vertex_handle v)
{
  Face_circulator fc = v->incident_faces(), fcbegin(fc);
  do {
    if(!is_infinite(fc))
      if( is_bad(fc) && fc->is_marked() )
	push_in_bad_faces(fc);
    fc++;
  } while(fc!=fcbegin);
}

template <class Tr>
inline
void Mesh_2<Tr>::
process_one_face()
{
  Face_handle f = bad_faces.front()->second;
  bad_faces.pop_front();
  refine_face(f);
}

//split all the bad faces
template <class Tr>
void Mesh_2<Tr>::
refine_face(const Face_handle f)
{
  Is_locally_gabriel_conform is_gabriel_conform;

  const Point& pc = circumcenter(f);

  List_of_edges zone_of_pc_boundary;
  List_of_face_handles zone_of_pc;

  // find conflicts around pc (starting from f as hint)
  get_conflicts_and_boundary(pc, 
			    std::back_inserter(zone_of_pc), 
			    std::back_inserter(zone_of_pc_boundary), 
			    f);
  // For the moment, we don't use the zone_of_pc.
  // It will be used when we will destroyed old bad faces in bad_faces

  bool split_the_face = true;
  bool keep_the_face_bad = false;

  for(typename List_of_edges::iterator it = zone_of_pc_boundary.begin();
      it!= zone_of_pc_boundary.end();
      it++)
    {
      const Face_handle& fh = it->first;
      const int& i = it->second;
      const Vertex_handle
	& va = fh->vertex(cw(i)),
	& vb = fh->vertex(ccw(i));
      if(fh->is_constrained(i) && !is_gabriel_conform(*this, fh, i, pc))
	{
	  split_the_face = false;
	  Cluster c,c2;
	  bool 
	    is_cluster_at_va = get_cluster(va,vb,c),
	    is_cluster_at_vb = get_cluster(vb,va,c2);
	  if( ( is_cluster_at_va &&  is_cluster_at_vb) || 
	      (!is_cluster_at_va && !is_cluster_at_vb) )
	    {
	      // two clusters or no cluster
	      add_contrained_edge_to_be_conform(va,vb);
	      keep_the_face_bad = true;
	    }
	  else
	    {
	      // only one cluster: c or c2
	      if(is_cluster_at_vb)
		c = c2;
// What Shewchuk says:
// - If the cluster is not reduced (all segments don't have the same
// length as [va,vb]), then split the edge
// - Else, let rmin be the minimum insertion radius introduced by the
// potential split, let T be the triangle whose circumcenter
// encroaches [va,vb] and let rg be the length of the shortest edge
// of T. If rmin >= rg, then split the edge.

	      if( !c.is_reduced() || 
		  c.rmin >= shortest_edge_squared_length(f) )
		{
		  add_contrained_edge_to_be_conform(va,vb);
		  keep_the_face_bad = true;
		}
	    }
	}
    }; // after here edges encroached by pc are in the list of edges to
       // be conformed.


  const Vertex_handle
    & va = f->vertex(0),
    & vb = f->vertex(1),
    & vc = f->vertex(2);

  if(split_the_face)
    {
      CGAL_assertion(f->is_marked());
      if(f->is_marked())
	split_face(f, pc);
    }
  else
    if(keep_the_face_bad)
      push_in_bad_faces(va, vb, vc);
}

// # used by refine_face
template <class Tr>
inline
void Mesh_2<Tr>::
split_face(const Face_handle& f, const Point& circum_center)
{
  bool marked = f->is_marked();

  List_of_face_handles zone_of_cc;
  List_of_edges zone_of_cc_boundary;

  get_conflicts_and_boundary(circum_center, 
			     std::back_inserter(zone_of_cc),
			     std::back_inserter(zone_of_cc_boundary),
			     f);
  for(typename List_of_face_handles::iterator fh_it = zone_of_cc.begin();
      fh_it != zone_of_cc.end();
      ++fh_it)
    bad_faces.erase(*fh_it);

  // insert the point in the triangulation with star_hole
  Vertex_handle v = star_hole(circum_center,
			      zone_of_cc_boundary.begin(),
			      zone_of_cc_boundary.end(),
			      zone_of_cc.begin(),
			      zone_of_cc.end());

  Face_circulator fc = incident_faces(v), fcbegin(fc);
  do {
    fc->set_marked(marked);
  } while (++fc != fcbegin);

  compute_new_bad_faces(v);
}

template <class Tr>
inline 
typename Mesh_2<Tr>::Vertex_handle
Mesh_2<Tr>::
virtual_insert_in_the_edge(Face_handle fh, int edge_index, const Point& p)
  // insert the point p in the edge (fh, edge_index). It updates seeds 
  // too.
{
  const Vertex_handle
    & va = fh->vertex(cw(edge_index)),
    & vb = fh->vertex(ccw(edge_index));

  bool 
    mark_at_right = fh->is_marked(),
    mark_at_left = fh->neighbor(edge_index)->is_marked();

  List_of_face_handles zone_of_p;

  // deconstrain the edge before finding the conflicts
  fh->set_constraint(edge_index,false);
  fh->neighbor(edge_index)->set_constraint(fh->mirror_index(edge_index),false);

  get_conflicts_and_boundary(p, 
			     std::back_inserter(zone_of_p), 
			     Emptyset_iterator(), fh);

  // reconstrain the edge
  fh->set_constraint(edge_index,true);
  fh->neighbor(edge_index)->set_constraint(fh->mirror_index(edge_index),true);

  for(typename List_of_face_handles::iterator fh_it = zone_of_p.begin();
      fh_it != zone_of_p.end();
      ++fh_it)
    bad_faces.erase(*fh_it);

  Vertex_handle vp = insert(p, Base::EDGE, fh, edge_index);
  // TODO, WARNING: this is not robust!
  // We should deconstrained the constrained edge, insert the two
  // subconstraints and re-constrain them

  //  CGAL_assertion(is_bad_faces_valid());

  is_edge(va, vp, fh, edge_index); 
  // set fh to the face at the right of [va,vp]

  Face_circulator fc = incident_faces(vp, fh), fcbegin(fc);
  // circulators are counter-clockwise, so we start at the right of
  // [va,vp]
  do {
    if( !is_infinite(fc) )
      fc->set_marked(mark_at_right);
    ++fc;
  } while ( fc->vertex(ccw(fc->index(vp))) != vb );
  // we are now at the left of [va,vb]
  do {
    if( !is_infinite(fc) )
      fc->set_marked(mark_at_left);
    ++fc;
  } while ( fc != fcbegin );
  compute_new_bad_faces(vp);

  return vp;
}


template <class Tr>
bool Mesh_2<Tr>::
is_bad_faces_valid()
{
  typedef std::list<std::pair<double, Face_handle> > Bad_faces_list;
  
  bool result = true;

  Bad_faces_list bad_faces_list;

  while(!bad_faces.empty())
    {
      double d = bad_faces.front()->first;
      Face_handle fh = bad_faces.front()->second;
      bad_faces.pop_front();
      
      bad_faces_list.push_back(std::make_pair(d, fh));

      const Vertex_handle
	& va = fh->vertex(0),
	& vb = fh->vertex(1),
	& vc = fh->vertex(2);

      Face_handle fh2;
      if( ! ( is_face(va, vb, vc, fh2) && (fh == fh2) && 
	  fh->is_marked() && is_bad(fh) ) )
	{
	  result = false;
	  std::cerr << "Invalid bad face: (" << va->point() << ", "
		    << vb->point() << ", " << vc->point() << ")" << std::endl;
	}
    }

  for(typename Bad_faces_list::iterator it = bad_faces_list.begin();
      it != bad_faces_list.end();
      ++it)
    bad_faces.insert(it->second, it->first);
  
  return result;
}

// ->traits?
//the shortest edge that are in a triangle
// # used by: refine_face, squared_minimum_sine
template <class Tr>
typename Mesh_2<Tr>::FT
Mesh_2<Tr>::
shortest_edge_squared_length(Face_handle f)
{
  Compute_squared_distance_2 squared_distance = 
    geom_traits().compute_squared_distance_2_object();
  const Point 
    & pa = (f->vertex(0))->point(),
    & pb = (f->vertex(1))->point(),
    & pc = (f->vertex(2))->point();
  FT a, b, c;
  a = squared_distance(pb, pc);
  b = squared_distance(pc, pa);
  c = squared_distance(pa, pb);
  return (min(a, min(b, c)));
}

CGAL_END_NAMESPACE

#endif // CGAL_MESH_2_H
