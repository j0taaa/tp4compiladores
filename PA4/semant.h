#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <map>
#include <set>
#include <vector>
#include <utility>
#define protected public
#include "cool-tree.h"
#undef protected
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

struct MethodInfo {
  Symbol return_type;
  std::vector<std::pair<Symbol, Symbol> > formals;
  Class_ owner;
};

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  void add_class(Class_ c, bool basic);
  void validate_inheritance(Classes classes);
  bool visit_cycle(Symbol c, std::set<Symbol>& visiting, std::set<Symbol>& done);
  void build_feature_tables();
  void build_features_for(Symbol c, std::set<Symbol>& done);
  void collect_method(Symbol cname, Class_ cls, method_class *m);
  void collect_attr(Symbol cname, Class_ cls, attr_class *a);
  ostream& error_stream;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);

  std::map<Symbol, Class_> classes;
  std::map<Symbol, Symbol> parent;
  std::map<Symbol, std::map<Symbol, MethodInfo> > methods;
  std::map<Symbol, std::map<Symbol, Symbol> > attrs;
  std::set<Symbol> basic_classes;
  bool hierarchy_ok;

  bool class_defined(Symbol s);
  bool valid_type(Symbol s, bool allow_self_type);
  Symbol real_type(Symbol s, Symbol current_class);
  bool conforms(Symbol child, Symbol parent_type, Symbol current_class);
  Symbol lub(Symbol a, Symbol b, Symbol current_class);
  MethodInfo *lookup_method(Symbol type, Symbol name, Symbol current_class);
  Symbol check_expr(Expression e, SymbolTable<Symbol, Symbol> &env, Symbol current_class, Class_ cls);
  void check_class(Class_ cls);
};


#endif
