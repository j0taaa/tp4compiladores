class Acc {
   total : Int <- 0;

   add(x : Int) : SELF_TYPE {
      {
         total <- total + x;
         self;
      }
   };

   value() : Int { total };
};

class Main inherits IO {
   main() : Object {
      let a : Acc <- new Acc in {
         a.add(1).add(2);
         out_int(a.value());
      }
   };
};
