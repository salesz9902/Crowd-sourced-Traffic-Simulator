#ifndef ROBOCAR_OSMREADER_HPP
#define ROBOCAR_OSMREADER_HPP

/**
 * @brief Justine - this is a rapid prototype for development of Robocar City Emulator
 *
 * @file osmreader.hpp
 * @author  Norbert Bátfai <nbatfai@gmail.com>
 * @version 0.0.10
 *
 * @section LICENSE
 *
 * Copyright (C) 2014 Norbert Bátfai, batfai.norbert@inf.unideb.hu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 * Robocar City Emulator and Robocar World Championship
 *
 * desc
 *
 */

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/index/map/sparse_mem_table.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/geom/haversine.hpp>
#include <osmium/geom/coordinates.hpp>
#include <google/protobuf/stubs/common.h>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <stdio.h>

#include <exception>
#include <stdexcept>

namespace justine
{
namespace robocar
{

typedef osmium::index::map::SparseMemMap<osmium::unsigned_object_id_type, osmium::Location> OSMLocations;

typedef std::vector<osmium::unsigned_object_id_type> WayNodesVect;

typedef std::vector<double> ProbabilityVect;
typedef std::pair<WayNodesVect, ProbabilityVect> WayNodesProbability;

typedef std::map<std::string, WayNodesVect> WayNodesMap;
//typedef osmium::index::map::StlMap<osmium::unsigned_object_id_type, osmium::Location> WaynodeLocations;
typedef std::map<osmium::unsigned_object_id_type, osmium::Location> WaynodeLocations;
typedef std::map<osmium::unsigned_object_id_type, WayNodesVect> Way2Nodes;

//typedef std::map<osmium::unsigned_object_id_type, WayNodesVect> AdjacencyList;
typedef std::map<osmium::unsigned_object_id_type, WayNodesProbability> AdjacencyList;
typedef osmium::index::map::SparseMemMap<osmium::unsigned_object_id_type, int > Vertices;

typedef std::map<osmium::unsigned_object_id_type, double> Neighborlist;
typedef std::map<osmium::unsigned_object_id_type, Neighborlist> KernelFile;

typedef std::map<osmium::unsigned_object_id_type, std::string> WayNames;

class OSMReader : public osmium::handler::Handler
{
public:
  OSMReader ( const char * osm_file,
              const char * kernel,
              AdjacencyList & alist,
              AdjacencyList & palist,
              WaynodeLocations & waynode_locations,
              WayNodesMap & busWayNodesMap,
              Way2Nodes & way2nodes,
              WayNames & way2name ) : alist ( alist ),
    palist ( palist ),
    waynode_locations ( waynode_locations ),
    busWayNodesMap ( busWayNodesMap ),
    way2nodes ( way2nodes ),
    way2name ( way2name )
  {

    try
      {

#ifdef DEBUG
        std::cout << "\n OSMReader is running... " << std::endl;
#endif

        std::ifstream file(kernel);
        std::string kernel_line;
        long source_node, neighbor_node;
        double frequency;
        double probability, prob_uncorr, prob_corr;
        
        while(std::getline(file, kernel_line)){

          std::sscanf(kernel_line.c_str(), 
            "%ld %ld {\'sums\': %lf, \'prob\': %lf, \'prob_corr\': %lf}",
            &source_node, &neighbor_node, &frequency, &prob_uncorr, &prob_corr);
          
          if (prob_corr <= 1 && prob_corr >= 0){
            probability = prob_corr;
            #ifdef DEBUG
              std::cout << " Corrected OK: " << prob_corr << "\n";
            #endif
          } else {
            probability = prob_uncorr;
            #ifdef DEBUG
              std::cout << " Corrected not OK! prob_corr " << prob_corr 
                << " prob_uncorr " << prob_uncorr << "\n";
            #endif
          }

          //auto search = kfile.find(source_node);

          kfile[source_node].insert(std::make_pair(neighbor_node, probability));
/*
          if (search != kfile.end()) {
            kfile[source_node].insert(std::make_pair(neighbor_node, probability));
          } else {
            if (neighbor_node != source_node){
              //std::map<osmium::unsigned_object_id_type, double> temp;
              //temp.insert(std::make_pair(neighbor_node, probability));
              //kfile.insert(std::make_pair(neighbor_node, temp));
              kfile[source_node].insert(std::make_pair(neighbor_node, probability));
            }

          }*/
        }

#ifdef DEBUG
        for (KernelFile::iterator it = kfile.begin(); it != kfile.end(); it++){
          std::cout << "Inspecting node: " << it->first << std::endl;
          for (Neighborlist::iterator node_it = it->second.begin(); node_it != it->second.end(); node_it++)
            std::cout << "Neighbor ID: " << node_it->first << " transition probability: " << node_it->second << std::endl;
        }
#endif

        osmium::io::File infile ( osm_file );
        osmium::io::Reader reader ( infile, osmium::osm_entity_bits::all );

        using SparseLocations = osmium::index::map::SparseMemMap<osmium::unsigned_object_id_type, osmium::Location>;
        osmium::handler::NodeLocationsForWays<SparseLocations> node_locations ( locations );

        osmium::apply ( reader, node_locations, *this );
        reader.close();

#ifdef DEBUG
        std::cout << " #OSM Nodes: " << nOSM_nodes << "\n";
        std::cout << " #OSM Highways: " << nOSM_ways << "\n";
        std::cout << " #Kms (Total length of highways) = " << sum_highhway_length/1000.0 << std::endl;
        std::cout << " #OSM Relations: " << nOSM_relations << "\n";
        std::cout << " #Highway Nodes: " << sum_highhway_nodes << "\n";
        std::cout << " #Unique Highway Nodes: " << sum_unique_highhway_nodes << "\n";
        std::cout << " E= (#Highway Nodes - #OSM Highways): " << sum_highhway_nodes - nOSM_ways << "\n";
        std::cout << " V= (#Unique Highway Nodes):" << sum_unique_highhway_nodes << "\n";
        std::cout << " V^2/E= "
                  <<
                  ( ( long ) sum_unique_highhway_nodes * ( long ) sum_unique_highhway_nodes )
                  / ( double ) ( sum_highhway_nodes - nOSM_ways )
                  << "\n";
        std::cout << " Edge_multiplicity: " << edge_multiplicity << std::endl;
        std::cout << " max edle length: " << max_edge_length << std::endl;
        std::cout << " edle length mean: " << mean_edge_length/cedges << std::endl;
        std::cout << " cedges: " << cedges << std::endl;

        std::cout << " #Buses = " << busWayNodesMap.size() << std::endl;

        std::cout << " Node locations " << locations.used_memory() /1024.0/1024.0 << " Mbytes" << std::endl;
        //std::cout << " Waynode locations " << waynode_locations.used_memory() /1024.0/1024.0 << " Mbytes" << std::endl;
        std::cout << " Vertices " << vert.used_memory() /1024.0/1024.0 << " Mbytes" << std::endl;
        std::cout << " way2nodes " << way2nodes.size() << "" << std::endl;

        std::set<osmium::unsigned_object_id_type> sum_vertices;
        std::map<osmium::unsigned_object_id_type, size_t>  word_map;
        int sum_edges {0};
        for ( auto busit = begin ( alist );
              busit != end ( alist ); ++busit )
          {

            sum_vertices.insert ( busit->first );
            sum_edges+=busit->second.first.size();

            for ( const auto &v : busit->second.first )
              {
                sum_vertices.insert ( v );
              }

          }
        std::cout << " #citymap edges = "<< sum_edges<< std::endl;
        std::cout << " #citymap vertices = "<< sum_vertices.size() << std::endl;
        std::cout << " #citymap vertices (deg- >= 1) = "<< alist.size() << std::endl;
        std::cout << " #onewayc = "<< onewayc<< std::endl;

#endif

        m_estimator *= 8;

      }
    catch ( std::exception &err )
      {

        google::protobuf::ShutdownProtobufLibrary();
        throw;
      }

  }

