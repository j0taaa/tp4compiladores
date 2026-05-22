

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"


extern int semant_debug;
extern char *curr_filename;
int curr_lineno = 1;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

static class__class *cnode(Class_ c)
{
    return dynamic_cast<class__class *>(c);
}



ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
    hierarchy_ok = true;
    install_basic_classes();
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        add_class(classes->nth(i), false);
    }
    validate_inheritance(classes);
    if (!class_defined(Main)) {
        semant_error() << "Class Main is not defined." << endl;
    }
    hierarchy_ok = !errors();
    if (hierarchy_ok) {
        build_feature_tables();
        if (methods[Main].find(main_meth) == methods[Main].end()) {
            semant_error(classes->nth(0)) << "No 'main' method in class Main." << endl;
        } else if (methods[Main][main_meth].formals.size() != 0) {
            semant_error(methods[Main][main_meth].owner) << "'main' method in class Main must have no arguments." << endl;
        }
    }
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
    curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    add_class(Object_class, true);
    add_class(IO_class, true);
    add_class(Int_class, true);
    add_class(Bool_class, true);
    add_class(Str_class, true);
}

void ClassTable::add_class(Class_ c, bool basic)
{
    class__class *cc = cnode(c);
    if (classes.find(cc->name) != classes.end()) {
        semant_error(c) << "Class " << cc->name << " was previously defined." << endl;
        return;
    }
    classes[cc->name] = c;
    parent[cc->name] = cc->parent;
    if (basic) basic_classes.insert(cc->name);
}

bool ClassTable::class_defined(Symbol s)
{
    return s == SELF_TYPE || classes.find(s) != classes.end();
}

bool ClassTable::valid_type(Symbol s, bool allow_self_type)
{
    if (s == SELF_TYPE) return allow_self_type;
    if (s == prim_slot) return true;
    return classes.find(s) != classes.end();
}

Symbol ClassTable::real_type(Symbol s, Symbol current_class)
{
    return s == SELF_TYPE ? current_class : s;
}

void ClassTable::validate_inheritance(Classes user_classes)
{
    for (int i = user_classes->first(); user_classes->more(i); i = user_classes->next(i)) {
        Class_ c = user_classes->nth(i);
        class__class *cc = cnode(c);
        if (cc->name == SELF_TYPE) {
            semant_error(c) << "SELF_TYPE cannot be redefined as a class name." << endl;
        }
        if (basic_classes.find(cc->name) != basic_classes.end()) {
            semant_error(c) << "Redefinition of basic class " << cc->name << "." << endl;
        }
        if (cc->parent == SELF_TYPE || cc->parent == Int || cc->parent == Bool || cc->parent == Str) {
            semant_error(c) << "Class " << cc->name << " cannot inherit class " << cc->parent << "." << endl;
        } else if (cc->parent != No_class && classes.find(cc->parent) == classes.end()) {
            semant_error(c) << "Class " << cc->name << " inherits from an undefined class " << cc->parent << "." << endl;
        }
    }
    std::set<Symbol> visiting;
    std::set<Symbol> done;
    for (std::map<Symbol, Class_>::iterator it = classes.begin(); it != classes.end(); ++it) {
        visit_cycle(it->first, visiting, done);
    }
}

bool ClassTable::visit_cycle(Symbol c, std::set<Symbol>& visiting, std::set<Symbol>& done)
{
    if (done.find(c) != done.end()) return false;
    if (visiting.find(c) != visiting.end()) {
        semant_error(classes[c]) << "Class " << c << ", or an ancestor of " << c << ", is involved in an inheritance cycle." << endl;
        return true;
    }
    visiting.insert(c);
    Symbol p = parent[c];
    if (p != No_class && classes.find(p) != classes.end()) visit_cycle(p, visiting, done);
    visiting.erase(c);
    done.insert(c);
    return false;
}

