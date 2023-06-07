# About this software
csvdiff is a diff tool to compare two CSV files containing numerical data.

```
$ csvdiff fileA.csv fileB.csv -r 1e-3
----------------
##10 #:1  <== 1.168140000000000e-01
##10 #:1  ==> 2.168140000000000e-01
@ Absolute error = 1.000000000000000e-01, Relative error = 8.013269975078731e-01

Found 1 lines contain different values beyond the tolerance

x:
-> max. abs. error = 1.000000000000000e-01 (tol. = 1.000000000000000e-10)
-> max. rel. error = 8.013269975078731e-01 (tol. = 1.000000000000000e-03)


Found a difference between the two files
* fileA.csv
* fileB.csv

```

# Usage
```
USAGE: 
   csvdiff  [-r <float>] [-a <float>] [-s] [--] [--version] [-h] <file
                path> <file path>


Where: 
   -r <float>,  --rel <float>
     relative tolerance

   -a <float>,  --abs <float>
     absolute tolerance

   -s,  --summary
     print only summary of the comparison.

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

   <file path>
     (required)  Path to a csv file.

   <file path>
     (required)  Path to a csv file.
```

# Third-party library
csvdiff uses the following library:
- TCLAP (Templatized Command Line Argument Parser), http://tclap.sourceforge.net

# License
Copyright 2016-2023, Norihiro Watanabe, All rights reserved.

csvdiff is free software; you can redistribute and use in source and 
binary forms, with or without modification. The software is distributed in the 
hope that it will be useful but WITHOUT ANY WARRANTY; without even the 
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