  ~OSMReader()
  {

    google::protobuf::ShutdownProtobufLibrary();

  }

  std::size_t get_estimated_memory() const
  {
    return m_estimator;
  }

  inline bool edge ( osmium::unsigned_object_id_type v1, osmium::unsigned_object_id_type v2 )
  {
    return ( std::find ( alist[v1].first.begin(), alist[v1].first.end(), v2 ) != alist[v1].first.end() );
  }

  void node ( osmium::Node& node )
  {
    ++nOSM_nodes;
  }

  int onewayc {0};
  int onewayf {false};

  void way ( osmium::Way& way )
  {

    const char* highway = way.tags() ["highway"];
    if ( !highway )
      return;
    // http://wiki.openstreetmap.org/wiki/Key:highway
    // Allowed: motorway, trunk, primary, second, tertiary, unclassified, service, motorway_link, trunk_link, primary_link, 
    // secondary_link, tertiary_link, living_street, road
    // Not allowed: pedestrian, track, bus_guideway, raceway, footway, bridleway, steps, path, cycleway, construction, proposed
    if ( !strcmp ( highway, "footway" )
         || !strcmp ( highway, "cycleway" )
	 || !strcmp ( highway, "pedestrian" )
	 || !strcmp ( highway, "bus_guideway" )
	 || !strcmp ( highway, "proposed" )
	 || !strcmp ( highway, "raceway" )
         || !strcmp ( highway, "bridleway" )
         || !strcmp ( highway, "track" )
         || !strcmp ( highway, "steps" )
         || !strcmp ( highway, "path" )
         || !strcmp ( highway, "construction" ) )
      return;

    onewayf = false;
    const char* oneway = way.tags() ["oneway"];
    if ( oneway )
      {
        onewayf = true;
        ++onewayc;
      }

    ++nOSM_ways;

    const char* wayname = way.tags() ["name"];
    if ( wayname )
      way2name[way.id()] = wayname;
    else
      way2name[way.id()] = "UNS "+std::to_string ( way.id() );

    const char* maxspeed = way.tags() ["maxspeed"];
    
    int speed {50};

    if (maxspeed){
      
      std::string speedLimit (maxspeed);
#ifdef DEBUG
      //std::cout << "Speed: " << maxspeed << "\n";
#endif
      try {
	speed = boost::lexical_cast<int>(speedLimit);
      } catch( boost::bad_lexical_cast const& ) {
#ifdef DEBUG
	std::cout << "Error: input string was not valid" << std::endl;
#endif
      }
    } else {
      if ( !strcmp ( highway, "motorway" )
	||  !strcmp ( highway, "motorway_link" ))
        speed = 130;
      if ( !strcmp ( highway, "trunk" ))
        speed = 110;
      if ( !strcmp ( highway, "residential" )
	||  !strcmp ( highway, "living_street" ))
        speed = 50;
      if ( !strcmp ( highway, "primary" ) 
        ||  !strcmp ( highway, "primary_link" ) 
	||  !strcmp ( highway, "secondary" )
	||  !strcmp ( highway, "secondary_link" ) 
        ||  !strcmp ( highway, "tertiary" )
	||  !strcmp ( highway, "tertiary_link" ))
        speed = 90;
      else
        speed = 50;
    }

    double ratio_for_speed = ((double)speed/3.6) * .2;
    
    // What if mph?

    double way_length = osmium::geom::haversine::distance ( way.nodes() );
    sum_highhway_length += way_length;

    int node_counter {0};
    int unique_node_counter {0};
    osmium::Location from_loc;

    osmium::unsigned_object_id_type vertex_old;

    for ( const osmium::NodeRef& nr : way.nodes() )
      {

        osmium::unsigned_object_id_type vertex = nr.positive_ref();

        way2nodes[way.id()].push_back ( vertex );

        try
          {
            vert.get ( vertex );
          }
        catch ( std::exception& e )
          {

            vert.set ( vertex, 1 );

            ++unique_node_counter;

            //waynode_locations.set ( vertex, nr.location() );
            waynode_locations[vertex] = nr.location();

          }

        if ( node_counter > 0 )
          {

            if ( !edge ( vertex_old, vertex ) )
              {

                alist[vertex_old].first.push_back ( vertex );

// 		if (!(std::find(alist[vertex_old].first.begin(), alist[vertex_old].first.end(), vertex_old) != alist[vertex_old].first.end())){
// 		  alist[vertex_old].first.push_back ( vertex_old );
// 		  alist[vertex_old].second.push_back ( 0 );
// 		}

                alist[vertex_old].second.push_back ( 0 );

                double edge_length = distance ( vertex_old, vertex );

                //palist[vertex_old].first.push_back ( edge_length / ratio_for_speed );
                palist[vertex_old].first.push_back ( edge_length / 3. );

		            int out_degree = alist[vertex_old].first.size();

                Neighborlist::iterator it;
                it = kfile[vertex_old].find(vertex);
		            if (it != kfile[vertex_old].end())
                  alist[vertex_old].second.at(alist[vertex_old].first.size()-1) = it->second;
                else
                  for (int i = 0; i < out_degree; i++){
                    alist[vertex_old].second.at(i) = 1/(double)out_degree;
                  }
    
                if ( edge_length>max_edge_length )
                  max_edge_length = edge_length;

                mean_edge_length += edge_length;

                ++m_estimator;
                ++cedges;


              }
            else
              ++edge_multiplicity;

            if ( !onewayf )
              {

                if ( !edge ( vertex, vertex_old ) )
                  {

		    alist[vertex].first.push_back ( vertex_old );

// 		    if (!(std::find(alist[vertex].first.begin(), alist[vertex].first.end(), vertex) != alist[vertex].first.end())){
// 		      alist[vertex].first.push_back ( vertex );
// 		      alist[vertex].second.push_back ( 0 );
// 		    }

                    alist[vertex].second.push_back ( 0 );

		    double edge_length = distance ( vertex_old, vertex );

                    //palist[vertex].first.push_back ( edge_length / ratio_for_speed );
                    palist[vertex].first.push_back ( edge_length / 3. );

		          int out_degree = alist[vertex].first.size();
		      
            Neighborlist::iterator it;
            it = kfile[vertex].find(vertex_old);
            if (it != kfile[vertex].end())
            alist[vertex].second.at(alist[vertex].first.size()-1) = it->second;
            else
              for (int i = 0; i < out_degree; i++){
                   alist[vertex].second.at(i) = 1/(double)out_degree;
                  }
          
                    if ( edge_length>max_edge_length )
                      max_edge_length = edge_length;

                    mean_edge_length += edge_length;

                    ++m_estimator;
                    ++cedges;


                  }
                else
                  ++edge_multiplicity;

              }

          }


        vertex_old = vertex;

        ++node_counter;
      }

    sum_highhway_nodes += node_counter;
    sum_unique_highhway_nodes  += unique_node_counter;

  }

