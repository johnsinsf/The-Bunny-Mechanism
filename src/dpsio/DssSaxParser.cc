/*
  File:    dpsio/DssSaxParser.cc
  Author:  John Sullivan, Dynamic Processing Systems LLC

  Copyright 2020 All Rights Reserved 

  No part of this file may be reproduced or transmitted without
  express written permission of the author.

  modified version of the SaxParser, attribution later

*/

#ifdef _USELIBXML

#include <glibmm/convert.h> 
#include <iostream>
#include "DssSaxParser.h"

extern log4cpp::Category& logger;

DssSaxParser::DssSaxParser() : xmlpp::SaxParser() {
  _levelid  = -1;
  _nodeid   = -1;
}

DssSaxParser::~DssSaxParser() {
}

void DssSaxParser::on_start_document() {
}

void 
DssSaxParser::on_end_document() {
}

void 
DssSaxParser::on_start_element(const Glib::ustring& name,
                                   const AttributeList& attributes) {
  _levelid++;
  _nodeid++;
  nodeType nt;
  string pn, pv;

  try {
    _start_elementMap[_nodeid] = name;
    nt.nodeid = _nodeid;
    nt.parentid = _levelid;
    nodeMap[name] = nt;
    _levelMap[_levelid] = _nodeid;
    //logger.error("inserted nodeMap " + name + " " + itoa(_nodeid));
  }
   catch(const Glib::ConvertError& ex) {
      logger.error("on_start_element(): exception caught while converting name: " + ex.what());
  }

  smap ts;
  for(const auto& attr_pair : attributes) {
    try {
      pn = attr_pair.name;
    }
    catch(const Glib::ConvertError& ex) {
      logger.error("on_start_element(): exception caught while getting attribute name: " + ex.what());
    }
    try {
      pv = attr_pair.value;
    }
    catch(const Glib::ConvertError& ex) {
      logger.error("on_start_element(): exception caught while converting name: " + ex.what());
    }
    if( pn.size() && pv.size() ) {
      ts[pn] = pv;
    }
  }
  if( ts.size() )
    attributeMap.insert( make_pair(_nodeid,ts) );
}

void 
DssSaxParser::on_end_element(const Glib::ustring& name ) {
  string n;
  try {
    n = name;
  }
  catch(const Glib::ConvertError& ex) {
    logger.error("on_characters(): exception caught while converting name : " + ex.what());
  }
  eKey ek;
  eData ed;
  // get previous node id for this name
  map<string,nodeType>::iterator I = nodeMap.find(n);
  nodeType nt;
  nt.nodeid = -1; nt.parentid = -1;
  if( I != nodeMap.end() ) {
    nt = nodeMap[n];
    nodeMap.erase(n);
  }
  ek.nodeid = nt.nodeid;
  ek.node   = _start_elementMap[nt.nodeid];
  ed.text   = _textMap[nt.nodeid];

  map<int,int>::iterator I2 = _levelMap.find(_levelid - 1);
  if( I2 != _levelMap.end() ) {
    ed.parentid = _levelMap[_levelid - 1];
  } else {
    ed.parentid = -1;
  }

  if( _levelid >= 0 ) _levelid--;

  elementMap.insert( make_pair(ek,ed) );
}

void 
DssSaxParser::on_characters(const Glib::ustring& text) {
  try {
    _textMap[_nodeid] = text;
  }
  catch(const Glib::ConvertError& ex) {
    logger.error("on_characters(): exception caught while converting text : " + ex.what());
  }
}

void 
DssSaxParser::on_comment(const Glib::ustring& text) {
  try {
    //logger.error("on_comment(): " + text);
  }
  catch(const Glib::ConvertError& ex) {
    logger.error("on_comment(): exception caught while converting text : " + ex.what());
  }
}

void 
DssSaxParser::on_warning(const Glib::ustring& text) {
  try {
    //logger.error("on_warning(): " + text);
  }
  catch(const Glib::ConvertError& ex) {
    logger.error("on_warning(): exception caught while converting text : " + ex.what());
  }
}

void 
DssSaxParser::on_error(const Glib::ustring& text) {
  try {
    //logger.error("on_error(): " + text);
  }
  catch(const Glib::ConvertError& ex) {
    logger.error("on_error(): exception caught while converting text : " + ex.what());
  }
}

void 
DssSaxParser::on_fatal_error(const Glib::ustring& text) {
  try {
    //logger.error("on_fatal_error(): " + text);
  }
  catch(const Glib::ConvertError& ex) {
    logger.error("on_fatal_error(): exception caught while converting text : " + ex.what());
  }
}

void 
DssSaxParser::print_maps( void )  {
  typedef map<eKey,eData>::const_iterator I;
  logger.error( "elementMap is:"); 
  for( I i = elementMap.begin(); i != elementMap.end(); ++i ) {
    logger.error( "  " + itoa(i->first.nodeid) + " " + i->first.node + " text " + i->second.text + " parent " + itoa(i->second.parentid));
  }
  typedef map<string,string> smap;
  typedef map<int, smap>::const_iterator I2;
  typedef map<string,string>::const_iterator I3;
  for( I2 i = attributeMap.begin(); i != attributeMap.end(); ++i ) {
    logger.error( "attributeMap is: " + itoa(i->first)); 
    for( I3 j = i->second.begin(); j != i->second.end(); ++j ) {
      logger.error( "  " + j->first + " " + j->second); 
    }
  }
  logger.error("node Map:");
  typedef map<string, nodeType>::const_iterator I4;

  for( I4 i = nodeMap.begin(); i != nodeMap.end(); ++i ) {
    logger.error("  first " + i->first + " " + itoa(i->second.nodeid) + " parent " + itoa(i->second.parentid));
  }
  logger.error("level Map:");
  typedef map<int, int>::const_iterator I5;
  for( I5 i = _levelMap.begin(); i != _levelMap.end(); ++i ) {
    logger.error("  first " + itoa(i->first) + " second " + itoa(i->second) );
  }
}

bool 
nodeCmp::operator() (const ::eKey& x, const ::eKey& y) const {
  if( x.nodeid != -1 ) {
    if( x.nodeid < y.nodeid )
      return true;

    if( x.nodeid > y.nodeid )
      return false;
  }
  if( x.node < y.node )
    return true;

  if( x.node > y.node )
    return false;

  return false;
}

#endif