void ClassTable::build_feature_tables()
{
    std::set<Symbol> done;
    for (std::map<Symbol, Class_>::iterator it = classes.begin(); it != classes.end(); ++it) {
        build_features_for(it->first, done);
    }
}

void ClassTable::build_features_for(Symbol cname, std::set<Symbol>& done)
{
    if (done.find(cname) != done.end()) return;
    Symbol p = parent[cname];
    if (p != No_class && classes.find(p) != classes.end()) {
        build_features_for(p, done);
        methods[cname] = methods[p];
        attrs[cname] = attrs[p];
    }
    Class_ cls = classes[cname];
    class__class *cc = cnode(cls);
    std::set<Symbol> local_attrs;
    std::set<Symbol> local_methods;
    for (int i = cc->features->first(); cc->features->more(i); i = cc->features->next(i)) {
        Feature f = cc->features->nth(i);
        if (method_class *m = dynamic_cast<method_class *>(f)) {
            if (local_methods.find(m->name) != local_methods.end()) {
                semant_error(cls->get_filename(), m) << "Method " << m->name << " is multiply defined in class." << endl;
            } else {
                local_methods.insert(m->name);
                collect_method(cname, cls, m);
            }
        } else if (attr_class *a = dynamic_cast<attr_class *>(f)) {
            if (local_attrs.find(a->name) != local_attrs.end()) {
                semant_error(cls->get_filename(), a) << "Attribute " << a->name << " is multiply defined in class." << endl;
            } else {
                local_attrs.insert(a->name);
                collect_attr(cname, cls, a);
            }
        }
    }
    done.insert(cname);
}

void ClassTable::collect_attr(Symbol cname, Class_ cls, attr_class *a)
{
    if (a->name == self) {
        semant_error(cls->get_filename(), a) << "'self' cannot be the name of an attribute." << endl;
    }
    if (!valid_type(a->type_decl, true)) {
        semant_error(cls->get_filename(), a) << "Class " << a->type_decl << " of attribute " << a->name << " is undefined." << endl;
    }
    Symbol p = parent[cname];
    if (p != No_class && attrs[p].find(a->name) != attrs[p].end()) {
        semant_error(cls->get_filename(), a) << "Attribute " << a->name << " is an attribute of an inherited class." << endl;
    }
    attrs[cname][a->name] = a->type_decl;
}

void ClassTable::collect_method(Symbol cname, Class_ cls, method_class *m)
{
    MethodInfo info;
    info.return_type = m->return_type;
    info.owner = cls;
    std::set<Symbol> seen_formals;
    if (!valid_type(m->return_type, true)) {
        semant_error(cls->get_filename(), m) << "Undefined return type " << m->return_type << " in method " << m->name << "." << endl;
    }
    for (int i = m->formals->first(); m->formals->more(i); i = m->formals->next(i)) {
        formal_class *f = dynamic_cast<formal_class *>(m->formals->nth(i));
        if (f->name == self) {
            semant_error(cls->get_filename(), f) << "'self' cannot be the name of a formal parameter." << endl;
        }
        if (f->type_decl == SELF_TYPE) {
            semant_error(cls->get_filename(), f) << "Formal parameter " << f->name << " cannot have type SELF_TYPE." << endl;
        } else if (!valid_type(f->type_decl, false)) {
            semant_error(cls->get_filename(), f) << "Class " << f->type_decl << " of formal parameter " << f->name << " is undefined." << endl;
        }
        if (seen_formals.find(f->name) != seen_formals.end()) {
            semant_error(cls->get_filename(), f) << "Formal parameter " << f->name << " is multiply defined." << endl;
        }
        seen_formals.insert(f->name);
        info.formals.push_back(std::make_pair(f->name, f->type_decl));
    }
    Symbol p = parent[cname];
    if (p != No_class && methods[p].find(m->name) != methods[p].end()) {
        MethodInfo old = methods[p][m->name];
        bool ok = old.return_type == info.return_type && old.formals.size() == info.formals.size();
        for (unsigned int i = 0; ok && i < old.formals.size(); ++i) {
            ok = old.formals[i].second == info.formals[i].second;
        }
        if (!ok) {
            semant_error(cls->get_filename(), m) << "In redefined method " << m->name << ", signature differs from inherited method." << endl;
        }
    }
    methods[cname][m->name] = info;
}

