class Base {
   me() : SELF_TYPE { self };
   id() : Int { 1 };
};

class Derived inherits Base {
   id() : Int { 2 };
   base_id() : Int { self@Base.id() };
   again() : SELF_TYPE { self.me() };
};

class Main {
   main() : Object {
      {
         (new Derived).again().id();
         (new Derived).base_id();
      }
   };
};
