class A inherits B { };
class B inherits C { };
class C inherits A { };

class BadInt inherits Int { };
class Missing inherits NotDefined { };

class Main {
   main() : Object { 0 };
};
