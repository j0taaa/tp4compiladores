class Parent {
   attr : Int <- 0;
   f(x : Int) : Int { x };
};

class Main inherits Parent {
   attr : Bool <- true;
   self : Int <- 1;
   f(x : Bool) : Bool { x };
   dup(a : Int, a : String) : Int { 0 };
   bad_return() : Unknown { 0 };
   bad_formal(x : SELF_TYPE) : Int { 0 };
};
