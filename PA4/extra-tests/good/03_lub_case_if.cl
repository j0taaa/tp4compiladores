class A { a() : Int { 1 }; };
class B inherits A { b() : Bool { true }; };
class C inherits A { c() : String { "c" }; };

class Main {
   pick(flag : Bool) : A {
      if flag then new B else new C fi
   };

   main() : Object {
      case self.pick(false) of
         b : B => b.a();
         c : C => c.c();
         a : A => a;
      esac
   };
};