bool ClassTable::conforms(Symbol child, Symbol parent_type, Symbol current_class)
{
    if (child == No_type) return true;
    if (child == parent_type) return true;
    if (parent_type == SELF_TYPE) return child == SELF_TYPE;
    if (!valid_type(parent_type, false)) return false;
    Symbol c = real_type(child, current_class);
    if (classes.find(c) == classes.end()) return false;
    while (c != No_class) {
        if (c == parent_type) return true;
        if (parent.find(c) == parent.end()) break;
        c = parent[c];
    }
    return false;
}

Symbol ClassTable::lub(Symbol a, Symbol b, Symbol current_class)
{
    if (a == No_type) return b;
    if (b == No_type) return a;
    if (a == SELF_TYPE && b == SELF_TYPE) return SELF_TYPE;
    a = real_type(a, current_class);
    b = real_type(b, current_class);
    if (classes.find(a) == classes.end() || classes.find(b) == classes.end()) return Object;
    std::set<Symbol> ancestors;
    for (Symbol x = a; x != No_class && parent.find(x) != parent.end(); x = parent[x]) ancestors.insert(x);
    for (Symbol y = b; y != No_class; y = parent[y]) {
        if (ancestors.find(y) != ancestors.end()) return y;
        if (parent.find(y) == parent.end()) break;
    }
    return Object;
}

MethodInfo *ClassTable::lookup_method(Symbol type, Symbol name, Symbol current_class)
{
    Symbol c = real_type(type, current_class);
    while (c != No_class && classes.find(c) != classes.end()) {
        if (methods[c].find(name) != methods[c].end()) return &methods[c][name];
        c = parent[c];
    }
    return NULL;
}

static SymbolTable<Symbol, Symbol>& add_symbol(SymbolTable<Symbol, Symbol>& env, Symbol name, Symbol type)
{
    env.addid(name, new Symbol(type));
    return env;
}

static Symbol usable_declared_type(ClassTable *ct, Symbol type)
{
    return ct->valid_type(type, true) ? type : Object;
}