  void relation ( osmium::Relation& rel )
  {

    ++nOSM_relations;

    const char* bus = rel.tags() ["route"];
    if ( bus && !strcmp ( bus, "bus" ) )
      {

        ++nbuses;

        std::string ref_key;

        try
          {
            const char* ref = rel.tags() ["ref"];
            if ( ref )
              ref_key.append ( ref );
            else
              ref_key.append ( "Not specified" );

          }
        catch ( std::exception& e )
          {
            std::cout << "There is no bus number."<< e.what() << std::endl;
          }

        osmium::RelationMemberList& rml = rel.members();
        for ( osmium::RelationMember& rm : rml )
          {

            if ( rm.type() == osmium::item_type::way )
              {

                busWayNodesMap[ref_key].push_back ( rm.ref() );

              }
          }

      }
  }

protected:

  Vertices vert;
  int nOSM_nodes {0};
  int nOSM_ways {0};
  int nOSM_relations {0};
  int sum_unique_highhway_nodes {0};
  int sum_highhway_nodes {0};
  int sum_highhway_length {0};
  int edge_multiplicity = 0;
  int nbuses {0};
  double max_edge_length {0.0};
  double mean_edge_length {0.0};
  int cedges {0};
  OSMLocations  locations;

private:

  inline double distance ( osmium::unsigned_object_id_type vertexa, osmium::unsigned_object_id_type vertexb )
  {

    osmium::Location A = locations.get ( vertexa );
    osmium::Location B = locations.get ( vertexb );
    osmium::geom::Coordinates ac {A};
    osmium::geom::Coordinates ab {B};

    return osmium::geom::haversine::distance ( ac, ab );
  }

  std::size_t m_estimator {0u};
  AdjacencyList & alist, & palist;
  WaynodeLocations & waynode_locations;
  WayNodesMap & busWayNodesMap;
  Way2Nodes & way2nodes;
  WayNames & way2name;
  KernelFile kfile;

};

}
} // justine::robocar::

#endif // ROBOCAR_OSMREADER_HPP

