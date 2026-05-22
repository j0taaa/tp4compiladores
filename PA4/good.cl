class Counter {
   value : Int <- 0;

   init(v : Int) : SELF_TYPE {
      {
         value <- v;
         self;
      }
   };

   inc() : SELF_TYPE {
      {
         value <- value + 1;
         self;
      }
   };

   get() : Int { value };
};

class FancyCounter inherits Counter {
   label : String <- "counter";

   rename(s : String) : SELF_TYPE {
      {
         label <- s.concat("!");
         self;
      }
   };

   choose(flag : Bool, other : Counter) : Counter {
      if flag then self else other fi
   };
};

class Main inherits IO {
   main() : Object {
      let c : FancyCounter <- (new FancyCounter).init(3).rename("ok") in {
         c.inc();
         out_int(c.get());
         out_string("\n");
         if isvoid c then 0 else c.get() fi;
         while c.get() < 5 loop c.inc() pool;
         case c.choose(true, new Counter) of
            f : FancyCounter => f.rename("case");
            base : Counter => base.inc();
            obj : Object => obj;
         esac;
         (c@Counter.inc()).get();
      }
   };
};