Symbol ClassTable::check_expr(Expression e, SymbolTable<Symbol, Symbol> &env, Symbol current_class, Class_ cls)
{
    if (assign_class *x = dynamic_cast<assign_class *>(e)) {
        Symbol rhs = check_expr(x->expr, env, current_class, cls);
        if (x->name == self) {
            semant_error(cls->get_filename(), x) << "Cannot assign to 'self'." << endl;
            return e->set_type(Object)->get_type();
        }
        Symbol *decl = env.lookup(x->name);
        if (decl == NULL) {
            semant_error(cls->get_filename(), x) << "Assignment to undeclared identifier " << x->name << "." << endl;
            return e->set_type(Object)->get_type();
        }
        if (!conforms(rhs, *decl, current_class)) {
            semant_error(cls->get_filename(), x) << "Type " << rhs << " of assigned expression does not conform to declared type " << *decl << " of identifier " << x->name << "." << endl;
        }
        return e->set_type(rhs)->get_type();
    }
    if (static_dispatch_class *x = dynamic_cast<static_dispatch_class *>(e)) {
        Symbol recv = check_expr(x->expr, env, current_class, cls);
        if (x->type_name == SELF_TYPE || !valid_type(x->type_name, false)) {
            semant_error(cls->get_filename(), x) << "Static dispatch to undefined class " << x->type_name << "." << endl;
            return e->set_type(Object)->get_type();
        }
        if (!conforms(recv, x->type_name, current_class)) {
            semant_error(cls->get_filename(), x) << "Expression type " << recv << " does not conform to declared static dispatch type " << x->type_name << "." << endl;
        }
        MethodInfo *m = lookup_method(x->type_name, x->name, current_class);
        if (m == NULL) {
            semant_error(cls->get_filename(), x) << "Static dispatch to undefined method " << x->name << "." << endl;
            return e->set_type(Object)->get_type();
        }
        int actual_count = x->actual->len();
        if (actual_count != (int)m->formals.size()) {
            semant_error(cls->get_filename(), x) << "Method " << x->name << " called with wrong number of arguments." << endl;
        }
        for (int i = x->actual->first(); x->actual->more(i); i = x->actual->next(i)) {
            Symbol actual_t = check_expr(x->actual->nth(i), env, current_class, cls);
            if (i < (int)m->formals.size() && !conforms(actual_t, m->formals[i].second, current_class)) {
                semant_error(cls->get_filename(), x->actual->nth(i)) << "In call of method " << x->name << ", type " << actual_t << " of parameter " << m->formals[i].first << " does not conform to declared type " << m->formals[i].second << "." << endl;
            }
        }
        return e->set_type(m->return_type == SELF_TYPE ? x->type_name : m->return_type)->get_type();
    }
    if (dispatch_class *x = dynamic_cast<dispatch_class *>(e)) {
        Symbol recv = check_expr(x->expr, env, current_class, cls);
        MethodInfo *m = lookup_method(recv, x->name, current_class);
        if (m == NULL) {
            semant_error(cls->get_filename(), x) << "Dispatch to undefined method " << x->name << "." << endl;
            for (int i = x->actual->first(); x->actual->more(i); i = x->actual->next(i)) check_expr(x->actual->nth(i), env, current_class, cls);
            return e->set_type(Object)->get_type();
        }
        if (x->actual->len() != (int)m->formals.size()) {
            semant_error(cls->get_filename(), x) << "Method " << x->name << " called with wrong number of arguments." << endl;
        }
        for (int i = x->actual->first(); x->actual->more(i); i = x->actual->next(i)) {
            Symbol actual_t = check_expr(x->actual->nth(i), env, current_class, cls);
            if (i < (int)m->formals.size() && !conforms(actual_t, m->formals[i].second, current_class)) {
                semant_error(cls->get_filename(), x->actual->nth(i)) << "In call of method " << x->name << ", type " << actual_t << " of parameter " << m->formals[i].first << " does not conform to declared type " << m->formals[i].second << "." << endl;
            }
        }
        return e->set_type(m->return_type == SELF_TYPE ? recv : m->return_type)->get_type();
    }
    if (cond_class *x = dynamic_cast<cond_class *>(e)) {
        if (check_expr(x->pred, env, current_class, cls) != Bool) semant_error(cls->get_filename(), x->pred) << "Predicate of 'if' does not have type Bool." << endl;
        Symbol t1 = check_expr(x->then_exp, env, current_class, cls);
        Symbol t2 = check_expr(x->else_exp, env, current_class, cls);
        return e->set_type(lub(t1, t2, current_class))->get_type();
    }
    if (loop_class *x = dynamic_cast<loop_class *>(e)) {
        if (check_expr(x->pred, env, current_class, cls) != Bool) semant_error(cls->get_filename(), x->pred) << "Loop condition does not have type Bool." << endl;
        check_expr(x->body, env, current_class, cls);
        return e->set_type(Object)->get_type();
    }
    if (typcase_class *x = dynamic_cast<typcase_class *>(e)) {
        check_expr(x->expr, env, current_class, cls);
        Symbol result = No_type;
        std::set<Symbol> branch_types;
        for (int i = x->cases->first(); x->cases->more(i); i = x->cases->next(i)) {
            branch_class *b = dynamic_cast<branch_class *>(x->cases->nth(i));
            if (b->name == self) semant_error(cls->get_filename(), b) << "'self' cannot be bound in a case branch." << endl;
            if (b->type_decl == SELF_TYPE || !valid_type(b->type_decl, false)) semant_error(cls->get_filename(), b) << "Case branch has undefined type " << b->type_decl << "." << endl;
            if (branch_types.find(b->type_decl) != branch_types.end()) semant_error(cls->get_filename(), b) << "Duplicate branch " << b->type_decl << " in case statement." << endl;
            branch_types.insert(b->type_decl);
            env.enterscope();
            add_symbol(env, b->name, usable_declared_type(this, b->type_decl));
            result = lub(result, check_expr(b->expr, env, current_class, cls), current_class);
            env.exitscope();
        }
        return e->set_type(result == No_type ? Object : result)->get_type();
    }
    if (block_class *x = dynamic_cast<block_class *>(e)) {
        Symbol last = Object;
        for (int i = x->body->first(); x->body->more(i); i = x->body->next(i)) last = check_expr(x->body->nth(i), env, current_class, cls);
        return e->set_type(last)->get_type();
    }
    if (let_class *x = dynamic_cast<let_class *>(e)) {
        if (!valid_type(x->type_decl, true)) semant_error(cls->get_filename(), x) << "Class " << x->type_decl << " of let-bound identifier " << x->identifier << " is undefined." << endl;
        Symbol init_t = check_expr(x->init, env, current_class, cls);
        if (init_t != No_type && !conforms(init_t, x->type_decl, current_class)) semant_error(cls->get_filename(), x) << "Inferred type " << init_t << " of initialization of " << x->identifier << " does not conform to identifier's declared type " << x->type_decl << "." << endl;
        env.enterscope();
        if (x->identifier == self) semant_error(cls->get_filename(), x) << "'self' cannot be bound in a let expression." << endl;
        else add_symbol(env, x->identifier, usable_declared_type(this, x->type_decl));
        Symbol body_t = check_expr(x->body, env, current_class, cls);
        env.exitscope();
        return e->set_type(body_t)->get_type();
    }
    if (plus_class *x = dynamic_cast<plus_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        if (t1 != Int || t2 != Int) semant_error(cls->get_filename(), x) << "Non-Int arguments: +." << endl;
        return e->set_type(Int)->get_type();
    }
    if (sub_class *x = dynamic_cast<sub_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        if (t1 != Int || t2 != Int) semant_error(cls->get_filename(), x) << "Non-Int arguments: -." << endl;
        return e->set_type(Int)->get_type();
    }
    if (mul_class *x = dynamic_cast<mul_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        if (t1 != Int || t2 != Int) semant_error(cls->get_filename(), x) << "Non-Int arguments: *." << endl;
        return e->set_type(Int)->get_type();
    }
    if (divide_class *x = dynamic_cast<divide_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        if (t1 != Int || t2 != Int) semant_error(cls->get_filename(), x) << "Non-Int arguments: /." << endl;
        return e->set_type(Int)->get_type();
    }
    if (neg_class *x = dynamic_cast<neg_class *>(e)) {
        if (check_expr(x->e1, env, current_class, cls) != Int) semant_error(cls->get_filename(), x) << "Argument of '~' has type other than Int." << endl;
        return e->set_type(Int)->get_type();
    }
    if (lt_class *x = dynamic_cast<lt_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        if (t1 != Int || t2 != Int) semant_error(cls->get_filename(), x) << "Non-Int arguments: <." << endl;
        return e->set_type(Bool)->get_type();
    }
    if (leq_class *x = dynamic_cast<leq_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        if (t1 != Int || t2 != Int) semant_error(cls->get_filename(), x) << "Non-Int arguments: <=." << endl;
        return e->set_type(Bool)->get_type();
    }
    if (eq_class *x = dynamic_cast<eq_class *>(e)) {
        Symbol t1 = check_expr(x->e1, env, current_class, cls);
        Symbol t2 = check_expr(x->e2, env, current_class, cls);
        bool basic1 = t1 == Int || t1 == Bool || t1 == Str;
        bool basic2 = t2 == Int || t2 == Bool || t2 == Str;
        if ((basic1 || basic2) && t1 != t2) semant_error(cls->get_filename(), x) << "Illegal comparison with a basic type." << endl;
        return e->set_type(Bool)->get_type();
    }
    if (comp_class *x = dynamic_cast<comp_class *>(e)) {
        if (check_expr(x->e1, env, current_class, cls) != Bool) semant_error(cls->get_filename(), x) << "Argument of 'not' has type other than Bool." << endl;
        return e->set_type(Bool)->get_type();
    }
    if (dynamic_cast<int_const_class *>(e)) return e->set_type(Int)->get_type();
    if (dynamic_cast<bool_const_class *>(e)) return e->set_type(Bool)->get_type();
    if (dynamic_cast<string_const_class *>(e)) return e->set_type(Str)->get_type();
    if (new__class *x = dynamic_cast<new__class *>(e)) {
        if (!valid_type(x->type_name, true)) semant_error(cls->get_filename(), x) << "'new' used with undefined class " << x->type_name << "." << endl;
        return e->set_type(valid_type(x->type_name, true) ? x->type_name : Object)->get_type();
    }
    if (isvoid_class *x = dynamic_cast<isvoid_class *>(e)) {
        check_expr(x->e1, env, current_class, cls);
        return e->set_type(Bool)->get_type();
    }
    if (dynamic_cast<no_expr_class *>(e)) return e->set_type(No_type)->get_type();
    if (object_class *x = dynamic_cast<object_class *>(e)) {
        if (x->name == self) return e->set_type(SELF_TYPE)->get_type();
        Symbol *t = env.lookup(x->name);
        if (t == NULL) {
            semant_error(cls->get_filename(), x) << "Undeclared identifier " << x->name << "." << endl;
            return e->set_type(Object)->get_type();
        }
        return e->set_type(*t)->get_type();
    }
    return e->set_type(Object)->get_type();
}

