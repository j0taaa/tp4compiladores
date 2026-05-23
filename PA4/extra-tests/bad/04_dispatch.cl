class A {
   f(x : Int, y : String) : Int { x };
};

class B inherits A { };

class Main {
   main() : Object {
      {
         (new A).f(1);
         (new A).f(true, "ok");
         (new A).missing();
         (new A)@B.f(1, "bad");
         (new A)@Unknown.f(1, "bad");
      }
   };
};
