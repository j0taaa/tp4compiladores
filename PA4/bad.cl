class Parent {
   inherited : Int <- 1;
   same(x : Int) : Int { x };
};

class Child inherits Parent {
   inherited : Bool <- true;
   same(x : Bool) : Bool { x };
   duplicate(a : Int, a : Bool) : Int { 0 };
   bad_formal(self : Int) : Int { 0 };
};

class Main {
   main() : Object {
      {
         missing <- 1;
         (new Child).same(true);
         (new Child)@Parent.same(true);
         (new Child).unknown();
         if 1 then 2 else 3 fi;
         while 0 loop 1 pool;
         1 = "x";
         ~true;
         not 1;
         new Unknown;
         let self : Int <- 1 in self;
         let y : Int <- "text" in y;
         case 0 of
            a : Int => a;
            b : Int => b;
            c : SELF_TYPE => c;
         esac;
      }
   };
};