void ClassTable::check_class(Class_ cls)
{
    class__class *cc = cnode(cls);
    SymbolTable<Symbol, Symbol> env;
    env.enterscope();
    add_symbol(env, self, SELF_TYPE);
    for (std::map<Symbol, Symbol>::iterator it = attrs[cc->name].begin(); it != attrs[cc->name].end(); ++it) {
        add_symbol(env, it->first, usable_declared_type(this, it->second));
    }
    for (int i = cc->features->first(); cc->features->more(i); i = cc->features->next(i)) {
        Feature f = cc->features->nth(i);
        if (attr_class *a = dynamic_cast<attr_class *>(f)) {
            Symbol init_t = check_expr(a->init, env, cc->name, cls);
            if (init_t != No_type && !conforms(init_t, a->type_decl, cc->name)) {
                semant_error(cls->get_filename(), a) << "Inferred type " << init_t << " of initialization of attribute " << a->name << " does not conform to declared type " << a->type_decl << "." << endl;
            }
        } else if (method_class *m = dynamic_cast<method_class *>(f)) {
            env.enterscope();
            for (int j = m->formals->first(); m->formals->more(j); j = m->formals->next(j)) {
                formal_class *fm = dynamic_cast<formal_class *>(m->formals->nth(j));
                if (fm->name != self) add_symbol(env, fm->name, usable_declared_type(this, fm->type_decl));
            }
            Symbol body_t = check_expr(m->expr, env, cc->name, cls);
            if (!conforms(body_t, m->return_type, cc->name)) {
                semant_error(cls->get_filename(), m) << "Inferred return type " << body_t << " of method " << m->name << " does not conform to declared return type " << m->return_type << "." << endl;
            }
            env.exitscope();
        }
    }
    env.exitscope();
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    if (classtable->hierarchy_ok) {
	for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
	    classtable->check_class(classes->nth(i));
	}
    }

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}
