/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef EXTRACTORCALLBACKS_H_
#define EXTRACTORCALLBACKS_H_

#include <stxxl.h>
#include "ExtractorStructs.h"

typedef stxxl::vector<NodeID> STXXLNodeIDVector;
typedef stxxl::vector<_Node> STXXLNodeVector;
typedef stxxl::vector<_Edge> STXXLEdgeVector;
typedef stxxl::vector<_Address> STXXLAddressVector;
typedef stxxl::vector<string> STXXLStringVector;


class ExtractorCallbacks{
private:
    STXXLNodeVector * allNodes;
    STXXLNodeIDVector * usedNodes;
    STXXLEdgeVector * allEdges;
    STXXLStringVector * nameVector;
    STXXLAddressVector * addressVector;
    Settings settings;
    StringMap * stringMap;

public:
    ExtractorCallbacks(STXXLNodeVector * aNodes, STXXLNodeIDVector * uNodes, STXXLEdgeVector * aEdges,  STXXLStringVector * nVector, STXXLAddressVector * adrVector, Settings s, StringMap * strMap){
        allNodes = aNodes;
        usedNodes = uNodes;
        allEdges = aEdges;
        nameVector = nVector;
        addressVector = adrVector;
        settings = s;
        stringMap = strMap;
    }

    ~ExtractorCallbacks() {
    }

    bool adressFunction(_Node n, HashTable<std::string, std::string> &keyVals) {
        std::string housenumber(keyVals.Find("addr:housenumber"));
        std::string housename(keyVals.Find("addr:housename"));
        std::string street(keyVals.Find("addr:street"));
        std::string state(keyVals.Find("addr:state"));
        std::string country(keyVals.Find("addr:country"));
        std::string postcode(keyVals.Find("addr:postcode"));
        std::string city(keyVals.Find("addr:city"));

        if(housenumber != "" || housename != "" || street != "") {
            if(housenumber == "")
                housenumber = housename;
            addressVector->push_back(_Address(n, housenumber, street, state, country, postcode, city));
        }
        return true;
    }

    bool nodeFunction(_Node &n) {
        allNodes->push_back(n);
        return true;
    }

    bool relationFunction(_Relation &r) {
        //do nothing;
        return true;
    }

    bool wayFunction(_Way &w) {
        std::string highway( w.keyVals.Find("highway") );
        std::string name( w.keyVals.Find("name") );
        std::string ref( w.keyVals.Find("ref"));
        std::string oneway( w.keyVals.Find("oneway"));
        std::string junction( w.keyVals.Find("junction") );
        std::string route( w.keyVals.Find("route") );
        std::string maxspeed( w.keyVals.Find("maxspeed") );
        std::string access( w.keyVals.Find("access") );
        std::string motorcar( w.keyVals.Find("motorcar") );

        if ( name != "" ) {
            w.name = name;
        } else if ( ref != "" ) {
            w.name = ref;
        }

        if ( oneway != "" ) {
            if ( oneway == "no" || oneway == "false" || oneway == "0" ) {
                w.direction = _Way::bidirectional;
            } else {
                if ( oneway == "yes" || oneway == "true" || oneway == "1" ) {
                    w.direction = _Way::oneway;
                } else {
                    if (oneway == "-1" )
                        w.direction = _Way::opposite;
                }
            }
        }
        if ( junction == "roundabout" ) {
            if ( w.direction == _Way::notSure ) {
                w.direction = _Way::oneway;
            }
            w.useful = true;
            if(w.type == -1)
                w.type = 9;
        }
        if ( route == "ferry") {
            for ( unsigned i = 0; i < settings.speedProfile.names.size(); i++ ) {
                if ( route == settings.speedProfile.names[i] ) {
                    w.type = i;
                    w.maximumSpeed = settings.speedProfile.speed[i];
                    w.useful = true;
                    w.direction = _Way::bidirectional;
                    break;
                }
            }
        }
        if ( highway != "" ) {
            for ( unsigned i = 0; i < settings.speedProfile.names.size(); i++ ) {
                if ( highway == settings.speedProfile.names[i] ) {
                    w.maximumSpeed = settings.speedProfile.speed[i];
                    w.type = i;
                    w.useful = true;
                    break;
                }
            }
            if ( highway == "motorway"  ) {
                if ( w.direction == _Way::notSure ) {
                    w.direction = _Way::oneway;
                }
            } else if ( highway == "motorway_link" ) {
                if ( w.direction == _Way::notSure ) {
                    w.direction = _Way::oneway;
                }
            }
        }
        if ( maxspeed != "" ) {
            double maxspeedNumber = atof( maxspeed.c_str() );
            if(maxspeedNumber != 0) {
                w.maximumSpeed = maxspeedNumber;
            }
        }

        if ( access != "" ) {
            if ( access == "private"  || access == "no" || access == "agricultural" || access == "forestry" || access == "delivery") {
                w.access = false;
            }
            if ( access == "yes"  || access == "designated" || access == "official" || access == "permissive") {
                w.access = true;
            }
        }
        if ( motorcar == "yes" ) {
            w.access = true;
        } else if ( motorcar == "no" ) {
            w.access = false;
        }

        if ( w.useful && w.access && w.path.size() ) {
            StringMap::iterator strit = stringMap->find(w.name);
            if(strit == stringMap->end())
            {
                w.nameID = nameVector->size();
                nameVector->push_back(w.name);
                stringMap->insert(std::make_pair(w.name, w.nameID) );
            } else {
                w.nameID = strit->second;
            }
            for ( unsigned i = 0; i < w.path.size(); ++i ) {
                usedNodes->push_back(w.path[i]);
            }

            if ( w.direction == _Way::opposite ){
                std::reverse( w.path.begin(), w.path.end() );
            }
            vector< NodeID > & path = w.path;
            assert(w.type > -1 || w.maximumSpeed != -1);
            assert(path.size()>0);

            if(w.maximumSpeed == -1)
                w.maximumSpeed = settings.speedProfile.speed[w.type];
            for(vector< NodeID >::size_type n = 0; n < path.size()-1; n++) {
                _Edge e;
                e.start = w.path[n];
                e.target = w.path[n+1];
                e.type = w.type;
                e.direction = w.direction;
                e.speed = w.maximumSpeed;
                e.nameID = w.nameID;
                allEdges->push_back(e);
            }
        }
        return true;
    }
};

#endif /* EXTRACTORCALLBACKS_H_ */
