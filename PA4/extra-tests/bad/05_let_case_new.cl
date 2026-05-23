class Main {
   main() : Object {
      {
         new Unknown;
         let self : Int <- 0 in self;
         let x : Missing <- 0 in x;
         let y : Bool <- 1 in y;
         case 0 of
            a : Int => a;
            b : Int => b;
            c : SELF_TYPE => c;
            self : String => self;
         esac;
      }
   };
};
