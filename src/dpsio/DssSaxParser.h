/*
  enhanced version of the SaxParser, added stl maps
*/

#ifndef __DssSaxParser
#define __DssSaxParser

#ifdef _USELIBXML 

//#include <fstream>
#include <libxml++/libxml++.h>
#include "dpsframework.h"

typedef struct {
  int    nodeid;
  int    parentid;
} nodeType;

typedef struct {
  int    nodeid;
  string node;
} eKey;

typedef struct {
  string text;
  int    parentid;
} eData;

class nodeCmp {
     public:
          bool operator () ( const ::eKey&,
                             const ::eKey& ) const;
};

class DssSaxParser : public xmlpp::SaxParser {
public:
  DssSaxParser ();
  ~DssSaxParser() override;
 
  typedef map<string,string> smap;

  map<int,  smap>  attributeMap;
  map<eKey, eData, class nodeCmp> elementMap;
  map<int,  string> _start_elementMap;
  map<int,  string> _textMap;
  map<int,  int> _levelMap;
  map<string, nodeType> nodeMap;
  void print_maps( void );

protected:
  void on_start_document() override;
  void on_end_document() override;
  void on_start_element(const Glib::ustring& name,
                                const AttributeList& properties) override;
  void on_end_element(const Glib::ustring& name) override;
  void on_characters(const Glib::ustring& characters) override;
  void on_comment(const Glib::ustring& text) override;
  void on_warning(const Glib::ustring& text) override;
  void on_error(const Glib::ustring& text) override;
  void on_fatal_error(const Glib::ustring& text) override;

  int  _nodeid;
  int  _levelid;
};
#endif

#endif
