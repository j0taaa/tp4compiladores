class Main {
   attr : Int <- 10;

   main() : Object {
      let attr : String <- "local" in {
         attr.length();
         let attr : Bool <- true in
            if attr then 1 else 0 fi;
      }
   };
};
